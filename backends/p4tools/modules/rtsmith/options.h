#ifndef BACKENDS_P4TOOLS_MODULES_RTSMITH_OPTIONS_H_
#define BACKENDS_P4TOOLS_MODULES_RTSMITH_OPTIONS_H_

#include <filesystem>
#include <optional>

#include "backends/p4tools/common/options.h"

namespace P4::P4Tools::RtSmith {

/// Encapsulates and processes command-line options for P4RtSmith.
class RtSmithOptions : public AbstractP4cToolOptions {
 public:
    RtSmithOptions();

    /// @returns the singleton instance of this class.
    static RtSmithOptions &get();

    const char *getIncludePath();

    /// @returns if the options are valid.
    [[nodiscard]] bool validateOptions() const override;

    /// @returns true when the --print-to-stdout option has been set.
    [[nodiscard]] bool printToStdout() const;

    /// @returns the path set with --output-dir.
    [[nodiscard]] std::filesystem::path outputDir() const;

    /// @returns the path set with --generate-config.
    [[nodiscard]] std::optional<std::string> configName() const;

    /// @returns the path to the user p4 info file set by the --user-p4info option.
    [[nodiscard]] std::optional<std::filesystem::path> userP4Info() const;

    /// @returns the path set with --generate-p4info.
    [[nodiscard]] std::optional<std::filesystem::path> p4InfoFilePath() const;

    /// @returns the control plane API to use.
    [[nodiscard]] std::string_view controlPlaneApi() const;

    /// @returns the path to the TOML file that would be used to set the fuzzer configurations.
    [[nodiscard]] std::optional<std::filesystem::path> fuzzerConfigPath() const;

    /// @returns the fuzzer configurations set in the string representation of configurations of
    /// format TOML.
    [[nodiscard]] std::optional<std::string> fuzzerConfigString() const;

    /// @brief Set the path to the TOML file.
    void setFuzzerConfigPath(std::string arg);

    /// @brief Set the string representation of the fuzzer configurations.
    void setFuzzerConfigString(std::string arg);

 protected:
    // Write the generated config to the specified file.
    std::optional<std::string> _configName = std::nullopt;

    /// The path to the output file of the config file.
    std::filesystem::path _outputDir;

    /// Whether to write the generated config to a file or to stdout.
    bool _printToStdout = false;

    // Use a user-supplied P4Info file instead of generating one.
    std::optional<std::filesystem::path> _userP4Info = std::nullopt;

    // Write the P4Runtime control plane API description to the specified file.
    std::optional<std::filesystem::path> _p4InfoFilePath;

    // The control plane API to use. Defaults to P4Runtime.
    std::string _controlPlaneApi = "P4RUNTIME";

    // The path to the TOML file that would be used to set the fuzzer configurations.
    std::optional<std::filesystem::path> _fuzzerConfigPath = std::nullopt;

    /// The string representation of the fuzzer configurations.
    std::optional<std::string> _fuzzerConfigString = std::nullopt;
};

}  // namespace P4::P4Tools::RtSmith

#endif /* BACKENDS_P4TOOLS_MODULES_RTSMITH_OPTIONS_H_ */
