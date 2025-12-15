#include "pirateproto2_scene.h"

// Enhanced scene handlers for PirateProto 2
const SceneManagerHandlers pirateproto2_scene_handlers = {
    .on_enter_handlers = pirateproto2_scene_on_enter_handlers,
    .on_event_handlers = pirateproto2_scene_on_event_handlers,
    .on_exit_handlers = pirateproto2_scene_on_exit_handlers,
    .scene_num = PirateProto2SceneCount,
};

// Scene enter handlers
void pirateproto2_scene_start_on_enter(void* context) {
    PirateProto2App* app = context;
    Submenu* submenu = app->submenu;
    
    submenu_reset(submenu);
    submenu_set_header(submenu, "PirateProto 2");
    
    submenu_add_item(
        submenu, "Receiver", PirateProto2CustomEventReceiver, pirateproto2_scene_start_callback, app);
    submenu_add_item(
        submenu, "Saved Signals", PirateProto2CustomEventSaved, pirateproto2_scene_start_callback, app);
    submenu_add_item(
        submenu, "Emulate", PirateProto2CustomEventEmulate, pirateproto2_scene_start_callback, app);
    
    // New menu items for PirateProto 2
    submenu_add_item(
        submenu, "Signal Analyzer", PirateProto2CustomEventSignalAnalyzer, pirateproto2_scene_start_callback, app);
    submenu_add_item(
        submenu, "Protocol Learner", PirateProto2CustomEventProtocolLearner, pirateproto2_scene_start_callback, app);
    
    submenu_add_item(
        submenu, "About", PirateProto2CustomEventAbout, pirateproto2_scene_start_callback, app);
    
    view_dispatcher_switch_to_view(app->view_dispatcher, PirateProto2ViewSubmenu);
}

bool pirateproto2_scene_start_on_event(void* context, SceneManagerEvent event) {
    PirateProto2App* app = context;
    bool consumed = false;

    if(event.type == SceneManagerEventTypeCustom) {
        switch(event.event) {
        case PirateProto2CustomEventReceiver:
            scene_manager_next_scene(app->scene_manager, PirateProto2SceneReceiver);
            consumed = true;
            break;
        case PirateProto2CustomEventSaved:
            scene_manager_next_scene(app->scene_manager, PirateProto2SceneSaved);
            consumed = true;
            break;
        case PirateProto2CustomEventEmulate:
            scene_manager_next_scene(app->scene_manager, PirateProto2SceneEmulate);
            consumed = true;
            break;
        case PirateProto2CustomEventSignalAnalyzer:
            scene_manager_next_scene(app->scene_manager, PirateProto2SceneSignalAnalyzer);
            consumed = true;
            break;
        case PirateProto2CustomEventProtocolLearner:
            scene_manager_next_scene(app->scene_manager, PirateProto2SceneProtocolLearner);
            consumed = true;
            break;
        case PirateProto2CustomEventAbout:
            scene_manager_next_scene(app->scene_manager, PirateProto2SceneAbout);
            consumed = true;
            break;
        }
    }

    return consumed;
}

void pirateproto2_scene_start_on_exit(void* context) {
    PirateProto2App* app = context;
    submenu_reset(app->submenu);
}

void pirateproto2_scene_start_callback(void* context, uint32_t index) {
    PirateProto2App* app = context;
    view_dispatcher_send_custom_event(app->view_dispatcher, index);
}

// Enhanced about scene with PirateProto 2 information
void pirateproto2_scene_about_on_enter(void* context) {
    PirateProto2App* app = context;
    Widget* widget = app->widget;
    
    widget_reset(widget);
    
    widget_add_text_scroll_element(
        widget, 0, 0, 128, 64, 
        "PirateProto 2 v2.0\n\n"
        "Advanced Automotive\n"
        "Key Fob Analysis Toolkit\n\n"
        "Enhanced Features:\n"
        "• Full Transmission\n"
        "• Signal Analysis\n"
        "• Protocol Learning\n"
        "• 20+ Protocols\n\n"
        "Created by:\n"
        "The Pirates' Plunder\n\n"
        "discord.gg/thepirates");
    
    view_dispatcher_switch_to_view(app->view_dispatcher, PirateProto2ViewWidget);
}

bool pirateproto2_scene_about_on_event(void* context, SceneManagerEvent event) {
    UNUSED(context);
    UNUSED(event);
    return false;
}

void pirateproto2_scene_about_on_exit(void* context) {
    PirateProto2App* app = context;
    widget_reset(app->widget);
}

// New Signal Analyzer Scene
void pirateproto2_scene_signal_analyzer_on_enter(void* context) {
    PirateProto2App* app = context;
    pirateproto2_view_signal_analyzer_set_callback(
        app->pirateproto2_signal_analyzer, pirateproto2_scene_signal_analyzer_callback, app);
    
    view_dispatcher_switch_to_view(app->view_dispatcher, PirateProto2ViewSignalAnalyzer);
}

bool pirateproto2_scene_signal_analyzer_on_event(void* context, SceneManagerEvent event) {
    PirateProto2App* app = context;
    bool consumed = false;

    if(event.type == SceneManagerEventTypeCustom) {
        switch(event.event) {
        case PirateProto2CustomEventBack:
            scene_manager_previous_scene(app->scene_manager);
            consumed = true;
            break;
        }
    }

    return consumed;
}

void pirateproto2_scene_signal_analyzer_on_exit(void* context) {
    PirateProto2App* app = context;
    pirateproto2_view_signal_analyzer_set_callback(app->pirateproto2_signal_analyzer, NULL, NULL);
}

void pirateproto2_scene_signal_analyzer_callback(void* context, uint32_t index) {
    PirateProto2App* app = context;
    view_dispatcher_send_custom_event(app->view_dispatcher, index);
}

// New Protocol Learner Scene
void pirateproto2_scene_protocol_learner_on_enter(void* context) {
    PirateProto2App* app = context;
    pirateproto2_view_protocol_learner_set_callback(
        app->pirateproto2_protocol_learner, pirateproto2_scene_protocol_learner_callback, app);
    
    view_dispatcher_switch_to_view(app->view_dispatcher, PirateProto2ViewProtocolLearner);
}

bool pirateproto2_scene_protocol_learner_on_event(void* context, SceneManagerEvent event) {
    PirateProto2App* app = context;
    bool consumed = false;

    if(event.type == SceneManagerEventTypeCustom) {
        switch(event.event) {
        case PirateProto2CustomEventBack:
            scene_manager_previous_scene(app->scene_manager);
            consumed = true;
            break;
        }
    }

    return consumed;
}

void pirateproto2_scene_protocol_learner_on_exit(void* context) {
    PirateProto2App* app = context;
    pirateproto2_view_protocol_learner_set_callback(app->pirateproto2_protocol_learner, NULL, NULL);
}

void pirateproto2_scene_protocol_learner_callback(void* context, uint32_t index) {
    PirateProto2App* app = context;
    view_dispatcher_send_custom_event(app->view_dispatcher, index);
}
