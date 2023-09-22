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

#include <algorithm>
#include <cmath>
#include <cstring>
#include <dirent.h>
#include <dump/pixel_dump.h>
#include <fstream>
#include <stdio.h>
#include <stdlib.h>
#include <sys/sysinfo.h>
#include <time.h>
#include <vector>
#include <sys/mman.h>
#include <sys/stat.h>

#include <android-base/file.h>
#include <android-base/strings.h>
#include "DumpstateUtil.h"
#include "dump_power.h"

void printTitle(const char *msg) {
    printf("\n------ %s ------\n", msg);
}

int getCommandOutput(const char *cmd, std::string *output) {
    char buffer[1024];
    FILE *pipe = popen(cmd, "r");
    if (!pipe) {
        return -1;
    }

    while (fgets(buffer, sizeof buffer, pipe) != NULL) {
        *output += buffer;
    }
    pclose(pipe);

    if (output->back() == '\n')
        output->pop_back();

    return 0;
}

bool isValidFile(const char *file) {
    FILE *fp = fopen(file, "r");
    if (fp != NULL) {
        fclose(fp);
        return true;
    }
    return false;
}

bool isValidDir(const char *directory) {
    DIR *dir = opendir(directory);
    if (dir == NULL)
        return false;

    closedir(dir);
    return true;
}

bool isUserBuild() {
    return ::android::os::dumpstate::PropertiesHelper::IsUserBuild();
}

int getFilesInDir(const char *directory, std::vector<std::string> *files) {
    std::string content;
    struct dirent *entry;

    DIR *dir = opendir(directory);
    if (dir == NULL)
        return -1;

    files->clear();
    while ((entry = readdir(dir)) != NULL)
        files->push_back(entry->d_name);
    closedir(dir);

    sort(files->begin(), files->end());
    return 0;
}

void dumpPowerStatsTimes() {
    const char *title = "Power Stats Times";
    char rBuff[128];
    struct timespec rTs;
    struct sysinfo info;
    int ret;

    printTitle(title);

    sysinfo(&info);

    const time_t boottime = time(NULL) - info.uptime;

    ret = clock_gettime(CLOCK_REALTIME, &rTs);
    if (ret)
        return;

    struct tm *nowTime = std::localtime(&rTs.tv_sec);

    std::strftime(rBuff, sizeof(rBuff), "%m/%d/%Y %H:%M:%S", nowTime);
    printf("Boot: %s", ctime(&boottime));
    printf("Now: %s\n", rBuff);
}

int readContentsOfDir(const char* title, const char* directory, const char* strMatch,
        bool useStrMatch = false, bool printDirectory = false) {
    std::vector<std::string> files;
    std::string content;
    std::string fileLocation;
    int ret;

    ret = getFilesInDir(directory, &files);
    if (ret < 0)
        return ret;

    printTitle(title);
    for (auto &file : files) {
        if (useStrMatch && std::string::npos == std::string(file).find(strMatch)) {
            continue;
        }

        fileLocation = std::string(directory) + std::string(file);
        if (!android::base::ReadFileToString(fileLocation, &content)) {
            continue;
        }
        if (printDirectory) {
            printf("\n\n%s\n", fileLocation.c_str());
        }
        if (content.back() == '\n')
            content.pop_back();
        printf("%s\n", content.c_str());
    }
    return 0;
}

void dumpAcpmStats() {
    const char* acpmDir = "/sys/devices/platform/acpm_stats/";
    const char* statsSubStr = "_stats";
    const char* acpmTitle = "ACPM stats";
    readContentsOfDir(acpmTitle, acpmDir, statsSubStr, true, true);
}

void dumpPowerSupplyStats() {
    const char* dumpList[][2] = {
            {"CPU PM stats", "/sys/devices/system/cpu/cpupm/cpupm/time_in_state"},
            {"GENPD summary", "/d/pm_genpd/pm_genpd_summary"},
            {"Power supply property battery", "/sys/class/power_supply/battery/uevent"},
            {"Power supply property dc", "/sys/class/power_supply/dc/uevent"},
            {"Power supply property gcpm", "/sys/class/power_supply/gcpm/uevent"},
            {"Power supply property gcpm_pps", "/sys/class/power_supply/gcpm_pps/uevent"},
            {"Power supply property main-charger", "/sys/class/power_supply/main-charger/uevent"},
            {"Power supply property dc-mains", "/sys/class/power_supply/dc-mains/uevent"},
            {"Power supply property tcpm", "/sys/class/power_supply/tcpm-source-psy-8-0025/uevent"},
            {"Power supply property usb", "/sys/class/power_supply/usb/uevent"},
            {"Power supply property wireless", "/sys/class/power_supply/wireless/uevent"},
    };

    for (const auto &row : dumpList) {
        dumpFileContent(row[0], row[1]);
    }
}

void dumpMaxFg() {
    const char *maxfgLoc = "/sys/class/power_supply/maxfg";
    const char *max77779fgDir = "/sys/class/power_supply/max77779fg";

    const char *maxfg [][2] = {
            {"Power supply property maxfg", "/sys/class/power_supply/maxfg/uevent"},
            {"m5_state", "/sys/class/power_supply/maxfg/m5_model_state"},
            {"maxfg logbuffer", "/dev/logbuffer_maxfg"},
            {"maxfg_monitor logbuffer", "/dev/logbuffer_maxfg_monitor"},
    };

    const char *max77779fgFiles [][2] = {
            {"Power supply property max77779fg", "/sys/class/power_supply/max77779fg/uevent"},
            {"m5_state", "/sys/class/power_supply/max77779fg/model_state"},
            {"max77779fg logbuffer", "/dev/logbuffer_max77779fg"},
            {"max77779fg_monitor logbuffer", "/dev/logbuffer_max77779fg_monitor"},
    };

    const char *maxfgSecondary [][2] = {
            {"Power supply property maxfg_base", "/sys/class/power_supply/maxfg_base/uevent"},
            {"Power supply property maxfg_secondary", "/sys/class/power_supply/maxfg_secondary/uevent"},
            {"model_state", "/sys/class/power_supply/maxfg_base/model_state"},
            {"maxfg_base", "/dev/logbuffer_maxfg_base"},
            {"maxfg_secondary", "/dev/logbuffer_maxfg_secondary"},
            {"maxfg_base_monitor logbuffer", "/dev/logbuffer_maxfg_base_monitor"},
            {"maxfg_secondary_monitor logbuffer", "/dev/logbuffer_maxfg_secondary_monitor"},
    };

    const char *maxfgHistoryName = "Maxim FG History";
    const char *maxfgHistoryDir = "/dev/maxfg_history";

    std::string content;


    if (isValidDir(maxfgLoc)) {
        for (const auto &row : maxfg) {
            dumpFileContent(row[0], row[1]);
        }
    } else if (isValidDir(max77779fgDir)) {
        for (const auto &row : max77779fgFiles) {
            dumpFileContent(row[0], row[1]);
        }
    } else {
        for (const auto &row : maxfgSecondary) {
            dumpFileContent(row[0], row[1]);
        }
    }

    if (isValidFile(maxfgHistoryDir)) {
        dumpFileContent(maxfgHistoryName, maxfgHistoryDir);
    }
}

