#include "vw.h"

#define TAG "VWProtocol"

static const SubGhzBlockConst subghz_protocol_vw_const = {
    .te_short = 500,
    .te_long = 1000,
    .te_delta = 120,
    .min_count_bit_for_found = 80,
};

typedef struct SubGhzProtocolDecoderVw
{
    SubGhzProtocolDecoderBase base;
    SubGhzBlockDecoder decoder;
    SubGhzBlockGeneric generic;

    ManchesterState manchester_state;
    uint64_t data_2; // Additional 16 bits (type byte + check byte)
} SubGhzProtocolDecoderVw;

typedef struct SubGhzProtocolEncoderVw
{
    SubGhzProtocolEncoderBase base;
    SubGhzProtocolBlockEncoder encoder;
    SubGhzBlockGeneric generic;
} SubGhzProtocolEncoderVw;

typedef enum
{
    VwDecoderStepReset = 0,
    VwDecoderStepFoundSync,
    VwDecoderStepFoundStart1,
    VwDecoderStepFoundStart2,
    VwDecoderStepFoundStart3,
    VwDecoderStepFoundData,
} VwDecoderStep;

const SubGhzProtocolDecoder subghz_protocol_vw_decoder = {
    .alloc = subghz_protocol_decoder_vw_alloc,
    .free = subghz_protocol_decoder_vw_free,
    .feed = subghz_protocol_decoder_vw_feed,
    .reset = subghz_protocol_decoder_vw_reset,
    .get_hash_data = subghz_protocol_decoder_vw_get_hash_data,
    .serialize = subghz_protocol_decoder_vw_serialize,
    .deserialize = subghz_protocol_decoder_vw_deserialize,
    .get_string = subghz_protocol_decoder_vw_get_string,
};

// Encoder implementation
static void vw_reconstruct_data(SubGhzProtocolEncoderVw *instance, uint8_t type, uint8_t check)
{
    // Reconstruct the 80-bit data structure
    // Format: [Type (8 bits)][Data (64 bits)][Check (8 bits)]
    
    // Set type byte (bits 72-79)
    instance->generic.data_count_bit = 80;
    
    // The data is stored in generic.data (64 bits) and data_2 (16 bits)
    // Type byte goes into the high 8 bits of data_2
    // Check byte goes into the low 8 bits of data_2
    
    // For encoding, we'll reconstruct the full 80-bit structure
    uint64_t full_data = 0;
    
    // Add type byte (MSB)
    full_data = (uint64_t)type << 72;
    
    // Add main 64-bit data
    full_data |= instance->generic.data;
    
    // Add check byte (LSB)
    full_data |= (uint64_t)check;
    
    // Store in generic.data for transmission (we'll handle the 80-bit structure in yield)
    instance->generic.data = full_data;
    
    // Store type and check in data_2 for serialization
    instance->data_2 = ((uint64_t)type << 8) | check;
}

static uint8_t vw_calculate_check_byte(uint32_t serial, uint8_t button)
{
    // Simple checksum calculation for VW protocol
    // This is a basic implementation - may need adjustment based on actual protocol
    uint8_t check = 0;
    
    check ^= (serial >> 16) & 0xFF;
    check ^= (serial >> 8) & 0xFF;
    check ^= serial & 0xFF;
    check ^= button << 4; // Button is in upper 4 bits
    
    return check;
}

void *subghz_protocol_encoder_vw_alloc(SubGhzEnvironment *environment)
{
    UNUSED(environment);
    SubGhzProtocolEncoderVw *instance = malloc(sizeof(SubGhzProtocolEncoderVw));
    instance->base.protocol = &vw_protocol;
    return instance;
}

void subghz_protocol_encoder_vw_free(void *context)
{
    furi_assert(context);
    SubGhzProtocolEncoderVw *instance = context;
    free(instance);
}

