#ifndef BACKENDS_P4TOOLS_MODULES_RTSMITH_CORE_CONFIG_H_
#define BACKENDS_P4TOOLS_MODULES_RTSMITH_CORE_CONFIG_H_

#include <filesystem>
#include <stdexcept>
#include <vector>

#include "backends/p4tools/common/lib/util.h"

namespace P4::P4Tools::RtSmith {

class FuzzerConfig {
 private:
    /// The maximum number of entries we are trying to generate for a table.
    int maxEntryGenCnt = 5;
    // The maximum attempts we are trying to generate an entry.
    int maxAttempts = 100;
    /// The maximum number of tables.
    int maxTables = 5;
    /// The string representations of tables to skip.
    std::vector<std::string> tablesToSkip;
    /// Threshold for deletion.
    uint64_t thresholdForDeletion = 30;
    /// The maximum number of updates.
    size_t maxUpdateCount = 10;
    /// The maximum time (in microseconds) for the update.
    uint64_t maxUpdateTimeInMicroseconds = 100000;
    /// The minimum time (in microseconds) for the update.
    uint64_t minUpdateTimeInMicroseconds = 50000;

 public:
    // Default constructor.
    FuzzerConfig() = default;

    // Default destructor.
    virtual ~FuzzerConfig() = default;

    /// Getters to access the fuzzer configurations.
    [[nodiscard]] int getMaxEntryGenCnt() const { return maxEntryGenCnt; }
    [[nodiscard]] int getMaxAttempts() const { return maxAttempts; }
    [[nodiscard]] int getMaxTables() const { return maxTables; }
    [[nodiscard]] const std::vector<std::string> &getTablesToSkip() const { return tablesToSkip; }
    [[nodiscard]] uint64_t getThresholdForDeletion() const { return thresholdForDeletion; }
    [[nodiscard]] size_t getMaxUpdateCount() const { return maxUpdateCount; }
    [[nodiscard]] uint64_t getMaxUpdateTimeInMicroseconds() const {
        return maxUpdateTimeInMicroseconds;
    }
    [[nodiscard]] uint64_t getMinUpdateTimeInMicroseconds() const {
        return minUpdateTimeInMicroseconds;
    }

    /// Setters to modify/override the fuzzer configurations.
    void setMaxEntryGenCnt(const int numEntries);
    void setMaxAttempts(const int numAttempts);
    void setMaxTables(const int numTables);
    void setTablesToSkip(const std::vector<std::string> &tables);
    void setThresholdForDeletion(const uint64_t threshold);
    void setMaxUpdateCount(const size_t count);
    void setMaxUpdateTimeInMicroseconds(const uint64_t micros);
    void setMinUpdateTimeInMicroseconds(const uint64_t micros);
};

}  // namespace P4::P4Tools::RtSmith

#endif /* BACKENDS_P4TOOLS_MODULES_RTSMITH_CORE_CONFIG_H_ */
