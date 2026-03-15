#include "app_main.h"

void app_scene_send(uint8_t ep) {

    uint16_t addrGroup = device_settings.scene[ep-1].groupId;

    epInfo_t dstEpInfo;
    TL_SETSTRUCTCONTENT(dstEpInfo, 0);
    dstEpInfo.profileId = HA_PROFILE_ID;
    recallScene_t recallScene;

    recallScene.sceneId = device_settings.scene[ep-1].sceneId;
    recallScene.groupId = device_settings.scene[ep-1].groupId;

    recallScene.transTime = 0;

    if (addrGroup) {
        dstEpInfo.dstAddrMode = APS_SHORT_GROUPADDR_NOEP;
        dstEpInfo.dstAddr.shortAddr = addrGroup;
        zcl_scene_recallSceneCmd(ep, &dstEpInfo, FALSE, &recallScene);
        DEBUG(DEBUG_SCENE_EN, "Recall scene command EP: %d, sceneID: %d, groupID: %d\r\n", ep, recallScene.sceneId, recallScene.groupId);
    } else {
        aps_binding_entry_t *bind_tbl = bindTblEntryGet();
        for (uint8_t j = 0; j < APS_BINDING_TABLE_NUM; j++) {
            if (bind_tbl->used && bind_tbl->clusterId == ZCL_CLUSTER_GEN_SCENES && bind_tbl->srcEp == ep) {
                dstEpInfo.dstAddrMode = bind_tbl->dstAddrMode;
                if (dstEpInfo.dstAddrMode == APS_SHORT_GROUPADDR_NOEP) {
                    dstEpInfo.dstAddr.shortAddr = bind_tbl->groupAddr;
                } else {
                    dstEpInfo.dstAddrMode = APS_LONG_DSTADDR_WITHEP;
                    dstEpInfo.dstEp = bind_tbl->dstExtAddrInfo.dstEp;
                    memcpy(dstEpInfo.dstAddr.extAddr, bind_tbl->dstExtAddrInfo.extAddr, sizeof(extAddr_t));
                }
                APP_DEBUG(DEBUG_SCENE_EN, "ep: %d, clId: 0x%04x, dstAddrMode: %d, ieee: 0x%02x%02x%02x%02x%02x%02x%02x%02x\r\n",
                        bind_tbl->srcEp, bind_tbl->clusterId, bind_tbl->dstAddrMode,
                        bind_tbl->dstExtAddrInfo.extAddr[0], bind_tbl->dstExtAddrInfo.extAddr[1],
                        bind_tbl->dstExtAddrInfo.extAddr[2], bind_tbl->dstExtAddrInfo.extAddr[3],
                        bind_tbl->dstExtAddrInfo.extAddr[4], bind_tbl->dstExtAddrInfo.extAddr[5],
                        bind_tbl->dstExtAddrInfo.extAddr[6], bind_tbl->dstExtAddrInfo.extAddr[7]);

//                recallScene.sceneId = device_settings.scene[ep-1].sceneId;
//                recallScene.groupId = device_settings.scene[ep-1].groupId;
//
//                recallScene.transTime = 0;

                zcl_scene_recallSceneCmd(ep, &dstEpInfo, FALSE, &recallScene);
                DEBUG(DEBUG_SCENE_EN, "Recall scene command EP: %d, sceneID: %d, groupID: %d\r\n", ep, recallScene.sceneId, recallScene.groupId);
            }
            bind_tbl++;
        }
//
//
//
//
//        dstEpInfo.dstAddrMode = APS_DSTADDR_EP_NOTPRESETNT;
    }
//    recallScene_t recallScene;
//
//    recallScene.sceneId = device_settings.scene[ep-1].sceneId;
//    recallScene.groupId = device_settings.scene[ep-1].groupId;
//
//    recallScene.transTime = 0;
//
//    zcl_scene_recallSceneCmd(ep, &dstEpInfo, FALSE, &recallScene);
//
//    DEBUG(DEBUG_SCENE_EN, "Recall scene command EP: %d, sceneID: %d, groupID: %d\r\n", ep, recallScene.sceneId, recallScene.groupId);
}