void dumpPowerSupplyDock() {
    const char* powerSupplyPropertyDockTitle = "Power supply property dock";
    const char* powerSupplyPropertyDockFile = "/sys/class/power_supply/dock/uevent";
    if (isValidFile(powerSupplyPropertyDockFile)) {
        dumpFileContent(powerSupplyPropertyDockTitle, powerSupplyPropertyDockFile);
    }
}

void dumpLogBufferTcpm() {
    const char* logbufferTcpmTitle = "Logbuffer TCPM";
    const char* logbufferTcpmFile = "/dev/logbuffer_tcpm";
    const char* debugTcpmFile = "/sys/kernel/debug/tcpm";
    const char* tcpmLogTitle = "TCPM logs";
    const char* tcpmFile = "/sys/kernel/debug/tcpm";
    const char* tcpmFileAlt = "/sys/kernel/debug/usb/tcpm";
    int retCode;

    dumpFileContent(logbufferTcpmTitle, logbufferTcpmFile);

    retCode = readContentsOfDir(tcpmLogTitle, isValidFile(debugTcpmFile) ? tcpmFile : tcpmFileAlt,
            NULL);
    if (retCode < 0)
        printTitle(tcpmLogTitle);
}

void dumpTcpc() {
    int ret;
    const char* max77759TcpcHead = "TCPC";
    const char* i2cSubDirMatch = "i2c-";
    const char* directory = "/sys/devices/platform/10d60000.hsi2c/";
    const char* max77759Tcpc [][2] {
            {"registers:", "/i2c-max77759tcpc/registers"},
            {"frs:", "/i2c-max77759tcpc/frs"},
            {"auto_discharge:", "/i2c-max77759tcpc/auto_discharge"},
            {"bcl2_enabled:", "/i2c-max77759tcpc/bcl2_enabled"},
            {"cc_toggle_enable:", "/i2c-max77759tcpc/cc_toggle_enable"},
            {"containment_detection:", "/i2c-max77759tcpc/containment_detection"},
            {"containment_detection_status:", "/i2c-max77759tcpc/containment_detection_status"},
    };

    std::vector<std::string> files;
    std::string content;

    printTitle(max77759TcpcHead);

    ret = getFilesInDir(directory, &files);
    if (ret < 0) {
        for (auto &tcpcVal : max77759Tcpc)
            printf("%s\n", tcpcVal[0]);
        return;
    }

    for (auto &file : files) {
        for (auto &tcpcVal : max77759Tcpc) {
            printf("%s ", tcpcVal[0]);
            if (std::string::npos == std::string(file).find(i2cSubDirMatch)) {
                continue;
            }

            std::string fileName = directory + file + "/" + std::string(tcpcVal[1]);

            if (!android::base::ReadFileToString(fileName, &content)) {
                continue;
            }

            printf("%s\n", content.c_str());
        }
    }
}

void dumpPdEngine() {
    const char* pdEngine [][2] {
            {"PD Engine logbuffer", "/dev/logbuffer_usbpd"},
            {"PPS-google_cpm logbuffer", "/dev/logbuffer_cpm"},
    };
    const char* ppsDcMsg = "PPS-dc logbuffer";
    const char* pca9468dir = "/dev/logbuffer_pca9468";
    const char* ln8411dir = "/dev/logbuffer_ln8411";

    for (const auto &row : pdEngine) {
        dumpFileContent(row[0], row[1]);
    }
    if (isValidFile(pca9468dir)) {
        dumpFileContent(ppsDcMsg, pca9468dir);
    } else {
        dumpFileContent(ppsDcMsg, ln8411dir);
    }
}

void dumpBatteryHealth() {
    const char* batteryHealth [][2] {
            {"Battery Health", "/sys/class/power_supply/battery/health_index_stats"},
            {"BMS logbuffer", "/dev/logbuffer_ssoc"},
            {"TTF logbuffer", "/dev/logbuffer_ttf"},
            {"TTF details", "/sys/class/power_supply/battery/ttf_details"},
            {"TTF stats", "/sys/class/power_supply/battery/ttf_stats"},
            {"aacr_state", "/sys/class/power_supply/battery/aacr_state"},
            {"pairing_state", "/sys/class/power_supply/battery/pairing_state"},
    };

    const char* maxqName = "maxq logbuffer";
    const char* maxqDir = "/dev/logbuffer_maxq";
    const char* tempDockDefendName = "TEMP/DOCK-DEFEND";
    const char* tempDockDefendDir = "/dev/logbuffer_bd";

    for (const auto &row : batteryHealth) {
        dumpFileContent(row[0], row[1]);
    }

    if (isValidFile(maxqDir))
        dumpFileContent(maxqName, maxqDir);

    dumpFileContent(tempDockDefendName, tempDockDefendDir);
}

