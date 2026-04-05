#include "backends/p4tools/modules/rtsmith/core/config.h"

#include <limits>

#include "lib/error.h"

namespace P4::P4Tools::RtSmith {

void FuzzerConfig::setMaxEntryGenCnt(const int numEntries) {
    if (numEntries < 0) {
        error(
            "ControlPlaneSmith: The maximum number of entries to generate must be a non-negative "
            "integer.");
        return;
    }
    maxEntryGenCnt = numEntries;
}

void FuzzerConfig::setMaxAttempts(const int numAttempts) {
    if (numAttempts <= 0) {
        error("ControlPlaneSmith: The number of attempts must be a positive integer.");
        return;
    }
    maxAttempts = numAttempts;
}

void FuzzerConfig::setMaxTables(const int numTables) {
    if (numTables < 0) {
        error("ControlPlaneSmith: The maximum number of tables must be a non-negative integer.");
        return;
    }
    maxTables = numTables;
}

void FuzzerConfig::setThresholdForDeletion(const uint64_t threshold) {
    if (threshold > 100) {
        error("RtSmith: Deletion threshold must be between 0 and 100");
        return;
    }
    thresholdForDeletion = threshold;
}

void FuzzerConfig::setTablesToSkip(const std::vector<std::string> &tables) {
    tablesToSkip = tables;
}

void FuzzerConfig::setMaxUpdateCount(const size_t count) { maxUpdateCount = count; }

void FuzzerConfig::setMaxUpdateTimeInMicroseconds(const uint64_t micros) {
    if (micros == 0 || micros > static_cast<uint64_t>(std::numeric_limits<int64_t>::max())) {
        error(
            "ControlPlaneSmith: The maximum wait time must fit in a positive signed 64-bit "
            "integer.");
        return;
    }
    maxUpdateTimeInMicroseconds = micros;
}
void FuzzerConfig::setMinUpdateTimeInMicroseconds(const uint64_t micros) {
    if (micros == 0 || micros > static_cast<uint64_t>(std::numeric_limits<int64_t>::max())) {
        error(
            "ControlPlaneSmith: The minimum wait time must fit in a positive signed 64-bit "
            "integer.");
        return;
    }
    minUpdateTimeInMicroseconds = micros;
}

}  // namespace P4::P4Tools::RtSmith
