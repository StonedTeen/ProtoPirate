// pirateproto2_app.c
#include "pirateproto2_app_i.h"

#include <furi.h>
#include <furi_hal.h>
#include "protocols/protocol_items.h"

#define TAG "PirateProto2App"

static bool pirateproto2_app_custom_event_callback(void *context, uint32_t event)
{
    furi_assert(context);
    PirateProto2App *app = context;
    return scene_manager_handle_custom_event(app->scene_manager, event);
}

static bool pirateproto2_app_back_event_callback(void *context)
{
    furi_assert(context);
    PirateProto2App *app = context;
    return scene_manager_handle_back_event(app->scene_manager);
}

static void pirateproto2_app_tick_event_callback(void *context)
{
    furi_assert(context);
    PirateProto2App *app = context;
    scene_manager_handle_tick_event(app->scene_manager);
}

PirateProto2App *pirateproto2_app_alloc()
{
    PirateProto2App *app = malloc(sizeof(PirateProto2App));

    FURI_LOG_I(TAG, "Allocating PirateProto 2 - Advanced Automotive Analysis Toolkit");

    // GUI
    app->gui = furi_record_open(RECORD_GUI);

    // View Dispatcher
    app->view_dispatcher = view_dispatcher_alloc();
    app->scene_manager = scene_manager_alloc(&pirateproto2_scene_handlers, app);

    view_dispatcher_set_event_callback_context(app->view_dispatcher, app);
    view_dispatcher_set_custom_event_callback(
        app->view_dispatcher, pirateproto2_app_custom_event_callback);
    view_dispatcher_set_navigation_event_callback(
        app->view_dispatcher, pirateproto2_app_back_event_callback);
    view_dispatcher_set_tick_event_callback(
        app->view_dispatcher, pirateproto2_app_tick_event_callback, 100);

    view_dispatcher_attach_to_gui(app->view_dispatcher, app->gui, ViewDispatcherTypeFullscreen);

    // Open Notification record
    app->notifications = furi_record_open(RECORD_NOTIFICATION);

    // Variable Item List
    app->variable_item_list = variable_item_list_alloc();
    view_dispatcher_add_view(
        app->view_dispatcher,
        PirateProto2ViewVariableItemList,
        variable_item_list_get_view(app->variable_item_list));

    // SubMenu
    app->submenu = submenu_alloc();
    view_dispatcher_add_view(
        app->view_dispatcher, PirateProto2ViewSubmenu, submenu_get_view(app->submenu));

    // Widget
    app->widget = widget_alloc();
    view_dispatcher_add_view(
        app->view_dispatcher, PirateProto2ViewWidget, widget_get_view(app->widget));

    // About View
    app->view_about = view_alloc();
    view_dispatcher_add_view(app->view_dispatcher, PirateProto2ViewAbout, app->view_about);

    // Receiver
    app->pirateproto2_receiver = pirateproto2_view_receiver_alloc();
    view_dispatcher_add_view(
        app->view_dispatcher,
        PirateProto2ViewReceiver,
        pirateproto2_view_receiver_get_view(app->pirateproto2_receiver));

    // Receiver Info
    app->pirateproto2_receiver_info = pirateproto2_view_receiver_info_alloc();
    view_dispatcher_add_view(
        app->view_dispatcher,
        PirateProto2ViewReceiverInfo,
        pirateproto2_view_receiver_info_get_view(app->pirateproto2_receiver_info));

    // Signal Analyzer (NEW)
    app->pirateproto2_signal_analyzer = pirateproto2_view_signal_analyzer_alloc();
    view_dispatcher_add_view(
        app->view_dispatcher,
        PirateProto2ViewSignalAnalyzer,
        pirateproto2_view_signal_analyzer_get_view(app->pirateproto2_signal_analyzer));

    // Protocol Learning (NEW)
    app->pirateproto2_protocol_learner = pirateproto2_view_protocol_learner_alloc();
    view_dispatcher_add_view(
        app->view_dispatcher,
        PirateProto2ViewProtocolLearner,
        pirateproto2_view_protocol_learner_get_view(app->pirateproto2_protocol_learner));

    // Init setting
    app->setting = subghz_setting_alloc();
    app->loaded_file_path = NULL;
    subghz_setting_load(app->setting, EXT_PATH("subghz/assets/setting_user"));

    // Init Worker & Protocol & History
    app->lock = PirateProto2LockOff;
    app->txrx = malloc(sizeof(PirateProto2TxRx));
    app->txrx->preset = malloc(sizeof(SubGhzRadioPreset));
    app->txrx->preset->name = furi_string_alloc();
    app->txrx->txrx_state = PirateProto2TxRxStateIDLE;
    app->txrx->rx_key_state = PirateProto2RxKeyStateIDLE;

    pirateproto2_preset_init(
        app, "AM650", subghz_setting_get_default_frequency(app->setting), NULL, 0);

    app->txrx->hopper_state = PirateProto2HopperStateOFF;
    app->txrx->hopper_idx_frequency = 0;
    app->txrx->hopper_timeout = 0;
    app->txrx->idx_menu_chosen = 0;

    app->txrx->history = pirateproto2_history_alloc();
    app->txrx->worker = subghz_worker_alloc();

    // Create environment with enhanced custom protocols
    app->txrx->environment = subghz_environment_alloc();

    FURI_LOG_I(TAG, "Registering %zu PirateProto 2 protocols", pirateproto2_protocol_registry.size);
    subghz_environment_set_protocol_registry(
        app->txrx->environment, (void *)&pirateproto2_protocol_registry);

    // Create receiver with enhanced capabilities
    app->txrx->receiver = subghz_receiver_alloc_init(app->txrx->environment);

    // Initialize SubGhz devices
    subghz_devices_init();

    // Try external CC1101 first, fallback to internal
    app->txrx->radio_device = 
        radio_device_loader_set(NULL, SubGhzRadioDeviceTypeExternalCC1101);

    if(!app->txrx->radio_device) {
        FURI_LOG_W(TAG, "External CC1101 not found, using internal CC1101.");
        app->txrx->radio_device = 
            radio_device_loader_set(NULL, SubGhzRadioDeviceTypeInternal);
        if(!app->txrx->radio_device) {
            FURI_LOG_E(TAG, "Failed to initialize internal CC1101.");
        }
    } else {
        FURI_LOG_I(TAG, "External CC1101 detected and initialized.");
    }

    if (!app->txrx->radio_device)
    {
        FURI_LOG_E(TAG, "Failed to initialize radio device!");
    }
    else
    {
        FURI_LOG_I(TAG, "Radio device initialized");
    }

    subghz_devices_reset(app->txrx->radio_device);
    subghz_devices_idle(app->txrx->radio_device);

    // Set filter to accept decodable AND sendable protocols (enhanced for v2)
    subghz_receiver_set_filter(app->txrx->receiver, 
        SubGhzProtocolFlag_Decodable | SubGhzProtocolFlag_Send | SubGhzProtocolFlag_Save);

    // Set up worker callbacks with enhanced features
    subghz_worker_set_overrun_callback(
        app->txrx->worker, (SubGhzWorkerOverrunCallback)subghz_receiver_reset);
    subghz_worker_set_pair_callback(
        app->txrx->worker, (SubGhzWorkerPairCallback)subghz_receiver_decode);
    subghz_worker_set_context(app->txrx->worker, app->txrx->receiver);

    // Initialize signal analysis module (NEW)
    app->signal_analyzer = pirateproto2_signal_analyzer_alloc();
    
    // Initialize protocol learning module (NEW)
    app->protocol_learner = pirateproto2_protocol_learner_alloc();

    furi_hal_power_suppress_charge_enter();

    scene_manager_next_scene(app->scene_manager, PirateProto2SceneStart);

    return app;
}

