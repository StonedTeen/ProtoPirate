#pragma once

#include <gui/scene_manager.h>

#include "../pirateproto2_app_i.h"

// Scene enter handlers
void pirateproto2_scene_start_on_enter(void* context);
bool pirateproto2_scene_start_on_event(void* context, SceneManagerEvent event);
void pirateproto2_scene_start_on_exit(void* context);
void pirateproto2_scene_start_callback(void* context, uint32_t index);

// Enhanced about scene
void pirateproto2_scene_about_on_enter(void* context);
bool pirateproto2_scene_about_on_event(void* context, SceneManagerEvent event);
void pirateproto2_scene_about_on_exit(void* context);

// New Signal Analyzer Scene
void pirateproto2_scene_signal_analyzer_on_enter(void* context);
bool pirateproto2_scene_signal_analyzer_on_event(void* context, SceneManagerEvent event);
void pirateproto2_scene_signal_analyzer_on_exit(void* context);
void pirateproto2_scene_signal_analyzer_callback(void* context, uint32_t index);

// New Protocol Learner Scene
void pirateproto2_scene_protocol_learner_on_enter(void* context);
bool pirateproto2_scene_protocol_learner_on_event(void* context, SceneManagerEvent event);
void pirateproto2_scene_protocol_learner_on_exit(void* context);
void pirateproto2_scene_protocol_learner_callback(void* context, uint32_t index);

// Enhanced scene handlers array
extern const SceneManagerHandler pirateproto2_scene_on_enter_handlers[];
extern const SceneManagerHandler pirateproto2_scene_on_event_handlers[];
extern const SceneManagerHandler pirateproto2_scene_on_exit_handlers[];

// Scene handlers for PirateProto 2
#define SCENE_HANDLER_PAIR(scene) \
    [scene] = { \
        .on_enter = pirateproto2_scene_##scene##_on_enter, \
        .on_event = pirateproto2_scene_##scene##_on_event, \
        .on_exit = pirateproto2_scene_##scene##_on_exit, \
    }

// Define scene handlers
const SceneManagerHandler pirateproto2_scene_on_enter_handlers[PirateProto2SceneCount] = {
    SCENE_HANDLER_PAIR(start),
    SCENE_HANDLER_PAIR(about),
    SCENE_HANDLER_PAIR(signal_analyzer),
    SCENE_HANDLER_PAIR(protocol_learner),
    // Other scenes will be added as implemented
};

const SceneManagerHandler pirateproto2_scene_on_event_handlers[PirateProto2SceneCount] = {
    SCENE_HANDLER_PAIR(start),
    SCENE_HANDLER_PAIR(about),
    SCENE_HANDLER_PAIR(signal_analyzer),
    SCENE_HANDLER_PAIR(protocol_learner),
    // Other scenes will be added as implemented
};

const SceneManagerHandler pirateproto2_scene_on_exit_handlers[PirateProto2SceneCount] = {
    SCENE_HANDLER_PAIR(start),
    SCENE_HANDLER_PAIR(about),
    SCENE_HANDLER_PAIR(signal_analyzer),
    SCENE_HANDLER_PAIR(protocol_learner),
    // Other scenes will be added as implemented
};

#undef SCENE_HANDLER_PAIR
