#include "app_main.h"

#define ADDR_DEVICE_BUTTON_CFG  0x75000
#define ID_DEVICE_BUTTON_CFG    0x1465

typedef struct {
    uint16_t id;
    uint8_t  device_model;
    uint8_t  crc;
} config_switch_model_t;

static bool first_start = true;
static uint8_t zb_modelId[14] = {12,'T','S','0','0','4','1','-','z','-','S','l','D',0};

device_button_model_t device_button_model = DEVICE_MODEL;
device_settings_t device_settings;

static void device_model_init() {

    switch(device_button_model) {
        case DEVICE_BUTTON_1:
            zb_modelId[6] = '1';
            break;
        case DEVICE_BUTTON_2:
            zb_modelId[6] = '2';
            break;
        case DEVICE_BUTTON_3:
            zb_modelId[6] = '3';
            break;
        case DEVICE_BUTTON_4:
            zb_modelId[6] = '4';
            break;
        case DEVICE_BUTTON_5:
            zb_modelId[6] = '5';
            break;
        case DEVICE_BUTTON_6:
            zb_modelId[6] = '6';
            break;
        default:
            zb_modelId[6] = '1';
            break;
    }
    memcpy(g_zcl_basicAttrs.modelId, zb_modelId, 9);
    button_init();
}

#if UART_PRINTF_MODE
static void print_setting_sr(nv_sts_t st, device_settings_t *device_settings_tmp, bool save) {

    DEBUG(DEBUG_SAVE_EN, "Settings %s. Return: %s\r\n", save?"saved":"restored", st==NV_SUCC?"Ok":"Error");

    for (uint8_t i = 0; i < DEVICE_BUTTON_MAX; i++) {
        DEBUG(DEBUG_SAVE_EN, "switchActions%d:     0x%02x\r\n", i, device_settings_tmp->switchActions[i]);
        DEBUG(DEBUG_SAVE_EN, "switchType%d:        0x%02x\r\n", i, device_settings_tmp->switchType[i]);
    }

}
#endif

nv_sts_t device_settings_default() {

    nv_sts_t st = NV_SUCC;

#if NV_ENABLE

    DEBUG(UART_PRINTF_MODE, "Saved device default settings\r\n");

    for (uint8_t i = 0; i < DEVICE_BUTTON_MAX; i++) {
        device_settings.switchActions[i] = ZCL_SWITCH_ACTION_TOGGLE;
        device_settings.switchType[i] = ZCL_SWITCH_TYPE_TOGGLE;
        device_settings.defaultMoveRate[i] = DEFAULT_MOVE_RATE;
    }

    device_settings.crc = checksum((uint8_t*)&device_settings, sizeof(device_settings_t)-1);
    st = nv_flashWriteNew(1, NV_MODULE_APP,  NV_ITEM_APP_USER_CFG, sizeof(device_settings_t), (uint8_t*)&device_settings);

#else
    st = NV_ENABLE_PROTECT_ERROR;
#endif

    return st;
}

nv_sts_t device_settings_restore() {

    nv_sts_t st = NV_SUCC;

#if NV_ENABLE

    device_settings_t device_settings_tmp;

    st = nv_flashReadNew(1, NV_MODULE_APP,  NV_ITEM_APP_USER_CFG, sizeof(device_settings_t), (uint8_t*)&device_settings_tmp);

    if (st == NV_SUCC && device_settings_tmp.crc == checksum((uint8_t*)&device_settings_tmp, sizeof(device_settings_t)-1)) {

        DEBUG(UART_PRINTF_MODE, "Restored device settings\r\n");
#if UART_PRINTF_MODE
        print_setting_sr(st, &device_settings_tmp, false);
#endif

    } else {
        /* default config */
        DEBUG(UART_PRINTF_MODE, "Default device settings \r\n");

        for (uint8_t i = 0; i < DEVICE_BUTTON_MAX; i++) {
            device_settings_tmp.switchActions[i] = ZCL_SWITCH_ACTION_TOGGLE;
            device_settings_tmp.switchType[i] = ZCL_SWITCH_TYPE_TOGGLE;
            device_settings_tmp.defaultMoveRate[i] = DEFAULT_MOVE_RATE;
        }
    }

    memcpy(&device_settings, &device_settings_tmp, (sizeof(device_settings_t)));
    for (uint8_t i = 0; i < DEVICE_BUTTON_MAX; i++) {
        g_zcl_onOffCfgAttrs[i].custom_swtichType = device_settings.switchType[i];
        g_zcl_onOffCfgAttrs[i].switchActions = device_settings.switchActions[i];
        g_zcl_levelAttrs[i].defaultMoveRate = device_settings.defaultMoveRate[i];
    }

#else
    st = NV_ENABLE_PROTECT_ERROR;
#endif

    return st;
}

nv_sts_t device_settings_save() {
    nv_sts_t st = NV_SUCC;

#if NV_ENABLE

    DEBUG(UART_PRINTF_MODE, "Saved device settings\r\n");

    device_settings.crc = checksum((uint8_t*)&device_settings, sizeof(device_settings_t)-1);
    st = nv_flashWriteNew(1, NV_MODULE_APP,  NV_ITEM_APP_USER_CFG, sizeof(device_settings_t), (uint8_t*)&device_settings);

#else
    st = NV_ENABLE_PROTECT_ERROR;
#endif

    return st;
}
void device_model_restore() {

    config_switch_model_t model_cfg;

    flash_read_page(ADDR_DEVICE_BUTTON_CFG, sizeof(config_switch_model_t), (uint8_t*)&model_cfg);

    if (model_cfg.id == ID_DEVICE_BUTTON_CFG && model_cfg.crc == checksum((uint8_t*)&model_cfg, sizeof(config_switch_model_t)-1)) {
        device_button_model = model_cfg.device_model;
        DEBUG(UART_PRINTF_MODE, "Model restore: TS004%d\r\n", device_button_model+1);
        device_model_init();
    } else {
        device_button_model = DEVICE_MODEL;
        DEBUG(UART_PRINTF_MODE, "Default model: TS004%d\r\n", device_button_model+1);
        device_model_save(device_button_model);
    }
}

void device_model_save(uint8_t model) {
    config_switch_model_t model_cfg;

    model_cfg.id = ID_DEVICE_BUTTON_CFG;
    device_button_model = model_cfg.device_model = model;

    flash_erase(ADDR_DEVICE_BUTTON_CFG);
    model_cfg.crc = checksum((uint8_t*)&(model_cfg), sizeof(config_switch_model_t)-1);
    flash_write(ADDR_DEVICE_BUTTON_CFG, sizeof(config_switch_model_t), (uint8_t*)&(model_cfg));

    DEBUG(UART_PRINTF_MODE, "Model save: TS004%d\r\n", device_button_model+1);

    device_model_init();
}

void device_init() {
    if (first_start) {
        first_start = false;
        device_model_restore();
    } else {
        device_model_init();
    }
}

