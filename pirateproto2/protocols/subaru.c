#include "subaru.h"

#define TAG "SubaruProtocol"

static const SubGhzBlockConst subghz_protocol_subaru_const = {
    .te_short = 800,
    .te_long = 1600,
    .te_delta = 250,
    .min_count_bit_for_found = 64,
};

typedef struct SubGhzProtocolDecoderSubaru
{
    SubGhzProtocolDecoderBase base;
    SubGhzBlockDecoder decoder;
    SubGhzBlockGeneric generic;

    uint16_t header_count;
    uint16_t bit_count;
    uint8_t data[8];

    uint64_t key;
    uint32_t serial;
    uint8_t button;
    uint16_t count;
} SubGhzProtocolDecoderSubaru;

typedef struct SubGhzProtocolEncoderSubaru
{
    SubGhzProtocolEncoderBase base;
    SubGhzProtocolBlockEncoder encoder;
    SubGhzBlockGeneric generic;
} SubGhzProtocolEncoderSubaru;

typedef enum
{
    SubaruDecoderStepReset = 0,
    SubaruDecoderStepCheckPreamble,
    SubaruDecoderStepFoundGap,
    SubaruDecoderStepFoundSync,
    SubaruDecoderStepSaveDuration,
    SubaruDecoderStepCheckDuration,
} SubaruDecoderStep;

const SubGhzProtocolDecoder subghz_protocol_subaru_decoder = {
    .alloc = subghz_protocol_decoder_subaru_alloc,
    .free = subghz_protocol_decoder_subaru_free,
    .feed = subghz_protocol_decoder_subaru_feed,
    .reset = subghz_protocol_decoder_subaru_reset,
    .get_hash_data = subghz_protocol_decoder_subaru_get_hash_data,
    .serialize = subghz_protocol_decoder_subaru_serialize,
    .deserialize = subghz_protocol_decoder_subaru_deserialize,
    .get_string = subghz_protocol_decoder_subaru_get_string,
};

// Encoder implementation
static void subaru_encode_count(uint8_t *KB, uint16_t count)
{
    // This is the inverse of subaru_decode_count
    uint8_t lo = count & 0xFF;
    uint8_t hi = (count >> 8) & 0xFF;
    
    // Set the bits based on lo byte
    if (!(lo & 0x01)) KB[4] |= 0x40;
    else KB[4] &= ~0x40;
    if (!(lo & 0x02)) KB[4] |= 0x80;
    else KB[4] &= ~0x80;
    if (!(lo & 0x04)) KB[5] |= 0x01;
    else KB[5] &= ~0x01;
    if (!(lo & 0x08)) KB[5] |= 0x02;
    else KB[5] &= ~0x02;
    if (!(lo & 0x10)) KB[6] |= 0x01;
    else KB[6] &= ~0x01;
    if (!(lo & 0x20)) KB[6] |= 0x02;
    else KB[6] &= ~0x02;
    if (!(lo & 0x40)) KB[5] |= 0x40;
    else KB[5] &= ~0x40;
    if (!(lo & 0x80)) KB[5] |= 0x80;
    else KB[5] &= ~0x80;
    
    // Set REG_SH1 and REG_SH2 from hi byte
    uint8_t REG_SH1 = (KB[7] << 4) & 0xF0;
    if (!(hi & 0x04)) REG_SH1 |= 0x04;
    else REG_SH1 &= ~0x04;
    if (!(hi & 0x08)) REG_SH1 |= 0x08;
    else REG_SH1 &= ~0x08;
    if (!(hi & 0x02)) REG_SH1 |= 0x02;
    else REG_SH1 &= ~0x02;
    if (!(hi & 0x01)) REG_SH1 |= 0x01;
    else REG_SH1 &= ~0x01;
    
    uint8_t REG_SH2 = ((KB[6] << 2) & 0xF0) | ((KB[7] >> 4) & 0x0F);
    if (!(hi & 0x20)) REG_SH2 |= 0x20;
    else REG_SH2 &= ~0x20;
    if (!(hi & 0x10)) REG_SH2 |= 0x10;
    else REG_SH2 &= ~0x10;
    
    // Update KB[5] and KB[6] with REG_SH values
    KB[5] = (KB[5] & 0x33) | ((REG_SH1 & 0x0C) << 2) | ((REG_SH1 & 0x03) << 6);
    KB[6] = (KB[6] & 0x3C) | ((REG_SH2 & 0xF0) >> 2) | ((REG_SH2 & 0x0F) << 4);
}

