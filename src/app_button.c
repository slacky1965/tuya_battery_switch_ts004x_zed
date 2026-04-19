#include "app_main.h"

#ifndef DEBOUNCE_BUTTON
#define DEBOUNCE_BUTTON     16      /* number of polls for debounce                 */
#endif
#define FR_COUNTER_MAX      5      /* number for factory reset                     */
#define BATTERY_COUNTER     4

typedef struct {
    bool        released;
    bool        pressed;
    bool        hold;
    bool        level_up;
    uint8_t     counter;
    uint8_t     debounce;
    uint32_t    pressed_time;
    uint32_t    released_time;
    uint32_t    hold_time;
    uint32_t    gpio;
} app_button_t;

enum {
    HOLD_NOT_PRESENT = 0,
    HOLD_PRESENT,
    HOLD_FIX
};

static ev_timer_event_t *timerClearSleepEvt = NULL;
static ev_timer_event_t *timerFactoryResetEvt = NULL;
static ev_timer_event_t *timerButtonFindBindEvt = NULL;
static app_button_t app_button[DEVICE_BUTTON_MAX];
static bool buttonFindBindFlag = false;
static bool factory_reset = false;

static int32_t clearSleepCb(void *args) {

    APP_DEBUG(DEBUG_PM_EN, "clearSleepCb\r\n");

    if ((!g_appCtx.timerSetPollRateEvt || !g_appCtx.timerSetPollRateEvt->used || g_appCtx.timerSetPollRateEvt->isBusy) &&
            !g_appCtx.ota && !buttonFindBindFlag) {
        g_appCtx.not_sleep = false;
    }

    timerClearSleepEvt = NULL;
    return -1;
}

static void clearSleepTimer() {
    if (timerClearSleepEvt) {
        TL_ZB_TIMER_CANCEL(&timerClearSleepEvt);
    }
    timerClearSleepEvt = TL_ZB_TIMER_SCHEDULE(clearSleepCb, NULL, TIMEOUT_100MS);
}

static void button_factory_reset_start() {

    APP_DEBUG(DEBUG_BUTTON_EN, "button_factory_reset_start\r\n");

    factory_reset = false;

    zb_factoryReset();

    g_appCtx.net_steer_start = true;
    TL_ZB_TIMER_SCHEDULE(net_steer_start_offCb, NULL, TIMEOUT_1p5MIN);
    light_blink_stop();
    light_blink_start(90, 100, 1000);
    app_setPollRate(TIMEOUT_2MIN);
}

static int32_t factoryResetCb(void *args) {

    APP_DEBUG(DEBUG_BUTTON_EN, "factoryResetCb\r\n");

    if (!g_appCtx.ota) g_appCtx.not_sleep = false;

    factory_reset = false;

    timerFactoryResetEvt = NULL;
    return -1;
}

static int32_t clearButtonFindBindFlagCb(void *args) {

    APP_DEBUG(DEBUG_BUTTON_EN, "clearButtonFindBindFlagCb\r\n");
    clearSleepTimer();
    buttonFindBindFlag = false;
    timerButtonFindBindEvt = NULL;
    return -1;
}

