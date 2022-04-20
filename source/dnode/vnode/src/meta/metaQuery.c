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
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "vnodeInt.h"

SMTbCursor *metaOpenTbCursor(SMeta *pMeta) {
  SMTbCursor *pTbCur = NULL;
#if 0
  SMetaDB    *pDB = pMeta->pDB;

  pTbCur = (SMTbCursor *)taosMemoryCalloc(1, sizeof(*pTbCur));
  if (pTbCur == NULL) {
    return NULL;
  }

  tdbDbcOpen(pDB->pTbDB, &pTbCur->pDbc);

#endif
  return pTbCur;
}

void metaCloseTbCursor(SMTbCursor *pTbCur) {
#if 0
  if (pTbCur) {
    if (pTbCur->pDbc) {
      tdbDbcClose(pTbCur->pDbc);
    }
    taosMemoryFree(pTbCur);
  }
#endif
}

char *metaTbCursorNext(SMTbCursor *pTbCur) {
#if 0
  void  *pKey = NULL;
  void  *pVal = NULL;
  int    kLen;
  int    vLen;
  int    ret;
  void  *pBuf;
  STbCfg tbCfg;

  for (;;) {
    ret = tdbDbNext(pTbCur->pDbc, &pKey, &kLen, &pVal, &vLen);
    if (ret < 0) break;
    pBuf = pVal;
    metaDecodeTbInfo(pBuf, &tbCfg);
    if (tbCfg.type == META_SUPER_TABLE) {
      taosMemoryFree(tbCfg.name);
      taosMemoryFree(tbCfg.stbCfg.pTagSchema);
      continue;
    } else if (tbCfg.type == META_CHILD_TABLE) {
      kvRowFree(tbCfg.ctbCfg.pTag);
    }

    return tbCfg.name;
  }

#endif
  return NULL;
}

STbCfg *metaGetTbInfoByUid(SMeta *pMeta, tb_uid_t uid) {
#if 0
  int      ret;
  SMetaDB *pMetaDb = pMeta->pDB;
  void    *pKey;
  void    *pVal;
  int      kLen;
  int      vLen;
  STbCfg  *pTbCfg;

  // Fetch
  pKey = &uid;
  kLen = sizeof(uid);
  pVal = NULL;
  ret = tdbDbGet(pMetaDb->pTbDB, pKey, kLen, &pVal, &vLen);
  if (ret < 0) {
    return NULL;
  }

  // Decode
  pTbCfg = taosMemoryMalloc(sizeof(*pTbCfg));
  metaDecodeTbInfo(pVal, pTbCfg);

  TDB_FREE(pVal);

  return pTbCfg;
#endif
  return NULL;
}

SSchemaWrapper *metaGetTableSchema(SMeta *pMeta, tb_uid_t uid, int32_t sver, bool isinline) {
  // return metaGetTableSchemaImpl(pMeta, uid, sver, isinline, false);
  return NULL;
}

SMCtbCursor *metaOpenCtbCursor(SMeta *pMeta, tb_uid_t uid) {
  SMCtbCursor *pCtbCur = NULL;
  // SMetaDB     *pDB = pMeta->pDB;
  // int          ret;

  // pCtbCur = (SMCtbCursor *)taosMemoryCalloc(1, sizeof(*pCtbCur));
  // if (pCtbCur == NULL) {
  //   return NULL;
  // }

  // pCtbCur->suid = uid;
  // ret = tdbDbcOpen(pDB->pCtbIdx, &pCtbCur->pCur);
  // if (ret < 0) {
  //   taosMemoryFree(pCtbCur);
  //   return NULL;
  // }

  return pCtbCur;
}

void metaCloseCtbCurosr(SMCtbCursor *pCtbCur) {
  // if (pCtbCur) {
  //   if (pCtbCur->pCur) {
  //     tdbDbcClose(pCtbCur->pCur);

  //     TDB_FREE(pCtbCur->pKey);
  //     TDB_FREE(pCtbCur->pVal);
  //   }

  //   taosMemoryFree(pCtbCur);
  // }
}

