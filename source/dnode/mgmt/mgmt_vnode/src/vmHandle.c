/*
 * Copyright (c) 2019 TAOS Data, Inc. <jhtao@taosdata.com>
 *
 * This program is free software: you can use, redistribute, and/or modify
 * it under the terms of the GNU Affero General Public License, version 3
 * or later ("AGPL"), as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program. If not, see <http:www.gnu.org/licenses/>.
 */

#define _DEFAULT_SOURCE
#include "vmInt.h"

void vmGetMonitorInfo(SMgmtWrapper *pWrapper, SMonVmInfo *vmInfo) {
  SVnodesMgmt *pMgmt = pWrapper->pMgmt;
  tfsGetMonitorInfo(pMgmt->pTfs, &vmInfo->tfs);

  taosWLockLatch(&pMgmt->latch);
  vmInfo->vstat.totalVnodes = pMgmt->state.totalVnodes;
  vmInfo->vstat.masterNum = pMgmt->state.masterNum;
  vmInfo->vstat.numOfSelectReqs = pMgmt->state.numOfSelectReqs - pMgmt->lastState.numOfSelectReqs;
  vmInfo->vstat.numOfInsertReqs = pMgmt->state.numOfInsertReqs - pMgmt->lastState.numOfInsertReqs;
  vmInfo->vstat.numOfInsertSuccessReqs = pMgmt->state.numOfInsertSuccessReqs - pMgmt->lastState.numOfInsertSuccessReqs;
  vmInfo->vstat.numOfBatchInsertReqs = pMgmt->state.numOfBatchInsertReqs - pMgmt->lastState.numOfBatchInsertReqs;
  vmInfo->vstat.numOfBatchInsertSuccessReqs =
      pMgmt->state.numOfBatchInsertSuccessReqs - pMgmt->lastState.numOfBatchInsertSuccessReqs;
  pMgmt->lastState = pMgmt->state;
  taosWUnLockLatch(&pMgmt->latch);
}

int32_t vmProcessGetMonVmInfoReq(SMgmtWrapper *pWrapper, SNodeMsg *pReq) {
  SMonVmInfo vmInfo = {0};
  vmGetMonitorInfo(pWrapper, &vmInfo);
  dmGetMonitorSysInfo(&vmInfo.sys);
  monGetLogs(&vmInfo.log);

  int32_t rspLen = tSerializeSMonVmInfo(NULL, 0, &vmInfo);
  if (rspLen < 0) {
    terrno = TSDB_CODE_INVALID_MSG;
    return -1;
  }

  void *pRsp = rpcMallocCont(rspLen);
  if (pRsp == NULL) {
    terrno = TSDB_CODE_OUT_OF_MEMORY;
    return -1;
  }

  tSerializeSMonVmInfo(pRsp, rspLen, &vmInfo);
  pReq->pRsp = pRsp;
  pReq->rspLen = rspLen;
  tFreeSMonVmInfo(&vmInfo);
  return 0;
}

int32_t vmProcessGetVnodeLoadsReq(SMgmtWrapper *pWrapper, SNodeMsg *pReq) {
  SMonVloadInfo vloads = {0};
  vmGetVnodeLoads(pWrapper, &vloads);

  int32_t rspLen = tSerializeSMonVloadInfo(NULL, 0, &vloads);
  if (rspLen < 0) {
    terrno = TSDB_CODE_INVALID_MSG;
    return -1;
  }

  void *pRsp = rpcMallocCont(rspLen);
  if (pRsp == NULL) {
    terrno = TSDB_CODE_OUT_OF_MEMORY;
    return -1;
  }

  tSerializeSMonVloadInfo(pRsp, rspLen, &vloads);
  pReq->pRsp = pRsp;
  pReq->rspLen = rspLen;
  tFreeSMonVloadInfo(&vloads);
  return 0;
}

