#include "backends/p4tools/modules/rtsmith/core/config.h"
#include "backends/p4tools/modules/rtsmith/core/toml_utils.h"
#include "backends/p4tools/modules/rtsmith/test/core/rtsmith_test.h"

namespace P4::P4Tools::Test {

namespace {

using namespace P4::literals;

class TOMLFuzzerConfigurationTest : public RtSmithTest {};

// Test of overriding fuzzer configurations via configurations specified in a TOML file.
TEST_F(TOMLFuzzerConfigurationTest, OverrideFuzzerConfigurationsViaTOMLFile) {
    auto source = generateTestProgram(R"(
    // Drop the packet.
    action acl_drop() {
        mark_to_drop(sm);
    }

    table drop_table {
        key = {
            hdr.eth_hdr.dst_addr : ternary @name("dst_eth");
        }
        actions = {
            acl_drop();
            @defaultonly NoAction();
        }
    }

    apply {
        if (hdr.eth_hdr.isValid()) {
            drop_table.apply();
        }
    })");
    auto autoContext = SetUp("bmv2", "v1model");
    auto &rtSmithOptions = RtSmith::RtSmithOptions::get();
    rtSmithOptions.target = "bmv2"_cs;
    rtSmithOptions.arch = "v1model"_cs;

    std::string configFilePath = "test/configuration.toml";
    rtSmithOptions.setFuzzerConfigPath(configFilePath);
    // Check if the fuzzer configuration path is set (and is set correctly).
    ASSERT_TRUE(rtSmithOptions.fuzzerConfigPath().has_value());
    ASSERT_EQ(rtSmithOptions.fuzzerConfigPath().value(), configFilePath);

    auto rtSmithResultOpt = P4::P4Tools::RtSmith::RtSmith::generateConfig(source, rtSmithOptions);
    // Check if the `RtSmithResult` object is generated successfully.
    ASSERT_TRUE(rtSmithResultOpt.has_value());

    auto compilerResult =
        P4::P4Tools::RtSmith::RtSmith::generateCompilerResult(source, rtSmithOptions);
    // Check if the `compilerResult` object is generated successfully.
    ASSERT_TRUE(compilerResult.has_value());

    const auto *programInfo = P4::P4Tools::RtSmith::RtSmithTarget::produceProgramInfo(
        compilerResult.value(), rtSmithOptions);
    // Check if the `programInfo` object is generated successfully.
    ASSERT_TRUE(programInfo != nullptr);

    // Parse the fuzzer configurations from the TOML file.
    toml::parse_result tomlConfig;
    try {
        tomlConfig = toml::parse_file(configFilePath);
    } catch (const toml::parse_error &e) {
        FAIL() << "ControlPlaneSmith: Failed to parse fuzzer configuration string: " << e.what();
    }

    // Check if the fuzzer configurations are overridden correctly.
    const P4::P4Tools::RtSmith::FuzzerConfig &fuzzerConfig = programInfo->getFuzzerConfig();
    // Check if the node exists and can be casted to a pointer to a node representation of an
    // integer. If not, do nothing and leave the default configuration as is.
    if (const auto maxEntryGenCntValue = P4::P4Tools::RtSmith::TOMLUtils::getAndCastTOMLNode<int>(
            tomlConfig, "maxEntryGenCnt")) {
        ASSERT_EQ(fuzzerConfig.getMaxEntryGenCnt(), maxEntryGenCntValue.value());
    } else {
        // Generate a fatal failure (that "returns from the current function") if the
        // configuration is not an integer.
        FAIL() << "ControlPlaneSmith: The maximum number of entries to generate must be an "
                  "integer.";
    }

    if (const auto maxAttemptsValue =
            P4::P4Tools::RtSmith::TOMLUtils::getAndCastTOMLNode<int>(tomlConfig, "maxAttempts")) {
        ASSERT_EQ(fuzzerConfig.getMaxAttempts(), maxAttemptsValue.value());
    } else {
        FAIL() << "ControlPlaneSmith: The maximum number of attempts must be an integer.";
    }

    if (const auto maxTablesValue =
            P4::P4Tools::RtSmith::TOMLUtils::getAndCastTOMLNode<int>(tomlConfig, "maxTables")) {
        ASSERT_EQ(fuzzerConfig.getMaxTables(), maxTablesValue.value());
    } else {
        FAIL() << "ControlPlaneSmith: The maximum number of tables must be an integer.";
    }

    const std::vector<std::string> &tablesToSkip = fuzzerConfig.getTablesToSkip();
    if (const auto tablesToSkipValue =
            P4::P4Tools::RtSmith::TOMLUtils::getAndCastTOMLNode<std::vector<std::string>>(
                tomlConfig, "tablesToSkip")) {
        const std::vector<std::string> &tablesToSkipConfig = tablesToSkipValue.value();
        ASSERT_EQ(tablesToSkipConfig.size(), tablesToSkip.size());
        for (size_t i = 0; i < tablesToSkipConfig.size(); i++) {
            ASSERT_EQ(tablesToSkipConfig[i], tablesToSkip[i]);
        }
    } else {
        FAIL() << "ControlPlaneSmith: The tables to skip must be an array.";
    }

    if (const auto thresholdForDeletionValue =
            P4::P4Tools::RtSmith::TOMLUtils::getAndCastTOMLNode<uint64_t>(tomlConfig,
                                                                          "thresholdForDeletion")) {
        ASSERT_EQ(fuzzerConfig.getThresholdForDeletion(),
                  static_cast<uint64_t>(thresholdForDeletionValue.value()));
    } else {
        FAIL() << "ControlPlaneSmith: The threshold for deletion must be an integer.";
    }

    if (const auto maxUpdateCountValue =
            P4::P4Tools::RtSmith::TOMLUtils::getAndCastTOMLNode<size_t>(tomlConfig,
                                                                        "maxUpdateCount")) {
        ASSERT_EQ(fuzzerConfig.getMaxUpdateCount(),
                  static_cast<size_t>(maxUpdateCountValue.value()));
    } else {
        FAIL() << "ControlPlaneSmith: The maximum number of updates must be an integer.";
    }

    if (const auto maxUpdateTimeInMicrosecondsValue =
            P4::P4Tools::RtSmith::TOMLUtils::getAndCastTOMLNode<uint64_t>(
                tomlConfig, "maxUpdateTimeInMicroseconds")) {
        ASSERT_EQ(fuzzerConfig.getMaxUpdateTimeInMicroseconds(),
                  static_cast<uint64_t>(maxUpdateTimeInMicrosecondsValue.value()));
    } else {
        FAIL() << "ControlPlaneSmith: The maximum wait time must be an integer.";
    }
    if (const auto minUpdateTimeInMicrosecondsValue =
            P4::P4Tools::RtSmith::TOMLUtils::getAndCastTOMLNode<uint64_t>(
                tomlConfig, "minUpdateTimeInMicroseconds")) {
        ASSERT_EQ(fuzzerConfig.getMinUpdateTimeInMicroseconds(),
                  static_cast<uint64_t>(minUpdateTimeInMicrosecondsValue.value()));
    } else {
        FAIL() << "ControlPlaneSmith: The minimum wait time must be an integer.";
    }
}

// Test of overriding fuzzer configurations via the string representation of the configurations of
// format TOML.
TEST_F(TOMLFuzzerConfigurationTest, OverrideFuzzerConfigurationsViaTOMLString) {
    auto source = generateTestProgram(R"(
    // Drop the packet.
    action acl_drop() {
        mark_to_drop(sm);
    }

    table drop_table {
        key = {
            hdr.eth_hdr.dst_addr : ternary @name("dst_eth");
        }
        actions = {
            acl_drop();
            @defaultonly NoAction();
        }
    }

    apply {
        if (hdr.eth_hdr.isValid()) {
            drop_table.apply();
        }
    })");
    auto autoContext = SetUp("bmv2", "v1model");
    auto &rtSmithOptions = RtSmith::RtSmithOptions::get();
    rtSmithOptions.target = "bmv2"_cs;
    rtSmithOptions.arch = "v1model"_cs;

    std::string configInString = R"(
    maxEntryGenCnt = 10
    maxAttempts = 200
    maxTables = 10
    tablesToSkip = ["table1", "table2"]
    thresholdForDeletion = 50
    maxUpdateCount = 20
    maxUpdateTimeInMicroseconds = 100001
    minUpdateTimeInMicroseconds = 50001
    )";
    rtSmithOptions.setFuzzerConfigString(configInString);
    // Check if the fuzzer configuration string is set (and is set correctly).
    ASSERT_TRUE(rtSmithOptions.fuzzerConfigString().has_value());
    ASSERT_EQ(rtSmithOptions.fuzzerConfigString().value(), configInString);

