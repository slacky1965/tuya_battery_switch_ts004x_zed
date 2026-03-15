#include "app_main.h"

static status_t st = 0xFF;

void app_level_move(uint8_t ep, uint8_t up_down) {
    epInfo_t dstEpInfo;
    TL_SETSTRUCTCONTENT(dstEpInfo, 0);

    move_t move;
    move.moveMode = up_down;
    move.rate = device_settings.defaultMoveRate[ep-1];
    move.optPresent = 0;

    TL_SETSTRUCTCONTENT(dstEpInfo, 0);
    dstEpInfo.profileId = HA_PROFILE_ID;

    uint16_t groupList[APS_GROUP_TABLE_NUM];
    uint8_t groupCnt = 0;
    aps_group_list_get(&groupCnt, groupList);

    /* command for groups */
    dstEpInfo.dstAddrMode = APS_SHORT_GROUPADDR_NOEP;
    for (uint8_t i = 0; i < groupCnt; i++) {
        dstEpInfo.dstAddr.shortAddr = groupList[i];
        zcl_level_moveWithOnOffCmd(ep, &dstEpInfo, FALSE, &move);
//        printf("groupAddr: 0x%04x\r\n", dstEpInfo.dstAddr.shortAddr);
    }

    /* command when binding */
    dstEpInfo.profileId = HA_PROFILE_ID;
//    dstEpInfo.dstAddrMode = APS_DSTADDR_EP_NOTPRESETNT;
//    dstEpInfo.dstAddrMode = APS_LONG_DSTADDR_WITHEP;

    aps_binding_entry_t *bind_tbl = bindTblEntryGet();
    for (uint8_t j = 0; j < APS_BINDING_TABLE_NUM; j++) {
        if (bind_tbl->used && bind_tbl->clusterId == ZCL_CLUSTER_GEN_LEVEL_CONTROL && bind_tbl->srcEp == ep) {
            dstEpInfo.dstAddrMode = bind_tbl->dstAddrMode;
            if (dstEpInfo.dstAddrMode == APS_SHORT_GROUPADDR_NOEP) {
                dstEpInfo.dstAddr.shortAddr = bind_tbl->groupAddr;
            } else {
                dstEpInfo.dstAddrMode = APS_LONG_DSTADDR_WITHEP;
                dstEpInfo.dstEp = bind_tbl->dstExtAddrInfo.dstEp;
                memcpy(dstEpInfo.dstAddr.extAddr, bind_tbl->dstExtAddrInfo.extAddr, sizeof(extAddr_t));
            }
            st = zcl_level_moveWithOnOffCmd(ep, &dstEpInfo, FALSE, &move);
            APP_DEBUG(DEBUG_LEVEL_EN, "Level move %s with rate: %d, status: 0x%02x, ep: %d, clId: 0x%04x, ieee: 0x%02x%02x%02x%02x%02x%02x%02x%02x\r\n",
                    up_down?"Down":"Up", move.rate, st, bind_tbl->srcEp, bind_tbl->clusterId,
                    bind_tbl->dstExtAddrInfo.extAddr[0], bind_tbl->dstExtAddrInfo.extAddr[1],
                    bind_tbl->dstExtAddrInfo.extAddr[2], bind_tbl->dstExtAddrInfo.extAddr[3],
                    bind_tbl->dstExtAddrInfo.extAddr[4], bind_tbl->dstExtAddrInfo.extAddr[5],
                    bind_tbl->dstExtAddrInfo.extAddr[6], bind_tbl->dstExtAddrInfo.extAddr[7]);
        }
        bind_tbl++;
    }
}