static void vmGenerateVnodeCfg(SCreateVnodeReq *pCreate, SVnodeCfg *pCfg) {
  pCfg->vgId = pCreate->vgId;
  pCfg->wsize = pCreate->cacheBlockSize;
  pCfg->ssize = pCreate->cacheBlockSize;
  pCfg->lsize = pCreate->cacheBlockSize;
  pCfg->isHeapAllocator = true;
  pCfg->ttl = 4;
  pCfg->keep = pCreate->daysToKeep0;
  pCfg->streamMode = pCreate->streamMode;
  pCfg->isWeak = true;
  pCfg->tsdbCfg.keep2 = pCreate->daysToKeep0;
  pCfg->tsdbCfg.keep0 = pCreate->daysToKeep2;
  pCfg->tsdbCfg.keep1 = pCreate->daysToKeep0;
  pCfg->tsdbCfg.lruCacheSize = pCreate->cacheBlockSize;
  pCfg->tsdbCfg.retentions = pCreate->pRetensions;
  pCfg->metaCfg.lruSize = pCreate->cacheBlockSize;
  pCfg->walCfg.fsyncPeriod = pCreate->fsyncPeriod;
  pCfg->walCfg.level = pCreate->walLevel;
  pCfg->walCfg.retentionPeriod = 10;
  pCfg->walCfg.retentionSize = 128;
  pCfg->walCfg.rollPeriod = 128;
  pCfg->walCfg.segSize = 128;
  pCfg->walCfg.vgId = pCreate->vgId;
  pCfg->hashBegin = pCreate->hashBegin;
  pCfg->hashEnd = pCreate->hashEnd;
  pCfg->hashMethod = pCreate->hashMethod;
}

static void vmGenerateWrapperCfg(SVnodesMgmt *pMgmt, SCreateVnodeReq *pCreate, SWrapperCfg *pCfg) {
  memcpy(pCfg->db, pCreate->db, TSDB_DB_FNAME_LEN);
  pCfg->dbUid = pCreate->dbUid;
  pCfg->dropped = 0;
  snprintf(pCfg->path, sizeof(pCfg->path), "%s%svnode%d", pMgmt->path, TD_DIRSEP, pCreate->vgId);
  pCfg->vgId = pCreate->vgId;
  pCfg->vgVersion = pCreate->vgVersion;
}

int32_t vmProcessCreateVnodeReq(SVnodesMgmt *pMgmt, SNodeMsg *pMsg) {
  SRpcMsg        *pReq = &pMsg->rpcMsg;
  SCreateVnodeReq createReq = {0};
  char            path[TSDB_FILENAME_LEN];

  if (tDeserializeSCreateVnodeReq(pReq->pCont, pReq->contLen, &createReq) != 0) {
    terrno = TSDB_CODE_INVALID_MSG;
    return -1;
  }

  dDebug("vgId:%d, create vnode req is received", createReq.vgId);

  SVnodeCfg vnodeCfg = {0};
  vmGenerateVnodeCfg(&createReq, &vnodeCfg);

  SWrapperCfg wrapperCfg = {0};
  vmGenerateWrapperCfg(pMgmt, &createReq, &wrapperCfg);

  SVnodeObj *pVnode = vmAcquireVnode(pMgmt, createReq.vgId);
  if (pVnode != NULL) {
    tFreeSCreateVnodeReq(&createReq);
    dDebug("vgId:%d, already exist", createReq.vgId);
    vmReleaseVnode(pMgmt, pVnode);
    terrno = TSDB_CODE_NODE_ALREADY_DEPLOYED;
    return -1;
  }

  // create vnode
  snprintf(path, TSDB_FILENAME_LEN, "vnode%svnode%d", TD_DIRSEP, vnodeCfg.vgId);
  if (vnodeCreate(path, &vnodeCfg, pMgmt->pTfs) < 0) {
    tFreeSCreateVnodeReq(&createReq);
    dError("vgId:%d, failed to create vnode since %s", createReq.vgId, terrstr());
    return -1;
  }

  SMsgCb msgCb = pMgmt->pDnode->data.msgCb;
  msgCb.pWrapper = pMgmt->pWrapper;
  msgCb.queueFps[QUERY_QUEUE] = vmPutMsgToQueryQueue;
  msgCb.queueFps[FETCH_QUEUE] = vmPutMsgToFetchQueue;
  msgCb.queueFps[APPLY_QUEUE] = vmPutMsgToApplyQueue;
  msgCb.qsizeFp = vmGetQueueSize;

  SVnode *pImpl = vnodeOpen(path, pMgmt->pTfs, msgCb);
  if (pImpl == NULL) {
    dError("vgId:%d, failed to create vnode since %s", createReq.vgId, terrstr());
    tFreeSCreateVnodeReq(&createReq);
    return -1;
  }

  int32_t code = vmOpenVnode(pMgmt, &wrapperCfg, pImpl);
  if (code != 0) {
    tFreeSCreateVnodeReq(&createReq);
    dError("vgId:%d, failed to open vnode since %s", createReq.vgId, terrstr());
    vnodeClose(pImpl);
    vnodeDestroy(path, pMgmt->pTfs);
    terrno = code;
    return code;
  }

  code = vmWriteVnodesToFile(pMgmt);
  if (code != 0) {
    tFreeSCreateVnodeReq(&createReq);
    vnodeClose(pImpl);
    vnodeDestroy(path, pMgmt->pTfs);
    terrno = code;
    return code;
  }

  return 0;
}

