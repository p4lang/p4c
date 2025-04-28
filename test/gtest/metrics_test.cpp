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

namespace P4::Test {
namespace fs = std::filesystem;

class MetricPassesTest : public P4CTest {
protected:
    std::string inputFile;
    std::string programName;
    fs::path metricsOutputPath;
    fs::path txtMetricsOutputPath;
    fs::path jsonMetricsOutputPath;

    void SetUpFrontend() {
        P4CContext& contextRef = P4CContext::get();
        auto* ctx = dynamic_cast<P4CContextWithOptions<CompilerOptions>*>(&contextRef);
        auto &opts = ctx->options();
        opts.langVersion = CompilerOptions::FrontendVersion::P4_16;
        opts.file = inputFile;
        opts.selectedMetrics = {
            "loc", "cyclomatic", "halstead", "unused-code", "nesting-depth",
            "header-general", "header-manipulation","header-modification",
            "match-action", "parser", "inlined", "extern"
        };

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
        std::string actual = readFileContent(txtMetricsOutputPath);
        std::string expected = readFileContent(expectedRelPath);

        ASSERT_NE(actual, "") << "Cannot open file: " << txtMetricsOutputPath<<std::endl;
        ASSERT_NE(expected, "") << "Cannot open file: " << expectedRelPath<<std::endl;
        EXPECT_EQ(actual, expected);
    }

    void TearDown() override {        
        if (fs::exists(txtMetricsOutputPath)) fs::remove(txtMetricsOutputPath);
        if (fs::exists(jsonMetricsOutputPath)) fs::remove(jsonMetricsOutputPath);
    }
};

#define DEFINE_METRIC_TEST(N)                                           \
TEST_F(MetricPassesTest, MetricsTest##N) {                              \
    inputFile = "../testdata/p4_16_samples/metrics/metrics_test_" #N ".p4";     \
    SetUpFrontend();                                                    \
    compareWithExpectedOutput(                                          \
        "../testdata/p4_16_samples_outputs/metrics/metrics_test_" #N "_metrics.expected"); \
}

DEFINE_METRIC_TEST(1)
DEFINE_METRIC_TEST(2)
DEFINE_METRIC_TEST(3)
DEFINE_METRIC_TEST(4)
DEFINE_METRIC_TEST(5)
DEFINE_METRIC_TEST(6)
DEFINE_METRIC_TEST(7)
DEFINE_METRIC_TEST(8)

}  // namespace P4::Test
