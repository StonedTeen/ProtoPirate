#pragma once
#include <furi.h>
#include <lib/subghz/protocols/base.h>
#include <lib/subghz/types.h>
#include <lib/subghz/blocks/const.h>
#include <lib/subghz/blocks/decoder.h>
#include <lib/subghz/blocks/encoder.h>
#include <lib/subghz/blocks/generic.h>
#include <lib/subghz/blocks/math.h>
#include <flipper_format/flipper_format.h>

#define GENERIC_FIXED_PROTOCOL_V1_NAME "generic fixed protocol  V1"

typedef struct SubGhzProtocolDecoderGenericfixedprotocolv1 SubGhzProtocolDecoderGenericfixedprotocolv1;

extern const SubGhzProtocol generic_fixed_protocol_v1_protocol;

void* subghz_protocol_decoder_generic_fixed_protocol_v1_alloc(SubGhzEnvironment* environment);
void subghz_protocol_decoder_generic_fixed_protocol_v1_free(void* context);
void subghz_protocol_decoder_generic_fixed_protocol_v1_reset(void* context);
void subghz_protocol_decoder_generic_fixed_protocol_v1_feed(void* context, bool level, uint32_t duration);
uint8_t subghz_protocol_decoder_generic_fixed_protocol_v1_get_hash_data(void* context);
SubGhzProtocolStatus subghz_protocol_decoder_generic_fixed_protocol_v1_serialize(
    void* context,
    FlipperFormat* flipper_format,
    SubGhzRadioPreset* preset);
SubGhzProtocolStatus
    subghz_protocol_decoder_generic_fixed_protocol_v1_deserialize(void* context, FlipperFormat* flipper_format);
void subghz_protocol_decoder_generic_fixed_protocol_v1_get_string(void* context, FuriString* output);