void dumpBatteryDefend() {
    const char* defendConfig [][3] {
            {"TRICKLE-DEFEND Config",
                    "/sys/devices/platform/google,battery/power_supply/battery/", "bd_"},
            {"DWELL-DEFEND Config", "/sys/devices/platform/google,charger/", "charge_s"},
            {"TEMP-DEFEND Config", "/sys/devices/platform/google,charger/", "bd_"},
    };

    std::vector<std::string> files;
    struct dirent *entry;
    std::string content;
    std::string fileLocation;

    for (auto &config : defendConfig) {
        DIR *dir = opendir(config[1]);
        if (dir == NULL)
            continue;

        printTitle(config[0]);
        while ((entry = readdir(dir)) != NULL) {
            if (std::string(entry->d_name).find(config[2]) != std::string::npos &&
                    strncmp(config[2], entry->d_name, strlen(config[2])) == 0) {
                files.push_back(entry->d_name);
            }
        }
        closedir(dir);

        sort(files.begin(), files.end());

        for (auto &file : files) {
            fileLocation = std::string(config[1]) + std::string(file);
            if (!android::base::ReadFileToString(fileLocation, &content)) {
                content = "\n";
            }

            printf("%s: %s", file.c_str(), content.c_str());

            if (content.back() != '\n')
                printf("\n");
        }

        files.clear();
    }
}

void printValuesOfDirectory(const char *directory, std::string debugfs, const char *strMatch) {
    std::vector<std::string> files;
    auto info = directory;
    std::string content;
    struct dirent *entry;
    DIR *dir = opendir(debugfs.c_str());
    if (dir == NULL)
        return;

    printTitle((debugfs + std::string(strMatch) + "/" + std::string(info)).c_str());
    while ((entry = readdir(dir)) != NULL)
        if (std::string(entry->d_name).find(strMatch) != std::string::npos)
            files.push_back(entry->d_name);
    closedir(dir);

    sort(files.begin(), files.end());

    for (auto &file : files) {
        std::string fileDirectory = debugfs + file;
        std::string fileLocation = fileDirectory + "/" + std::string(info);
        if (!android::base::ReadFileToString(fileLocation, &content)) {
            content = "\n";
        }

        printf("%s:\n%s", fileDirectory.c_str(), content.c_str());

        if (content.back() != '\n')
            printf("\n");
    }
    files.clear();
}

void dumpChgUserDebug() {
    const char *chgDebugMax77759 [][2] {
            {"max77759_chg registers dump", "/d/max77759_chg/registers/"},
            {"max77729_pmic registers dump", "/d/max77729_pmic/registers/"},
    };
    const char *chgDebugMax77779 [][2] {
            {"max77779_chg registers dump", "/d/max77779_chg/registers/"},
            {"max77779_pmic registers dump", "/d/max77779_pmic/registers/"},
    };

    const std::string debugfs = "/d/";

    const char *maxFgDir = "/d/maxfg";
    const char *maxFgStrMatch = "maxfg";
    const char *maxFg77779StrMatch = "max77779fg";
    const char *baseChgDir = "/d/max77759_chg";
    const char *dcRegName = "DC_registers dump";
    const char *dcRegDir = "/sys/class/power_supply/dc-mains/device/registers_dump";
    const char *chgTblName = "Charging table dump";
    const char *chgTblDir = "/d/google_battery/chg_raw_profile";

    const char *maxFgInfo [] {
            "fg_model",
            "algo_ver",
            "model_ok",
            "registers",
            "nv_registers",
    };

    const char *max77779FgInfo [] {
            "fg_model",
            "algo_ver",
            "model_ok",
            "registers",
            "debug_registers",
    };

    if (isUserBuild())
        return;

    dumpFileContent(dcRegName, dcRegDir);

    if (isValidFile(baseChgDir)) {
        for (auto &row : chgDebugMax77759) {
            dumpFileContent(row[0], row[1]);
        }
    } else {
        for (auto &row : chgDebugMax77779) {
            dumpFileContent(row[0], row[1]);
        }
    }

    dumpFileContent(chgTblName, chgTblDir);

    if (isValidFile(maxFgDir)) {
        for (auto & directory : maxFgInfo) {
            printValuesOfDirectory(directory, debugfs, maxFgStrMatch);
        }
    } else {
        for (auto & directory : max77779FgInfo) {
            printValuesOfDirectory(directory, debugfs, maxFg77779StrMatch);
        }
    }
}

void dumpBatteryEeprom() {
    const char *title = "Battery EEPROM";
    const char *files[] {
            "/sys/devices/platform/10ca0000.hsi2c/i2c-10/10-0050/eeprom",
            "/sys/devices/platform/10c90000.hsi2c/i2c-9/9-0050/eeprom",
    };
    std::string result;
    std::string xxdCmd;

    printTitle(title);
    for (auto &file : files) {
        if (!isValidFile(file))
            continue;

        xxdCmd = "xxd " + std::string(file);

        int ret = getCommandOutput(xxdCmd.c_str(), &result);
        if (ret < 0)
            return;

        printf("%s\n", result.c_str());
    }
}

void dumpChargerStats() {
    const char *chgStatsTitle = "Charger Stats";
    const char *chgStatsLocation = "/sys/class/power_supply/battery/charge_details";
    const char *chargerStats [][3] {
            {"Google Charger", "/sys/kernel/debug/google_charger/", "pps_"},
            {"Google Battery", "/sys/kernel/debug/google_battery/", "ssoc_"},
    };
    std::vector<std::string> files;
    std::string content;
    struct dirent *entry;

    dumpFileContent(chgStatsTitle, chgStatsLocation);

    if (isUserBuild())
        return;

    for (auto &stat : chargerStats) {
        DIR *dir = opendir(stat[1]);
        if (dir == NULL)
            return;

        printTitle(stat[0]);
        while ((entry = readdir(dir)) != NULL)
            if (std::string(entry->d_name).find(stat[2]) != std::string::npos)
                files.push_back(entry->d_name);
        closedir(dir);

        sort(files.begin(), files.end());

        for (auto &file : files) {
            std::string fileLocation = std::string(stat[1]) + file;
            if (!android::base::ReadFileToString(fileLocation, &content)) {
                content = "\n";
            }

            printf("%s: %s", file.c_str(), content.c_str());

            if (content.back() != '\n')
                printf("\n");
        }
        files.clear();
    }
}

