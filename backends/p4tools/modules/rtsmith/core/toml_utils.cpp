#include "backends/p4tools/modules/rtsmith/core/toml_utils.h"

#include "lib/error.h"

namespace P4::P4Tools::RtSmith {
namespace {

template <typename T, typename Setter>
void overrideValue(const toml::table &config, const char *name, FuzzerConfig &fuzzer,
                   Setter setter) {
    if (!config.contains(name)) return;
    auto value = TOMLUtils::getAndCastTOMLNode<T>(config, name);
    if (!value) {
        error("RtSmith: Invalid type or out-of-range value for %1%", name);
        return;
    }
    (fuzzer.*setter)(*value);
}

void overrideConfig(FuzzerConfig &fuzzer, const toml::table &config) {
    overrideValue<int>(config, "maxEntryGenCnt", fuzzer, &FuzzerConfig::setMaxEntryGenCnt);
    overrideValue<int>(config, "maxAttempts", fuzzer, &FuzzerConfig::setMaxAttempts);
    overrideValue<int>(config, "maxTables", fuzzer, &FuzzerConfig::setMaxTables);
    overrideValue<std::vector<std::string>>(config, "tablesToSkip", fuzzer,
                                            &FuzzerConfig::setTablesToSkip);
    overrideValue<uint64_t>(config, "thresholdForDeletion", fuzzer,
                            &FuzzerConfig::setThresholdForDeletion);
    overrideValue<size_t>(config, "maxUpdateCount", fuzzer, &FuzzerConfig::setMaxUpdateCount);
    overrideValue<uint64_t>(config, "maxUpdateTimeInMicroseconds", fuzzer,
                            &FuzzerConfig::setMaxUpdateTimeInMicroseconds);
    overrideValue<uint64_t>(config, "minUpdateTimeInMicroseconds", fuzzer,
                            &FuzzerConfig::setMinUpdateTimeInMicroseconds);
    if (fuzzer.getMinUpdateTimeInMicroseconds() > fuzzer.getMaxUpdateTimeInMicroseconds()) {
        error("RtSmith: Minimum update time must not exceed maximum update time");
    }
}

}  // namespace

void TOMLUtils::overrideFuzzerConfigs(FuzzerConfig &fuzzerConfig, std::filesystem::path path) {
    try {
        overrideConfig(fuzzerConfig, toml::parse_file(path.string()));
    } catch (const toml::parse_error &e) {
        error("RtSmith: Failed to parse fuzzer configuration file: %1%", e.what());
    }
}

void TOMLUtils::overrideFuzzerConfigsInString(FuzzerConfig &fuzzerConfig,
                                              std::string configInString) {
    try {
        overrideConfig(fuzzerConfig, toml::parse(configInString));
    } catch (const toml::parse_error &e) {
        error("RtSmith: Failed to parse fuzzer configuration string: %1%", e.what());
    }
}

}  // namespace P4::P4Tools::RtSmith
