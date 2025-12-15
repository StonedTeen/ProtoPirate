#include "fiat_v0.h"
#include <lib/toolbox/manchester_decoder.h>

#define TAG "FiatProtocolV0"

// Forward declarations
struct SubGhzProtocolEncoderFiatV0;

static const SubGhzBlockConst subghz_protocol_fiat_v0_const = {
    .te_short = 200,
    .te_long = 400,
    .te_delta = 100,
    .min_count_bit_for_found = 64,
};

struct SubGhzProtocolDecoderFiatV0 {
    SubGhzProtocolDecoderBase base;
    SubGhzBlockDecoder decoder;
    SubGhzBlockGeneric generic;
    ManchesterState manchester_state;
    uint8_t decoder_state;
    uint16_t preamble_count;
    uint32_t data_low;
    uint32_t data_high;
    uint8_t bit_count;
    uint32_t hop;
    uint32_t fix;
    uint8_t endbyte;
    uint8_t final_count;
    uint32_t te_last;
};

typedef enum {
    FiatV0DecoderStepReset = 0,
    FiatV0DecoderStepPreamble = 1,
    FiatV0DecoderStepData = 2,
} FiatV0DecoderStep;

struct SubGhzProtocolEncoderFiatV0 {
    SubGhzProtocolEncoderBase base;
    SubGhzProtocolBlockEncoder encoder;
    SubGhzBlockGeneric generic;
};

const SubGhzProtocolDecoder subghz_protocol_fiat_v0_decoder = {
    .alloc = subghz_protocol_decoder_fiat_v0_alloc,
    .free = subghz_protocol_decoder_fiat_v0_free,
    .feed = subghz_protocol_decoder_fiat_v0_feed,
    .reset = subghz_protocol_decoder_fiat_v0_reset,
    .get_hash_data = subghz_protocol_decoder_fiat_v0_get_hash_data,
    .serialize = subghz_protocol_decoder_fiat_v0_serialize,
    .deserialize = subghz_protocol_decoder_fiat_v0_deserialize,
    .get_string = subghz_protocol_decoder_fiat_v0_get_string,
};

// Encoder implementation
static void fiat_reconstruct_data(SubGhzProtocolEncoderFiatV0 *instance)
{
    // Fiat V0 protocol has a complex 64-bit structure
    // The first 32 bits are the "hop" value
    // The next 32 bits are the "fix" value
    // Additional bytes follow for endbyte and other data
    
    // For encoding, we'll use the hop and fix values
    uint64_t data = ((uint64_t)instance->generic.cnt << 32) | instance->generic.serial;
    instance->generic.data = data;
    
    // The endbyte is stored in the button field for compatibility
    // In Fiat protocol, this represents the final command byte
}

void *subghz_protocol_encoder_fiat_v0_alloc(SubGhzEnvironment *environment)
{
    UNUSED(environment);
    SubGhzProtocolEncoderFiatV0 *instance = malloc(sizeof(SubGhzProtocolEncoderFiatV0));
    instance->base.protocol = &fiat_protocol_v0;
    return instance;
}

void subghz_protocol_encoder_fiat_v0_free(void *context)
{
    furi_assert(context);
    SubGhzProtocolEncoderFiatV0 *instance = context;
    free(instance);
}

SubGhzProtocolStatus subghz_protocol_encoder_fiat_v0_deserialize(void *context, FlipperFormat *flipper_format)
{
    furi_assert(context);
    SubGhzProtocolEncoderFiatV0 *instance = context;

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

        // Extract hop and fix from key
        instance->generic.cnt = (uint32_t)(key >> 32); // Hop value
        instance->generic.serial = key & 0xFFFFFFFF;     // Fix value

        // Read endbyte (stored as button for compatibility)
        uint32_t endbyte;
        if (flipper_format_read_uint32(flipper_format, "EndByte", &endbyte, 1))
        {
            instance->generic.btn = (uint8_t)endbyte;
        }
        else if (flipper_format_read_uint32(flipper_format, "Btn", &endbyte, 1))
        {
            instance->generic.btn = (uint8_t)endbyte;
        }
        else
        {
            instance->generic.btn = 0x00; // Default endbyte
        }

        // Reconstruct the data structure
        fiat_reconstruct_data(instance);

        res = SubGhzProtocolStatusOk;
    } while (false);

    return res;
}