static void subaru_reconstruct_key(SubGhzProtocolEncoderSubaru *instance)
{
    uint8_t KB[8] = {0};
    
    // Set button (lower 4 bits of KB[0])
    KB[0] = instance->generic.btn & 0x0F;
    
    // Set serial (KB[1], KB[2], KB[3])
    KB[1] = (instance->generic.serial >> 16) & 0xFF;
    KB[2] = (instance->generic.serial >> 8) & 0xFF;
    KB[3] = instance->generic.serial & 0xFF;
    
    // Set counter using the complex encoding
    subaru_encode_count(KB, (uint16_t)instance->generic.cnt);
    
    // Reconstruct the 64-bit key
    instance->generic.data = ((uint64_t)KB[0] << 56) | ((uint64_t)KB[1] << 48) |
                             ((uint64_t)KB[2] << 40) | ((uint64_t)KB[3] << 32) |
                             ((uint64_t)KB[4] << 24) | ((uint64_t)KB[5] << 16) |
                             ((uint64_t)KB[6] << 8) | ((uint64_t)KB[7]);
}

void *subghz_protocol_encoder_subaru_alloc(SubGhzEnvironment *environment)
{
    UNUSED(environment);
    SubGhzProtocolEncoderSubaru *instance = malloc(sizeof(SubGhzProtocolEncoderSubaru));
    instance->base.protocol = &subaru_protocol;
    return instance;
}

void subghz_protocol_encoder_subaru_free(void *context)
{
    furi_assert(context);
    SubGhzProtocolEncoderSubaru *instance = context;
    free(instance);
}

SubGhzProtocolStatus subghz_protocol_encoder_subaru_deserialize(void *context, FlipperFormat *flipper_format)
{
    furi_assert(context);
    SubGhzProtocolEncoderSubaru *instance = context;

    SubGhzProtocolStatus res = SubGhzProtocolStatusError;
    do
    {
        // Read protocol name and validate
        FuriString *temp_str = furi_string_alloc();
        if (!flipper_format_read_string(flipper_format, "Protocol", temp_str))
        {
            FURI_LOG_E(TAG, "Missing Protocol");
            furi_string_free(temp_str);
            break;
        }

        if (!furi_string_equal(temp_str, instance->base.protocol->name))
        {
            FURI_LOG_E(TAG, "Wrong protocol %s != %s",
                       furi_string_get_cstr(temp_str), instance->base.protocol->name);
            furi_string_free(temp_str);
            break;
        }
        furi_string_free(temp_str);

        // Read bit count as uint32_t first
        uint32_t bit_count_temp;
        if (!flipper_format_read_uint32(flipper_format, "Bit", &bit_count_temp, 1))
        {
            FURI_LOG_E(TAG, "Missing Bit");
            break;
        }
        instance->generic.data_count_bit = (uint16_t)bit_count_temp;

        if (instance->generic.data_count_bit != 64)
        {
            FURI_LOG_E(TAG, "Wrong bit count %u", instance->generic.data_count_bit);
            break;
        }

        // Read key data
        temp_str = furi_string_alloc();
        if (!flipper_format_read_string(flipper_format, "Key", temp_str))
        {
            FURI_LOG_E(TAG, "Missing Key");
            furi_string_free(temp_str);
            break;
        }

        // Parse hex key
        uint64_t key = 0;
        if (sscanf(furi_string_get_cstr(temp_str), "%016llX", &key) != 1)
        {
            FURI_LOG_E(TAG, "Failed to parse Key");
            furi_string_free(temp_str);
            break;
        }
        furi_string_free(temp_str);

        instance->generic.data = key;

        // Read serial, button, counter
        flipper_format_read_uint32(flipper_format, "Serial", &instance->generic.serial, 1);
        
        uint32_t btn_temp;
        if (flipper_format_read_uint32(flipper_format, "Btn", &btn_temp, 1))
        {
            instance->generic.btn = (uint8_t)btn_temp;
        }
        
        flipper_format_read_uint32(flipper_format, "Cnt", &instance->generic.cnt, 1);

        // Reconstruct the key with updated values
        subaru_reconstruct_key(instance);

        res = SubGhzProtocolStatusOk;
    } while (false);

    return res;
}

