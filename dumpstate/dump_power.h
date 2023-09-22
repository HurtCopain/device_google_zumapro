/*
 * Copyright 2023 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __DUMP_POWER_H
#define __DUMP_POWER_H

#include "uapi/brownout_stats.h"

/* BrownoutBinaryStatsConfig */
#define NSEC_PER_SEC             1000000000L
#define DUMP_TIMES               12
#define FVP_STATS_SIZE           4096
#define UP_DOWN_LINK_SIZE        512
#define STAT_NAME_SIZE           48
#define STATS_MAX_SIZE           64

struct PmicSpecific {
    const char *const OdpmDir;
    const char *const OdpmEnabledRailsPath;
    const char *const PmicNamePath;
    const char *const PmicName;
};

struct numericStat {
    char name[STAT_NAME_SIZE];
    int value;
};

struct BrownoutStatsExtend {
    struct brownout_stats brownoutStats;
    char fvpStats[FVP_STATS_SIZE];
    char pcieModem[UP_DOWN_LINK_SIZE];
    char pcieWifi[UP_DOWN_LINK_SIZE];
    struct numericStat numericStats[STATS_MAX_SIZE];
    timeval eventReceivedTime;
    timeval dumpTime;
    unsigned int eventIdx;
};

struct OdpmInstantPower {
    struct timespec time;
    double value;
};

struct BrownoutBinaryStatsConfig {
    const char *const StoringPath;
    const char *const BackupPath;
    const char *const ThismealFileName;
    const char *const LastmealFileName;
    const std::vector<PmicSpecific> pmic;
};

#endif /* __DUMP_POWER_H */
