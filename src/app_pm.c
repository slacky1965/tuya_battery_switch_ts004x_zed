#include "app_main.h"

#if PM_ENABLE
/**
 *  @brief Definition for wakeup source and level for PM
 */

static drv_pm_pinCfg_t pin_PmCfg[] = {
    {
        BUTTON1_GPIO,
        PM_WAKEUP_LEVEL_LOW
    },
    {
        BUTTON2_GPIO,
        PM_WAKEUP_LEVEL_LOW
    },
    {
        BUTTON3_GPIO,
        PM_WAKEUP_LEVEL_LOW
    },
    {
        BUTTON4_GPIO,
        PM_WAKEUP_LEVEL_LOW
    },
    {
        BUTTON4_6_GPIO,
        PM_WAKEUP_LEVEL_LOW
    },
    {
        BUTTON5_GPIO,
        PM_WAKEUP_LEVEL_LOW
    },
    {
        BUTTON6_GPIO,
        PM_WAKEUP_LEVEL_LOW
    },
};

void app_wakeupPinConfig() {
    drv_pm_wakeupPinConfig(pin_PmCfg, sizeof(pin_PmCfg)/sizeof(drv_pm_pinCfg_t));
}


void app_wakeupPinLevelChange() {
    drv_pm_wakeupPinLevelChange(pin_PmCfg, 1);
}

static void app_drv_pm_lowPowerEnter(void) {
#if PM_ENABLE
    drv_pm_wakeup_src_e wakeupSrc = PM_WAKEUP_SRC_PAD;
    u32 sleepTime = 0;
    bool longSleep = 0;

#if !defined(__PROJECT_TL_BOOT_LOADER__) && !defined(__PROJECT_TL_SNIFFER__)
    if (tl_stackBusy() || !zb_isTaskDone()) {
        return;
    }

    apsCleanToStopSecondClock();
#endif

    u32 r = drv_disable_irq();

    ev_timer_event_t *timerEvt = ev_timer_nearestGet();
    if (timerEvt) {
        wakeupSrc |= PM_WAKEUP_SRC_TIMER;
        sleepTime = timerEvt->timeout;

        if (sleepTime) {
            if (sleepTime > PM_NORMAL_SLEEP_MAX) {
                longSleep = 1;
            } else {
                longSleep = 0;
            }
        } else {
            drv_restore_irq(r);
            return;
        }
    }

    DEBUG(DEBUG_PM_EN, "Not long deep sleep start with time: %d ms\r\n", sleepTime);

#if defined(MCU_CORE_826x)
    drv_pm_sleep_mode_e sleepMode = (wakeupSrc & PM_WAKEUP_SRC_TIMER) ? PM_SLEEP_MODE_SUSPEND : PM_SLEEP_MODE_DEEPSLEEP;
#elif defined(MCU_CORE_8258) || defined(MCU_CORE_8278) || defined(MCU_CORE_B91) || defined(MCU_CORE_B92) || defined(MCU_CORE_TL721X) || defined(MCU_CORE_TL321X)
    drv_pm_sleep_mode_e sleepMode = (wakeupSrc & PM_WAKEUP_SRC_TIMER) ? PM_SLEEP_MODE_DEEP_WITH_RETENTION : PM_SLEEP_MODE_DEEPSLEEP;
#endif

#if !defined(__PROJECT_TL_BOOT_LOADER__) && !defined(__PROJECT_TL_SNIFFER__)
    rf_paShutDown();
    if (sleepMode == PM_SLEEP_MODE_DEEPSLEEP) {
        drv_pm_deepSleep_frameCnt_set(ss_outgoingFrameCntGet());
    }
#endif

    if (!longSleep) {
        drv_pm_sleep(sleepMode, wakeupSrc, sleepTime);
    } else {
        drv_pm_longSleep(sleepMode, wakeupSrc, sleepTime);
    }

    drv_restore_irq(r);
#endif
}

void app_lowPowerEnter() {

//    printf("app_lowPowerEnter(). g_appCtx.not_sleep: %d\r\n", g_appCtx.not_sleep);

    uint32_t durationMs = 0;

    app_wakeupPinLevelChange();

    if (g_appCtx.not_sleep) {
        /* SDK deep sleep with SRAM retention */
#if DEBUG_PM_EN
        app_drv_pm_lowPowerEnter();
#else
        drv_pm_lowPowerEnter();
#endif
    } else /*if (zb_isDeviceJoinedNwk())*/{
        /* app deep sleep with SRAM retention */
        if (tl_stackBusy() || !zb_isTaskDone()) {
            DEBUG(DEBUG_PM_EN, "Stack or Task busy. Return from deep sleep start\r\n");
            return;
        }

        apsCleanToStopSecondClock();

        uint32_t r = drv_disable_irq();
        rf_paShutDown();

        durationMs = g_appCtx.timerBatteryEvt->timeout /*TIME_LONG_DEEP_SLEEP * 1000*/;

        DEBUG(DEBUG_PM_EN, "Long deep sleep start with time: %d sec\r\n", durationMs / 1000);

        drv_pm_longSleep(PM_SLEEP_MODE_DEEP_WITH_RETENTION, PM_WAKEUP_SRC_PAD | PM_WAKEUP_SRC_TIMER, durationMs);

        drv_restore_irq(r);

    }
}

#endif


//int32_t check_sleepCb(void *args) {
//
//    if (zb_getLocalShortAddr() < 0xFFF8) {
//
//        if (g_appCtx.ota) {
//            printf("check_sleepCb - OTA\r\n");
//            g_appCtx.timerCheckSleepEvt = NULL;
//            return -1;
//        }
//
//        printf("check_sleepCb - reset\r\n");
//        sleep_ms(250);
//
//        zb_resetDevice();
//        return -1;
//    }
//
////    if (zb_isDeviceJoinedNwk()) {
////
////        printf("check_sleepCb - reset\r\n");
////        sleep_ms(250);
////
////        zb_resetDevice();
////        return -1;
////    }
//
//    printf("check_sleepCb - no joined\r\n");
//    return 0;
//}