void subghz_protocol_encoder_subaru_stop(void *context)
{
    UNUSED(context);
}

LevelDuration subghz_protocol_encoder_subaru_yield(void *context)
{
    furi_assert(context);
    SubGhzProtocolEncoderSubaru *instance = context;

    // Static variables to maintain state between calls
    static uint8_t bit_index = 0;
    static bool is_high = true;
    static uint8_t preamble_count = 0;

    // Send preamble (long pulses)
    if (preamble_count < 24) // Subaru needs longer preamble
    {
        if (is_high)
        {
            is_high = false;
            return level_duration_make(true, subghz_protocol_subaru_const.te_long);
        }
        else
        {
            is_high = true;
            preamble_count++;
            return level_duration_make(false, subghz_protocol_subaru_const.te_long);
        }
    }

    // Send gap
    if (preamble_count == 24)
    {
        preamble_count++;
        return level_duration_make(true, 3000); // Gap duration
    }

    if (preamble_count == 25)
    {
        preamble_count++;
        return level_duration_make(false, 3000); // Gap duration
    }

    // Send sync
    if (preamble_count == 26)
    {
        preamble_count++;
        return level_duration_make(true, subghz_protocol_subaru_const.te_long);
    }

    if (preamble_count == 27)
    {
        preamble_count++;
        bit_index = 0;
        is_high = true;
        return level_duration_make(false, subghz_protocol_subaru_const.te_long);
    }

    // Send data bits
    if (bit_index < instance->generic.data_count_bit)
    {
        bool current_bit = (instance->generic.data >> (63 - bit_index)) & 1;
        
        if (is_high)
        {
            is_high = false;
            // Subaru encoding: 1 = short high, 0 = long high
            uint32_t duration = current_bit ? subghz_protocol_subaru_const.te_short : subghz_protocol_subaru_const.te_long;
            return level_duration_make(true, duration);
        }
        else
        {
            is_high = true;
            bit_index++;
            // LOW pulse duration matches the previous HIGH duration
            uint32_t duration = current_bit ? subghz_protocol_subaru_const.te_short : subghz_protocol_subaru_const.te_long;
            
            if (bit_index >= instance->generic.data_count_bit)
            {
                // Reset for next transmission
                preamble_count = 0;
                bit_index = 0;
                return level_duration_make(false, duration + 5000); // Add gap
            }
            
            return level_duration_make(false, duration);
        }
    }

    return level_duration_reset();
}

const SubGhzProtocolEncoder subghz_protocol_subaru_encoder = {
    .alloc = subghz_protocol_encoder_subaru_alloc,
    .free = subghz_protocol_encoder_subaru_free,
    .deserialize = subghz_protocol_encoder_subaru_deserialize,
    .stop = subghz_protocol_encoder_subaru_stop,
    .yield = subghz_protocol_encoder_subaru_yield,
};

const SubGhzProtocol subaru_protocol = {
    .name = SUBARU_PROTOCOL_NAME,
    .type = SubGhzProtocolTypeDynamic,
    .flag = SubGhzProtocolFlag_433 | SubGhzProtocolFlag_AM | SubGhzProtocolFlag_Decodable | SubGhzProtocolFlag_Save | SubGhzProtocolFlag_Send,
    .decoder = &subghz_protocol_subaru_decoder,
    .encoder = &subghz_protocol_subaru_encoder,
};