void dumpWlcLogs() {
    const char *dumpWlcList [][2] {
            {"WLC Logs", "/dev/logbuffer_wireless"},
            {"WLC VER", "/sys/class/power_supply/wireless/device/version"},
            {"WLC STATUS", "/sys/class/power_supply/wireless/device/status"},
            {"WLC FW Version", "/sys/class/power_supply/wireless/device/fw_rev"},
            {"RTX", "/dev/logbuffer_rtx"},
    };

    for (auto &row : dumpWlcList) {
        if (!isValidFile(row[1]))
            printTitle(row[0]);
        dumpFileContent(row[0], row[1]);
    }
}

void dumpGvoteables() {
    const char *directory = "/sys/kernel/debug/gvotables/";
    const char *statusName = "/status";
    const char *title = "gvotables";
    std::string content;
    std::vector<std::string> files;
    int ret;

    if (isUserBuild())
        return;

    ret = getFilesInDir(directory, &files);
    if (ret < 0)
        return;

    printTitle(title);
    for (auto &file : files) {
        std::string fileLocation = std::string(directory) + file + std::string(statusName);
        if (!android::base::ReadFileToString(fileLocation, &content)) {
            continue;
        }

        printf("%s: %s", file.c_str(), content.c_str());

        if (content.back() != '\n')
            printf("\n");
    }
    files.clear();
}

void dumpMitigation() {
    const char *mitigationList [][2] {
            {"Lastmeal" , "/data/vendor/mitigation/lastmeal.txt"},
            {"Thismeal" , "/data/vendor/mitigation/thismeal.txt"},
    };

    for (auto &row : mitigationList) {
        if (!isValidFile(row[1]))
            printTitle(row[0]);
        dumpFileContent(row[0], row[1]);
    }
}

bool readSysfsToDouble(const std::string &path, double *val) {
    std::string file_contents;

    if (!android::base::ReadFileToString(path, &file_contents)) {
        return false;
    } else if (sscanf(file_contents.c_str(), "%lf", val) != 1) {
        return false;
    }
    return true;
}

void readLPFPowerBitResolutions(const char *odpmDir, double *bitResolutions) {
    char path[128];

    for (int i = 0; i < METER_CHANNEL_MAX; i++) {
        snprintf(path, 128, "%s/in_power%d_scale", odpmDir, i);
        if (!readSysfsToDouble(path, &bitResolutions[i])) {
            /* using large negative value to notify this odpm value is invalid */
            bitResolutions[i] = -1000;
        }
    }
}

void readLPFChannelNames(const char *odpmEnabledRailsPath, char **lpfChannelNames) {
    char *line = NULL;
    size_t len = 0;
    ssize_t read;

    FILE *fp = fopen(odpmEnabledRailsPath, "r");
    if (fp == NULL)
        return;

    int c = 0;
    while ((read = getline(&line, &len, fp)) != -1 && read != 0) {
        lpfChannelNames[c] = (char *)malloc(read);
        if (lpfChannelNames[c] != nullptr) {
            snprintf(lpfChannelNames[c], read, "%s", line);
        }
        if (++c == METER_CHANNEL_MAX)
            break;
    }
    fclose(fp);

    if (line)
        free(line);
}

int getMainPmicID(const char *mainPmicNamePath, const char *mainPmicName) {
    std::string content;
    int ret = 0;

    if (!android::base::ReadFileToString(mainPmicNamePath, &content)) {
        printf("Failed to open %s, set device0 as main pmic\n", mainPmicNamePath);
        return ret;
    }

    if (strcmp(content.c_str(), mainPmicName) != 0) {
        ret = 1;
    }

    return ret;
}

void freeLpfChannelNames(char **lpfChannelNames) {
    for (int c = 0; c < METER_CHANNEL_MAX; c++){
        free(lpfChannelNames[c]);
    }
}

void printUTC(struct timespec time, const char *stat) {
    char timeBuff[128];
    if (strlen(stat) > 0) {
        printf("%s: ", stat);
    }
    std::strftime(timeBuff, sizeof(timeBuff), "%m/%d/%Y_%H:%M:%S", std::localtime(&time.tv_sec));
    printf("%s.%lu",timeBuff, time.tv_nsec);
}

void printUTC(timeval time, const char *stat) {
    char timeBuff[128];
    if (strlen(stat) > 0) {
        printf("%s: ", stat);
    }
    std::strftime(timeBuff, sizeof(timeBuff), "%m/%d/%Y_%H:%M:%S", std::localtime(&time.tv_sec));
    /* convert usec to nsec */
    printf("%s.%lu000",timeBuff, time.tv_usec);
}

