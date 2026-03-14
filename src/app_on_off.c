#include "app_main.h"

static void cmdOnOffSend(uint8_t ep, epInfo_t *dstEpInfo, uint8_t command) {

    /* command 0x00 - off, 0x01 - on, 0x02 - toggle */

    switch(command) {
        case ZCL_CMD_ONOFF_OFF:
            DEBUG(DEBUG_ONOFF_EN, "OnOff command: off\r\n");
            zcl_onOff_offCmd(ep, dstEpInfo, FALSE);
            break;
        case ZCL_CMD_ONOFF_ON:
            DEBUG(DEBUG_ONOFF_EN, "OnOff command: on\r\n");
            zcl_onOff_onCmd(ep, dstEpInfo, FALSE);
            break;
        case ZCL_CMD_ONOFF_TOGGLE:
            DEBUG(DEBUG_ONOFF_EN, "OnOff command: toggle\r\n");
            zcl_onOff_toggleCmd(ep, dstEpInfo, FALSE);
            break;
        default:
            break;
    }
}

void app_cmdOnOff(uint8_t ep, uint8_t command, uint8_t cmd_mode) {

    epInfo_t dstEpInfo;
    TL_SETSTRUCTCONTENT(dstEpInfo, 0);
    dstEpInfo.profileId = HA_PROFILE_ID;

    uint16_t groupList[APS_GROUP_TABLE_NUM];
    uint8_t groupCnt = 0;
    aps_group_list_get(&groupCnt, groupList);

    app_bind_cmd_t *bind_cmd = NULL;

    switch (cmd_mode) {
        case BIND_CMD_MODE_SINGLE:
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
            dstEpInfo.dstAddrMode = APS_DSTADDR_EP_NOTPRESETNT;
            cmdOnOffSend(ep, &dstEpInfo, command);
            break;
        case BIND_CMD_MODE_CHECK_ANSWER:
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

            if (g_app_bind_tbl->timerCmdRspEvt) {
                TL_ZB_TIMER_CANCEL(&g_app_bind_tbl->timerCmdRspEvt);
            }

            app_bind_cmd_init();

            uint8_t idx = 0;
            bool find_bind_addr = false;
            /* command when binding */
            for (uint8_t i = 0; i < g_app_bind_tbl->idx; i++) {
                if (g_app_bind_tbl->dev_bind[i].bind_entry->clusterId == ZCL_CLUSTER_GEN_ON_OFF) {
                    if (!g_app_bind_tbl->dev_bind[i].short_addr) {
                        idx = i;
                        find_bind_addr = true;
                    }
                    bind_cmd = get_bind_cmd_free();
                    if (bind_cmd && g_app_bind_tbl->dev_bind[i].short_addr) {
                        bind_cmd->used = true;
                        bind_cmd->command = command;
                        bind_cmd->type_cmd = BIND_CMD_ONOFF;
                        bind_cmd->cluster = ZCL_CLUSTER_GEN_ON_OFF;
                        bind_cmd->cmd_send = true;
                        bind_cmd->short_addr = g_app_bind_tbl->dev_bind[i].short_addr;
                        bind_cmd->repeat = false;
                        bind_cmd->seq_num = zcl_seqNum;
                    }
                }
            }
            if (find_bind_addr) {
                app_update_addr_bind(idx);
            }
            TL_SETSTRUCTCONTENT(dstEpInfo, 0);
            dstEpInfo.profileId = HA_PROFILE_ID;
            dstEpInfo.dstAddrMode = APS_DSTADDR_EP_NOTPRESETNT;
            cmdOnOffSend(ep, &dstEpInfo, command);
            g_app_bind_tbl->timerCmdRspEvt = TL_ZB_TIMER_SCHEDULE(check_bind_cmd_rspCb, NULL, TIMEOUT_1SEC);
            break;
        case BIND_CMD_MODE_REPEAT:
            break;
        default:
            return;
            break;
    }

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
    dstEpInfo.dstAddrMode = APS_DSTADDR_EP_NOTPRESETNT;
    cmdOnOffSend(ep, &dstEpInfo, command);
//    printf("OnOffCmf send. Command: 0x%02x\r\n", command);
}

