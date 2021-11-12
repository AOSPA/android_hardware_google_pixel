/*
 * Copyright (C) 2021 The Android Open Source Project
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

#define LOG_TAG "powerhal-libperfmgr"
#define ATRACE_TAG (ATRACE_TAG_POWER | ATRACE_TAG_HAL)

#include "CpuLoadReader.h"

#include <android-base/logging.h>
#include <inttypes.h>
#include <utils/Trace.h>

#include <fstream>
#include <sstream>
#include <string>

namespace aidl {
namespace google {
namespace hardware {
namespace power {
namespace impl {
namespace pixel {

void CpuLoadReader::init() {
    mPreviousCpuTimes = readCpuTimes();
}

bool CpuLoadReader::getRecentCpuLoads(std::vector<CpuLoad> *result) {
    ATRACE_CALL();
    if (result == nullptr) {
        LOG(ERROR) << "Got nullptr output in getRecentCpuLoads";
        return false;
    }
    std::map<uint32_t, CpuTime> cpuTimes = readCpuTimes();
    if (cpuTimes.empty()) {
        LOG(ERROR) << "Failed to find any CPU times";
        return false;
    }
    for (const auto &[cpuId, cpuTime] : cpuTimes) {
        const auto previousCpuTime = mPreviousCpuTimes.find(cpuId);
        if (previousCpuTime == mPreviousCpuTimes.end()) {
            LOG(ERROR) << "Couldn't find CPU " << cpuId << " in previous CPU times";
            return false;
        }
        const auto recentIdleTimeMs = cpuTime.idleTimeMs - previousCpuTime->second.idleTimeMs;
        const auto recentTotalTimeMs = cpuTime.totalTimeMs - previousCpuTime->second.totalTimeMs;
        if (recentIdleTimeMs > recentTotalTimeMs) {
            LOG(ERROR) << "Found more recent idle time than total time: idle=" << recentIdleTimeMs
                       << ", total=" << recentTotalTimeMs;
            return false;
        }
        const double idleTimeFraction = static_cast<double>(recentIdleTimeMs) / (recentTotalTimeMs);
        result->push_back({.cpuId = cpuId, .idleTimeFraction = idleTimeFraction});
    }
    mPreviousCpuTimes = cpuTimes;
    return true;
}

std::map<uint32_t, CpuTime> CpuLoadReader::getPreviousCpuTimes() const {
    return mPreviousCpuTimes;
}

std::map<uint32_t, CpuTime> CpuLoadReader::readCpuTimes() {
    ATRACE_CALL();
    std::map<uint32_t, CpuTime> cpuTimes;

    std::unique_ptr<std::istream> file = mFilesystem->readFileStream("/proc/stat");
    std::string line;
    ATRACE_BEGIN("loop");
    while (std::getline(*file, line)) {
        ATRACE_NAME("parse");
        uint32_t cpuId;
        // Times reported when the CPU is active.
        uint64_t user, nice, system, irq, softIrq, steal, guest, guestNice;
        // Times reported when the CPU is idle.
        uint64_t idle, ioWait;
        // Order & values taken from `fs/proc/stat.c`.
        if (std::sscanf(line.c_str(),
                        "cpu%d %" PRIu64 " %" PRIu64 " %" PRIu64 " %" PRIu64 " %" PRIu64 " %" PRIu64
                        " %" PRIu64 " %" PRIu64 " %" PRIu64 " %" PRIu64 " ",
                        &cpuId, &user, &nice, &system, &idle, &ioWait, &irq, &softIrq, &steal,
                        &guest, &guestNice) != 11) {
            continue;
        }
        uint64_t idleTimeJiffies = idle + ioWait;
        uint64_t totalTimeJiffies =
                user + nice + system + irq + softIrq + steal + guest + guestNice + idleTimeJiffies;
        cpuTimes[cpuId] = {.idleTimeMs = jiffiesToMs(idleTimeJiffies),
                           .totalTimeMs = jiffiesToMs(totalTimeJiffies)};
    }
    ATRACE_END();
    return cpuTimes;
}

uint64_t CpuLoadReader::jiffiesToMs(uint64_t jiffies) {
    return (jiffies * 1000) / sysconf(_SC_CLK_TCK);
}

}  // namespace pixel
}  // namespace impl
}  // namespace power
}  // namespace hardware
}  // namespace google
}  // namespace aidl