void printODPMChannelSummary(std::vector<odpm_instant_data> &odpmData,
                        double *lpfBitResolutions, char **lpfChannelNames) {
    std::vector<timespec> validTime;
    std::vector<OdpmInstantPower> instPower[METER_CHANNEL_MAX];
    std::vector<OdpmInstantPower> instPowerMax;
    std::vector<OdpmInstantPower> instPowerMin;
    std::vector<double> instPowerList;
    std::vector<double> instPowerStd;

    if (odpmData.size() == 0)
        return;

    /* initial Max, Min, Sum for sorting*/
    timespec curTime = odpmData[0].time;
    validTime.emplace_back(curTime);
    for (int c = 0; c < METER_CHANNEL_MAX; c++) {
        double power = lpfBitResolutions[c] * odpmData[0].value[c];
        instPower[c].emplace_back((OdpmInstantPower){curTime, power});
        instPowerMax.emplace_back((OdpmInstantPower){curTime, power});
        instPowerMin.emplace_back((OdpmInstantPower){curTime, power});
        instPowerList.emplace_back(power);
    }

    for (auto lpf = (odpmData.begin() + 1); lpf != odpmData.end(); lpf++) {
        curTime = lpf->time;
        /* remove duplicate data by checking the odpm instant data dump time */
        auto it =  std::find_if(validTime.begin(), validTime.end(),
                                [&_ts = curTime] (const timespec &ts) ->
                                bool {return _ts.tv_sec  == ts.tv_sec && _ts.tv_nsec  == ts.tv_nsec;});
        if (it == validTime.end()) {
            validTime.emplace_back(curTime);
            for (int c = 0; c < METER_CHANNEL_MAX; c++){
                double power = lpfBitResolutions[c] * lpf->value[c];
                instPower[c].emplace_back((OdpmInstantPower){curTime, power});
                instPowerList[c] += power;
                if (power > instPowerMax[c].value) {
                    instPowerMax[c].value = power;
                    instPowerMax[c].time = curTime;
                }
                if (power < instPowerMin[c].value) {
                    instPowerMin[c].value = power;
                    instPowerMin[c].time = curTime;
                }
            }
        }
    }

    int n = validTime.size();
    for (int c = 0; c < METER_CHANNEL_MAX; c++) {
        /* sort instant power by time */
        std::sort(instPower[c].begin(), instPower[c].end(),
                  [] (const auto &i, const auto &j)
                  {return i.time.tv_sec <= j.time.tv_sec && i.time.tv_nsec < j.time.tv_nsec;});
        /* compute std for each channel */
        double avg = instPowerList[c] / n;
        double mse = 0;
        for (int i = 0; i < n; i++) {
            mse += pow(instPower[c][i].value - avg, 2);
        }
        instPowerStd.emplace_back(pow(mse / n, 0.5));
    }

    /* print Max, Min, Avg, Std */
    for (int c = 0; c < METER_CHANNEL_MAX; c++) {
        printf("%s Max: %.2f Min: %.2f Avg: %.2f Std: %.2f\n", lpfChannelNames[c],
               instPowerMax[c].value,
               instPowerMin[c].value,
               instPowerList[c] / n,
               instPowerStd[c]);
    }
    printf("\n");

    /* print time */
    printf("time ");
    for (int i = 0; i < n; i++) {
        printUTC(instPower[0][i].time, "");
        printf(" ");
    }
    printf("\n");

    /* print instant power by channel */
    for (int c = 0; c < METER_CHANNEL_MAX; c++){
        printf("%s ", lpfChannelNames[c]);
        for (int i = 0; i < n; i++) {
            printf("%.2f ", instPower[c][i].value);
        }
        printf("\n");
    }

    printf("\n");
}

void printLatency(const struct BrownoutStatsExtend *brownoutStatsExtend) {
    /* received latency */
    timespec recvLatency;
    recvLatency.tv_sec = brownoutStatsExtend[0].eventReceivedTime.tv_sec - \
                         brownoutStatsExtend[0].brownoutStats.triggered_time.tv_sec;

    signed long long temp = brownoutStatsExtend[0].eventReceivedTime.tv_usec * 1000;
    if (temp >= brownoutStatsExtend[0].brownoutStats.triggered_time.tv_nsec)
        recvLatency.tv_nsec = brownoutStatsExtend[0].eventReceivedTime.tv_usec * 1000 - \
                              brownoutStatsExtend[0].brownoutStats.triggered_time.tv_nsec;
    else
        recvLatency.tv_nsec = NSEC_PER_SEC - \
                              brownoutStatsExtend[0].brownoutStats.triggered_time.tv_nsec \
                              + brownoutStatsExtend[0].eventReceivedTime.tv_usec * 1000;

    /* dump latency */
    timespec dumpLatency;
    dumpLatency.tv_sec = brownoutStatsExtend[0].dumpTime.tv_sec - \
                         brownoutStatsExtend[0].eventReceivedTime.tv_sec;

    temp = brownoutStatsExtend[0].dumpTime.tv_usec;
    if (temp >= brownoutStatsExtend[0].eventReceivedTime.tv_usec)
        dumpLatency.tv_nsec = (brownoutStatsExtend[0].dumpTime.tv_usec - \
                                brownoutStatsExtend[0].eventReceivedTime.tv_usec) * 1000;
    else
        dumpLatency.tv_nsec = NSEC_PER_SEC - \
                              brownoutStatsExtend[0].eventReceivedTime.tv_usec * 1000 +	\
                              brownoutStatsExtend[0].dumpTime.tv_usec * 1000;

    /* total latency */
    timespec totalLatency;
    totalLatency.tv_sec = brownoutStatsExtend[0].dumpTime.tv_sec - \
                          brownoutStatsExtend[0].brownoutStats.triggered_time.tv_sec;
    temp = brownoutStatsExtend[0].dumpTime.tv_usec * 1000;
    if (temp >= brownoutStatsExtend[0].brownoutStats.triggered_time.tv_nsec)
        totalLatency.tv_nsec = brownoutStatsExtend[0].dumpTime.tv_usec * 1000 - \
            brownoutStatsExtend[0].brownoutStats.triggered_time.tv_nsec;
    else
        totalLatency.tv_nsec = NSEC_PER_SEC - \
                               brownoutStatsExtend[0].brownoutStats.triggered_time.tv_nsec + \
                               brownoutStatsExtend[0].dumpTime.tv_usec * 1000;

    printf("recvLatency %ld.%09ld\n", recvLatency.tv_sec, recvLatency.tv_nsec);
    printf("dumpLatency %ld.%09ld\n", dumpLatency.tv_sec, dumpLatency.tv_nsec);
    printf("totalLatency %ld.%09ld\n\n", totalLatency.tv_sec, totalLatency.tv_nsec);

}

