#include "app_main.h"

#ifndef DEBOUNCE_BUTTON
#define DEBOUNCE_BUTTON     16      /* number of polls for debounce                 */
#endif
#define FR_COUNTER_MAX      5      /* number for factory reset                     */

typedef struct {
    bool        released;
    bool        pressed;
    bool        hold;
    uint8_t     counter;
    uint8_t     debounce;
    uint32_t    pressed_time;
    uint32_t    released_time;
    uint32_t    hold_time;
    uint32_t    gpio;
} app_button_t;

static app_button_t app_button[DEVICE_BUTTON_MAX];
//static app_button_t *app_button = app_button_cfg;

static void button_factory_reset_start(void *args) {

    DEBUG(DEBUG_BUTTON_EN, "button_factory_reset_start\r\n");

    zb_factoryReset();

    g_appCtx.net_steer_start = true;
    TL_ZB_TIMER_SCHEDULE(net_steer_start_offCb, NULL, TIMEOUT_1MIN30SEC);
    light_blink_stop();
    light_blink_start(90, 100, 1000);
    app_setPollRate(TIMEOUT_2MIN);
}

static int32_t resetMsiTimerCb(void *args) {

    uint8_t i = (uint8_t)((uint32_t)args);

    zcl_msInputAttr_t *msInputAttr = zcl_msInputAttrsGet();
    msInputAttr += i;
    msInputAttr->value = ACTION_EMPTY;


    return -1;
}

static void read_switch_multifunction(uint8_t i) {
    app_button_t *button = &app_button[i];
    zcl_msInputAttr_t *msInputAttr = zcl_msInputAttrsGet();
    msInputAttr += i;

    if (!drv_gpio_read(button->gpio)) {
        if (button->pressed) {
            if (clock_time_exceed(button->hold_time, TIMEOUT_TICK_750MS)) {
                if (!button->hold) {
                    button->hold = true;
                    DEBUG(DEBUG_BUTTON_EN, "Press and hold button: %d\r\n", i+1);
                    msInputAttr->value = ACTION_HOLD;
                }
            }
        }
        if (button->debounce != DEBOUNCE_BUTTON) {
            button->debounce++;
            if (button->debounce == DEBOUNCE_BUTTON) {
                button->pressed = true;
                DEBUG(DEBUG_BUTTON_EN, "Key %d pressed\r\n", i+1);
                light_blink_start(1, 30, 1);
                if (!clock_time_exceed(button->pressed_time, TIMEOUT_TICK_750MS)) {
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
            if (button->debounce == 1 && button->pressed) {
                button->released = true;
                DEBUG(DEBUG_BUTTON_EN, "Key %d released\r\n", i+1);
            }
        }
    }

    if (button->released && clock_time_exceed(button->pressed_time, TIMEOUT_TICK_750MS)) {
        if (button->hold) {
            msInputAttr->value = ACTION_RELEASE;
            DEBUG(DEBUG_BUTTON_EN, "Released button: %d\r\n", i+1);
        } else {
            switch(button->counter) {
                case ACTION_SINGLE:                                         // 1
                case ACTION_DOUBLE:                                         // 2
                case ACTION_TRIPLE:                                         // 3
                    msInputAttr->value = button->counter;
                    DEBUG(DEBUG_BUTTON_EN, "Button %d press %d times\r\n", i+1, msInputAttr->value);
                    break;
//                case ACTION_CLEAR:                                          // 250
//                    msInputAttr->value = ACTION_EMPTY;                      // 300
//                    report = true;
//                    break;
                default:
                    if (button->counter >= FR_COUNTER_MAX) {
                        DEBUG(DEBUG_BUTTON_EN, "Reset Factory\r\n");
                        button_factory_reset_start(NULL);
                    }
                    break;
            }

        }
        button->counter = 0;
        button->pressed = false;
        button->released = false;
        button->hold = false;
        TL_ZB_TIMER_SCHEDULE(resetMsiTimerCb, (void*)((uint32_t)i), TIMEOUT_750MS);
    }
}

static void read_button_toggle(uint8_t i) {
    uint8_t cmd_onoff;

    app_button_t *button = &app_button[i];

    if (!drv_gpio_read(button->gpio)) {
        if (button->debounce != DEBOUNCE_BUTTON) {
            button->debounce++;
            if (button->debounce == DEBOUNCE_BUTTON) {
                DEBUG(DEBUG_BUTTON_EN, "Key %d pressed\r\n", i+1);
                light_blink_start(1, 30, 1);
                if (button->pressed && !clock_time_exceed(button->pressed_time, TIMEOUT_TICK_750MS)) {
                    button->pressed_time = clock_time();
                    button->counter++;
                    if (button->counter >= FR_COUNTER_MAX) {
                        DEBUG(DEBUG_BUTTON_EN, "Factory reset\r\n");
                        button_factory_reset_start(NULL);
                        button->counter = 0;
                    }
                } else {
                    button->pressed = true;
                    button->pressed_time = clock_time();
                    button->counter = 1;
                    if(zb_isDeviceJoinedNwk()) {
//                        app_setPollRate(TIMEOUT_5SEC);
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
                        cmdOnOff(i+1, cmd_onoff);
                    } else if (!zb_isDeviceFactoryNew()) {
                        zb_rejoinReq(zb_apsChannelMaskGet(), g_bdbAttrs.scanDuration);
                    }
                }
            }
        }
    } else {
        if (button->debounce != 1) {
            button->debounce--;
            if (button->debounce == 1 && button->pressed) {
                button->released = true;
                DEBUG(DEBUG_BUTTON_EN, "Key %d released\r\n", i+1);
                if(button->counter == 1 && zb_isDeviceJoinedNwk()) {
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
                        cmdOnOff(i+1, cmd_onoff);
                    }
                }
//                button->released_time = clock_time();
            }
        }
    }

    if (button->released && clock_time_exceed(button->pressed_time, TIMEOUT_TICK_750MS)) {
        button->counter = 0;
        button->pressed = false;
        button->released = false;
    }

}

void button_handler() {

    for (uint8_t i = 0; i < (device_button_model + 1); i++) {
        if (device_settings.switchType[i] == ZCL_SWITCH_TYPE_MULTIFUNCTION) {
            read_switch_multifunction(i);
        } else {
            read_button_toggle(i);
        }
//        switch(device_settings.switchType[i]) {
//            case ZCL_SWITCH_TYPE_TOGGLE:
//            case ZCL_SWITCH_TYPE_MOMENTARY:
//                break;
//            case ZCL_SWITCH_TYPE_MULTIFUNCTION:
//                break;
//            default:
//                break;
//        }
    }
}

u8 button_idle() {
    app_button_t *button = NULL;
    for (uint8_t i = 0; i < (device_button_model + 1); i++) {
        button = &app_button[i];
        if ((button->debounce != 1 && button->debounce != DEBOUNCE_BUTTON) || button->pressed || button->counter) {
            return true;
        }
    }
    return false;
}

void button_init() {
    app_button[0].gpio = BUTTON1_GPIO;
    app_button[1].gpio = BUTTON2_GPIO;
    app_button[2].gpio = BUTTON3_GPIO;
    app_button[3].gpio = BUTTON4_GPIO;
//    app_button[4].gpio = BUTTON1_GPIO;
//    app_button[5].gpio = BUTTON1_GPIO;
    app_button_t *button = NULL;
    for (uint8_t i = 0; i < DEVICE_BUTTON_MAX; i++) {
        button = &app_button[i];
        button->debounce = 1;
        button->hold = false;
        button->counter = 0;
        button->pressed = false;
        button->released = false;
        button->pressed_time = clock_time();
    }
}