static void read_button_level(uint8_t i) {
    uint8_t up_down;
    uint8_t cmdOnOff = 0xFF;
    app_button_t *button = &app_button[i];
    zcl_levelAttr_t *levelAttr = zcl_levelAttrsGet();
    levelAttr += i;

    switch(device_settings.switchType[i]) {
        case ZCL_CUSTOM_SWITCH_TYPE_LEVEL_MOVE_UP:
            up_down = LEVEL_MOVE_UP;
            cmdOnOff = ZCL_CMD_ONOFF_ON;
            break;
        case ZCL_CUSTOM_SWITCH_TYPE_LEVEL_MOVE_DOWN:
            up_down = LEVEL_MOVE_DOWN;
            cmdOnOff = ZCL_CMD_ONOFF_OFF;
            break;
        default:
            break;
    }

    if (!drv_gpio_read(button->gpio)) {
        if (button->pressed) {
            if (clock_time_exceed(button->hold_time, TIMEOUT_TICK_500MS)) {
                if (button->hold == HOLD_NOT_PRESENT) {
                    button->hold = HOLD_PRESENT;
                    APP_DEBUG(DEBUG_BUTTON_EN, "Level. Press and hold button: %d\r\n", i+1);
                    if (buttonFindBindFlag) {
                        buttonFindBindFlag = false;
                        light_blink_start(1, 500, 100);
                        if (timerButtonFindBindEvt) {
                            TL_ZB_TIMER_CANCEL(&timerButtonFindBindEvt);
                        }
                        app_findBindStart(i);
                    } else if (factory_reset) {
                        if (timerFactoryResetEvt) {
                            TL_ZB_TIMER_CANCEL(&timerFactoryResetEvt);
                        }
                        button_factory_reset_start();
                    } else {
                        if (device_settings.switchType[i] == ZCL_CUSTOM_SWITCH_TYPE_LEVEL_MOVE) {
                            if (!button->level_up) {
                                up_down = LEVEL_MOVE_UP;
                                button->level_up = true;
                            } else {
                                up_down = LEVEL_MOVE_DOWN;
                                button->level_up = false;
                            }
                        }
                        APP_DEBUG(DEBUG_BUTTON_EN, "Level. Key: %d, up_down: %d, button->level_up: %d\r\n", i+1, up_down, button->level_up);
                        app_move_to_level(i+1, up_down);
                    }
                }
            }
        }
        if (button->debounce != DEBOUNCE_BUTTON) {
            button->debounce++;
            if (button->debounce == DEBOUNCE_BUTTON) {
                button->pressed = true;
                g_appCtx.not_sleep = true;
//                APP_DEBUG(DEBUG_BUTTON_EN, "Key %d pressed level\r\n", i+1);
                light_blink_start(1, 30, 1);
                if (!clock_time_exceed(button->pressed_time, TIMEOUT_TICK_500MS)) {
                    button->counter++;
                } else {
                    button->counter = 1;
                    if (!zb_isDeviceJoinedNwk() && !zb_isDeviceFactoryNew()) {
                        zb_rejoinReq(zb_apsChannelMaskGet(), g_bdbAttrs.scanDuration);
                    }
                }
                button->hold_time = button->pressed_time = clock_time();
            }
        }
    } else {
        if (button->debounce != 1) {
            button->debounce--;
            if (button->debounce == 1 && (button->pressed || button->hold == HOLD_FIX)) {
                button->released = true;
                g_appCtx.not_sleep = true;
//                APP_DEBUG(DEBUG_BUTTON_EN, "Key %d released level\r\n", i+1);
            }
        }
    }

    if (button->released && clock_time_exceed(button->pressed_time, TIMEOUT_TICK_500MS)) {
        if (button->counter >= FR_COUNTER_MAX) {
            APP_DEBUG(DEBUG_BUTTON_EN, "Reset Factory is ready from level\r\n");
            factory_reset = true;
            light_blink_stop();
            light_on();
            if (timerFactoryResetEvt) {
                TL_ZB_TIMER_CANCEL(&timerFactoryResetEvt);
            }
            timerFactoryResetEvt = TL_ZB_TIMER_SCHEDULE(factoryResetCb, NULL, TIMEOUT_3SEC);
        } else {
            if (button->hold) {
                APP_DEBUG(DEBUG_BUTTON_EN, "Level. Released button: %d\r\n", i+1);
                app_stop_level(i+1);
            } else {
                APP_DEBUG(DEBUG_BUTTON_EN, "Level. Button %d press %d times\r\n", i+1, button->counter);
                switch(button->counter) {
                    case ACTION_SINGLE:                                         // 1
                        if (device_settings.switchType[i] == ZCL_CUSTOM_SWITCH_TYPE_LEVEL_MOVE) {
                            cmdOnOff = ZCL_CMD_ONOFF_TOGGLE;
                        }
                        app_cmdOnOff(i+1, cmdOnOff);
                        break;
                    case ACTION_DOUBLE:                                         // 2
                        if (device_settings.switchType[i] != ZCL_CUSTOM_SWITCH_TYPE_LEVEL_MOVE) {
                            app_step_level(i+1, up_down);
                        }
                        break;
                    case BATTERY_COUNTER:                                       // 4
                        buttonFindBindFlag = true;
                        light_blink_stop();
                        light_blink_start(1, 2000, 1);
                        batteryCb(NULL);
                        if (!g_appCtx.timerSetPollRateEvt || !g_appCtx.timerSetPollRateEvt->used) {
                            app_setPollRate(TIMEOUT_20SEC);
                        }
                        if (timerButtonFindBindEvt) TL_ZB_TIMER_CANCEL(&timerButtonFindBindEvt);
                        timerButtonFindBindEvt = TL_ZB_TIMER_SCHEDULE(clearButtonFindBindFlagCb, NULL, TIMEOUT_3SEC);
                        break;
                    default:
                        break;
                }

            }
            if (!repeat_cmd_num) clearSleepTimer();
        }

        button->counter = 0;
        button->pressed = false;
        button->released = false;
        button->hold = HOLD_NOT_PRESENT;
    } else if (button->pressed && button->counter == 1 && button->hold == HOLD_PRESENT) {
        button->hold = HOLD_FIX;
        button->counter = 0;
        button->pressed = false;
        if (!repeat_cmd_num) clearSleepTimer();
    }
}