void printBRStatsSummary(const struct BrownoutBinaryStatsConfig cfg,
                         const struct BrownoutStatsExtend *brownoutStatsExtend) {
    int mainPmicID = 0;
	mainPmicID = getMainPmicID(cfg.pmic[mainPmicID].PmicNamePath, cfg.pmic[mainPmicID].PmicName);
    int subPmicID = !mainPmicID;
    double mainLpfBitResolutions[METER_CHANNEL_MAX];
    double subLpfBitResolutions[METER_CHANNEL_MAX];
    char *mainLpfChannelNames[METER_CHANNEL_MAX];
    char *subLpfChannelNames[METER_CHANNEL_MAX];
    std::vector<odpm_instant_data> odpmData[2];

    /* print out the triggered_time in first dump */
    printUTC(brownoutStatsExtend[0].brownoutStats.triggered_time, "triggered_time");
    printf("\n");
    printf("triggered_idx: %d\n", brownoutStatsExtend[0].brownoutStats.triggered_idx);
    printLatency(brownoutStatsExtend);

    /* skip time invalid odpm instant data */
    for (int i = 0; i < DUMP_TIMES; i++) {
        for (int d = 0; d < DATA_LOGGING_LEN; d++) {
            if (brownoutStatsExtend[i].brownoutStats.main_odpm_instant_data[d].time.tv_sec != 0) {
                odpmData[mainPmicID].emplace_back(brownoutStatsExtend[i].brownoutStats.main_odpm_instant_data[d]);
            }
            if (brownoutStatsExtend[i].brownoutStats.sub_odpm_instant_data[d].time.tv_sec != 0) {
                odpmData[subPmicID].emplace_back(brownoutStatsExtend[i].brownoutStats.sub_odpm_instant_data[d]);
            }
        }
    }

    /* read odpm resolutions and channel names */
    readLPFPowerBitResolutions(cfg.pmic[mainPmicID].OdpmDir, mainLpfBitResolutions);
    readLPFPowerBitResolutions(cfg.pmic[subPmicID].OdpmDir, subLpfBitResolutions);
    readLPFChannelNames(cfg.pmic[mainPmicID].OdpmEnabledRailsPath, mainLpfChannelNames);
    readLPFChannelNames(cfg.pmic[subPmicID].OdpmEnabledRailsPath, subLpfChannelNames);

    printODPMChannelSummary(odpmData[mainPmicID], mainLpfBitResolutions, mainLpfChannelNames);
    printODPMChannelSummary(odpmData[subPmicID], subLpfBitResolutions, subLpfChannelNames);

    freeLpfChannelNames(mainLpfChannelNames);
    freeLpfChannelNames(subLpfChannelNames);
}

void printOdpmInstantData(struct odpm_instant_data odpmInstantData) {
    if (odpmInstantData.time.tv_sec == 0 &&
        odpmInstantData.time.tv_nsec == 0) {
        return;
    }
    printUTC(odpmInstantData.time, "");
    for (int i = 0; i < METER_CHANNEL_MAX; i++){
        printf("%d ", odpmInstantData.value[i]);
    }
    printf("\n");
}

void printBRStats(struct BrownoutStatsExtend *brownoutStatsExtend) {
    printUTC(brownoutStatsExtend->brownoutStats.triggered_time, "triggered_time");
    printf("\n");
    printf("triggered_idx: %d\n", brownoutStatsExtend->brownoutStats.triggered_idx);

    printf("main_odpm_instant_data: \n");
    for (int d = 0; d < DATA_LOGGING_LEN; d++) {
        printOdpmInstantData(brownoutStatsExtend->brownoutStats.main_odpm_instant_data[d]);
    }
    printf("sub_odpm_instant_data: \n");
    for (int d = 0; d < DATA_LOGGING_LEN; d++) {
        printOdpmInstantData(brownoutStatsExtend->brownoutStats.sub_odpm_instant_data[d]);
    }

    printf("\n");
    printf("fvp_stats:\n");
    printf("%s\n\n", brownoutStatsExtend->fvpStats);
    printf("pcie_modem:\n");
    printf("%s\n\n", brownoutStatsExtend->pcieModem);
    printf("pcie_wifi:\n");
    printf("%s\n\n", brownoutStatsExtend->pcieWifi);
    for (int i = 0; i < STATS_MAX_SIZE; i++) {
        if (strlen(brownoutStatsExtend->numericStats[i].name) > 0)
            printf("%s: %d\n", brownoutStatsExtend->numericStats[i].name,
                               brownoutStatsExtend->numericStats[i].value);
    }
    printUTC(brownoutStatsExtend->eventReceivedTime, "eventReceivedTime");
    printf("\n");
    printUTC(brownoutStatsExtend->dumpTime, "dumpTime");
    printf("\n");
    printf("eventIdx: %d\n", brownoutStatsExtend->eventIdx);
}

void dumpBRStats(const struct BrownoutBinaryStatsConfig cfg,
                 const char* logPath, const char *title) {
    struct BrownoutStatsExtend brownoutStatsExtend[DUMP_TIMES];
    size_t memSize = sizeof(struct BrownoutStatsExtend) * DUMP_TIMES;

    int fd = open(logPath, O_RDONLY);
    if (fd < 0) {
        printf("Failed to open %s\n", logPath);
        return;
    }

    size_t logFileSize = lseek(fd, 0, SEEK_END);
    if (memSize != logFileSize) {
        printf("Invalid log size!\n");
        printf("BrownoutStatsExtend size: %lu\n", memSize);
        printf("%s size: %lu\n", title, logFileSize);
        close(fd);
        return;
    }

    char *logFileAddr = (char *) mmap(NULL, logFileSize, PROT_READ, MAP_PRIVATE, fd, 0);
    close(fd);

    memcpy(&brownoutStatsExtend, logFileAddr, logFileSize);
    munmap(logFileAddr, logFileSize);

    printTitle(title);
    printBRStatsSummary(cfg, brownoutStatsExtend);

    printf("== RAW ==\n");
    for (int i = 0; i < DUMP_TIMES; i++) {
        printf("== Dump %d ==\n", i);
        printBRStats(&brownoutStatsExtend[i]);
        printf("=============\n\n");
    }
}

void dumpMitigationBinaryStats(const struct BrownoutBinaryStatsConfig cfg) {
    if (access(cfg.StoringPath, F_OK) != 0) {
        printf("Failed to access %s\n", cfg.StoringPath);
    } else {
        dumpBRStats(cfg, cfg.StoringPath, cfg.ThismealFileName);
    }
    if (access(cfg.BackupPath, F_OK) != 0) {
        printf("Failed to access %s\n", cfg.StoringPath);
    } else {
        dumpBRStats(cfg, cfg.BackupPath, cfg.LastmealFileName);
    }
}