int32_t vmProcessAlterVnodeReq(SVnodesMgmt *pMgmt, SNodeMsg *pMsg) {
  SRpcMsg       *pReq = &pMsg->rpcMsg;
  SAlterVnodeReq alterReq = {0};
  if (tDeserializeSCreateVnodeReq(pReq->pCont, pReq->contLen, &alterReq) != 0) {
    terrno = TSDB_CODE_INVALID_MSG;
    return -1;
  }

  dDebug("vgId:%d, alter vnode req is received", alterReq.vgId);

  SVnodeCfg vnodeCfg = {0};
  vmGenerateVnodeCfg(&alterReq, &vnodeCfg);

  SVnodeObj *pVnode = vmAcquireVnode(pMgmt, alterReq.vgId);
  if (pVnode == NULL) {
    dDebug("vgId:%d, failed to alter vnode since %s", alterReq.vgId, terrstr());
    return -1;
  }

  if (alterReq.vgVersion == pVnode->vgVersion) {
    vmReleaseVnode(pMgmt, pVnode);
    dDebug("vgId:%d, no need to alter vnode cfg for version unchanged ", alterReq.vgId);
    return 0;
  }

  if (vnodeAlter(pVnode->pImpl, &vnodeCfg) != 0) {
    dError("vgId:%d, failed to alter vnode since %s", alterReq.vgId, terrstr());
    vmReleaseVnode(pMgmt, pVnode);
    return -1;
  }

  int32_t oldVersion = pVnode->vgVersion;
  pVnode->vgVersion = alterReq.vgVersion;
  int32_t code = vmWriteVnodesToFile(pMgmt);
  if (code != 0) {
    pVnode->vgVersion = oldVersion;
  }

  vmReleaseVnode(pMgmt, pVnode);
  return code;
}

int32_t vmProcessDropVnodeReq(SVnodesMgmt *pMgmt, SNodeMsg *pMsg) {
  SRpcMsg      *pReq = &pMsg->rpcMsg;
  SDropVnodeReq dropReq = {0};
  if (tDeserializeSDropVnodeReq(pReq->pCont, pReq->contLen, &dropReq) != 0) {
    terrno = TSDB_CODE_INVALID_MSG;
    return -1;
  }

  int32_t vgId = dropReq.vgId;
  dDebug("vgId:%d, drop vnode req is received", vgId);

  SVnodeObj *pVnode = vmAcquireVnode(pMgmt, vgId);
  if (pVnode == NULL) {
    dDebug("vgId:%d, failed to drop since %s", vgId, terrstr());
    terrno = TSDB_CODE_NODE_NOT_DEPLOYED;
    return -1;
  }

  pVnode->dropped = 1;
  if (vmWriteVnodesToFile(pMgmt) != 0) {
    pVnode->dropped = 0;
    vmReleaseVnode(pMgmt, pVnode);
    return -1;
  }

  vmCloseVnode(pMgmt, pVnode);
  vmWriteVnodesToFile(pMgmt);

  return 0;
}

int32_t vmProcessSyncVnodeReq(SVnodesMgmt *pMgmt, SNodeMsg *pMsg) {
  SRpcMsg      *pReq = &pMsg->rpcMsg;
  SSyncVnodeReq syncReq = {0};
  tDeserializeSDropVnodeReq(pReq->pCont, pReq->contLen, &syncReq);

  int32_t vgId = syncReq.vgId;
  dDebug("vgId:%d, sync vnode req is received", vgId);

  SVnodeObj *pVnode = vmAcquireVnode(pMgmt, vgId);
  if (pVnode == NULL) {
    dDebug("vgId:%d, failed to sync since %s", vgId, terrstr());
    return -1;
  }

  if (vnodeSync(pVnode->pImpl) != 0) {
    dError("vgId:%d, failed to sync vnode since %s", vgId, terrstr());
    vmReleaseVnode(pMgmt, pVnode);
    return -1;
  }

  vmReleaseVnode(pMgmt, pVnode);
  return 0;
}