static void subaru_decode_count(const uint8_t *KB, uint16_t *count)
{
    uint8_t lo = 0;
    if ((KB[4] & 0x40) == 0)
        lo |= 0x01;
    if ((KB[4] & 0x80) == 0)
        lo |= 0x02;
    if ((KB[5] & 0x01) == 0)
        lo |= 0x04;
    if ((KB[5] & 0x02) == 0)
        lo |= 0x08;
    if ((KB[6] & 0x01) == 0)
        lo |= 0x10;
    if ((KB[6] & 0x02) == 0)
        lo |= 0x20;
    if ((KB[5] & 0x40) == 0)
        lo |= 0x40;
    if ((KB[5] & 0x80) == 0)
        lo |= 0x80;

    uint8_t REG_SH1 = (KB[7] << 4) & 0xF0;
    if (KB[5] & 0x04)
        REG_SH1 |= 0x04;
    if (KB[5] & 0x08)
        REG_SH1 |= 0x08;
    if (KB[6] & 0x80)
        REG_SH1 |= 0x02;
    if (KB[6] & 0x40)
        REG_SH1 |= 0x01;

    uint8_t REG_SH2 = ((KB[6] << 2) & 0xF0) | ((KB[7] >> 4) & 0x0F);

    uint8_t SER0 = KB[3];
    uint8_t SER1 = KB[1];
    uint8_t SER2 = KB[2];

    uint8_t total_rot = 4 + lo;
    for (uint8_t i = 0; i < total_rot; ++i)
    {
        uint8_t t_bit = (SER0 >> 7) & 1;
        SER0 = ((SER0 << 1) & 0xFE) | ((SER1 >> 7) & 1);
        SER1 = ((SER1 << 1) & 0xFE) | ((SER2 >> 7) & 1);
        SER2 = ((SER2 << 1) & 0xFE) | t_bit;
    }

    uint8_t T1 = SER1 ^ REG_SH1;
    uint8_t T2 = SER2 ^ REG_SH2;

    uint8_t hi = 0;
    if ((T1 & 0x10) == 0)
        hi |= 0x04;
    if ((T1 & 0x20) == 0)
        hi |= 0x08;
    if ((T2 & 0x80) == 0)
        hi |= 0x02;
    if ((T2 & 0x40) == 0)
        hi |= 0x01;
    if ((T1 & 0x01) == 0)
        hi |= 0x40;
    if ((T1 & 0x02) == 0)
        hi |= 0x80;
    if ((T2 & 0x08) == 0)
        hi |= 0x20;
    if ((T2 & 0x04) == 0)
        hi |= 0x10;

    *count = ((hi << 8) | lo) & 0xFFFF;
}

static void subaru_add_bit(SubGhzProtocolDecoderSubaru *instance, bool bit)
{
    if (instance->bit_count < 64)
    {
        uint8_t byte_idx = instance->bit_count / 8;
        uint8_t bit_idx = 7 - (instance->bit_count % 8);
        if (bit)
        {
            instance->data[byte_idx] |= (1 << bit_idx);
        }
        else
        {
            instance->data[byte_idx] &= ~(1 << bit_idx);
        }
        instance->bit_count++;
    }
}

static bool subaru_process_data(SubGhzProtocolDecoderSubaru *instance)
{
    if (instance->bit_count < 64)
    {
        return false;
    }

    uint8_t *b = instance->data;

    instance->key = ((uint64_t)b[0] << 56) | ((uint64_t)b[1] << 48) |
                    ((uint64_t)b[2] << 40) | ((uint64_t)b[3] << 32) |
                    ((uint64_t)b[4] << 24) | ((uint64_t)b[5] << 16) |
                    ((uint64_t)b[6] << 8) | ((uint64_t)b[7]);

    instance->serial = ((uint32_t)b[1] << 16) | ((uint32_t)b[2] << 8) | b[3];
    instance->button = b[0] & 0x0F;
    subaru_decode_count(b, &instance->count);

    return true;
}

void *subghz_protocol_decoder_subaru_alloc(SubGhzEnvironment *environment)
{
    UNUSED(environment);
    SubGhzProtocolDecoderSubaru *instance = malloc(sizeof(SubGhzProtocolDecoderSubaru));
    instance->base.protocol = &subaru_protocol;
    instance->generic.protocol_name = instance->base.protocol->name;
    return instance;
}

void subghz_protocol_decoder_subaru_free(void *context)
{
    furi_assert(context);
    SubGhzProtocolDecoderSubaru *instance = context;
    free(instance);
}