SubGhzProtocolStatus subghz_protocol_encoder_vw_deserialize(void *context, FlipperFormat *flipper_format)
{
    furi_assert(context);
    SubGhzProtocolEncoderVw *instance = context;

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

        if (instance->generic.data_count_bit != 80)
        {
            FURI_LOG_E(TAG, "Wrong bit count %u", instance->generic.data_count_bit);
            break;
        }

        // Read key data (this will be the 64-bit main data)
        temp_str = furi_string_alloc();
        if (!flipper_format_read_string(flipper_format, "Key", temp_str))
        {
            FURI_LOG_E(TAG, "Missing Key");
            furi_string_free(temp_str);
            break;
        }

        // Parse hex key (64 bits)
        uint64_t key = 0;
        if (sscanf(furi_string_get_cstr(temp_str), "%016llX", &key) != 1)
        {
            FURI_LOG_E(TAG, "Failed to parse Key");
            furi_string_free(temp_str);
            break;
        }
        furi_string_free(temp_str);

        instance->generic.data = key;

        // Read type and check bytes
        uint32_t type, check;
        if (flipper_format_read_uint32(flipper_format, "Type", &type, 1))
        {
            // Type byte
        }
        else
        {
            type = 0x00; // Default type
        }
        
        if (flipper_format_read_uint32(flipper_format, "Check", &check, 1))
        {
            // Check byte
        }
        else
        {
            // Calculate check byte from button
            uint32_t btn;
            if (flipper_format_read_uint32(flipper_format, "Btn", &btn, 1))
            {
                check = vw_calculate_check_byte(instance->generic.serial, (uint8_t)btn);
            }
            else
            {
                check = 0x00; // Default check
            }
        }

        // Reconstruct the full data structure
        vw_reconstruct_data(instance, (uint8_t)type, (uint8_t)check);

        res = SubGhzProtocolStatusOk;
    } while (false);

    return res;
}

void subghz_protocol_encoder_vw_stop(void *context)
{
    UNUSED(context);
}

LevelDuration subghz_protocol_encoder_vw_yield(void *context)
{
    furi_assert(context);
    SubGhzProtocolEncoderVw *instance = context;

    // Static variables to maintain state between calls
    static uint8_t bit_index = 0;
    static bool is_high = true;
    static uint8_t preamble_count = 0;
    static uint8_t sync_step = 0;

    // Send preamble (short pulses, ~43 times)
    if (preamble_count < 43)
    {
        if (is_high)
        {
            is_high = false;
            return level_duration_make(true, subghz_protocol_vw_const.te_short);
        }
        else
        {
            is_high = true;
            preamble_count++;
            return level_duration_make(false, subghz_protocol_vw_const.te_short);
        }
    }

    // Send sync pattern
    if (sync_step == 0)
    {
        sync_step++;
        return level_duration_make(true, subghz_protocol_vw_const.te_long);
    }
    else if (sync_step == 1)
    {
        sync_step++;
        return level_duration_make(false, subghz_protocol_vw_const.te_short);
    }
    else if (sync_step == 2)
    {
        sync_step++;
        return level_duration_make(true, subghz_protocol_vw_const.te_med);
    }
    else if (sync_step == 3)
    {
        sync_step++;
        return level_duration_make(false, subghz_protocol_vw_const.te_med);
    }
    else if (sync_step == 4)
    {
        sync_step++;
        bit_index = 0;
        is_high = true;
        return level_duration_make(true, subghz_protocol_vw_const.te_short);
    }

    // Send data bits (Manchester encoding)
    if (bit_index < instance->generic.data_count_bit)
    {
        // Extract the current bit from the 80-bit data
        uint8_t byte_index = bit_index / 8;
        uint8_t bit_in_byte = 7 - (bit_index % 8);
        bool current_bit;
        
        if (bit_index < 72)
        {
            // Main 64-bit data + 8 bits of type
            uint64_t temp_data = instance->generic.data;
            current_bit = (temp_data >> (71 - bit_index)) & 1;
        }
        else
        {
            // Check byte (last 8 bits)
            uint8_t check_byte = instance->data_2 & 0xFF;
            current_bit = (check_byte >> (7 - (bit_index - 72))) & 1;
        }
        
        if (is_high)
        {
            is_high = false;
            // Manchester encoding: 1 = short high, 0 = long high
            uint32_t duration = current_bit ? subghz_protocol_vw_const.te_short : subghz_protocol_vw_const.te_long;
            return level_duration_make(true, duration);
        }
        else
        {
            is_high = true;
            bit_index++;
            // LOW pulse duration matches the previous HIGH duration
            uint32_t duration = current_bit ? subghz_protocol_vw_const.te_short : subghz_protocol_vw_const.te_long;
            
            if (bit_index >= instance->generic.data_count_bit)
            {
                // Reset for next transmission
                preamble_count = 0;
                sync_step = 0;
                bit_index = 0;
                return level_duration_make(false, duration + 3000); // Add gap
            }
            
            return level_duration_make(false, duration);
        }
    }

    return level_duration_reset();
}

