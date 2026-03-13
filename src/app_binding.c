#include "app_main.h"

enum {
    BIND_ADDRMODE_GROUP = 1,
    BIND_ADDRMODE_IEEE = 3,
};

static app_bind_tbl_t app_bind_tbl = {
        .timerGetIeeeCoordinatorEvt = NULL,
        .idx = 0,
        .extAddrCoordinator = {0},
        .addrCoordinatorSet = false,
};

static ev_timer_event_t *timerCheckShortAddrRspEvt = NULL;
static uint8_t check_short_addr_rsp_count = 0;
app_bind_tbl_t *g_app_bind_tbl = &app_bind_tbl;
static uint8_t idx = 0;
static uint8_t not_sleep = 0xff;

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

    if (idx < g_app_bind_tbl->idx) {
        if (check_short_addr_rsp_count++ < 3) {
            APP_DEBUG(DEBUG_BINDING_EN, "No rsp. idx: %d, counter: %d\r\n", idx, check_short_addr_rsp_count);
            TL_SCHEDULE_TASK(app_get_short_addr, NULL);
            return 0;
        } else {
            idx++;
            if (idx < g_app_bind_tbl->idx) {
                check_short_addr_rsp_count = 0;
                APP_DEBUG(DEBUG_BINDING_EN, "No rsp. Next. idx: %d, counter: %d\r\n", idx, check_short_addr_rsp_count);
                TL_SCHEDULE_TASK(app_get_short_addr, NULL);
            }
        }
    }
    APP_DEBUG(DEBUG_BINDING_EN, "All ok. idx: %d, counter: %d\r\n", idx, check_short_addr_rsp_count);
    g_appCtx.not_sleep = not_sleep;
    not_sleep = 0xff;
    timerCheckShortAddrRspEvt = NULL;
    return -1;
}

static void app_get_short_addrCb(void *args) {
    zdo_zdpDataInd_t *p = (zdo_zdpDataInd_t *)args;
    zdo_nwk_addr_resp_t *rsp = (zdo_nwk_addr_resp_t *)p->zpdu;

    if (timerCheckShortAddrRspEvt) {
        TL_ZB_TIMER_CANCEL(&timerCheckShortAddrRspEvt);
    }
    timerCheckShortAddrRspEvt = TL_ZB_TIMER_SCHEDULE(check_short_addr_rspCb, NULL, TIMEOUT_2SEC);

    if (rsp->status == ZDO_SUCCESS) {
//        APP_DEBUG(DEBUG_BINDING_EN, "idx: %d, addr: 0x%04x\r\n", idx, rsp->nwk_addr_remote);
        g_app_bind_tbl->dev_bind[idx].short_addr = rsp->nwk_addr_remote;
    }

    idx++;
    do {
        if (idx < g_app_bind_tbl->idx) {
            if (!g_app_bind_tbl->dev_bind[idx].short_addr) {
                TL_SCHEDULE_TASK(app_get_short_addr, NULL);
                break;
            }
        }
        idx++;
    } while(idx < g_app_bind_tbl->idx);
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


void app_update_bind_tbl(void *args) {

    APP_DEBUG(DEBUG_BINDING_EN, "app_update_bind_tbl\r\n");

    memset(&app_bind_tbl.dev_bind, 0, sizeof(app_bind_tbl));

    app_bind_tbl.idx = 0;
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