static void read_button_multifunction(uint8_t i) {
    bool report = false;
    app_button_t *button = &app_button[i];
    zcl_msInputAttr_t *msInputAttr = zcl_msInputAttrsGet();
    msInputAttr += i;

    if (!drv_gpio_read(button->gpio)) {
        if (button->pressed) {
            if (clock_time_exceed(button->hold_time, TIMEOUT_TICK_500MS)) {
                if (button->hold == HOLD_NOT_PRESENT) {
                    button->hold = HOLD_PRESENT;
                    if (buttonFindBindFlag) {
                        buttonFindBindFlag = false;
                        light_blink_start(1, 500, 100);
                        if (timerButtonFindBindEvt) {
                            TL_ZB_TIMER_CANCEL(&timerButtonFindBindEvt);
                        }
                        app_findBindStart(i);
                    } else if (factory_reset) {
                        if (timerFactoryResetEvt) {
                            TL_ZB_TIMER_CANCEL(&timerFactoryResetEvt);
                        }
                        button_factory_reset_start();
                    } else {
                        APP_DEBUG(DEBUG_BUTTON_EN, "Multifunction. Press and hold button: %d\r\n", i+1);
                        msInputAttr->value = ACTION_HOLD;
//                        APP_DEBUG(DEBUG_REPORTING_EN, "MSI report ep: %d value %d\r\n", i+1, msInputAttr->value);
                        app_forcedReport(i+1, ZCL_CLUSTER_GEN_MULTISTATE_INPUT_BASIC, ZCL_MULTISTATE_INPUT_ATTRID_PRESENT_VALUE);
                    }
                }
            }
        }
        if (button->debounce != DEBOUNCE_BUTTON) {
            button->debounce++;
            if (button->debounce == DEBOUNCE_BUTTON) {
                button->pressed = true;
                g_appCtx.not_sleep = true;
//                APP_DEBUG(DEBUG_BUTTON_EN, "Key %d pressed multifunction\r\n", i+1);
                light_blink_start(1, 30, 1);
                if (!clock_time_exceed(button->pressed_time, TIMEOUT_TICK_500MS)) {
                    button->counter++;
                } else {
                    button->counter = 1;
                    if (!zb_isDeviceJoinedNwk() && !zb_isDeviceFactoryNew()) {
                        zb_rejoinReq(zb_apsChannelMaskGet(), g_bdbAttrs.scanDuration);
                    }
                }
                button->hold_time = button->pressed_time = clock_time();
            }
        }
    } else {
        if (button->debounce != 1) {
            button->debounce--;
            if (button->debounce == 1 && (button->pressed || button->hold == HOLD_FIX)) {
                button->released = true;
                g_appCtx.not_sleep = true;
//                APP_DEBUG(DEBUG_BUTTON_EN, "Key %d released multifunction\r\n", i+1);
            }
        }
    }

    if (button->released && clock_time_exceed(button->pressed_time, TIMEOUT_TICK_500MS)) {
        if (button->counter >= FR_COUNTER_MAX) {
            APP_DEBUG(DEBUG_BUTTON_EN, "Reset Factory is ready from multifunction\r\n");
            factory_reset = true;
            light_blink_stop();
            light_on();
            if (timerFactoryResetEvt) {
                TL_ZB_TIMER_CANCEL(&timerFactoryResetEvt);
            }
            timerFactoryResetEvt = TL_ZB_TIMER_SCHEDULE(factoryResetCb, NULL, TIMEOUT_3SEC);
        } else {
            if (button->hold) {
                msInputAttr->value = ACTION_RELEASE;
                report = true;
                APP_DEBUG(DEBUG_BUTTON_EN, "Multifunction. Released button: %d\r\n", i+1);
            } else {
                APP_DEBUG(DEBUG_BUTTON_EN, "Multifunction. Button %d press %d times\r\n", i+1, button->counter);
                switch(button->counter) {
                    case ACTION_SINGLE:                                         // 1
                    case ACTION_DOUBLE:                                         // 2
                    case ACTION_TRIPLE:                                         // 3
                        msInputAttr->value = button->counter;
                        report = true;
                        break;
                    case BATTERY_COUNTER:
                        buttonFindBindFlag = true;
                        light_blink_stop();
                        light_blink_start(1, 2000, 1);
                        batteryCb(NULL);
                        if (!g_appCtx.timerSetPollRateEvt || !g_appCtx.timerSetPollRateEvt->used) {
                            app_setPollRate(TIMEOUT_20SEC);
                        }
                        if (timerButtonFindBindEvt) TL_ZB_TIMER_CANCEL(&timerButtonFindBindEvt);
                        timerButtonFindBindEvt = TL_ZB_TIMER_SCHEDULE(clearButtonFindBindFlagCb, NULL, TIMEOUT_3SEC);
                        break;
                    default:
                        break;
                }

            }
            if (report) {
//                APP_DEBUG(DEBUG_REPORTING_EN, "MSI report ep: %d value %d\r\n", i+1, msInputAttr->value);
                app_forcedReport(i+1, ZCL_CLUSTER_GEN_MULTISTATE_INPUT_BASIC, ZCL_MULTISTATE_INPUT_ATTRID_PRESENT_VALUE);
            }
            clearSleepTimer();
        }

        button->counter = 0;
        button->pressed = false;
        button->released = false;
        button->hold = HOLD_NOT_PRESENT;
    } else if (button->pressed && button->counter == 1 && button->hold == HOLD_PRESENT) {
        button->hold = HOLD_FIX;
        button->counter = 0;
        button->pressed = false;
        clearSleepTimer();
    }
}

