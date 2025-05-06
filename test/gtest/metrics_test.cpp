/*
Tests for the frontend code metric collection passes in frontends/p4/metrics.
The tests run the frontend of the compiler and compare the code metrics
output with the expected output. P4 programs for testing are located in 
testdata/p4_16_samples, and the expected metric outputs are located in
testdata/p4_16_samples_outputs.
*/

#include <gtest/gtest.h>

#include <fstream>
#include <sstream>
#include <string>
#include <filesystem>
#include <cstdlib>

#include "helpers.h"                    
#include "frontends/common/options.h"
#include "frontends/common/parseInput.h"
#include "frontends/p4/frontend.h"

using namespace P4::literals;

namespace P4::Test {
namespace fs = std::filesystem;

class MetricPassesTest : public P4CTest {
protected:
    std::string inputFile;
    std::string programName;
    fs::path metricsOutputPath;
    fs::path txtMetricsOutputPath;
    fs::path jsonMetricsOutputPath;

    void SetUpFrontend(bool allMetrics) {
        P4CContext& contextRef = P4CContext::get();
        auto* ctx = dynamic_cast<P4CContextWithOptions<CompilerOptions>*>(&contextRef);
        auto &opts = ctx->options();
        opts.langVersion = CompilerOptions::FrontendVersion::P4_16;
        opts.file = inputFile;
        if(allMetrics){
            opts.selectedMetrics = {
                "loc"_cs, "cyclomatic"_cs, "halstead"_cs, "unused-code"_cs, "nesting-depth"_cs,
                "header-general"_cs, "header-manipulation"_cs,"header-modification"_cs,
                "match-action"_cs, "parser"_cs, "inlined"_cs, "extern"_cs
            };
        }

        else opts.selectedMetrics = {"loc"_cs, "cyclomatic"_cs, "header-manipulation"_cs};

        const IR::P4Program* program = parseP4File(opts);
        ASSERT_NE(program, nullptr) << "Parsing failed for " << inputFile;
    
        FrontEnd frontend;
        const IR::P4Program* result = frontend.run(opts, program, &std::cerr);
        ASSERT_NE(result, nullptr) << "Frontend pipeline failed for " << inputFile;
    
        metricsOutputPath = "../testdata/p4_16_samples/metrics/" + fs::path(inputFile).stem().string();
        txtMetricsOutputPath = jsonMetricsOutputPath = metricsOutputPath;
        txtMetricsOutputPath += "_metrics.txt";
        jsonMetricsOutputPath += "_metrics.json";
    }
    
    std::string readFileContent(const fs::path& path) {
        std::ifstream in(path);
        if (!in.is_open()) {
            std::cerr << "Cannot open file: " << path.string();
            return "";
        }
        std::stringstream buf;
        buf << in.rdbuf();
        return buf.str();
    }

    void compareWithExpectedOutput(const std::string& expectedRelPath) {
        std::string actual = readFileContent(jsonMetricsOutputPath);
        std::string expected = readFileContent(expectedRelPath);

        ASSERT_NE(actual, "") << "Cannot open file: " << jsonMetricsOutputPath<<std::endl;
        ASSERT_NE(expected, "") << "Cannot open file: " << expectedRelPath<<std::endl;
        EXPECT_EQ(actual, expected);
    }

    void TearDown() override {        
        if (fs::exists(txtMetricsOutputPath)) fs::remove(txtMetricsOutputPath);
        if (fs::exists(jsonMetricsOutputPath)) fs::remove(jsonMetricsOutputPath);
    }
};

#define DEFINE_METRIC_TEST(N)                                                        \
TEST_F(MetricPassesTest, MetricsTest##N) {                                           \
    inputFile = "../testdata/p4_16_samples/metrics/metrics_test_" #N ".p4";          \
    SetUpFrontend(true);                                                             \
    compareWithExpectedOutput(                                                       \
        "../testdata/p4_16_samples_outputs/metrics/metrics_test_" #N "_metrics.json");\
}

DEFINE_METRIC_TEST(1)
DEFINE_METRIC_TEST(2)
DEFINE_METRIC_TEST(3)
DEFINE_METRIC_TEST(4)
DEFINE_METRIC_TEST(5)
DEFINE_METRIC_TEST(6)
DEFINE_METRIC_TEST(7)
DEFINE_METRIC_TEST(8)

TEST_F(MetricPassesTest, MetricsTest9) {
    inputFile = "../testdata/p4_16_samples/metrics/metrics_test_8.p4";
    SetUpFrontend(false);
    compareWithExpectedOutput(
        "../testdata/p4_16_samples_outputs/metrics/metrics_test_9_metrics.json");
}

TEST_F(MetricPassesTest, MetricsTest10) {
    inputFile = "../testdata/p4_16_samples/metrics/metrics_test_8.p4";
    SetUpFrontend(true);

    // Check if the metric values got written to compiler context.
    Metrics& metrics = P4CContext::get().options().metrics;

    EXPECT_EQ(metrics.linesOfCode, 150u);
    EXPECT_EQ(metrics.cyclomaticComplexity.at(cstring::literal("MyIngress")), 7u);
    EXPECT_EQ(metrics.halsteadMetrics.uniqueOperators, 26u);
    EXPECT_NEAR(metrics.halsteadMetrics.effort, 43921.6, 0.1);
    EXPECT_EQ(metrics.unusedCodeInstances.states, 1u);
    EXPECT_EQ(metrics.nestingDepth.maxNestingDepth, 2u);
    EXPECT_EQ(metrics.headerMetrics.numHeaders, 3u);
    EXPECT_EQ(metrics.headerManipulationMetrics.total.numOperations, 3u);
    EXPECT_EQ(metrics.headerModificationMetrics.total.totalSize, 136u);
    EXPECT_EQ(metrics.matchActionTableMetrics.numTables, 3u);
    EXPECT_EQ(metrics.parserMetrics.totalStates, 5u);
    EXPECT_EQ(metrics.externMetrics.externFunctions, 24u);
    EXPECT_EQ(metrics.inlinedActions, 0u);
}

}  // namespace P4::Test