int32_t vmProcessCompactVnodeReq(SVnodesMgmt *pMgmt, SNodeMsg *pMsg) {
  SRpcMsg         *pReq = &pMsg->rpcMsg;
  SCompactVnodeReq compatcReq = {0};
  tDeserializeSDropVnodeReq(pReq->pCont, pReq->contLen, &compatcReq);

  int32_t vgId = compatcReq.vgId;
  dDebug("vgId:%d, compact vnode req is received", vgId);

  SVnodeObj *pVnode = vmAcquireVnode(pMgmt, vgId);
  if (pVnode == NULL) {
    dDebug("vgId:%d, failed to compact since %s", vgId, terrstr());
    return -1;
  }

  if (vnodeCompact(pVnode->pImpl) != 0) {
    dError("vgId:%d, failed to compact vnode since %s", vgId, terrstr());
    vmReleaseVnode(pMgmt, pVnode);
    return -1;
  }

  vmReleaseVnode(pMgmt, pVnode);
  return 0;
}

void vmInitMsgHandle(SMgmtWrapper *pWrapper) {
  dmSetMsgHandle(pWrapper, TDMT_MON_VM_INFO, vmProcessMonitorMsg, DEFAULT_HANDLE);
  dmSetMsgHandle(pWrapper, TDMT_MON_VM_LOAD, vmProcessMonitorMsg, DEFAULT_HANDLE);

  // Requests handled by VNODE
  dmSetMsgHandle(pWrapper, TDMT_VND_SUBMIT, (NodeMsgFp)vmProcessWriteMsg, DEFAULT_HANDLE);
  dmSetMsgHandle(pWrapper, TDMT_VND_QUERY, (NodeMsgFp)vmProcessQueryMsg, DEFAULT_HANDLE);
  dmSetMsgHandle(pWrapper, TDMT_VND_QUERY_CONTINUE, (NodeMsgFp)vmProcessQueryMsg, DEFAULT_HANDLE);
  dmSetMsgHandle(pWrapper, TDMT_VND_FETCH, (NodeMsgFp)vmProcessFetchMsg, DEFAULT_HANDLE);
  dmSetMsgHandle(pWrapper, TDMT_VND_FETCH_RSP, (NodeMsgFp)vmProcessFetchMsg, DEFAULT_HANDLE);
  dmSetMsgHandle(pWrapper, TDMT_VND_ALTER_TABLE, (NodeMsgFp)vmProcessWriteMsg, DEFAULT_HANDLE);
  dmSetMsgHandle(pWrapper, TDMT_VND_UPDATE_TAG_VAL, (NodeMsgFp)vmProcessWriteMsg, DEFAULT_HANDLE);
  dmSetMsgHandle(pWrapper, TDMT_VND_TABLE_META, (NodeMsgFp)vmProcessFetchMsg, DEFAULT_HANDLE);
  dmSetMsgHandle(pWrapper, TDMT_VND_TABLES_META, (NodeMsgFp)vmProcessFetchMsg, DEFAULT_HANDLE);
  dmSetMsgHandle(pWrapper, TDMT_VND_MQ_CONSUME, (NodeMsgFp)vmProcessQueryMsg, DEFAULT_HANDLE);
  dmSetMsgHandle(pWrapper, TDMT_VND_MQ_QUERY, (NodeMsgFp)vmProcessQueryMsg, DEFAULT_HANDLE);
  dmSetMsgHandle(pWrapper, TDMT_VND_MQ_CONNECT, (NodeMsgFp)vmProcessWriteMsg, DEFAULT_HANDLE);
  dmSetMsgHandle(pWrapper, TDMT_VND_MQ_DISCONNECT, (NodeMsgFp)vmProcessWriteMsg, DEFAULT_HANDLE);
  dmSetMsgHandle(pWrapper, TDMT_VND_MQ_SET_CUR, (NodeMsgFp)vmProcessWriteMsg, DEFAULT_HANDLE);
  dmSetMsgHandle(pWrapper, TDMT_VND_RES_READY, (NodeMsgFp)vmProcessFetchMsg, DEFAULT_HANDLE);
  dmSetMsgHandle(pWrapper, TDMT_VND_TASKS_STATUS, (NodeMsgFp)vmProcessFetchMsg, DEFAULT_HANDLE);
  dmSetMsgHandle(pWrapper, TDMT_VND_CANCEL_TASK, (NodeMsgFp)vmProcessFetchMsg, DEFAULT_HANDLE);
  dmSetMsgHandle(pWrapper, TDMT_VND_DROP_TASK, (NodeMsgFp)vmProcessFetchMsg, DEFAULT_HANDLE);
  dmSetMsgHandle(pWrapper, TDMT_VND_CREATE_STB, (NodeMsgFp)vmProcessWriteMsg, DEFAULT_HANDLE);
  dmSetMsgHandle(pWrapper, TDMT_VND_ALTER_STB, (NodeMsgFp)vmProcessWriteMsg, DEFAULT_HANDLE);
  dmSetMsgHandle(pWrapper, TDMT_VND_DROP_STB, (NodeMsgFp)vmProcessWriteMsg, DEFAULT_HANDLE);
  dmSetMsgHandle(pWrapper, TDMT_VND_CREATE_TABLE, (NodeMsgFp)vmProcessWriteMsg, DEFAULT_HANDLE);
  dmSetMsgHandle(pWrapper, TDMT_VND_DROP_TABLE, (NodeMsgFp)vmProcessWriteMsg, DEFAULT_HANDLE);
  dmSetMsgHandle(pWrapper, TDMT_VND_CREATE_SMA, (NodeMsgFp)vmProcessWriteMsg, DEFAULT_HANDLE);
  dmSetMsgHandle(pWrapper, TDMT_VND_CANCEL_SMA, (NodeMsgFp)vmProcessWriteMsg, DEFAULT_HANDLE);
  dmSetMsgHandle(pWrapper, TDMT_VND_DROP_SMA, (NodeMsgFp)vmProcessWriteMsg, DEFAULT_HANDLE);
  dmSetMsgHandle(pWrapper, TDMT_VND_MQ_SET_CONN, (NodeMsgFp)vmProcessWriteMsg, DEFAULT_HANDLE);
  dmSetMsgHandle(pWrapper, TDMT_VND_MQ_REB, (NodeMsgFp)vmProcessWriteMsg, DEFAULT_HANDLE);
  dmSetMsgHandle(pWrapper, TDMT_VND_MQ_CANCEL_CONN, (NodeMsgFp)vmProcessWriteMsg, DEFAULT_HANDLE);
  dmSetMsgHandle(pWrapper, TDMT_VND_MQ_SET_CUR, (NodeMsgFp)vmProcessFetchMsg, DEFAULT_HANDLE);
  dmSetMsgHandle(pWrapper, TDMT_VND_CONSUME, (NodeMsgFp)vmProcessFetchMsg, DEFAULT_HANDLE);
  dmSetMsgHandle(pWrapper, TDMT_VND_TASK_DEPLOY, (NodeMsgFp)vmProcessWriteMsg, DEFAULT_HANDLE);
  dmSetMsgHandle(pWrapper, TDMT_VND_QUERY_HEARTBEAT, (NodeMsgFp)vmProcessFetchMsg, DEFAULT_HANDLE);
  dmSetMsgHandle(pWrapper, TDMT_VND_TASK_PIPE_EXEC, (NodeMsgFp)vmProcessFetchMsg, DEFAULT_HANDLE);
  dmSetMsgHandle(pWrapper, TDMT_VND_TASK_MERGE_EXEC, (NodeMsgFp)vmProcessMergeMsg, DEFAULT_HANDLE);
  dmSetMsgHandle(pWrapper, TDMT_VND_TASK_WRITE_EXEC, (NodeMsgFp)vmProcessWriteMsg, DEFAULT_HANDLE);
  dmSetMsgHandle(pWrapper, TDMT_VND_STREAM_TRIGGER, (NodeMsgFp)vmProcessFetchMsg, DEFAULT_HANDLE);

  dmSetMsgHandle(pWrapper, TDMT_DND_CREATE_VNODE, vmProcessMgmtMsg, DEFAULT_HANDLE);
  dmSetMsgHandle(pWrapper, TDMT_DND_ALTER_VNODE, vmProcessMgmtMsg, DEFAULT_HANDLE);
  dmSetMsgHandle(pWrapper, TDMT_DND_DROP_VNODE, vmProcessMgmtMsg, DEFAULT_HANDLE);
  dmSetMsgHandle(pWrapper, TDMT_DND_SYNC_VNODE, vmProcessMgmtMsg, DEFAULT_HANDLE);
  dmSetMsgHandle(pWrapper, TDMT_DND_COMPACT_VNODE, vmProcessMgmtMsg, DEFAULT_HANDLE);
}