static void read_button_scene(uint8_t i) {
    app_button_t *button = &app_button[i];

    if (!drv_gpio_read(button->gpio)) {
        if (button->pressed) {
            if (clock_time_exceed(button->hold_time, TIMEOUT_TICK_500MS)) {
                if (!button->hold) {
                    button->hold = true;
                    APP_DEBUG(DEBUG_BUTTON_EN, "Scene. Press and hold button: %d\r\n", i+1);
                    if (buttonFindBindFlag) {
                        buttonFindBindFlag = false;
                        light_blink_start(1, 500, 100);
                        if (timerButtonFindBindEvt) {
                            TL_ZB_TIMER_CANCEL(&timerButtonFindBindEvt);
                        }
                        app_findBindStart(i);
                    } else if (factory_reset) {
                        if (timerFactoryResetEvt) {
                            TL_ZB_TIMER_CANCEL(&timerFactoryResetEvt);
                        }
                        button_factory_reset_start();
                    }
                }
            }
        }
        if (button->debounce != DEBOUNCE_BUTTON) {
            button->debounce++;
            if (button->debounce == DEBOUNCE_BUTTON) {
//                APP_DEBUG(DEBUG_BUTTON_EN, "Key %d pressed scene\r\n", i+1);
                light_blink_start(1, 30, 1);
                if (button->counter == 0) {
                    button->pressed = true;
                    button->counter++;
                    g_appCtx.not_sleep = true;
                    if(zb_isDeviceJoinedNwk()) {
                        app_scene_send(i+1);
                    } else if (!zb_isDeviceFactoryNew()) {
                        zb_rejoinReq(zb_apsChannelMaskGet(), g_bdbAttrs.scanDuration);
                    }
                } else if (button->pressed && !clock_time_exceed(button->pressed_time, TIMEOUT_TICK_500MS)) {
                    button->counter++;
                    if (button->counter >= FR_COUNTER_MAX) {
                        APP_DEBUG(DEBUG_BUTTON_EN, "Reset Factory is ready from scene\r\n");
                        g_appCtx.not_sleep = true;
                        factory_reset = true;
                        light_blink_stop();
                        light_on();
                        if (timerFactoryResetEvt) {
                            TL_ZB_TIMER_CANCEL(&timerFactoryResetEvt);
                        }
                        timerFactoryResetEvt = TL_ZB_TIMER_SCHEDULE(factoryResetCb, NULL, TIMEOUT_3SEC);
                    }
                }
                button->hold_time = button->pressed_time = clock_time();
            }
        }
    } else {
        if (button->debounce != 1) {
            button->debounce--;
            if (button->debounce == 1 && button->pressed) {
                button->released = true;
//                APP_DEBUG(DEBUG_BUTTON_EN, "Key %d released scene\r\n", i+1);
            }
        }
    }

    if (button->released && clock_time_exceed(button->pressed_time, TIMEOUT_TICK_500MS)) {
        APP_DEBUG(DEBUG_BUTTON_EN, "Scene. Button %d pressed %d times\r\n", i+1, button->counter);
        if (button->counter == BATTERY_COUNTER) {
            buttonFindBindFlag = true;
            light_blink_stop();
            light_blink_start(1, 2000, 1);
            batteryCb(NULL);
            if (!g_appCtx.timerSetPollRateEvt || !g_appCtx.timerSetPollRateEvt->used) {
                app_setPollRate(TIMEOUT_20SEC);
            }
            if (timerButtonFindBindEvt) TL_ZB_TIMER_CANCEL(&timerButtonFindBindEvt);
            timerButtonFindBindEvt = TL_ZB_TIMER_SCHEDULE(clearButtonFindBindFlagCb, NULL, TIMEOUT_3SEC);
        }
        if (!repeat_cmd_num) clearSleepTimer();
        button->counter = 0;
        button->pressed = false;
        button->released = false;
        button->hold = false;
    }
}

