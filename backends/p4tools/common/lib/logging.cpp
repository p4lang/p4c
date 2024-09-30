#include "backends/p4tools/common/lib/logging.h"

#include <fstream>
#include <unordered_map>

#include "lib/error.h"
#include "lib/log.h"
#include "lib/timer.h"

namespace P4::P4Tools {

void enableInformationLogging() { ::P4::Log::addDebugSpec("tools_info:4"); }

void enablePerformanceLogging() { ::P4::Log::addDebugSpec("tools_performance:4"); }

void printPerformanceReport(const std::optional<std::filesystem::path> &basePath) {
    // Do not emit a report if performance logging is not enabled.
    if (!Log::fileLogLevelIsAtLeast("tools_performance", 4)) {
        return;
    }
    printFeature("tools_performance", 4, "============ Timers ============");
    using TimerData = std::unordered_map<std::string, std::string>;
    std::vector<TimerData> timerList;
    for (const auto &c : Util::getTimers()) {
        TimerData timerData;
        timerData["time"] = std::to_string(c.milliseconds);
        if (c.timerName.empty()) {
            printFeature("tools_performance", 4, "Total: %i ms", c.milliseconds);
            timerData["pct"] = "100";
            timerData["name"] = "total";
            timerData["invocations"] = std::to_string(c.invocations);
        } else {
            timerData["pct"] = std::to_string(c.relativeToParent * 100);
            auto timePerInvocation =
                static_cast<float>(c.milliseconds) / static_cast<float>(c.invocations);
            printFeature("tools_performance", 4,
                         "%s: %i ms (%i ms per invocation, %0.2f %% of parent)", c.timerName,
                         c.milliseconds, timePerInvocation, c.relativeToParent * 100);
            auto prunedName = c.timerName;
            prunedName.erase(remove_if(prunedName.begin(), prunedName.end(), isspace),
                             prunedName.end());
            timerData["name"] = prunedName;
            timerData["invocations"] = std::to_string(c.invocations);
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
            ::P4::error("Failed to open the performance report file %1%", perfFilePath.c_str());
            return;
        }

        perfFile << "Timer,Total Time,Percentage\n";
        for (const auto &timerData : timerList) {
            perfFile << timerData.at("name") << "," << timerData.at("time") << ","
                     << timerData.at("pct") << "," << timerData.at("invocations") << "\n";
        }
        perfFile.close();
    }
}
}  // namespace P4::P4Tools