void subghz_protocol_decoder_subaru_reset(void *context)
{
    furi_assert(context);
    SubGhzProtocolDecoderSubaru *instance = context;
    instance->decoder.parser_step = SubaruDecoderStepReset;
    instance->decoder.te_last = 0;
    instance->header_count = 0;
    instance->bit_count = 0;
    memset(instance->data, 0, sizeof(instance->data));
}

void subghz_protocol_decoder_subaru_feed(void *context, bool level, uint32_t duration)
{
    furi_assert(context);
    SubGhzProtocolDecoderSubaru *instance = context;

    switch (instance->decoder.parser_step)
    {
    case SubaruDecoderStepReset:
        if (level && DURATION_DIFF(duration, subghz_protocol_subaru_const.te_long) < subghz_protocol_subaru_const.te_delta)
        {
            instance->decoder.parser_step = SubaruDecoderStepCheckPreamble;
            instance->decoder.te_last = duration;
            instance->header_count = 1;
        }
        break;

    case SubaruDecoderStepCheckPreamble:
        if (!level)
        {
            if (DURATION_DIFF(duration, subghz_protocol_subaru_const.te_long) < subghz_protocol_subaru_const.te_delta)
            {
                instance->header_count++;
            }
            else if (duration > 2000 && duration < 3500)
            {
                if (instance->header_count > 20)
                {
                    instance->decoder.parser_step = SubaruDecoderStepFoundGap;
                }
                else
                {
                    instance->decoder.parser_step = SubaruDecoderStepReset;
                }
            }
            else
            {
                instance->decoder.parser_step = SubaruDecoderStepReset;
            }
        }
        else
        {
            if (DURATION_DIFF(duration, subghz_protocol_subaru_const.te_long) < subghz_protocol_subaru_const.te_delta)
            {
                instance->decoder.te_last = duration;
                instance->header_count++;
            }
            else
            {
                instance->decoder.parser_step = SubaruDecoderStepReset;
            }
        }
        break;

    case SubaruDecoderStepFoundGap:
        if (level && duration > 2000 && duration < 3500)
        {
            instance->decoder.parser_step = SubaruDecoderStepFoundSync;
        }
        else
        {
            instance->decoder.parser_step = SubaruDecoderStepReset;
        }
        break;

    case SubaruDecoderStepFoundSync:
        if (!level && DURATION_DIFF(duration, subghz_protocol_subaru_const.te_long) < subghz_protocol_subaru_const.te_delta)
        {
            instance->decoder.parser_step = SubaruDecoderStepSaveDuration;
            instance->bit_count = 0;
            memset(instance->data, 0, sizeof(instance->data));
        }
        else
        {
            instance->decoder.parser_step = SubaruDecoderStepReset;
        }
        break;

    case SubaruDecoderStepSaveDuration:
        if (level)
        {
            // HIGH pulse duration encodes the bit:
            // Short HIGH (~800µs) = 1
            // Long HIGH (~1600µs) = 0
            if (DURATION_DIFF(duration, subghz_protocol_subaru_const.te_short) < subghz_protocol_subaru_const.te_delta)
            {
                // Short HIGH = bit 1
                subaru_add_bit(instance, true);
                instance->decoder.te_last = duration;
                instance->decoder.parser_step = SubaruDecoderStepCheckDuration;
            }
            else if (DURATION_DIFF(duration, subghz_protocol_subaru_const.te_long) < subghz_protocol_subaru_const.te_delta)
            {
                // Long HIGH = bit 0
                subaru_add_bit(instance, false);
                instance->decoder.te_last = duration;
                instance->decoder.parser_step = SubaruDecoderStepCheckDuration;
            }
            else if (duration > 3000)
            {
                // End of transmission
                if (instance->bit_count >= 64)
                {
                    if (subaru_process_data(instance))
                    {
                        instance->generic.data = instance->key;
                        instance->generic.data_count_bit = 64;
                        instance->generic.serial = instance->serial;
                        instance->generic.btn = instance->button;
                        instance->generic.cnt = instance->count;

                        if (instance->base.callback)
                        {
                            instance->base.callback(&instance->base, instance->base.context);
                        }
                    }
                }
                instance->decoder.parser_step = SubaruDecoderStepReset;
            }
            else
            {
                instance->decoder.parser_step = SubaruDecoderStepReset;
            }
        }
        else
        {
            instance->decoder.parser_step = SubaruDecoderStepReset;
        }
        break;

    case SubaruDecoderStepCheckDuration:
        if (!level)
        {
            // LOW pulse - just validates timing, doesn't encode bit
            if (DURATION_DIFF(duration, subghz_protocol_subaru_const.te_short) < subghz_protocol_subaru_const.te_delta ||
                DURATION_DIFF(duration, subghz_protocol_subaru_const.te_long) < subghz_protocol_subaru_const.te_delta)
            {
                instance->decoder.parser_step = SubaruDecoderStepSaveDuration;
            }
            else if (duration > 3000)
            {
                // Gap - end of packet
                if (instance->bit_count >= 64)
                {
                    if (subaru_process_data(instance))
                    {
                        instance->generic.data = instance->key;
                        instance->generic.data_count_bit = 64;
                        instance->generic.serial = instance->serial;
                        instance->generic.btn = instance->button;
                        instance->generic.cnt = instance->count;

                        if (instance->base.callback)
                        {
                            instance->base.callback(&instance->base, instance->base.context);
                        }
                    }
                }
                instance->decoder.parser_step = SubaruDecoderStepReset;
            }
            else
            {
                instance->decoder.parser_step = SubaruDecoderStepReset;
            }
        }
        else
        {
            instance->decoder.parser_step = SubaruDecoderStepReset;
        }
        break;
    }
}

