#include "hyundai_v1.h"

#define TAG "hyundai  V1"

// Basic hyundai  V1 protocol implementation
// This is a stub implementation for build purposes

static const SubGhzBlockConst subghz_protocol_hyundai_v1_const = {
    .te_short = 250,
    .te_long = 500,
    .te_delta = 100,
    .min_count_bit_for_found = 64,
};

struct SubGhzProtocolDecoderHyundaiv1 {
    SubGhzProtocolDecoderBase base;
    SubGhzBlockDecoder decoder;
    SubGhzBlockGeneric generic;
};

void subghz_protocol_decoder_hyundai_v1_reset(void *context) {
    furi_assert(context);
    SubGhzProtocolDecoderHyundaiv1 *instance = context;
    instance->decoder.parser_step = 0;
}

void* subghz_protocol_decoder_hyundai_v1_alloc(SubGhzEnvironment* environment) {
    UNUSED(environment);
    SubGhzProtocolDecoderHyundaiv1* instance = malloc(sizeof(SubGhzProtocolDecoderHyundaiv1));
    instance->base.protocol = &hyundai_v1_protocol;
    instance->generic.protocol_name = instance->base.protocol->name;
    subghz_protocol_decoder_hyundai_v1_reset(instance);
    return instance;
}

void subghz_protocol_decoder_hyundai_v1_free(void* context) {
    furi_assert(context);
    SubGhzProtocolDecoderHyundaiv1* instance = context;
    free(instance);
}

void subghz_protocol_decoder_hyundai_v1_feed(void* context, bool level, uint32_t duration) {
    UNUSED(context);
    UNUSED(level);
    UNUSED(duration);
    // Stub implementation
}

uint8_t subghz_protocol_decoder_hyundai_v1_get_hash_data(void* context) {
    furi_assert(context);
    SubGhzProtocolDecoderHyundaiv1* instance = context;
    return subghz_protocol_blocks_get_hash_data(&instance->decoder, 8);
}

SubGhzProtocolStatus subghz_protocol_decoder_hyundai_v1_serialize(
    void* context,
    FlipperFormat* flipper_format,
    SubGhzRadioPreset* preset) {
    furi_assert(context);
    SubGhzProtocolDecoderHyundaiv1* instance = context;
    return subghz_block_generic_serialize(&instance->generic, flipper_format, preset);
}

SubGhzProtocolStatus
    subghz_protocol_decoder_hyundai_v1_deserialize(void* context, FlipperFormat* flipper_format) {
    furi_assert(context);
    SubGhzProtocolDecoderHyundaiv1* instance = context;
    return subghz_block_generic_deserialize_check_count_bit(
        &instance->generic, flipper_format, subghz_protocol_hyundai_v1_const.min_count_bit_for_found);
}

void subghz_protocol_decoder_hyundai_v1_get_string(void* context, FuriString* output) {
    furi_assert(context);
    SubGhzProtocolDecoderHyundaiv1* instance = context;
    furi_string_cat_printf(output, "%s %dbit\r\nKey:STUB", instance->generic.protocol_name, instance->generic.data_count_bit);
}

const SubGhzProtocolDecoder subghz_protocol_hyundai_v1_decoder = {
    .alloc = subghz_protocol_decoder_hyundai_v1_alloc,
    .free = subghz_protocol_decoder_hyundai_v1_free,
    .feed = subghz_protocol_decoder_hyundai_v1_feed,
    .reset = subghz_protocol_decoder_hyundai_v1_reset,
    .get_hash_data = subghz_protocol_decoder_hyundai_v1_get_hash_data,
    .serialize = subghz_protocol_decoder_hyundai_v1_serialize,
    .deserialize = subghz_protocol_decoder_hyundai_v1_deserialize,
    .get_string = subghz_protocol_decoder_hyundai_v1_get_string,
};

const SubGhzProtocolEncoder subghz_protocol_hyundai_v1_encoder = {
    .alloc = NULL,
    .free = NULL,
    .deserialize = NULL,
    .stop = NULL,
    .yield = NULL,
};

const SubGhzProtocol hyundai_v1_protocol = {
    .name = HYUNDAI_V1_NAME,
    .type = SubGhzProtocolTypeStatic,
    .flag = SubGhzProtocolFlag_433 | SubGhzProtocolFlag_FM | SubGhzProtocolFlag_Decodable | SubGhzProtocolFlag_Save,
    .decoder = &subghz_protocol_hyundai_v1_decoder,
    .encoder = &subghz_protocol_hyundai_v1_encoder,
};