void subghz_protocol_encoder_fiat_v0_stop(void *context)
{
    UNUSED(context);
}

LevelDuration subghz_protocol_encoder_fiat_v0_yield(void *context)
{
    furi_assert(context);
    SubGhzProtocolEncoderFiatV0 *instance = context;

    // Static variables to maintain state between calls
    static uint8_t bit_index = 0;
    static bool is_high = true;
    static uint8_t preamble_count = 0;

    // Send preamble (0x96 = 150 short pulses)
    if (preamble_count < 150)
    {
        if (is_high)
        {
            is_high = false;
            return level_duration_make(true, subghz_protocol_fiat_v0_const.te_short);
        }
        else
        {
            is_high = true;
            preamble_count++;
            return level_duration_make(false, subghz_protocol_fiat_v0_const.te_short);
        }
    }

    // Send gap
    if (preamble_count == 150)
    {
        preamble_count++;
        return level_duration_make(true, 800); // Gap duration
    }

    if (preamble_count == 151)
    {
        preamble_count++;
        bit_index = 0;
        is_high = true;
        return level_duration_make(false, 800); // Gap duration
    }

    // Send data bits (Manchester encoding)
    if (bit_index < instance->generic.data_count_bit)
    {
        bool current_bit = (instance->generic.data >> (63 - bit_index)) & 1;
        
        if (is_high)
        {
            is_high = false;
            // Manchester encoding: 1 = short high, 0 = long high
            uint32_t duration = current_bit ? subghz_protocol_fiat_v0_const.te_short : subghz_protocol_fiat_v0_const.te_long;
            return level_duration_make(true, duration);
        }
        else
        {
            is_high = true;
            bit_index++;
            // LOW pulse duration matches the previous HIGH duration
            uint32_t duration = current_bit ? subghz_protocol_fiat_v0_const.te_short : subghz_protocol_fiat_v0_const.te_long;
            
            if (bit_index >= instance->generic.data_count_bit)
            {
                // Reset for next transmission
                preamble_count = 0;
                bit_index = 0;
                return level_duration_make(false, duration + 2000); // Add gap
            }
            
            return level_duration_make(false, duration);
        }
    }

    return level_duration_reset();
}

const SubGhzProtocolEncoder subghz_protocol_fiat_v0_encoder = {
    .alloc = subghz_protocol_encoder_fiat_v0_alloc,
    .free = subghz_protocol_encoder_fiat_v0_free,
    .deserialize = subghz_protocol_encoder_fiat_v0_deserialize,
    .stop = subghz_protocol_encoder_fiat_v0_stop,
    .yield = subghz_protocol_encoder_fiat_v0_yield,
};

const SubGhzProtocol fiat_protocol_v0 = {
    .name = FIAT_PROTOCOL_V0_NAME,
    .type = SubGhzProtocolTypeDynamic,
    .flag = SubGhzProtocolFlag_433 | SubGhzProtocolFlag_FM | SubGhzProtocolFlag_Decodable | SubGhzProtocolFlag_Save | SubGhzProtocolFlag_Send,
    .decoder = &subghz_protocol_fiat_v0_decoder,
    .encoder = &subghz_protocol_fiat_v0_encoder,
};

void* subghz_protocol_decoder_fiat_v0_alloc(SubGhzEnvironment* environment) {
    UNUSED(environment);
    SubGhzProtocolDecoderFiatV0* instance = malloc(sizeof(SubGhzProtocolDecoderFiatV0));
    instance->base.protocol = &fiat_protocol_v0;
    instance->generic.protocol_name = instance->base.protocol->name;
    return instance;
}

void subghz_protocol_decoder_fiat_v0_free(void* context) {
    furi_assert(context);
    SubGhzProtocolDecoderFiatV0* instance = context;
    free(instance);
}