    auto rtSmithResultOpt = P4::P4Tools::RtSmith::RtSmith::generateConfig(source, rtSmithOptions);
    // Check if the `RtSmithResult` object is generated successfully.
    ASSERT_TRUE(rtSmithResultOpt.has_value());

    auto compilerResult =
        P4::P4Tools::RtSmith::RtSmith::generateCompilerResult(source, rtSmithOptions);
    // Check if the `compilerResult` object is generated successfully.
    ASSERT_TRUE(compilerResult.has_value());

    const auto *programInfo = P4::P4Tools::RtSmith::RtSmithTarget::produceProgramInfo(
        compilerResult.value(), rtSmithOptions);
    // Check if the `programInfo` object is generated successfully.
    ASSERT_TRUE(programInfo != nullptr);

    // Parse the fuzzer configurations from the string.
    toml::parse_result tomlConfig;
    try {
        tomlConfig = toml::parse(configInString);
    } catch (const toml::parse_error &e) {
        FAIL() << "ControlPlaneSmith: Failed to parse fuzzer configuration string: " << e.what();
    }

    // Check if the fuzzer configurations are overridden correctly.
    const P4::P4Tools::RtSmith::FuzzerConfig &fuzzerConfig = programInfo->getFuzzerConfig();
    // Check if the node exists and can be casted to a pointer to a node representation of an
    // integer. If not, do nothing and leave the default configuration as is.
    if (const auto maxEntryGenCntValue = P4::P4Tools::RtSmith::TOMLUtils::getAndCastTOMLNode<int>(
            tomlConfig, "maxEntryGenCnt")) {
        ASSERT_EQ(fuzzerConfig.getMaxEntryGenCnt(), maxEntryGenCntValue.value());
    } else {
        // Generate a fatal failure (that "returns from the current function") if the
        // configuration is not an integer.
        FAIL() << "ControlPlaneSmith: The maximum number of entries to generate must be an "
                  "integer.";
    }