uint8_t subghz_protocol_decoder_subaru_get_hash_data(void *context)
{
    furi_assert(context);
    SubGhzProtocolDecoderSubaru *instance = context;
    return subghz_protocol_blocks_get_hash_data(
        &instance->decoder, (instance->decoder.decode_count_bit / 8) + 1);
}

SubGhzProtocolStatus subghz_protocol_decoder_subaru_serialize(
    void *context,
    FlipperFormat *flipper_format,
    SubGhzRadioPreset *preset)
{
    furi_assert(context);
    SubGhzProtocolDecoderSubaru *instance = context;

    SubGhzProtocolStatus ret = SubGhzProtocolStatusError;

    ret = subghz_block_generic_serialize(&instance->generic, flipper_format, preset);

    if (ret == SubGhzProtocolStatusOk)
    {
        // Subaru specific data - the counter uses special decoding
        flipper_format_write_uint32(flipper_format, "Serial", &instance->serial, 1);

        uint32_t temp = instance->button;
        flipper_format_write_uint32(flipper_format, "Btn", &temp, 1);

        temp = instance->count;
        flipper_format_write_uint32(flipper_format, "Cnt", &temp, 1);

        // Save raw data for exact reproduction
        uint32_t raw_high = (uint32_t)(instance->key >> 32);
        uint32_t raw_low = (uint32_t)(instance->key & 0xFFFFFFFF);
        flipper_format_write_uint32(flipper_format, "DataHi", &raw_high, 1);
        flipper_format_write_uint32(flipper_format, "DataLo", &raw_low, 1);
    }

    return ret;
}

SubGhzProtocolStatus subghz_protocol_decoder_subaru_deserialize(void *context, FlipperFormat *flipper_format)
{
    furi_assert(context);
    SubGhzProtocolDecoderSubaru *instance = context;
    return subghz_block_generic_deserialize_check_count_bit(
        &instance->generic, flipper_format, subghz_protocol_subaru_const.min_count_bit_for_found);
}

void subghz_protocol_decoder_subaru_get_string(void *context, FuriString *output)
{
    furi_assert(context);
    SubGhzProtocolDecoderSubaru *instance = context;

    uint32_t key_hi = (uint32_t)(instance->key >> 32);
    uint32_t key_lo = (uint32_t)(instance->key & 0xFFFFFFFF);

    furi_string_cat_printf(
        output,
        "%s %dbit\r\n"
        "Key:%08lX%08lX\r\n"
        "Sn:%06lX Btn:%X Cnt:%04X\r\n",
        instance->generic.protocol_name,
        instance->generic.data_count_bit,
        key_hi,
        key_lo,
        instance->serial,
        instance->button,
        instance->count);
}
