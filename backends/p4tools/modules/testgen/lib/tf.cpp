#include "backends/p4tools/modules/testgen/lib/tf.h"

#include <boost/none.hpp>

#include "backends/p4tools/common/lib/util.h"
#include "lib/timer.h"

namespace P4Tools {

namespace P4Testgen {

TF::TF(cstring testName, boost::optional<unsigned int> seed = boost::none)
    : testName(testName), seed(seed) {}

void TF::printPerformanceReport() const {
    // Do not emit a report if performance logging is not enabled.
    if (!Log::fileLogLevelIsAtLeast("performance", 4)) {
        return;
    }
    printFeature("performance", 4, "============ Timers ============");
    inja::json dataJson;
    inja::json timerList = inja::json::array();
    for (const auto &c : Util::getTimers()) {
        inja::json timerData;
        timerData["time"] = c.milliseconds;
        if (c.timerName.empty()) {
            printFeature("performance", 4, "Total: %i ms", c.milliseconds);
            timerData["pct"] = "100";
            timerData["name"] = "total";
        } else {
            timerData["pct"] = c.relativeToParent * 100;
            printFeature("performance", 4, "%s: %i ms (%0.2f %% of parent)", c.timerName,
                         c.milliseconds, c.relativeToParent * 100);
            auto prunedName = c.timerName;
            prunedName.erase(remove_if(prunedName.begin(), prunedName.end(), isspace),
                             prunedName.end());
            timerData["name"] = prunedName;
        }
        timerList.emplace_back(timerData);
    }
    dataJson["timers"] = timerList;
    static const std::string TEST_CASE(R"""(Timer,Total Time,Percentage
## for timer in timers
{{timer.name}},{{timer.time}},{{timer.pct}}
## endfor
)""");

    auto perfFile = std::ofstream(testName + "_perf.csv");
    inja::render_to(perfFile, TEST_CASE, dataJson);
    perfFile.flush();
}

}  // namespace P4Testgen

}  // namespace P4Tools