    if (const auto maxAttemptsValue =
            P4::P4Tools::RtSmith::TOMLUtils::getAndCastTOMLNode<int>(tomlConfig, "maxAttempts")) {
        ASSERT_EQ(fuzzerConfig.getMaxAttempts(), maxAttemptsValue.value());
    } else {
        FAIL() << "ControlPlaneSmith: The maximum number of attempts must be an integer.";
    }

    if (const auto maxTablesValue =
            P4::P4Tools::RtSmith::TOMLUtils::getAndCastTOMLNode<int>(tomlConfig, "maxTables")) {
        ASSERT_EQ(fuzzerConfig.getMaxTables(), maxTablesValue.value());
    } else {
        FAIL() << "ControlPlaneSmith: The maximum number of tables must be an integer.";
    }

    const std::vector<std::string> &tablesToSkip = fuzzerConfig.getTablesToSkip();
    if (const auto tablesToSkipValue =
            P4::P4Tools::RtSmith::TOMLUtils::getAndCastTOMLNode<std::vector<std::string>>(
                tomlConfig, "tablesToSkip")) {
        const std::vector<std::string> &tablesToSkipConfig = tablesToSkipValue.value();
        ASSERT_EQ(tablesToSkipConfig.size(), tablesToSkip.size());
        for (size_t i = 0; i < tablesToSkipConfig.size(); i++) {
            ASSERT_EQ(tablesToSkipConfig[i], tablesToSkip[i]);
        }
    } else {
        FAIL() << "ControlPlaneSmith: The tables to skip must be an array.";
    }

    if (const auto thresholdForDeletionValue =
            P4::P4Tools::RtSmith::TOMLUtils::getAndCastTOMLNode<uint64_t>(tomlConfig,
                                                                          "thresholdForDeletion")) {
        ASSERT_EQ(fuzzerConfig.getThresholdForDeletion(),
                  static_cast<uint64_t>(thresholdForDeletionValue.value()));
    } else {
        FAIL() << "ControlPlaneSmith: The threshold for deletion must be an integer.";
    }

    if (const auto maxUpdateCountValue =
            P4::P4Tools::RtSmith::TOMLUtils::getAndCastTOMLNode<size_t>(tomlConfig,
                                                                        "maxUpdateCount")) {
        ASSERT_EQ(fuzzerConfig.getMaxUpdateCount(),
                  static_cast<size_t>(maxUpdateCountValue.value()));
    } else {
        FAIL() << "ControlPlaneSmith: The maximum number of updates must be an integer.";
    }

    if (const auto maxUpdateTimeInMicrosecondsValue =
            P4::P4Tools::RtSmith::TOMLUtils::getAndCastTOMLNode<uint64_t>(
                tomlConfig, "maxUpdateTimeInMicroseconds")) {
        ASSERT_EQ(fuzzerConfig.getMaxUpdateTimeInMicroseconds(),
                  static_cast<uint64_t>(maxUpdateTimeInMicrosecondsValue.value()));
    } else {
        FAIL() << "ControlPlaneSmith: The maximum wait time must be an integer.";
    }
    if (const auto minUpdateTimeInMicrosecondsValue =
            P4::P4Tools::RtSmith::TOMLUtils::getAndCastTOMLNode<uint64_t>(
                tomlConfig, "minUpdateTimeInMicroseconds")) {
        ASSERT_EQ(fuzzerConfig.getMinUpdateTimeInMicroseconds(),
                  static_cast<uint64_t>(minUpdateTimeInMicrosecondsValue.value()));
    } else {
        FAIL() << "ControlPlaneSmith: The minimum wait time must be an integer.";
    }
}

}  // anonymous namespace

}  // namespace P4::P4Tools::Test