const SubGhzProtocolEncoder subghz_protocol_vw_encoder = {
    .alloc = subghz_protocol_encoder_vw_alloc,
    .free = subghz_protocol_encoder_vw_free,
    .deserialize = subghz_protocol_encoder_vw_deserialize,
    .stop = subghz_protocol_encoder_vw_stop,
    .yield = subghz_protocol_encoder_vw_yield,
};

const SubGhzProtocol vw_protocol = {
    .name = VW_PROTOCOL_NAME,
    .type = SubGhzProtocolTypeDynamic,
    .flag = SubGhzProtocolFlag_433 | SubGhzProtocolFlag_AM | SubGhzProtocolFlag_Decodable | SubGhzProtocolFlag_Save | SubGhzProtocolFlag_Send,
    .decoder = &subghz_protocol_vw_decoder,
    .encoder = &subghz_protocol_vw_encoder,
};

// Fixed manchester_advance for VW protocol
static bool vw_manchester_advance(
    ManchesterState state,
    ManchesterEvent event,
    ManchesterState *next_state,
    bool *data)
{

    bool result = false;
    ManchesterState new_state = ManchesterStateMid1;

    if (event == ManchesterEventReset)
    {
        new_state = ManchesterStateMid1;
    }
    else if (state == ManchesterStateMid0 || state == ManchesterStateMid1)
    {
        if (event == ManchesterEventShortHigh)
        {
            new_state = ManchesterStateStart1;
        }
        else if (event == ManchesterEventShortLow)
        {
            new_state = ManchesterStateStart0;
        }
        else
        {
            new_state = ManchesterStateMid1;
        }
    }
    else if (state == ManchesterStateStart1)
    {
        if (event == ManchesterEventShortLow)
        {
            new_state = ManchesterStateMid1;
            result = true;
            if (data)
                *data = true;
        }
        else if (event == ManchesterEventLongLow)
        {
            new_state = ManchesterStateStart0;
            result = true;
            if (data)
                *data = true;
        }
        else
        {
            new_state = ManchesterStateMid1;
        }
    }
    else if (state == ManchesterStateStart0)
    {
        if (event == ManchesterEventShortHigh)
        {
            new_state = ManchesterStateMid0;
            result = true;
            if (data)
                *data = false;
        }
        else if (event == ManchesterEventLongHigh)
        {
            new_state = ManchesterStateStart1;
            result = true;
            if (data)
                *data = false;
        }
        else
        {
            new_state = ManchesterStateMid1;
        }
    }

    *next_state = new_state;
    return result;
}

static uint8_t vw_get_bit_index(uint8_t bit)
{
    uint8_t bit_index = 0;

    if (bit < 72 && bit >= 8)
    {
        // use generic.data (bytes 1-8)
        bit_index = bit - 8;
    }
    else
    {
        // use data_2
        if (bit >= 72)
        {
            bit_index = bit - 64; // byte 0 = type
        }
        if (bit < 8)
        {
            bit_index = bit; // byte 9 = check digit
        }
        bit_index |= 0x80; // mark for data_2
    }

    return bit_index;
}

