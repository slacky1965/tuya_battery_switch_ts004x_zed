#include "app_main.h"

enum {
    BIND_ADDRMODE_GROUP = 1,
    BIND_ADDRMODE_IEEE = 3,
};

static app_bind_tbl_t app_bind_tbl = {
        .timerGetIeeeCoordinatorEvt = NULL,
        .timerCmdRspEvt = NULL,
        .idx = 0,
        .extAddrCoordinator = {0},
        .addrCoordinatorSet = false,
};

static ev_timer_event_t *timerCheckShortAddrRspEvt = NULL;
static uint8_t check_short_addr_rsp_count = 0;
static uint8_t idx = 0;
static uint8_t not_sleep = 0xff;

app_bind_tbl_t *g_app_bind_tbl = &app_bind_tbl;
app_bind_cmd_t app_bind_cmd[BIND_CMD_NUM];
uint8_t repeat_bind_cmd = BIND_CMD_NONE;

static void app_get_short_addr(void *args);

static void app_getIeeeCb(void *args) {
    zdo_zdpDataInd_t *p = (zdo_zdpDataInd_t *)args;
    zdo_ieee_addr_resp_t *rsp = (zdo_ieee_addr_resp_t*)p->zpdu;

    if (rsp->status == ZDO_SUCCESS) {
        ZB_IEEE_ADDR_COPY(g_app_bind_tbl->extAddrCoordinator, rsp->ieee_addr_remote);
        g_app_bind_tbl->addrCoordinatorSet = true;
    }
}

static void app_getIeee(uint16_t dstAddr) {
    zdo_ieee_addr_req_t pReq;
    u8 sn = 0;
    pReq.nwk_addr_interest = dstAddr;
    pReq.req_type = ZDO_ADDR_REQ_SINGLE_RESP;
    pReq.start_index = 0;
    zb_zdoIeeeAddrReq(dstAddr, &pReq, &sn, app_getIeeeCb);
}

static int32_t check_short_addr_rspCb(void *args) {

    for (uint8_t i = 0; i < g_app_bind_tbl->idx; i++) {
        if (!g_app_bind_tbl->dev_bind[i].short_addr) {
            if (check_short_addr_rsp_count++ < 3) {
                APP_DEBUG(DEBUG_BINDING_EN, "No rsp. idx: %d, counter: %d. Continue get addr\r\n", idx, check_short_addr_rsp_count);
                TL_SCHEDULE_TASK(app_get_short_addr, NULL);
                return 0;
            } else {
                APP_DEBUG(DEBUG_BINDING_EN, "No rsp. idx: %d, counter: %d\r\n", idx, check_short_addr_rsp_count);
            }
        }
    }

    APP_DEBUG(DEBUG_BINDING_EN, "All ok. idx: %d, counter: %d\r\n", idx, check_short_addr_rsp_count);
    g_appCtx.not_sleep = not_sleep;
    not_sleep = 0xff;
    check_short_addr_rsp_count = 0;
    timerCheckShortAddrRspEvt = NULL;
    return -1;
}

