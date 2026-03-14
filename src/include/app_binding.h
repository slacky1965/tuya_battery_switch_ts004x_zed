#ifndef SRC_INCLUDE_APP_BINDING_H_
#define SRC_INCLUDE_APP_BINDING_H_

#define BIND_CMD_NUM (APS_BINDING_TABLE_NUM * 4)
enum {
    BIND_CMD_NONE  = 0x00,
    BIND_CMD_ONOFF = 0x01,
    BIND_CMD_MULTI = 0x02,
    BIND_CMD_SCENE = 0x04,
    BIND_CMD_LEVEL = 0x08,
};

enum {
    BIND_CMD_MODE_SINGLE = 0,
    BIND_CMD_MODE_CHECK_ANSWER,
    BIND_CMD_MODE_REPEAT
};

typedef struct {
    aps_binding_entry_t *bind_entry;
    uint16_t short_addr;
} dev_bind_t;

typedef struct {
    ev_timer_event_t *timerGetIeeeCoordinatorEvt;
    ev_timer_event_t *timerCmdRspEvt;
    uint8_t onoff_num;
    uint8_t level_num;
    uint8_t multi_num;
    uint8_t scene_num;
    uint8_t idx;
    bool    addrCoordinatorSet;
    addrExt_t  extAddrCoordinator;
    dev_bind_t dev_bind[APS_BINDING_TABLE_NUM];
    uint16_t addr_onoff[APS_BINDING_TABLE_NUM];
    uint16_t addr_level[APS_BINDING_TABLE_NUM];
    uint16_t addr_multi[APS_BINDING_TABLE_NUM];
    uint16_t addr_scene[APS_BINDING_TABLE_NUM];
    uint8_t  cmd;
} app_bind_tbl_t;

typedef struct {
    uint8_t  used :1;
    uint8_t  cmd_send :1;
    uint8_t  status_rsp :1;
    uint8_t  repeat :1;
    uint8_t  command :4;
    uint8_t  seq_num;
    uint8_t  ep;
    uint8_t  cluster;
    uint8_t  type_cmd;
    uint16_t short_addr;
} app_bind_cmd_t;


extern app_bind_tbl_t *g_app_bind_tbl;

void app_update_bind_tbl(void *args);
void app_update_addr_bind(uint8_t i);
int32_t app_getIeeeCoordinatorCb(void *args);
void app_bind_cmd_init();
app_bind_cmd_t *get_bind_cmd_free();
int32_t check_bind_cmd_rspCb(void *args);
void set_send_bind_cmd(aps_data_ind_t *pIndInfo, uint8_t seq_num);

#endif /* SRC_INCLUDE_APP_BINDING_H_ */