tb_uid_t metaCtbCursorNext(SMCtbCursor *pCtbCur) {
  // int         ret;
  // SCtbIdxKey *pCtbIdxKey;

  // ret = tdbDbNext(pCtbCur->pCur, &pCtbCur->pKey, &pCtbCur->kLen, &pCtbCur->pVal, &pCtbCur->vLen);
  // if (ret < 0) {
  //   return 0;
  // }

  // pCtbIdxKey = pCtbCur->pKey;

  // return pCtbIdxKey->uid;
  return 0;
}

STSchema *metaGetTbTSchema(SMeta *pMeta, tb_uid_t uid, int32_t sver) {
#if 0
  tb_uid_t        quid;
  SSchemaWrapper *pSW;
  STSchemaBuilder sb;
  SSchema        *pSchema;
  STSchema       *pTSchema;
  STbCfg         *pTbCfg;

  pTbCfg = metaGetTbInfoByUid(pMeta, uid);
  if (pTbCfg->type == META_CHILD_TABLE) {
    quid = pTbCfg->ctbCfg.suid;
  } else {
    quid = uid;
  }

  pSW = metaGetTableSchemaImpl(pMeta, quid, sver, true, true);
  if (pSW == NULL) {
    return NULL;
  }

  tdInitTSchemaBuilder(&sb, 0);
  for (int i = 0; i < pSW->nCols; i++) {
    pSchema = pSW->pSchema + i;
    tdAddColToSchema(&sb, pSchema->type, pSchema->flags, pSchema->colId, pSchema->bytes);
  }
  pTSchema = tdGetSchemaFromBuilder(&sb);
  tdDestroyTSchemaBuilder(&sb);

  return pTSchema;
#endif
  return NULL;
}

STSmaWrapper *metaGetSmaInfoByTable(SMeta *pMeta, tb_uid_t uid) {
#if 0
#ifdef META_TDB_SMA_TEST
  STSmaWrapper *pSW = NULL;

  pSW = taosMemoryCalloc(1, sizeof(*pSW));
  if (pSW == NULL) {
    return NULL;
  }

  SMSmaCursor *pCur = metaOpenSmaCursor(pMeta, uid);
  if (pCur == NULL) {
    taosMemoryFree(pSW);
    return NULL;
  }

  void       *pBuf = NULL;
  SSmaIdxKey *pSmaIdxKey = NULL;

  while (true) {
    // TODO: lock during iterate?
    if (tdbDbNext(pCur->pCur, &pCur->pKey, &pCur->kLen, NULL, &pCur->vLen) == 0) {
      pSmaIdxKey = pCur->pKey;
      ASSERT(pSmaIdxKey != NULL);

      void *pSmaVal = metaGetSmaInfoByIndex(pMeta, pSmaIdxKey->smaUid, false);

      if (pSmaVal == NULL) {
        tsdbWarn("no tsma exists for indexUid: %" PRIi64, pSmaIdxKey->smaUid);
        continue;
      }

      ++pSW->number;
      STSma *tptr = (STSma *)taosMemoryRealloc(pSW->tSma, pSW->number * sizeof(STSma));
      if (tptr == NULL) {
        TDB_FREE(pSmaVal);
        metaCloseSmaCursor(pCur);
        tdDestroyTSmaWrapper(pSW);
        taosMemoryFreeClear(pSW);
        return NULL;
      }
      pSW->tSma = tptr;
      pBuf = pSmaVal;
      if (tDecodeTSma(pBuf, pSW->tSma + pSW->number - 1) == NULL) {
        TDB_FREE(pSmaVal);
        metaCloseSmaCursor(pCur);
        tdDestroyTSmaWrapper(pSW);
        taosMemoryFreeClear(pSW);
        return NULL;
      }
      TDB_FREE(pSmaVal);
      continue;
    }
    break;
  }

  metaCloseSmaCursor(pCur);

  return pSW;

#endif
#endif
  return NULL;
}