void dumpMitigationStats() {
    int ret;
    const char *directory = "/sys/devices/virtual/pmic/mitigation/last_triggered_count/";
    const char *capacityDirectory = "/sys/devices/virtual/pmic/mitigation/last_triggered_capacity/";
    const char *timestampDirectory =
            "/sys/devices/virtual/pmic/mitigation/last_triggered_timestamp/";
    const char *voltageDirectory = "/sys/devices/virtual/pmic/mitigation/last_triggered_voltage/";
    const char *capacitySuffix = "_cap";
    const char *timeSuffix = "_time";
    const char *voltageSuffix = "_volt";
    const char *countSuffix = "_count";
    const char *title = "Mitigation Stats";

    std::vector<std::string> files;
    std::string content;
    std::string fileLocation;
    std::string source;
    std::string subModuleName;
    int count;
    int soc;
    int time;
    int voltage;

    ret = getFilesInDir(directory, &files);
    if (ret < 0)
        return;

    printTitle(title);
    printf("Source\t\tCount\tSOC\tTime\tVoltage\n");

    for (auto &file : files) {
        fileLocation = std::string(directory) + std::string(file);
        if (!android::base::ReadFileToString(fileLocation, &content)) {
            continue;
        }

        ret = atoi(android::base::Trim(content).c_str());
        if (ret == -1)
            continue;
        count = ret;

        subModuleName = std::string(file);
        subModuleName.erase(subModuleName.find(countSuffix), strlen(countSuffix));

        fileLocation = std::string(capacityDirectory) + std::string(subModuleName) +
                std::string(capacitySuffix);
        if (!android::base::ReadFileToString(fileLocation, &content)) {
            continue;
        }
        ret = atoi(android::base::Trim(content).c_str());
        if (ret == -1)
            continue;
        soc = ret;

        fileLocation = std::string(timestampDirectory) + std::string(subModuleName) +
                std::string(timeSuffix);
        if (!android::base::ReadFileToString(fileLocation, &content)) {
            continue;
        }
        ret = atoi(android::base::Trim(content).c_str());
        if (ret == -1)
            continue;
        time = ret;

        fileLocation = std::string(voltageDirectory) + std::string(subModuleName) +
                std::string(voltageSuffix);
        if (!android::base::ReadFileToString(fileLocation, &content)) {
            continue;
        }
        ret = atoi(android::base::Trim(content).c_str());
        if (ret == -1)
            continue;
        voltage = ret;
        printf("%s \t%i\t%i\t%i\t%i\n", subModuleName.c_str(), count, soc, time, voltage);
    }
}

void dumpMitigationDirs() {
    const int paramCount = 4;
    const char *titles[] = {
            "Clock Divider Ratio",
            "Clock Stats",
            "Triggered Level",
            "Instruction",
    };
    const char *directories[] = {
            "/sys/devices/virtual/pmic/mitigation/clock_ratio/",
            "/sys/devices/virtual/pmic/mitigation/clock_stats/",
            "/sys/devices/virtual/pmic/mitigation/triggered_lvl/",
            "/sys/devices/virtual/pmic/mitigation/instruction/",
    };
    const char *paramSuffix[] = {"_ratio", "_stats", "_lvl", ""};
    const char *titleRowVal[] = {
            "Source\t\tRatio",
            "Source\t\tStats",
            "Source\t\tLevel",
            "",
    };
    const int eraseCnt[] = {6, 6, 4, 0};
    const bool useTitleRow[] = {true, true, true, false};

    std::vector<std::string> files;
    std::string content;
    std::string fileLocation;
    std::string source;
    std::string subModuleName;
    std::string readout;

    for (int i = 0; i < paramCount; i++) {
        printTitle(titles[i]);
        if (useTitleRow[i]) {
            printf("%s\n", titleRowVal[i]);
        }

        getFilesInDir(directories[i], &files);

        for (auto &file : files) {
            fileLocation = std::string(directories[i]) + std::string(file);
            if (!android::base::ReadFileToString(fileLocation, &content)) {
                continue;
            }

            readout = android::base::Trim(content);

            subModuleName = std::string(file);
            subModuleName.erase(subModuleName.find(paramSuffix[i]), eraseCnt[i]);

            if (useTitleRow[i]) {
                printf("%s \t%s\n", subModuleName.c_str(), readout.c_str());
            } else {
                printf("%s=%s\n", subModuleName.c_str(), readout.c_str());
            }
        }
    }
}