static void vw_add_bit(SubGhzProtocolDecoderVw *instance, bool level)
{
    if (instance->generic.data_count_bit >= subghz_protocol_vw_const.min_count_bit_for_found)
    {
        return;
    }

    uint8_t bit_index_full = subghz_protocol_vw_const.min_count_bit_for_found - 1 - instance->generic.data_count_bit;
    uint8_t bit_index_masked = vw_get_bit_index(bit_index_full);
    uint8_t bit_index = bit_index_masked & 0x7F;

    if (bit_index_masked & 0x80)
    {
        // use data_2
        if (level)
        {
            instance->data_2 |= (1ULL << bit_index);
        }
        else
        {
            instance->data_2 &= ~(1ULL << bit_index);
        }
    }
    else
    {
        // use data
        if (level)
        {
            instance->generic.data |= (1ULL << bit_index);
        }
        else
        {
            instance->generic.data &= ~(1ULL << bit_index);
        }
    }

    instance->generic.data_count_bit++;

    if (instance->generic.data_count_bit >= subghz_protocol_vw_const.min_count_bit_for_found)
    {
        if (instance->base.callback)
        {
            instance->base.callback(&instance->base, instance->base.context);
        }
    }
}

void *subghz_protocol_decoder_vw_alloc(SubGhzEnvironment *environment)
{
    UNUSED(environment);
    SubGhzProtocolDecoderVw *instance = malloc(sizeof(SubGhzProtocolDecoderVw));
    instance->base.protocol = &vw_protocol;
    instance->generic.protocol_name = instance->base.protocol->name;
    return instance;
}

void subghz_protocol_decoder_vw_free(void *context)
{
    furi_assert(context);
    SubGhzProtocolDecoderVw *instance = context;
    free(instance);
}

void subghz_protocol_decoder_vw_reset(void *context)
{
    furi_assert(context);
    SubGhzProtocolDecoderVw *instance = context;
    instance->decoder.parser_step = VwDecoderStepReset;
    instance->generic.data_count_bit = 0;
    instance->generic.data = 0;
    instance->data_2 = 0;
    instance->manchester_state = ManchesterStateMid1;
}

void subghz_protocol_decoder_vw_feed(void *context, bool level, uint32_t duration)
{
    furi_assert(context);
    SubGhzProtocolDecoderVw *instance = context;

    uint32_t te_short = subghz_protocol_vw_const.te_short;
    uint32_t te_long = subghz_protocol_vw_const.te_long;
    uint32_t te_delta = subghz_protocol_vw_const.te_delta;
    uint32_t te_med = (te_long + te_short) / 2;
    uint32_t te_end = te_long * 5;

    ManchesterEvent event = ManchesterEventReset;

    switch (instance->decoder.parser_step)
    {
    case VwDecoderStepReset:
        if (DURATION_DIFF(duration, te_short) < te_delta)
        {
            instance->decoder.parser_step = VwDecoderStepFoundSync;
        }
        break;

    case VwDecoderStepFoundSync:
        if (DURATION_DIFF(duration, te_short) < te_delta)
        {
            // Stay - sync pattern repeats ~43 times
            break;
        }

        if (level && DURATION_DIFF(duration, te_long) < te_delta)
        {
            instance->decoder.parser_step = VwDecoderStepFoundStart1;
            break;
        }

        instance->decoder.parser_step = VwDecoderStepReset;
        break;

    case VwDecoderStepFoundStart1:
        if (!level && DURATION_DIFF(duration, te_short) < te_delta)
        {
            instance->decoder.parser_step = VwDecoderStepFoundStart2;
            break;
        }

        instance->decoder.parser_step = VwDecoderStepReset;
        break;

    case VwDecoderStepFoundStart2:
        if (level && DURATION_DIFF(duration, te_med) < te_delta)
        {
            instance->decoder.parser_step = VwDecoderStepFoundStart3;
            break;
        }

        instance->decoder.parser_step = VwDecoderStepReset;
        break;

    case VwDecoderStepFoundStart3:
        if (DURATION_DIFF(duration, te_med) < te_delta)
        {
            // Stay - med pattern repeats
            break;
        }

        if (level && DURATION_DIFF(duration, te_short) < te_delta)
        {
            // Start data collection
            vw_manchester_advance(
                instance->manchester_state,
                ManchesterEventReset,
                &instance->manchester_state,
                NULL);
            vw_manchester_advance(
                instance->manchester_state,
                ManchesterEventShortHigh,
                &instance->manchester_state,
                NULL);
            instance->generic.data_count_bit = 0;
            instance->generic.data = 0;
            instance->data_2 = 0;
            instance->decoder.parser_step = VwDecoderStepFoundData;
            break;
        }

        instance->decoder.parser_step = VwDecoderStepReset;
        break;

    case VwDecoderStepFoundData:
        if (DURATION_DIFF(duration, te_short) < te_delta)
        {
            event = level ? ManchesterEventShortHigh : ManchesterEventShortLow;
        }

        if (DURATION_DIFF(duration, te_long) < te_delta)
        {
            event = level ? ManchesterEventLongHigh : ManchesterEventLongLow;
        }

        // Last bit can be arbitrarily long
        if (instance->generic.data_count_bit == subghz_protocol_vw_const.min_count_bit_for_found - 1 &&
            !level && duration > te_end)
        {
            event = ManchesterEventShortLow;
        }

        if (event == ManchesterEventReset)
        {
            subghz_protocol_decoder_vw_reset(instance);
        }
        else
        {
            bool new_level;
            if (vw_manchester_advance(
                    instance->manchester_state,
                    event,
                    &instance->manchester_state,
                    &new_level))
            {
                vw_add_bit(instance, new_level);
            }
        }
        break;
    }
}

