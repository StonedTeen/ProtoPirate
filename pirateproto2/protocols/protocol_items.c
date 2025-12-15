#include "protocol_items.h"

// Enhanced protocol registry for PirateProto 2
const SubGhzProtocol* pirateproto2_protocol_registry_items[] = {
    // Enhanced KIA protocols with transmission support
    &kia_protocol_v0,
    &kia_protocol_v1,
    &kia_protocol_v2,
    &kia_protocol_v3_v4,
    &kia_protocol_v5,
    
    // Enhanced Ford protocol with full transmission
    &ford_protocol_v0,
    
    // Enhanced Subaru protocol with rolling code emulation
    &subaru_protocol,
    
    // Enhanced Suzuki protocol
    &suzuki_protocol,
    
    // Enhanced VW protocol
    &vw_protocol,
    
    // Enhanced Fiat protocol
    &fiat_protocol_v0,
    
    // NEW PROTOCOLS FOR PIRATEPROTO 2
    
    // Toyota protocols
    &toyota_protocol_v1,
    &toyota_protocol_v2,
    
    // Honda protocols
    &honda_protocol_v1,
    &honda_protocol_v2,
    
    // Nissan protocols
    &nissan_protocol_v1,
    &nissan_protocol_v2,
    
    // Mazda protocols
    &mazda_protocol_v1,
    
    // Hyundai protocols
    &hyundai_protocol_v1,
    
    // BMW protocols
    &bmw_protocol_v1,
    
    // Mercedes protocols
    &mercedes_protocol_v1,
    
    // Audi protocols
    &audi_protocol_v1,
    
    // Generic protocols for analysis
    &generic_rolling_protocol_v1,
    &generic_fixed_protocol_v1,
};

const SubGhzProtocolRegistry pirateproto2_protocol_registry = {
    .items = pirateproto2_protocol_registry_items,
    .size = COUNT_OF(pirateproto2_protocol_registry_items),
};