STbCfg *metaGetTbInfoByName(SMeta *pMeta, char *tbname, tb_uid_t *uid) {
#if 0
  void *pKey;
  void *pVal;
  void *ppKey;
  int   pkLen;
  int   kLen;
  int   vLen;
  int   ret;

  pKey = tbname;
  kLen = strlen(tbname) + 1;
  pVal = NULL;
  ppKey = NULL;
  ret = tdbDbPGet(pMeta->pDB->pNameIdx, pKey, kLen, &ppKey, &pkLen, &pVal, &vLen);
  if (ret < 0) {
    return NULL;
  }

  ASSERT(pkLen == kLen + sizeof(uid));

  *uid = *(tb_uid_t *)POINTER_SHIFT(ppKey, kLen);
  TDB_FREE(ppKey);
  TDB_FREE(pVal);

  return metaGetTbInfoByUid(pMeta, *uid);
#endif
  return NULL;
}

int metaGetTbNum(SMeta *pMeta) {
  // TODO
  // ASSERT(0);
  return 0;
}

SArray *metaGetSmaTbUids(SMeta *pMeta, bool isDup) {
#if 0
  // TODO
  // ASSERT(0); // comment this line to pass CI
  // return NULL:
#ifdef META_TDB_SMA_TEST
  SArray  *pUids = NULL;
  SMetaDB *pDB = pMeta->pDB;
  void    *pKey;

  // TODO: lock?
  SMSmaCursor *pCur = metaOpenSmaCursor(pMeta, 0);
  if (pCur == NULL) {
    return NULL;
  }
  // TODO: lock?

  SSmaIdxKey *pSmaIdxKey = NULL;
  tb_uid_t    uid = 0;
  while (true) {
    // TODO: lock during iterate?
    if (tdbDbNext(pCur->pCur, &pCur->pKey, &pCur->kLen, NULL, &pCur->vLen) == 0) {
      ASSERT(pSmaIdxKey != NULL);
      pSmaIdxKey = pCur->pKey;

      if (pSmaIdxKey->uid == 0 || pSmaIdxKey->uid == uid) {
        continue;
      }
      uid = pSmaIdxKey->uid;

      if (!pUids) {
        pUids = taosArrayInit(16, sizeof(tb_uid_t));
        if (!pUids) {
          metaCloseSmaCursor(pCur);
          return NULL;
        }
      }

      taosArrayPush(pUids, &uid);

      continue;
    }
    break;
  }

  metaCloseSmaCursor(pCur);

  return pUids;
#endif
#endif
  return NULL;
}

void *metaGetSmaInfoByIndex(SMeta *pMeta, int64_t indexUid, bool isDecode) {
#if 0
  // TODO
  // ASSERT(0);
  // return NULL;
#ifdef META_TDB_SMA_TEST
  SMetaDB *pDB = pMeta->pDB;
  void    *pKey = NULL;
  void    *pVal = NULL;
  int      kLen = 0;
  int      vLen = 0;
  int      ret = -1;

  // Set key
  pKey = (void *)&indexUid;
  kLen = sizeof(indexUid);

  // Query
  ret = tdbDbGet(pDB->pSmaDB, pKey, kLen, &pVal, &vLen);
  if (ret != 0 || !pVal) {
    return NULL;
  }

  if (!isDecode) {
    // return raw value
    return pVal;
  }

  // Decode
  STSma *pCfg = (STSma *)taosMemoryCalloc(1, sizeof(STSma));
  if (pCfg == NULL) {
    taosMemoryFree(pVal);
    return NULL;
  }

  void *pBuf = pVal;
  if (tDecodeTSma(pBuf, pCfg) == NULL) {
    tdDestroyTSma(pCfg);
    taosMemoryFree(pCfg);
    TDB_FREE(pVal);
    return NULL;
  }

  TDB_FREE(pVal);
  return pCfg;
#endif
#endif
  return NULL;
}