uint8_t subghz_protocol_decoder_vw_get_hash_data(void *context)
{
    furi_assert(context);
    SubGhzProtocolDecoderVw *instance = context;
    return subghz_protocol_blocks_get_hash_data(
        &instance->decoder, (instance->decoder.decode_count_bit / 8) + 1);
}

SubGhzProtocolStatus subghz_protocol_decoder_vw_serialize(
    void *context,
    FlipperFormat *flipper_format,
    SubGhzRadioPreset *preset)
{
    furi_assert(context);
    SubGhzProtocolDecoderVw *instance = context;

    SubGhzProtocolStatus ret = SubGhzProtocolStatusError;

    ret = subghz_block_generic_serialize(&instance->generic, flipper_format, preset);

    if (ret == SubGhzProtocolStatusOk)
    {
        // Add VW-specific data
        uint32_t type = (instance->data_2 >> 8) & 0xFF;
        uint32_t check = instance->data_2 & 0xFF;
        uint32_t btn = (check >> 4) & 0xF;

        flipper_format_write_uint32(flipper_format, "Type", &type, 1);
        flipper_format_write_uint32(flipper_format, "Check", &check, 1);
        flipper_format_write_uint32(flipper_format, "Btn", &btn, 1);
    }

    return ret;
}

SubGhzProtocolStatus subghz_protocol_decoder_vw_deserialize(void *context, FlipperFormat *flipper_format)
{
    furi_assert(context);
    SubGhzProtocolDecoderVw *instance = context;
    return subghz_block_generic_deserialize_check_count_bit(
        &instance->generic, flipper_format, subghz_protocol_vw_const.min_count_bit_for_found);
}

static const char *vw_get_button_name(uint8_t btn)
{
    switch (btn)
    {
    case 0x1:
        return "UNLOCK";
    case 0x2:
        return "LOCK";
    case 0x3:
        return "Un+Lk";
    case 0x4:
        return "TRUNK";
    case 0x5:
        return "Un+Tr";
    case 0x6:
        return "Lk+Tr";
    case 0x7:
        return "Un+Lk+Tr";
    case 0x8:
        return "PANIC";
    default:
        return "Unknown";
    }
}

void subghz_protocol_decoder_vw_get_string(void *context, FuriString *output)
{
    furi_assert(context);
    SubGhzProtocolDecoderVw *instance = context;

    uint8_t type = (instance->data_2 >> 8) & 0xFF;
    uint8_t check = instance->data_2 & 0xFF;
    uint8_t btn = (check >> 4) & 0xF;

    uint32_t key_high = (instance->generic.data >> 32) & 0xFFFFFFFF;
    uint32_t key_low = instance->generic.data & 0xFFFFFFFF;

    furi_string_cat_printf(
        output,
        "%s %dbit\r\n"
        "Key:%02X%08lX%08lX%02X\r\n"
        "Type:%02X Btn:%X %s\r\n",
        instance->generic.protocol_name,
        instance->generic.data_count_bit,
        type,
        key_high,
        key_low,
        check,
        type,
        btn,
        vw_get_button_name(btn));
}