void dumpIrqDurationCounts() {
    const char *title = "IRQ Duration Counts";
    const char *colNames = "Source\t\t\t\tlt_5ms_cnt\tbt_5ms_to_10ms_cnt\tgt_10ms_cnt\tCode"
            "\tCurrent Threshold (uA)\tCurrent Reading (uA)\n";
    const int nonOdpmChannelCnt = 12;
    const int odpmChCnt = 12;

    enum Duration {
        LT_5MS,
        BT_5MS_10MS,
        GT_10MS,
        DUR_MAX,
    };
    const char *irqDurDirectories[] = {
            "/sys/devices/virtual/pmic/mitigation/irq_dur_cnt/less_than_5ms_count",
            "/sys/devices/virtual/pmic/mitigation/irq_dur_cnt/between_5ms_to_10ms_count",
            "/sys/devices/virtual/pmic/mitigation/irq_dur_cnt/greater_than_10ms_count",
    };

    enum PowerWarn {
        MAIN,
        SUB,
        PWRWARN_MAX,
    };
    const char *pwrwarnDirectories[] = {
            "/sys/devices/virtual/pmic/mitigation/main_pwrwarn/",
            "/sys/devices/virtual/pmic/mitigation/sub_pwrwarn/",
    };

    const char *lpfCurrentDirs[] = {
            "/sys/devices/platform/acpm_mfd_bus@15500000/i2c-7/7-001f/s2mpg14-meter/"
                    "s2mpg14-odpm/iio:device1/lpf_current",
            "/sys/devices/platform/acpm_mfd_bus@15510000/i2c-8/8-002f/s2mpg15-meter/"
                    "s2mpg15-odpm/iio:device0/lpf_current",
    };

    bool titlesInitialized = false;

    std::vector<std::string> channelNames;
    std::vector<std::string> channelData[DUR_MAX];
    std::vector<std::string> pwrwarnThreshold[PWRWARN_MAX];
    std::vector<std::string> pwrwarnCode[PWRWARN_MAX];
    std::vector<std::string> lpfCurrentVals[PWRWARN_MAX];
    std::vector<std::string> files;

    std::string content;
    std::string token;
    std::string tokenCh;
    std::string fileLocation;

    for (int i = 0; i < DUR_MAX; i++) {
        if (!android::base::ReadFileToString(irqDurDirectories[i], &content)) {
            return;
        }

        std::istringstream tokenStream(content);

        while (std::getline(tokenStream, token, '\n')) {
            if (!titlesInitialized) {
                tokenCh = token;
                tokenCh.erase(tokenCh.find(':'), tokenCh.length());
                channelNames.push_back(tokenCh);
            }

            // there is a space after the ':' which needs to be removed
            token.erase(0, token.find(':') + 1);
            channelData[i].push_back(token);

        }
        if (!titlesInitialized)
            titlesInitialized = true;
    }

    for (int i = 0; i < PWRWARN_MAX; i++) {
        getFilesInDir(pwrwarnDirectories[i], &files);

        for (auto &file : files) {
            fileLocation = std::string(pwrwarnDirectories[i]) + std::string(file);
            if (!android::base::ReadFileToString(fileLocation, &content)) {
                continue;
            }

            std::string readout;

            readout = android::base::Trim(content);

            std::string readoutThreshold = readout;
            readoutThreshold.erase(0, readoutThreshold.find('=') + 1);

            std::string readoutCode = readout;
            readoutCode.erase(readoutCode.find('='), readoutCode.length());

            pwrwarnThreshold[i].push_back(readoutThreshold);
            pwrwarnCode[i].push_back(readoutCode);
        }
    }

    for (int i = 0; i < PWRWARN_MAX; i++) {
        if (!android::base::ReadFileToString(lpfCurrentDirs[i], &content)) {
            continue;
        }

        std::istringstream tokenStream(content);

        bool first = true;
        while (std::getline(tokenStream, token, '\n')) {
            token.erase(0, token.find(' '));
            if (first) {
                first = false;
                continue;
            }
            lpfCurrentVals[i].push_back(token);
        }
    }

    printTitle(title);
    printf("%s", colNames);

    for (uint i = 0; i < channelNames.size(); i++) {
        std::string code = "";
        std::string threshold = "";
        std::string current = "";
        std::string ltDataMsg = "";
        std::string btDataMsg = "";
        std::string gtDataMsg = "";
        int pmicSel = 0;
        int offset = 0;
        std::string channelNameSuffix = "      \t";
        if (i >= nonOdpmChannelCnt) {
            offset = nonOdpmChannelCnt;
            if (i >= (odpmChCnt + nonOdpmChannelCnt)) {
                pmicSel = 1;
                offset = odpmChCnt + nonOdpmChannelCnt;
            }
            channelNameSuffix = "";

            code = pwrwarnCode[pmicSel][i - offset];
            threshold = pwrwarnThreshold[pmicSel][i - offset];
            current = lpfCurrentVals[pmicSel][i - offset];
        }

        if (i < channelData[0].size())
            ltDataMsg = channelData[0][i];

        if (i < channelData[1].size())
            btDataMsg = channelData[1][i];

        if (i < channelData[2].size())
            gtDataMsg = channelData[2][i];

        std::string adjustedChannelName = channelNames[i] + channelNameSuffix;
        printf("%s     \t%s\t\t%s\t\t\t%s\t\t%s    \t%s       \t\t%s\n",
                adjustedChannelName.c_str(),
                ltDataMsg.c_str(),
                btDataMsg.c_str(),
                gtDataMsg.c_str(),
                code.c_str(),
                threshold.c_str(),
                current.c_str());
    }
}

int main() {
    const struct BrownoutBinaryStatsConfig cfg = {
        .StoringPath = "/data/vendor/mitigation/thismeal.bin",
        .BackupPath = "/data/vendor/mitigation/lastmeal.bin",
        .ThismealFileName = "thismeal.bin",
        .LastmealFileName = "lastmeal.bin",
        .pmic = {
                    /* Main Pmic */
                    {
                        .OdpmDir = "/sys/bus/iio/devices/iio:device0",
                        .OdpmEnabledRailsPath = "/sys/bus/iio/devices/iio:device0/enabled_rails",
                        .PmicNamePath = "/sys/bus/iio/devices/iio:device0/name",
                        .PmicName = "s2mpg14-odpm\n",
                    },
                    /* Sub Pmic */
                    {
                        .OdpmDir = "/sys/bus/iio/devices/iio:device1",
                        .OdpmEnabledRailsPath = "/sys/bus/iio/devices/iio:device1/enabled_rails",
                        .PmicNamePath = "/sys/bus/iio/devices/iio:device1/name",
                        .PmicName = "s2mpg15-odpm\n",
                    },
        },
    };

    dumpPowerStatsTimes();
    dumpAcpmStats();
    dumpPowerSupplyStats();
    dumpMaxFg();
    dumpPowerSupplyDock();
    dumpLogBufferTcpm();
    dumpTcpc();
    dumpPdEngine();
    dumpBatteryHealth();
    dumpBatteryDefend();
    dumpChgUserDebug();
    dumpBatteryEeprom();
    dumpChargerStats();
    dumpWlcLogs();
    dumpGvoteables();
    dumpMitigation();
    dumpMitigationStats();
    dumpMitigationDirs();
    dumpIrqDurationCounts();
    dumpMitigationBinaryStats(cfg);
}