void pirateproto2_app_free(PirateProto2App *app)
{
    furi_assert(app);

    FURI_LOG_I(TAG, "Freeing PirateProto 2 - Advanced Automotive Analysis Toolkit");

    // Make sure we're not receiving
    if (app->txrx->txrx_state == PirateProto2TxRxStateRx)
    {
        subghz_worker_stop(app->txrx->worker);
        subghz_devices_stop_async_rx(app->txrx->radio_device);
    }

    if (app->loaded_file_path)
    {
        furi_string_free(app->loaded_file_path);
    }

    subghz_devices_sleep(app->txrx->radio_device);
    radio_device_loader_end(app->txrx->radio_device);

    subghz_devices_deinit();

    // Free new modules
    if (app->signal_analyzer)
    {
        pirateproto2_signal_analyzer_free(app->signal_analyzer);
    }
    
    if (app->protocol_learner)
    {
        pirateproto2_protocol_learner_free(app->protocol_learner);
    }

    // Submenu
    view_dispatcher_remove_view(app->view_dispatcher, PirateProto2ViewSubmenu);
    submenu_free(app->submenu);

    // Variable Item List
    view_dispatcher_remove_view(app->view_dispatcher, PirateProto2ViewVariableItemList);
    variable_item_list_free(app->variable_item_list);

    // About View
    view_dispatcher_remove_view(app->view_dispatcher, PirateProto2ViewAbout);
    view_free(app->view_about);

    // Widget
    view_dispatcher_remove_view(app->view_dispatcher, PirateProto2ViewWidget);
    widget_free(app->widget);

    // Receiver
    view_dispatcher_remove_view(app->view_dispatcher, PirateProto2ViewReceiver);
    pirateproto2_view_receiver_free(app->pirateproto2_receiver);

    // Receiver Info
    view_dispatcher_remove_view(app->view_dispatcher, PirateProto2ViewReceiverInfo);
    pirateproto2_view_receiver_info_free(app->pirateproto2_receiver_info);

    // Signal Analyzer (NEW)
    view_dispatcher_remove_view(app->view_dispatcher, PirateProto2ViewSignalAnalyzer);
    pirateproto2_view_signal_analyzer_free(app->pirateproto2_signal_analyzer);

    // Protocol Learning (NEW)
    view_dispatcher_remove_view(app->view_dispatcher, PirateProto2ViewProtocolLearner);
    pirateproto2_view_protocol_learner_free(app->pirateproto2_protocol_learner);

    // Setting
    subghz_setting_free(app->setting);

    // Worker & Protocol & History
    subghz_receiver_free(app->txrx->receiver);
    subghz_environment_free(app->txrx->environment);
    pirateproto2_history_free(app->txrx->history);
    subghz_worker_free(app->txrx->worker);
    furi_string_free(app->txrx->preset->name);
    free(app->txrx->preset);
    free(app->txrx);

    // View dispatcher
    view_dispatcher_free(app->view_dispatcher);
    scene_manager_free(app->scene_manager);

    // Notifications
    furi_record_close(RECORD_NOTIFICATION);
    app->notifications = NULL;

    // Close records
    furi_record_close(RECORD_GUI);

    furi_hal_power_suppress_charge_exit();

    free(app);
}

int32_t pirateproto2_app(void *p)
{
    UNUSED(p);
    PirateProto2App *pirateproto2_app = pirateproto2_app_alloc();

    view_dispatcher_run(pirateproto2_app->view_dispatcher);

    pirateproto2_app_free(pirateproto2_app);

    return 0;
}