static void read_button_toggle(uint8_t i) {
    uint8_t cmd_onoff;

    app_button_t *button = &app_button[i];

    if (!drv_gpio_read(button->gpio)) {
        if (button->pressed) {
            if (clock_time_exceed(button->hold_time, TIMEOUT_TICK_500MS)) {
                if (button->hold == HOLD_NOT_PRESENT) {
                    button->hold = HOLD_PRESENT;
                    APP_DEBUG(DEBUG_BUTTON_EN, "Toggle. Press and hold button: %d\r\n", i+1);
                    if (buttonFindBindFlag) {
                        buttonFindBindFlag = false;
                        light_blink_start(1, 500, 100);
                        if (timerButtonFindBindEvt) {
                            TL_ZB_TIMER_CANCEL(&timerButtonFindBindEvt);
                        }
                        app_findBindStart(i);
                    } else if (factory_reset) {
                        if (timerFactoryResetEvt) {
                            TL_ZB_TIMER_CANCEL(&timerFactoryResetEvt);
                        }
                        button_factory_reset_start();
                    }
                }
            }
        }
        if (button->debounce != DEBOUNCE_BUTTON) {
            button->debounce++;
            if (button->debounce == DEBOUNCE_BUTTON) {
//                APP_DEBUG(DEBUG_BUTTON_EN, "Key %d pressed toggle\r\n", i+1);
                light_blink_start(1, 30, 1);
                if (button->counter == 0) {
                    button->pressed = true;
                    button->counter++;
                    g_appCtx.not_sleep = true;
                    if(zb_isDeviceJoinedNwk()) {
                        cmd_onoff = ZCL_CMD_ONOFF_ON;
                        switch(device_settings.switchActions[i]) {
                            case ZCL_SWITCH_ACTION_OFF_ON:
                                cmd_onoff = ZCL_CMD_ONOFF_ON;
                                break;
                            case ZCL_SWITCH_ACTION_ON_OFF:
                                cmd_onoff = ZCL_CMD_ONOFF_OFF;
                                break;
                            case ZCL_SWITCH_ACTION_TOGGLE:
                                cmd_onoff = ZCL_CMD_ONOFF_TOGGLE;
                                break;
                            default:
                                break;
                        }
                        app_cmdOnOff(i+1, cmd_onoff);
                    } else if (!zb_isDeviceFactoryNew()) {
                        zb_rejoinReq(zb_apsChannelMaskGet(), g_bdbAttrs.scanDuration);
                    }
                } else if (button->pressed && !clock_time_exceed(button->pressed_time, TIMEOUT_TICK_500MS)) {
                    button->counter++;
                }
                button->hold_time = button->pressed_time = clock_time();
            }
        }
    } else {
        if (button->debounce != 1) {
            button->debounce--;
            if (button->debounce == 1 && (button->pressed || button->hold == HOLD_FIX)) {
                button->released = true;
                g_appCtx.not_sleep = true;
//                APP_DEBUG(DEBUG_BUTTON_EN, "Key %d released toggle\r\n", i+1);
                if((button->counter == 1 || button->hold == HOLD_FIX) && zb_isDeviceJoinedNwk()) {
                    if (device_settings.switchType[i] == ZCL_SWITCH_TYPE_MOMENTARY) {
                        cmd_onoff = ZCL_SWITCH_ACTION_ON_OFF;
                        switch(device_settings.switchActions[i]) {
                            case ZCL_SWITCH_ACTION_OFF_ON:
                                cmd_onoff = ZCL_CMD_ONOFF_OFF;
                                break;
                            case ZCL_SWITCH_ACTION_ON_OFF:
                                cmd_onoff = ZCL_CMD_ONOFF_ON;
                                break;
                            case ZCL_SWITCH_ACTION_TOGGLE:
                                cmd_onoff = ZCL_CMD_ONOFF_TOGGLE;
                                break;
                            default:
                                break;
                        }
                        app_cmdOnOff(i+1, cmd_onoff);
                    }
                }
            }
        }
    }

    if (button->released && clock_time_exceed(button->pressed_time, TIMEOUT_TICK_500MS)) {
        APP_DEBUG(DEBUG_BUTTON_EN, "Toggle. Button %d pressed %d times\r\n", i+1, button->counter);
        if (button->counter == BATTERY_COUNTER) {
            buttonFindBindFlag = true;
            light_blink_stop();
            light_blink_start(1, 2000, 1);
            batteryCb(NULL);
            if (!g_appCtx.timerSetPollRateEvt || !g_appCtx.timerSetPollRateEvt->used) {
                app_setPollRate(TIMEOUT_20SEC);
            }
            if (timerButtonFindBindEvt) TL_ZB_TIMER_CANCEL(&timerButtonFindBindEvt);
            timerButtonFindBindEvt = TL_ZB_TIMER_SCHEDULE(clearButtonFindBindFlagCb, NULL, TIMEOUT_3SEC);
        } else if (button->counter >= FR_COUNTER_MAX) {
            APP_DEBUG(DEBUG_BUTTON_EN, "Reset Factory is ready from toggle\r\n");
            g_appCtx.not_sleep = true;
            factory_reset = true;
            light_blink_stop();
            light_on();
            if (timerFactoryResetEvt) {
                TL_ZB_TIMER_CANCEL(&timerFactoryResetEvt);
            }
            timerFactoryResetEvt = TL_ZB_TIMER_SCHEDULE(factoryResetCb, NULL, TIMEOUT_3SEC);
#if UART_PRINTF_MODE && DEBUG_BUTTON_EN
        } else if (button->counter == 3) {
            aps_binding_entry_t *bind_tbl = bindTblEntryGet();
            for (uint8_t j = 0; j < APS_BINDING_TABLE_NUM; j++) {
                if (bind_tbl->used) {
                    APP_DEBUG(DEBUG_BUTTON_EN, "Table num: %d used\r\n", j);
                    APP_DEBUG(DEBUG_BUTTON_EN, "    srcEp:      %d\r\n", bind_tbl->srcEp);
                    APP_DEBUG(DEBUG_BUTTON_EN, "    dstEp:      %d\r\n", bind_tbl->dstExtAddrInfo.dstEp);
                    APP_DEBUG(DEBUG_BUTTON_EN, "    clusterId:  0x%04x\r\n", bind_tbl->clusterId);
                    if (bind_tbl->dstAddrMode == APS_LONG_DSTADDR_WITHEP) {
                        APP_DEBUG(DEBUG_LEVEL_EN, "    dst_ieee:   0x%02x%02x%02x%02x%02x%02x%02x%02x\r\n",
                                bind_tbl->dstExtAddrInfo.extAddr[0], bind_tbl->dstExtAddrInfo.extAddr[1],
                                bind_tbl->dstExtAddrInfo.extAddr[2], bind_tbl->dstExtAddrInfo.extAddr[3],
                                bind_tbl->dstExtAddrInfo.extAddr[4], bind_tbl->dstExtAddrInfo.extAddr[5],
                                bind_tbl->dstExtAddrInfo.extAddr[6], bind_tbl->dstExtAddrInfo.extAddr[7]);
                    } else {
                        APP_DEBUG(DEBUG_LEVEL_EN, "    shortAddr:   0x%04x\r\n", bind_tbl->groupAddr);
                    }
                    APP_DEBUG(DEBUG_LEVEL_EN, "\r\n\n");
                } else {
                    APP_DEBUG(DEBUG_BUTTON_EN, "Table num: %d not used\r\n\n", j);
                }
                bind_tbl++;
            }
#endif
        }

        if (!repeat_cmd_num) clearSleepTimer();
        button->counter = 0;
        button->pressed = false;
        button->released = false;
        button->hold = HOLD_NOT_PRESENT;
    } else if (button->pressed && button->counter == 1 && button->hold == HOLD_PRESENT) {
        button->hold = HOLD_FIX;
        button->counter = 0;
        button->pressed = false;
        if (!repeat_cmd_num) clearSleepTimer();
    }
}

