#ifndef SRC_INCLUDE_APP_DEVICE_H_
#define SRC_INCLUDE_APP_DEVICE_H_

#ifndef DEVICE_MODEL
#define DEVICE_MODEL    DEVICE_BUTTON_1
#endif

typedef enum {
    DEVICE_BUTTON_1 = 0,                    /* TS0041 */
    DEVICE_BUTTON_2,                        /* TS0042 */
    DEVICE_BUTTON_3,                        /* TS0043 */
    DEVICE_BUTTON_4,                        /* TS0044 */
    DEVICE_BUTTON_5,                        /* TS0045 */
    DEVICE_BUTTON_6,                        /* TS0046 */
    DEVICE_BUTTON_MAX
} device_button_model_t;

typedef struct __attribute__((packed)) {
    uint8_t switchType[DEVICE_BUTTON_MAX];      // 0x00 - toggle, 0x01 - momentary
    uint8_t switchActions[DEVICE_BUTTON_MAX];
    uint8_t crc;
} device_settings_t;

extern device_button_model_t device_button_model;
extern device_settings_t device_settings;

void device_model_restore();
void device_model_save(uint8_t model);
void device_init();
nv_sts_t device_settings_restore();
nv_sts_t device_settings_save();
nv_sts_t device_settings_default();

#endif /* SRC_INCLUDE_APP_DEVICE_H_ */