void subghz_protocol_decoder_fiat_v0_reset(void* context) {
    furi_assert(context);
    SubGhzProtocolDecoderFiatV0* instance = context;
    instance->decoder.parser_step = FiatV0DecoderStepReset;
    instance->decoder_state = 0;
    instance->preamble_count = 0;
    instance->data_low = 0;
    instance->data_high = 0;
    instance->bit_count = 0;
    instance->hop = 0;
    instance->fix = 0;
    instance->endbyte = 0;
    instance->final_count = 0;
    instance->te_last = 0;
    instance->manchester_state = ManchesterStateMid1;
}

void subghz_protocol_decoder_fiat_v0_feed(void* context, bool level, uint32_t duration) {
    furi_assert(context);
    SubGhzProtocolDecoderFiatV0* instance = context;
    uint32_t te_short = (uint32_t)subghz_protocol_fiat_v0_const.te_short;
    uint32_t te_long = (uint32_t)subghz_protocol_fiat_v0_const.te_long;
    uint32_t te_delta = (uint32_t)subghz_protocol_fiat_v0_const.te_delta;
    uint32_t gap_threshold = 800;
    uint32_t diff;
    switch(instance->decoder_state) {
    case FiatV0DecoderStepReset:
        if(!level) {
            return;
        }
        if(duration < te_short) {
            diff = te_short - duration;
        } else {
            diff = duration - te_short;
        }
        if(diff < te_delta) {
            instance->data_low = 0;
            instance->data_high = 0;
            instance->decoder_state = FiatV0DecoderStepPreamble;
            instance->te_last = duration;
            instance->preamble_count = 0;
            instance->bit_count = 0;
            manchester_advance(
                instance->manchester_state,
                ManchesterEventReset,
                &instance->manchester_state,
                NULL);
        }
        break;
    case FiatV0DecoderStepPreamble:
        if(level) {
            return;
        }
        if(duration < te_short) {
            diff = te_short - duration;
            if(diff < te_delta) {
                instance->preamble_count++;
                instance->te_last = duration;
                if(instance->preamble_count >= 0x96) {
                    if(duration < gap_threshold) {
                        diff = gap_threshold - duration;
                    } else {
                        diff = duration - gap_threshold;
                    }
                    if(diff < te_delta) {
                        instance->decoder_state = FiatV0DecoderStepData;
                        instance->preamble_count = 0;
                        instance->data_low = 0;
                        instance->data_high = 0;
                        instance->bit_count = 0;
                        instance->te_last = duration;
                        return;
                    }
                }
            } else {
                instance->decoder_state = FiatV0DecoderStepReset;
                if(instance->preamble_count >= 0x96) {
                    if(duration < gap_threshold) {
                        diff = gap_threshold - duration;
                    } else {
                        diff = duration - gap_threshold;
                    }
                    if(diff < te_delta) {
                        instance->decoder_state = FiatV0DecoderStepData;
                        instance->preamble_count = 0;
                        instance->data_low = 0;
                        instance->data_high = 0;
                        instance->bit_count = 0;
                        instance->te_last = duration;
                        return;
                    }
                }
            }
        } else {
            diff = duration - te_short;
            if(diff < te_delta) {
                instance->preamble_count++;
                instance->te_last = duration;
            } else {
                instance->decoder_state = FiatV0DecoderStepReset;
            }
            if(instance->preamble_count >= 0x96) {
                if(duration >= 799) {
                    diff = duration - gap_threshold;
                } else {
                    diff = gap_threshold - duration;
                }
                if(diff < te_delta) {
                    instance->decoder_state = FiatV0DecoderStepData;
                    instance->preamble_count = 0;
                    instance->data_low = 0;
                    instance->data_high = 0;
                    instance->bit_count = 0;
                    instance->te_last = duration;
                    return;
                }
            }
        }
        break;
    case FiatV0DecoderStepData:
        ManchesterEvent event = ManchesterEventReset;
        if(duration < te_short) {
            diff = te_short - duration;
            if(diff < te_delta) {
                event = level ? ManchesterEventShortLow : ManchesterEventShortHigh;
            }
        } else {
            diff = duration - te_short;
            if(diff < te_delta) {
                event = level ? ManchesterEventShortLow : ManchesterEventShortHigh;
            } else {
                if(duration < te_long) {
                    diff = te_long - duration;
                } else {
                    diff = duration - te_long;
                }
                if(diff < te_delta) {
                    event = level ? ManchesterEventLongLow : ManchesterEventLongHigh;
                }
            }
        }

        if(event != ManchesterEventReset) {
            bool data_bit_bool;
            if(manchester_advance(
                   instance->manchester_state,
                   event,
                   &instance->manchester_state,
                   &data_bit_bool)) {
                uint32_t new_bit = data_bit_bool ? 1 : 0;

                uint32_t carry = (instance->data_low >> 31) & 1;
                instance->data_low = (instance->data_low << 1) | new_bit;
                instance->data_high = (instance->data_high << 1) | carry;

                instance->bit_count++;

                if(instance->bit_count == 0x40) {
                    instance->fix = instance->data_low;
                    instance->hop = instance->data_high;
                    instance->data_low = 0;
                    instance->data_high = 0;
                }

                if(instance->bit_count > 0x46) {
                    instance->final_count = instance->bit_count;

                    instance->endbyte = (uint8_t)instance->data_low;

                    instance->generic.data = ((uint64_t)instance->hop << 32) | instance->fix;
                    instance->generic.data_count_bit = 64;
                    instance->generic.serial = instance->fix;
                    instance->generic.btn =
                        instance->endbyte; // still exported as btn for UI compatibility
                    instance->generic.cnt = instance->hop;

                    if(instance->base.callback) {
                        instance->base.callback(&instance->base, instance->base.context);
                    }

                    instance->data_low = 0;
                    instance->data_high = 0;
                    instance->bit_count = 0;
                    instance->decoder_state = FiatV0DecoderStepReset;
                }
            }
        }
        instance->te_last = duration;
        break;
    }
}