void button_handler() {

    for (uint8_t i = 0; i < (device_button_model + 1); i++) {
        switch(device_settings.switchType[i]) {
            case ZCL_SWITCH_TYPE_TOGGLE:
            case ZCL_SWITCH_TYPE_MOMENTARY:
                read_button_toggle(i);
                break;
            case ZCL_SWITCH_TYPE_MULTIFUNCTION:
                read_button_multifunction(i);
                break;
            case ZCL_CUSTOM_SWITCH_TYPE_LEVEL_MOVE:
            case ZCL_CUSTOM_SWITCH_TYPE_LEVEL_MOVE_UP:
            case ZCL_CUSTOM_SWITCH_TYPE_LEVEL_MOVE_DOWN:
                read_button_level(i);
                break;
            case ZCL_CUSTOM_SWITCH_TYPE_SCENE:
                read_button_scene(i);
                break;
            default:
                break;
        }
    }
}

uint8_t button_idle() {
    app_button_t *button = NULL;
    for (uint8_t i = 0; i < (device_button_model + 1); i++) {
        button = &app_button[i];
        if ((button->debounce != 1 && button->debounce != DEBOUNCE_BUTTON) ||
                button->pressed || button->counter || factory_reset || buttonFindBindFlag) {
            return true;
        }
    }
    return false;
}

