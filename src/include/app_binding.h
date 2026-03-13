#ifndef SRC_INCLUDE_APP_BINDING_H_
#define SRC_INCLUDE_APP_BINDING_H_

typedef struct {
    aps_binding_entry_t *bind_entry;
    uint16_t short_addr;
} dev_bind_t;

typedef struct {
    ev_timer_event_t *timerGetIeeeCoordinatorEvt;
    uint8_t idx;
    bool addrCoordinatorSet;
    addrExt_t extAddrCoordinator;
    dev_bind_t dev_bind[APS_BINDING_TABLE_NUM];
} app_bind_tbl_t;

extern app_bind_tbl_t *g_app_bind_tbl;

void app_update_bind_tbl(void *args);
int32_t app_getIeeeCoordinatorCb(void *args);

#endif /* SRC_INCLUDE_APP_BINDING_H_ */
