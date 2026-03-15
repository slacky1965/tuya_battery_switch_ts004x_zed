#include "app_main.h"

static void cmdOnOffSend(uint8_t ep, epInfo_t *dstEpInfo, uint8_t command) {

    /* command 0x00 - off, 0x01 - on, 0x02 - toggle */
    status_t st = 0xFF;
    switch(command) {
        case ZCL_CMD_ONOFF_OFF:
            DEBUG(DEBUG_ONOFF_EN, "OnOff command: off");
            st = zcl_onOff_offCmd(ep, dstEpInfo, FALSE);
            break;
        case ZCL_CMD_ONOFF_ON:
            DEBUG(DEBUG_ONOFF_EN, "OnOff command: on");
            st = zcl_onOff_onCmd(ep, dstEpInfo, FALSE);
            break;
        case ZCL_CMD_ONOFF_TOGGLE:
            DEBUG(DEBUG_ONOFF_EN, "OnOff command: toggle");
            st = zcl_onOff_toggleCmd(ep, dstEpInfo, FALSE);
            break;
        default:
            DEBUG(DEBUG_ONOFF_EN, "OnOff command: unknown");
            break;
    }
    DEBUG(DEBUG_ONOFF_EN, ", status: 0x%02x\r\n", st);
}

void app_cmdOnOff(uint8_t ep, uint8_t command) {

    epInfo_t dstEpInfo;
    TL_SETSTRUCTCONTENT(dstEpInfo, 0);
    dstEpInfo.profileId = HA_PROFILE_ID;

    uint16_t groupList[APS_GROUP_TABLE_NUM];
    uint8_t groupCnt = 0;
    aps_group_list_get(&groupCnt, groupList);

    /* command for groups */
    dstEpInfo.dstAddrMode = APS_SHORT_GROUPADDR_NOEP;
    for (uint8_t i = 0; i < groupCnt; i++) {
        aps_group_tbl_ent_t *grEntry = aps_group_search(groupList[i], ep);
        if (grEntry) {
            APP_DEBUG(DEBUG_ONOFF_EN, "src_ep: %d, dst_ep: %d, addr: 0x%04x\r\n", ep, grEntry->n_endpoints, grEntry->group_addr);
            dstEpInfo.dstAddr.shortAddr = grEntry->group_addr;
            cmdOnOffSend(ep, &dstEpInfo, command);
        }
    }

    /* command when binding */
    TL_SETSTRUCTCONTENT(dstEpInfo, 0);
    dstEpInfo.profileId = HA_PROFILE_ID;
//    dstEpInfo.dstAddrMode = APS_DSTADDR_EP_NOTPRESETNT;
//    dstEpInfo.dstAddrMode = APS_SHORT_GROUPADDR_NOEP;
//    dstEpInfo.dstAddrMode = APS_LONG_DSTADDR_WITHEP;

    aps_binding_entry_t *bind_tbl = bindTblEntryGet();
    for (uint8_t j = 0; j < APS_BINDING_TABLE_NUM; j++) {
        if (bind_tbl->used && bind_tbl->clusterId == ZCL_CLUSTER_GEN_ON_OFF && bind_tbl->srcEp == ep) {
            dstEpInfo.dstAddrMode = bind_tbl->dstAddrMode;
            if (dstEpInfo.dstAddrMode == APS_SHORT_GROUPADDR_NOEP) {
                dstEpInfo.dstAddr.shortAddr = bind_tbl->groupAddr;
            } else {
                dstEpInfo.dstAddrMode = APS_LONG_DSTADDR_WITHEP;
                dstEpInfo.dstEp = bind_tbl->dstExtAddrInfo.dstEp;
                memcpy(dstEpInfo.dstAddr.extAddr, bind_tbl->dstExtAddrInfo.extAddr, sizeof(extAddr_t));
            }
            APP_DEBUG(DEBUG_ONOFF_EN, "ep: %d, clId: 0x%04x, dstAddrMode: %d, ieee: 0x%02x%02x%02x%02x%02x%02x%02x%02x\r\n",
                    bind_tbl->srcEp, bind_tbl->clusterId, bind_tbl->dstAddrMode,
                    bind_tbl->dstExtAddrInfo.extAddr[0], bind_tbl->dstExtAddrInfo.extAddr[1],
                    bind_tbl->dstExtAddrInfo.extAddr[2], bind_tbl->dstExtAddrInfo.extAddr[3],
                    bind_tbl->dstExtAddrInfo.extAddr[4], bind_tbl->dstExtAddrInfo.extAddr[5],
                    bind_tbl->dstExtAddrInfo.extAddr[6], bind_tbl->dstExtAddrInfo.extAddr[7]);
            cmdOnOffSend(ep, &dstEpInfo, command);
        }
        bind_tbl++;
    }
//    cmdOnOffSend(ep, &dstEpInfo, command);
//    printf("OnOffCmf send. Command: 0x%02x\r\n", command);
}