uint8_t subghz_protocol_decoder_fiat_v0_get_hash_data(void* context) {
    furi_assert(context);
    SubGhzProtocolDecoderFiatV0* instance = context;
    SubGhzBlockDecoder decoder = {
        .decode_data = instance->generic.data,
        .decode_count_bit = instance->generic.data_count_bit};
    return subghz_protocol_blocks_get_hash_data(&decoder, (decoder.decode_count_bit / 8) + 1);
}

SubGhzProtocolStatus subghz_protocol_decoder_fiat_v0_serialize(
    void* context,
    FlipperFormat* flipper_format,
    SubGhzRadioPreset* preset) {
    furi_assert(context);
    SubGhzProtocolDecoderFiatV0* instance = context;
    return subghz_block_generic_serialize(&instance->generic, flipper_format, preset);
}

SubGhzProtocolStatus
    subghz_protocol_decoder_fiat_v0_deserialize(void* context, FlipperFormat* flipper_format) {
    furi_assert(context);
    SubGhzProtocolDecoderFiatV0* instance = context;
    return subghz_block_generic_deserialize_check_count_bit(
        &instance->generic, flipper_format, subghz_protocol_fiat_v0_const.min_count_bit_for_found);
}

void subghz_protocol_decoder_fiat_v0_get_string(void* context, FuriString* output) {
    furi_assert(context);
    SubGhzProtocolDecoderFiatV0* instance = context;

    uint32_t code_found_hi = instance->hop;
    uint32_t code_found_lo = instance->fix;

    furi_string_cat_printf(
        output,
        "%s %dbit\r\n"
        "Key:%08lX%08lX\r\n"
        "Hop:%08lX Fix:%08lX\r\n"
        "EndByte:%02X\r\n",
        instance->generic.protocol_name,
        instance->generic.data_count_bit,
        code_found_hi,
        code_found_lo,
        instance->hop,
        instance->fix,
        instance->endbyte);
}
