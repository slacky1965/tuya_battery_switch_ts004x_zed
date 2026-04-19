#include "app_main.h"

static repeat_cmd_t repeat_cmd[REPEAT_CMD_NUM];
uint8_t repeat_cmd_num = 0;

repeat_cmd_t *app_find_repeat_cmd(uint16_t clId, uint8_t srcEp, uint8_t dstEp, uint8_t addrMode, app_addr_t addr) {

    for (uint8_t i = 0; i < REPEAT_CMD_NUM; i++) {
        if (repeat_cmd[i].used) {
            if (repeat_cmd[i].clId == clId && repeat_cmd[i].srcEp == srcEp &&
                    repeat_cmd[i].dstEp == dstEp && repeat_cmd[i].dstAddrMode == addrMode) {
                if (repeat_cmd[i].dstAddrMode == APS_SHORT_GROUPADDR_NOEP) {
                    if (repeat_cmd[i].dstAddr.addr_short == addr.addr_short) {
                        APP_DEBUG(DEBUG_REPEAT_EN, "i: %d\r\n", i);
                        return &repeat_cmd[i];
                    }
                } else {
                    if (ZB_64BIT_ADDR_CMP(repeat_cmd[i].dstAddr.addr_long, addr.addr_long)) {
                        APP_DEBUG(DEBUG_REPEAT_EN, "i: %d\r\n", i);
                        return &repeat_cmd[i];
                    }
                }
            }
        }
    }

    return NULL;
}

void app_del_repeat_cmd(uint16_t clId, uint8_t srcEp, uint8_t dstEp, uint8_t addrMode, app_addr_t addr) {

    for (uint8_t i = 0; i < REPEAT_CMD_NUM; i++) {
        if (repeat_cmd[i].used) {
            if (repeat_cmd[i].clId == clId && repeat_cmd[i].srcEp == srcEp &&
                    repeat_cmd[i].dstEp == dstEp && repeat_cmd[i].dstAddrMode == addrMode) {
                if (repeat_cmd[i].dstAddrMode == APS_SHORT_GROUPADDR_NOEP) {
                    if (repeat_cmd[i].dstAddr.addr_short == addr.addr_short) {
                        repeat_cmd[i].used = false;
                    }
                } else {
                    if (ZB_64BIT_ADDR_CMP(repeat_cmd[i].dstAddr.addr_long, addr.addr_long)) {
                        repeat_cmd[i].used = false;
                    }
                }
            }
        }
    }
}

bool app_add_repeat_cmd(uint16_t clId, uint8_t srcEp, uint8_t dstEp, uint8_t addrMode, app_addr_t addr, uint8_t cmdId, void *args) {

    APP_DEBUG(DEBUG_REPEAT_EN, "clId: 0x%04x, srcEp: %d, dstEp: %d, addrMode: %d\r\n", clId, srcEp, dstEp, addrMode);
    for (uint8_t i = 0; i < REPEAT_CMD_NUM; i++) {
        if (!repeat_cmd[i].used) {
            repeat_cmd[i].used = true;
            repeat_cmd[i].clId = clId;
            repeat_cmd[i].srcEp = srcEp;
            repeat_cmd[i].dstEp = dstEp;
            repeat_cmd[i].dstAddrMode = addrMode;
            memcpy(&repeat_cmd[i].dstAddr, &addr, sizeof(app_addr_t));
            repeat_cmd[i].cmdId = cmdId;
            if (args) {
                if (clId == ZCL_CLUSTER_GEN_LEVEL_CONTROL) {
                    switch(cmdId) {
                        case ZCL_CMD_LEVEL_MOVE_TO_LEVEL_WITH_ON_OFF:
                        case ZCL_CMD_LEVEL_MOVE_TO_LEVEL:
                            memcpy(&repeat_cmd[i].move2Level, (moveToLvl_t*)args, sizeof(moveToLvl_t));
                            break;
                        case ZCL_CMD_LEVEL_MOVE_WITH_ON_OFF:
                        case ZCL_CMD_LEVEL_MOVE:
                            memcpy(&repeat_cmd[i].move, (move_t*)args, sizeof(move_t));
                            break;
                        case ZCL_CMD_LEVEL_STOP:
                            memcpy(&repeat_cmd[i].stop, (stop_t*)args, sizeof(stop_t));
                            break;
                        case ZCL_CMD_LEVEL_STEP:
                            memcpy(&repeat_cmd[i].step, (step_t*)args, sizeof(step_t));
                            break;
                        default:
                            break;
                    }
                } else if (clId == ZCL_CLUSTER_GEN_SCENES) {
                    memcpy(&repeat_cmd[i].recallScene, (recallScene_t*)args, sizeof(recallScene_t));
                }
            }
            return true;
        }
    }

    return false;
}

void app_reset_repeat_cmd() {
    memset(repeat_cmd, 0, sizeof(repeat_cmd));
}
