#pragma once

#include <gui/gui.h>
#include <gui/view_dispatcher.h>
#include <gui/scene_manager.h>
#include <gui/modules/submenu.h>
#include <gui/modules/widget.h>
#include <gui/modules/variable_item_list.h>
#include <notification/notification.h>
#include <storage/storage.h>
#include <lib/subghz/subghz_worker.h>
#include <lib/subghz/subghz_setting.h>
#include <lib/subghz/receiver.h>
#include <lib/subghz/transmitter.h>
#include <lib/subghz/environment.h>
#include <lib/subghz/protocols/base.h>
#include <lib/subghz/blocks/decoder.h>
#include <lib/subghz/blocks/encoder.h>
#include <lib/subghz/blocks/generic.h>
#include <lib/subghz/blocks/math.h>
#include <lib/subghz/types.h>

#include "views/pirateproto2_receiver.h"
#include "views/pirateproto2_receiver_info.h"
#include "helpers/pirateproto2_storage.h"
#include "helpers/pirateproto2_types.h"
#include "helpers/radio_device_loader.h"

// New views for PirateProto 2
#include "views/pirateproto2_signal_analyzer.h"
#include "views/pirateproto2_protocol_learner.h"
#include "helpers/pirateproto2_signal_analyzer.h"
#include "helpers/pirateproto2_protocol_learner.h"

typedef enum {
    PirateProto2ViewNone,
    PirateProto2ViewSubmenu,
    PirateProto2ViewVariableItemList,
    PirateProto2ViewWidget,
    PirateProto2ViewReceiver,
    PirateProto2ViewReceiverInfo,
    PirateProto2ViewAbout,
    // New views for PirateProto 2
    PirateProto2ViewSignalAnalyzer,
    PirateProto2ViewProtocolLearner,
} PirateProto2View;

typedef enum {
    PirateProto2CustomEventStart,
    PirateProto2CustomEventAbout,
    PirateProto2CustomEventReceiverConfig,
    PirateProto2CustomEventReceiver,
    PirateProto2CustomEventReceiverInfo,
    PirateProto2CustomEventSaved,
    PirateProto2CustomEventSavedInfo,
    PirateProto2CustomEventEmulate,
    // New events for PirateProto 2
    PirateProto2CustomEventSignalAnalyzer,
    PirateProto2CustomEventProtocolLearner,
} PirateProto2CustomEvent;

typedef enum {
    PirateProto2LockOff,
    PirateProto2LockOn,
} PirateProto2Lock;

typedef enum {
    PirateProto2TxRxStateIDLE,
    PirateProto2TxRxStateRx,
    PirateProto2TxRxStateTx,
    PirateProto2TxRxStateLoad,
    PirateProto2TxRxStateSave,
    PirateProto2TxRxStateDelete,
    PirateProto2TxRxStateEmulate,
    PirateProto2TxRxStateSleep,
} PirateProto2TxRxState;

typedef enum {
    PirateProto2RxKeyStateIDLE,
    PirateProto2RxKeyStateNeedSave,
    PirateProto2RxKeyStateNew,
    PirateProto2RxKeyStateBack,
    PirateProto2RxKeyStateNeedAdd,
} PirateProto2RxKeyState;

typedef enum {
    PirateProto2HopperStateOFF,
    PirateProto2HopperStateRun,
    PirateProto2HopperStatePause,
    PirateProto2HopperStateRSSITimeOut,
} PirateProto2HopperState;

typedef struct PirateProto2App PirateProto2App;

struct PirateProto2App {
    Gui* gui;
    ViewDispatcher* view_dispatcher;
    SceneManager* scene_manager;
    NotificationApp* notifications;
    VariableItemList* variable_item_list;
    Submenu* submenu;
    Widget* widget;
    View* view_about;

    PirateProto2Lock lock;
    PirateProto2ViewReceiver* pirateproto2_receiver;
    PirateProto2ViewReceiverInfo* pirateproto2_receiver_info;

    // New components for PirateProto 2
    PirateProto2ViewSignalAnalyzer* pirateproto2_signal_analyzer;
    PirateProto2ViewProtocolLearner* pirateproto2_protocol_learner;
    PirateProto2SignalAnalyzer* signal_analyzer;
    PirateProto2ProtocolLearner* protocol_learner;

    SubGhzSetting* setting;
    FuriString* loaded_file_path;

    typedef struct PirateProto2TxRx PirateProto2TxRx;
    PirateProto2TxRx* txrx;
};

struct PirateProto2TxRx {
    SubGhzWorker* worker;
    SubGhzEnvironment* environment;
    SubGhzReceiver* receiver;
    SubGhzSetting* setting;
    SubGhzRadioPreset* preset;
    SubGhzRadioDevice* radio_device;
    PirateProto2History* history;

    PirateProto2TxRxState txrx_state;
    PirateProto2RxKeyState rx_key_state;
    PirateProto2HopperState hopper_state;
    uint8_t hopper_idx_frequency;
    uint32_t hopper_timeout;
    uint8_t idx_menu_chosen;
};

typedef enum {
    PirateProto2SceneStart,
    PirateProto2SceneReceiverConfig,
    PirateProto2SceneReceiver,
    PirateProto2SceneReceiverInfo,
    PirateProto2SceneSaved,
    PirateProto2SceneSavedInfo,
    PirateProto2SceneEmulate,
    PirateProto2SceneAbout,
    // New scenes for PirateProto 2
    PirateProto2SceneSignalAnalyzer,
    PirateProto2SceneProtocolLearner,
    PirateProto2SceneCount,
} PirateProto2Scene;

extern const SceneManagerHandlers pirateproto2_scene_handlers;

void pirateproto2_preset_init(
    PirateProto2App* app,
    const char* preset_name,
    uint32_t frequency,
    uint8_t* custom_preset_data,
    uint32_t custom_preset_data_size);