void button_clear_sleep() {
    if (timerClearSleepEvt) TL_ZB_TIMER_CANCEL(&timerClearSleepEvt);
}

void button_init() {
    app_button[0].gpio = BUTTON1_GPIO;
    if (device_button_model == DEVICE_BUTTON_2) {
        app_button[1].gpio = BUTTON3_GPIO;
        app_button[2].gpio = BUTTON2_GPIO;
    } else {
        app_button[1].gpio = BUTTON2_GPIO;
        app_button[2].gpio = BUTTON3_GPIO;
    }
    if (device_button_model == DEVICE_BUTTON_6) {
        app_button[3].gpio = BUTTON4_6_GPIO;
        app_button[4].gpio = BUTTON5_GPIO;
        app_button[5].gpio = BUTTON6_GPIO;
    } else {
        app_button[3].gpio = BUTTON4_GPIO;
    }
    app_button_t *button = NULL;
    for (uint8_t i = 0; i < DEVICE_BUTTON_MAX; i++) {
        button = &app_button[i];
        button->debounce = 1;
        button->hold = false;
        button->counter = 0;
        button->pressed = false;
        button->released = false;
        button->level_up = false;
        button->pressed_time = clock_time();
    }
}

//void clearButtonSleepTimer() {
//    if (timerClearSleepEvt) {
//        TL_ZB_TIMER_CANCEL(&timerClearSleepEvt);
//    }
//    timerClearSleepEvt = TL_ZB_TIMER_SCHEDULE(clearSleepCb, NULL, TIMEOUT_100MS);
//}

void clearButtonSleepTimer() {

    if ((!g_appCtx.timerSetPollRateEvt || !g_appCtx.timerSetPollRateEvt->used ||  g_appCtx.timerSetPollRateEvt->isBusy) &&
            !g_appCtx.ota && !buttonFindBindFlag) {
        g_appCtx.not_sleep = false;
    }
}