static void app_get_short_addrCb(void *args) {
    zdo_zdpDataInd_t *p = (zdo_zdpDataInd_t *)args;
    zdo_nwk_addr_resp_t *rsp = (zdo_nwk_addr_resp_t *)p->zpdu;

    if (timerCheckShortAddrRspEvt) {
        TL_ZB_TIMER_CANCEL(&timerCheckShortAddrRspEvt);
    }
    timerCheckShortAddrRspEvt = TL_ZB_TIMER_SCHEDULE(check_short_addr_rspCb, NULL, TIMEOUT_1SEC);

    if (rsp->status == ZDO_SUCCESS) {
        aps_binding_entry_t *bind_tbl = g_app_bind_tbl->dev_bind[idx].bind_entry;
        g_app_bind_tbl->dev_bind[idx].short_addr = rsp->nwk_addr_remote;
        switch(bind_tbl->clusterId) {
            case ZCL_CLUSTER_GEN_ON_OFF:
                g_app_bind_tbl->addr_onoff[g_app_bind_tbl->onoff_num++] = rsp->nwk_addr_remote;
                APP_DEBUG(DEBUG_BINDING_EN, "clId: 0x%04x, addr: 0x%04x, num: %d\r\n", bind_tbl->clusterId, rsp->nwk_addr_remote, g_app_bind_tbl->onoff_num);
                break;
            case ZCL_CLUSTER_GEN_LEVEL_CONTROL:
                g_app_bind_tbl->addr_level[g_app_bind_tbl->level_num++] = rsp->nwk_addr_remote;
                APP_DEBUG(DEBUG_BINDING_EN, "clId: 0x%04x, addr: 0x%04x, num: %d\r\n", bind_tbl->clusterId, rsp->nwk_addr_remote, g_app_bind_tbl->level_num);
                break;
            case ZCL_CLUSTER_GEN_MULTISTATE_INPUT_BASIC:
                g_app_bind_tbl->addr_multi[g_app_bind_tbl->multi_num++] = rsp->nwk_addr_remote;
                APP_DEBUG(DEBUG_BINDING_EN, "clId: 0x%04x, addr: 0x%04x, num: %d\r\n", bind_tbl->clusterId, rsp->nwk_addr_remote, g_app_bind_tbl->multi_num);
                break;
            case ZCL_CLUSTER_GEN_SCENES:
                g_app_bind_tbl->addr_scene[g_app_bind_tbl->scene_num++] = rsp->nwk_addr_remote;
                APP_DEBUG(DEBUG_BINDING_EN, "clId: 0x%04x, addr: 0x%04x, num: %d\r\n", bind_tbl->clusterId, rsp->nwk_addr_remote, g_app_bind_tbl->scene_num);
                break;
            default:
                break;
        }

    }

    for(idx++; idx < g_app_bind_tbl->idx; idx++) {
        if (!g_app_bind_tbl->dev_bind[idx].short_addr) {
            TL_SCHEDULE_TASK(app_get_short_addr, NULL);
            break;
        }
    }
}

static void app_get_short_addr(void *args) {
    if (not_sleep == 0xff) {
        not_sleep = g_appCtx.not_sleep;
        g_appCtx.not_sleep = true;
    }
    uint8_t sn = 0;
    zdo_nwk_addr_req_t req;
    ZB_IEEE_ADDR_COPY(req.ieee_addr_interest, g_app_bind_tbl->dev_bind[idx].bind_entry->dstExtAddrInfo.extAddr);
    req.req_type = ZDO_ADDR_REQ_SINGLE_RESP;
    req.start_index = 0;
    zb_zdoNwkAddrReq(NWK_BROADCAST_ROUTER_COORDINATOR, &req, &sn, app_get_short_addrCb);
    APP_DEBUG(DEBUG_BINDING_EN, "app_get_short_addr: idx: %d\r\n", idx);
}

void app_update_addr_bind(uint8_t i) {

    idx = i;
    check_short_addr_rsp_count = 0;
    TL_SCHEDULE_TASK(app_get_short_addr, NULL);
}

void app_update_bind_tbl(void *args) {

    APP_DEBUG(DEBUG_BINDING_EN, "app_update_bind_tbl\r\n");

    memset(&app_bind_tbl.dev_bind, 0, sizeof(app_bind_tbl));

    app_bind_tbl.idx = 0;
    app_bind_tbl.onoff_num = 0;
    app_bind_tbl.level_num = 0;
    app_bind_tbl.multi_num = 0;
    app_bind_tbl.scene_num = 0;
    memset(&app_bind_tbl.addr_onoff, 0, sizeof(app_bind_tbl.addr_onoff));
    memset(&app_bind_tbl.addr_level, 0, sizeof(app_bind_tbl.addr_level));
    memset(&app_bind_tbl.addr_multi, 0, sizeof(app_bind_tbl.addr_multi));
    memset(&app_bind_tbl.addr_scene, 0, sizeof(app_bind_tbl.addr_scene));
    aps_binding_entry_t *bind_tbl = bindTblEntryGet();

    for (uint8_t i = 0; i < APS_BINDING_TABLE_NUM; i++) {
        if (bind_tbl->used) {
            if (bind_tbl->clusterId == ZCL_CLUSTER_GEN_ON_OFF ||
                    bind_tbl->clusterId == ZCL_CLUSTER_GEN_LEVEL_CONTROL ||
                    bind_tbl->clusterId == ZCL_CLUSTER_GEN_MULTISTATE_INPUT_BASIC ||
                    bind_tbl->clusterId == ZCL_CLUSTER_GEN_SCENES) {
                /* exclude the coordinator's address */
                if (bind_tbl->dstAddrMode == BIND_ADDRMODE_IEEE &&
                        ZB_IEEE_ADDR_CMP(bind_tbl->dstExtAddrInfo.extAddr, app_bind_tbl.extAddrCoordinator)) {
                    bind_tbl++;
                    continue;
                }
                app_bind_tbl.dev_bind[app_bind_tbl.idx++].bind_entry = bind_tbl;
                APP_DEBUG(DEBUG_BINDING_EN, "ep: %d, dstAddrMode: %d, ieee: 0x%02x%02x%02x%02x%02x%02x%02x%02x, idx: %d\r\n",
                        bind_tbl->srcEp, bind_tbl->dstAddrMode,
                        bind_tbl->dstExtAddrInfo.extAddr[0],
                        bind_tbl->dstExtAddrInfo.extAddr[1],
                        bind_tbl->dstExtAddrInfo.extAddr[2],
                        bind_tbl->dstExtAddrInfo.extAddr[3],
                        bind_tbl->dstExtAddrInfo.extAddr[4],
                        bind_tbl->dstExtAddrInfo.extAddr[5],
                        bind_tbl->dstExtAddrInfo.extAddr[6],
                        bind_tbl->dstExtAddrInfo.extAddr[7],
                        app_bind_tbl.idx);
            }
        }
        bind_tbl++;
    }

    if (app_bind_tbl.idx) {
        idx = 0;
        check_short_addr_rsp_count = 0;
        TL_SCHEDULE_TASK(app_get_short_addr, NULL);
    }
}


