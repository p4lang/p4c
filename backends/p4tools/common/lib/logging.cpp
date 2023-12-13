#include "backends/p4tools/common/lib/logging.h"

#include <fstream>
#include <unordered_map>

#include "lib/log.h"
#include "lib/timer.h"

namespace P4Tools {

void enableInformationLogging() { ::Log::addDebugSpec("test_info:4"); }

void enablePerformanceLogging() { ::Log::addDebugSpec("performance:4"); }

void printPerformanceReport(const std::optional<std::filesystem::path> &basePath) {
    // Do not emit a report if performance logging is not enabled.
    if (!Log::fileLogLevelIsAtLeast("performance", 4)) {
        return;
    }
    printFeature("performance", 4, "============ Timers ============");
    using TimerData = std::unordered_map<std::string, std::string>;
    std::vector<TimerData> timerList;
    for (const auto &c : Util::getTimers()) {
        TimerData timerData;
        timerData["time"] = std::to_string(c.milliseconds);
        if (c.timerName.empty()) {
            printFeature("performance", 4, "Total: %i ms", c.milliseconds);
            timerData["pct"] = "100";
            timerData["name"] = "total";
        } else {
            timerData["pct"] = std::to_string(c.relativeToParent * 100);
            printFeature("performance", 4, "%s: %i ms (%0.2f %% of parent)", c.timerName,
                         c.milliseconds, c.relativeToParent * 100);
            auto prunedName = c.timerName;
            prunedName.erase(remove_if(prunedName.begin(), prunedName.end(), isspace),
                             prunedName.end());
            timerData["name"] = prunedName;
        }
        timerList.emplace_back(timerData);
    }
    // Write the report to the file, if one was provided.
    if (basePath.has_value()) {
        auto perfFilePath = basePath.value();
        perfFilePath.concat("_perf");
        perfFilePath.replace_extension(".csv");
        auto perfFile = std::ofstream(perfFilePath, std::ios::out | std::ios::app);
        if (!perfFile.is_open()) {
            ::error("Failed to open the performance report file %1%", perfFilePath.c_str());
            return;
        }

        perfFile << "Timer,Total Time,Percentage\n";
        for (const auto &timerData : timerList) {
            perfFile << timerData.at("name") << "," << timerData.at("time") << ","
                     << timerData.at("pct") << "\n";
        }
        perfFile.close();
    }
}
}  // namespace P4Tools