void app_level_stop(uint8_t ep) {

    epInfo_t dstEpInfo;
    TL_SETSTRUCTCONTENT(dstEpInfo, 0);

    stop_t stop;
    stop.optPresent = 0;

    TL_SETSTRUCTCONTENT(dstEpInfo, 0);
    dstEpInfo.profileId = HA_PROFILE_ID;

    uint16_t groupList[APS_GROUP_TABLE_NUM];
    uint8_t groupCnt = 0;
    aps_group_list_get(&groupCnt, groupList);

    /* command for groups */
    dstEpInfo.dstAddrMode = APS_SHORT_GROUPADDR_NOEP;
    for (uint8_t i = 0; i < groupCnt; i++) {
        dstEpInfo.dstAddr.shortAddr = groupList[i];
        zcl_level_stopCmd(ep, &dstEpInfo, FALSE, &stop);
//        printf("groupAddr: 0x%04x\r\n", dstEpInfo.dstAddr.shortAddr);
    }

    /* command when binding */
    dstEpInfo.profileId = HA_PROFILE_ID;
//    dstEpInfo.dstAddrMode = APS_DSTADDR_EP_NOTPRESETNT;
//    dstEpInfo.dstAddrMode = APS_LONG_DSTADDR_WITHEP;

    aps_binding_entry_t *bind_tbl = bindTblEntryGet();
    for (uint8_t j = 0; j < APS_BINDING_TABLE_NUM; j++) {
        if (bind_tbl->used && bind_tbl->clusterId == ZCL_CLUSTER_GEN_LEVEL_CONTROL && bind_tbl->srcEp == ep) {
            dstEpInfo.dstAddrMode = bind_tbl->dstAddrMode;
            if (dstEpInfo.dstAddrMode == APS_SHORT_GROUPADDR_NOEP) {
                dstEpInfo.dstAddr.shortAddr = bind_tbl->groupAddr;
            } else {
                dstEpInfo.dstAddrMode = APS_LONG_DSTADDR_WITHEP;
                dstEpInfo.dstEp = bind_tbl->dstExtAddrInfo.dstEp;
                memcpy(dstEpInfo.dstAddr.extAddr, bind_tbl->dstExtAddrInfo.extAddr, sizeof(extAddr_t));
            }
            st = zcl_level_stopCmd(ep, &dstEpInfo, FALSE, &stop);
            APP_DEBUG(DEBUG_LEVEL_EN, "Level stop, status: 0x%02x, ep: %d, clId: 0x%04x, ieee: 0x%02x%02x%02x%02x%02x%02x%02x%02x\r\n",
                    st, bind_tbl->srcEp, bind_tbl->clusterId,
                    bind_tbl->dstExtAddrInfo.extAddr[0], bind_tbl->dstExtAddrInfo.extAddr[1],
                    bind_tbl->dstExtAddrInfo.extAddr[2], bind_tbl->dstExtAddrInfo.extAddr[3],
                    bind_tbl->dstExtAddrInfo.extAddr[4], bind_tbl->dstExtAddrInfo.extAddr[5],
                    bind_tbl->dstExtAddrInfo.extAddr[6], bind_tbl->dstExtAddrInfo.extAddr[7]);
        }
        bind_tbl++;
    }
}

void app_level_step(uint8_t ep, uint8_t up_down) {
    epInfo_t dstEpInfo;
    TL_SETSTRUCTCONTENT(dstEpInfo, 0);

    step_t step;
    step.stepMode = up_down;
    step.stepSize = 25;
    step.transitionTime = 1;
    step.optPresent = 0;

    TL_SETSTRUCTCONTENT(dstEpInfo, 0);
    dstEpInfo.profileId = HA_PROFILE_ID;

    uint16_t groupList[APS_GROUP_TABLE_NUM];
    uint8_t groupCnt = 0;
    aps_group_list_get(&groupCnt, groupList);

    /* command for groups */
    dstEpInfo.dstAddrMode = APS_SHORT_GROUPADDR_NOEP;
    for (uint8_t i = 0; i < groupCnt; i++) {
        dstEpInfo.dstAddr.shortAddr = groupList[i];
        zcl_level_stepWithOnOffCmd(ep, &dstEpInfo, FALSE, &step);
//        printf("groupAddr: 0x%04x\r\n", dstEpInfo.dstAddr.shortAddr);
    }

    /* command when binding */
    dstEpInfo.profileId = HA_PROFILE_ID;
//    dstEpInfo.dstAddrMode = APS_DSTADDR_EP_NOTPRESETNT;
//    dstEpInfo.dstAddrMode = APS_LONG_DSTADDR_WITHEP;

    aps_binding_entry_t *bind_tbl = bindTblEntryGet();
    for (uint8_t j = 0; j < APS_BINDING_TABLE_NUM; j++) {
        if (bind_tbl->used && bind_tbl->clusterId == ZCL_CLUSTER_GEN_LEVEL_CONTROL && bind_tbl->srcEp == ep) {
            dstEpInfo.dstAddrMode = bind_tbl->dstAddrMode;
            if (dstEpInfo.dstAddrMode == APS_SHORT_GROUPADDR_NOEP) {
                dstEpInfo.dstAddr.shortAddr = bind_tbl->groupAddr;
            } else {
                dstEpInfo.dstAddrMode = APS_LONG_DSTADDR_WITHEP;
                dstEpInfo.dstEp = bind_tbl->dstExtAddrInfo.dstEp;
                memcpy(dstEpInfo.dstAddr.extAddr, bind_tbl->dstExtAddrInfo.extAddr, sizeof(extAddr_t));
            }
            st = zcl_level_stepWithOnOffCmd(ep, &dstEpInfo, FALSE, &step);
            APP_DEBUG(DEBUG_LEVEL_EN, "Level step %s, status: 0x%02x, ep: %d, clId: 0x%04x, ieee: 0x%02x%02x%02x%02x%02x%02x%02x%02x\r\n",
                    up_down?"Down":"Up", st, bind_tbl->srcEp, bind_tbl->clusterId,
                    bind_tbl->dstExtAddrInfo.extAddr[0], bind_tbl->dstExtAddrInfo.extAddr[1],
                    bind_tbl->dstExtAddrInfo.extAddr[2], bind_tbl->dstExtAddrInfo.extAddr[3],
                    bind_tbl->dstExtAddrInfo.extAddr[4], bind_tbl->dstExtAddrInfo.extAddr[5],
                    bind_tbl->dstExtAddrInfo.extAddr[6], bind_tbl->dstExtAddrInfo.extAddr[7]);
        }
        bind_tbl++;
    }
}