int32_t app_getIeeeCoordinatorCb(void *args) {

//    printf("app_bindTimerCb: network: %d\r\n", zb_isDeviceJoinedNwk());
    if (zb_isDeviceJoinedNwk()) {
        if (!g_app_bind_tbl->addrCoordinatorSet) {
            app_getIeee(0x0000);
            return 0;
        }
        TL_SCHEDULE_TASK(app_update_bind_tbl, NULL);
        g_app_bind_tbl->timerGetIeeeCoordinatorEvt = NULL;
        return -1;
    }

    return 0;
}

void app_bind_cmd_init() {
    memset(&app_bind_cmd, 0, sizeof(app_bind_cmd));
}

app_bind_cmd_t *get_bind_cmd_free() {
    for (uint8_t i = 0; i < APS_BINDING_TABLE_NUM; i++) {
        if (!app_bind_cmd[i].used) {
            return &app_bind_cmd[i];
        }
    }
    return NULL;
}

int32_t check_bind_cmd_rspCb(void *args) {

    repeat_bind_cmd = BIND_CMD_NONE;
    for (uint8_t i = 0; i < BIND_CMD_NUM; i++) {
        if (app_bind_cmd[i].used && app_bind_cmd[i].repeat) {
            repeat_bind_cmd = app_bind_cmd[i].command;
            switch(app_bind_cmd[i].cluster) {
                case ZCL_CLUSTER_GEN_ON_OFF:
                    repeat_bind_cmd |= BIND_CMD_ONOFF;
                    break;
                case ZCL_CLUSTER_GEN_MULTISTATE_INPUT_BASIC:
                    repeat_bind_cmd |= BIND_CMD_MULTI;
                    break;
                case ZCL_CLUSTER_GEN_SCENES:
                    repeat_bind_cmd |= BIND_CMD_SCENE;
                    break;
                case ZCL_CLUSTER_GEN_LEVEL_CONTROL:
                    break;
                default:
                    break;
            }
        }
    }

    g_app_bind_tbl->timerCmdRspEvt = NULL;
    return -1;
}

void set_send_bind_cmd(aps_data_ind_t *pIndInfo, uint8_t seq_num) {
    for (uint8_t i = 0; i < BIND_CMD_NUM; i++) {
        if (app_bind_cmd[i].used && app_bind_cmd[i].seq_num == seq_num &&
                pIndInfo->src_short_addr == app_bind_cmd[i].short_addr &&
                pIndInfo->src_ep == app_bind_cmd[i].ep &&
                pIndInfo->cluster_id == app_bind_cmd[i].cluster) {
            app_bind_cmd[i].repeat = true;
//            app_bind_cmd[i].used = false;
            DEBUG(DEBUG_BINDING_EN, "i: %d, set_send_dev_onoff_cmd. seq_num: %d\r\n", i, seq_num);
        }
    }
}
