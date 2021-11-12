/*
 * Copyright (c) 2019 TAOS Data, Inc. <cli@taosdata.com>
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

#include "syncInt.h"
#include "sync_raft_config_change.h"
#include "sync_raft_progress.h"
#include "sync_raft_progress_tracker.h"

static int checkAndCopy(SSyncRaftChanger* changer, SSyncRaftProgressTrackerConfig* config, SSyncRaftProgressMap* progressMap);
static int checkAndReturn(SSyncRaftProgressTrackerConfig* config, SSyncRaftProgressMap* progressMap);
static int checkInvariants(SSyncRaftProgressTrackerConfig* config, SSyncRaftProgressMap* progressMap);
static int checkInvariants(SSyncRaftProgressTrackerConfig* config, SSyncRaftProgressMap* progressMap);
static bool hasJointConfig(const SSyncRaftProgressTrackerConfig* config);
static int applyConfig(SSyncRaftChanger* changer, const SSyncRaftProgressTrackerConfig* config,
                        const SSyncRaftProgressMap* progressMap, const SSyncConfChangeSingleArray* css);
static int symDiff(const SSyncCluster* l, const SSyncCluster* r);

// Simple carries out a series of configuration changes that (in aggregate)
// mutates the incoming majority config Voters[0] by at most one. This method
// will return an error if that is not the case, if the resulting quorum is
// zero, or if the configuration is in a joint state (i.e. if there is an
// outgoing configuration).
int syncRaftChangerSimpleConfig(SSyncRaftChanger* changer, const SSyncConfChangeSingleArray* css,
                            SSyncRaftProgressTrackerConfig* config, SSyncRaftProgressMap* progressMap) {
  int ret;

  ret = checkAndCopy(changer, config, progressMap);
  if (ret != 0) {
    return ret;
  }

  if (hasJointConfig(config)) {
    return -1;
  }

  ret = applyConfig(changer, config, progressMap, css);
  if (ret != 0) {
    return ret;
  }

  int n = symDiff(syncRaftJointConfigIncoming(&changer->tracker->config.voters),
                  syncRaftJointConfigIncoming(&config->voters));
  if (n > 1) {
    syncError("more than one voter changed without entering joint config");
    return -1;
  }

  return checkAndReturn(config, progressMap); 
}

// EnterJoint verifies that the outgoing (=right) majority config of the joint
// config is empty and initializes it with a copy of the incoming (=left)
// majority config. That is, it transitions from
//
//     (1 2 3)&&()
// to
//     (1 2 3)&&(1 2 3).
//
// The supplied changes are then applied to the incoming majority config,
// resulting in a joint configuration that in terms of the Raft thesis[1]
// (Section 4.3) corresponds to `C_{new,old}`.
//
// [1]: https://github.com/ongardie/dissertation/blob/master/online-trim.pdf
int syncRaftChangerEnterJoint(SSyncRaftChanger* changer, const SSyncConfChangeSingleArray* css,
                            SSyncRaftProgressTrackerConfig* config, SSyncRaftProgressMap* progressMap) {

}

// checkAndCopy copies the tracker's config and progress map (deeply enough for
// the purposes of the Changer) and returns those copies. It returns an error
// if checkInvariants does.
static int checkAndCopy(SSyncRaftChanger* changer, SSyncRaftProgressTrackerConfig* config, SSyncRaftProgressMap* progressMap) {
  syncRaftCloneTrackerConfig(&changer->tracker->config, config);
  int i;
  for (i = 0; i < TSDB_MAX_REPLICA; ++i) {
    SSyncRaftProgress* progress = &(changer->tracker->progressMap.progress[i]);
    if (progress->id == SYNC_NON_NODE_ID) {
      continue;
    }
    syncRaftCopyProgress(progress, &(progressMap->progress[i]));
  }
  return checkAndReturn(config, progressMap);
}

// checkAndReturn calls checkInvariants on the input and returns either the
// resulting error or the input.
static int checkAndReturn(SSyncRaftProgressTrackerConfig* config, SSyncRaftProgressMap* progressMap) {
  if (checkInvariants(config, progressMap) != 0) {
    return -1;
  }

  return 0;
}

// checkInvariants makes sure that the config and progress are compatible with
// each other. This is used to check both what the Changer is initialized with,
// as well as what it returns.
static int checkInvariants(SSyncRaftProgressTrackerConfig* config, SSyncRaftProgressMap* progressMap) {
  int ret = syncRaftCheckProgress(config, progressMap);
  if (ret != 0) {
    return ret;
  }

  int i;
	// Any staged learner was staged because it could not be directly added due
	// to a conflicting voter in the outgoing config.
  for (i = 0; i < TSDB_MAX_REPLICA; ++i) {
    if (!syncRaftJointConfigInOutgoing(&config->voters, config->learnersNext.nodeId[i])) {
      return -1;
    }
    if (progressMap->progress[i].id != SYNC_NON_NODE_ID && progressMap->progress[i].isLearner) {
      syncError("%d is in LearnersNext, but is already marked as learner", progressMap->progress[i].id);
      return -1;
    }
  }
  // Conversely Learners and Voters doesn't intersect at all.
  for (i = 0; i < TSDB_MAX_REPLICA; ++i) {
    if (syncRaftJointConfigInIncoming(&config->voters, config->learners.nodeId[i])) {
      syncError("%d is in Learners and voter.incoming", progressMap->progress[i].id);
      return -1;
    }
    if (progressMap->progress[i].id != SYNC_NON_NODE_ID && !progressMap->progress[i].isLearner) {
      syncError("%d is in Learners, but is not marked as learner", progressMap->progress[i].id);
      return -1;
    }
  }

  if (!hasJointConfig(config)) {
    // We enforce that empty maps are nil instead of zero.
    if (config->learnersNext.replica > 0) {
      syncError("cfg.LearnersNext must be nil when not joint");
      return -1;
    }
    if (config->autoLeave) {
      syncError("AutoLeave must be false when not joint");
      return -1;
    }
  }

  return 0;
}

static bool hasJointConfig(const SSyncRaftProgressTrackerConfig* config) {
  return config->voters.outgoing.replica > 0;
}

static int applyConfig(SSyncRaftChanger* changer, const SSyncRaftProgressTrackerConfig* config,
                        const SSyncRaftProgressMap* progressMap, const SSyncConfChangeSingleArray* css) {
  int i;

  for (i = 0; i < css->n; ++i) {
    const SSyncConfChangeSingle* cs = &(css->changes[i]);
    if (cs->nodeId == SYNC_NON_NODE_ID) {
      continue;
    }

    ESyncRaftConfChangeType type = cs->type;
    switch (type) {

    }
  }

  if (config->voters.incoming.replica == 0) {
    return -1;
  }

  return 0;
}

// symdiff returns the count of the symmetric difference between the sets of
// uint64s, i.e. len( (l - r) \union (r - l)).
static int symDiff(const SSyncCluster* l, const SSyncCluster* r) {
  int n;
  int i;
  int j0, j1;
  const SSyncCluster* pairs[2][2] = {
    {l, r}, // count elems in l but not in r
    {r, l}, // count elems in r but not in l
  };

  for (n = 0, i = 0; i < 2; ++i) {
    const SSyncCluster** pp = pairs[i];

    const SSyncCluster* p0 = pp[0];
    const SSyncCluster* p1 = pp[1];
    for (j0 = 0; j0 < p0->replica; ++j0) {
      SyncNodeId id = p0->nodeInfo[j0].nodeId;
      if (id == SYNC_NON_NODE_ID) {
        continue;
      }
      for (j1 = 0; j1 < p1->replica; ++j1) {
        if (p1->nodeInfo[j1].nodeId != SYNC_NON_NODE_ID && p1->nodeInfo[j1].nodeId != id) {
          n+=1;
        }
      }
    }
  }

  return n;
}