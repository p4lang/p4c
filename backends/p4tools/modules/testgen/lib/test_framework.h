#ifndef BACKENDS_P4TOOLS_MODULES_TESTGEN_LIB_TEST_FRAMEWORK_H_
#define BACKENDS_P4TOOLS_MODULES_TESTGEN_LIB_TEST_FRAMEWORK_H_

#include <cstddef>
#include <filesystem>
#include <functional>
#include <iosfwd>
#include <map>
#include <optional>
#include <string>
#include <utility>

#include <inja/inja.hpp>

#include "backends/p4tools/common/lib/format_int.h"
#include "backends/p4tools/common/lib/trace_event.h"
#include "lib/castable.h"
#include "lib/cstring.h"

#include "backends/p4tools/modules/testgen/lib/test_backend_configuration.h"
#include "backends/p4tools/modules/testgen/lib/test_object.h"
#include "backends/p4tools/modules/testgen/lib/test_spec.h"

namespace P4Tools::P4Testgen {

/// Type definitions for abstract tests.
struct AbstractTest : ICastable {};
/// TODO: It would be nice if this were a reference to signal non-nullness.
/// Consider using an optional_ref implementation.
using AbstractTestReference = const AbstractTest *;
using AbstractTestReferenceOrError = std::optional<AbstractTestReference>;
using AbstractTestList = std::vector<AbstractTestReference>;

/// Template to convert a list of abstract tests to a list of concretely typed tests.
/// Only works for subclasses of AbstractTest.
template <class ConcreteTest,
          typename = std::enable_if_t<std::is_base_of_v<AbstractTest, ConcreteTest>>>
std::vector<const ConcreteTest *> convertAbstractTestsToConcreteTests(
    const P4Tools::P4Testgen::AbstractTestList &testList) {
    std::vector<const ConcreteTest *> result;
    std::transform(testList.begin(), testList.end(), std::back_inserter(result),
                   [](AbstractTestReference test) { return test->checkedTo<ConcreteTest>(); });
    return result;
}

/// An file path which may or may not be set. Can influence the execution behavior the test
/// framework.
using OptionalFilePath = std::optional<std::filesystem::path>;

/// THe default base class for the various test frameworks. Every test framework has a test
/// name and a seed associated with it. Also contains a variety of common utility functions.
class TestFramework {
 private:
    /// Configuration options for the test back end.
    std::reference_wrapper<const TestBackendConfiguration> testBackendConfiguration;

 protected:
    /// Creates a generic test framework.
    explicit TestFramework(const TestBackendConfiguration &testBackendConfiguration);

    /// Converts the traces of this test into a string representation and Inja object.
    static inja::json getTrace(const TestSpec *testSpec) {
        inja::json traceList = inja::json::array();
        const auto *traces = testSpec->getTraces();
        if (traces != nullptr) {
            for (const auto &trace : *traces) {
                std::stringstream ss;
                ss << trace;
                traceList.push_back(ss.str());
            }
        }
        return traceList;
    }

    /// Checks whether a table object has an action profile or selector associated with it.
    /// If that is the case, we set a boolean flag for this particular inja object.
    template <class ProfileType, class SelectorType>
    static void checkForTableActionProfile(inja::json &tblJson, std::map<cstring, cstring> &apAsMap,
                                           const TableConfig *tblConfig) {
        const auto *apObject = tblConfig->getProperty("action_profile", false);
        if (apObject != nullptr) {
            const auto *actionProfile = apObject->checkedTo<ProfileType>();
            tblJson["has_ap"] = true;
            // Check if we have an Action Selector too.
            // TODO: Change this to check in ActionSelector with table
            // property "action_selectors".
            const auto *asObject = tblConfig->getProperty("action_selector", false);
            if (asObject != nullptr) {
                const auto *actionSelector = asObject->checkedTo<SelectorType>();
                apAsMap[actionProfile->getProfileDecl()->controlPlaneName()] =
                    actionSelector->getSelectorDecl()->controlPlaneName();
                tblJson["has_as"] = true;
            }
        }
    }

    /// Check whether the table object has an overridden default action.
    /// In this case, we assume there are no keys and we just set the default action of the table.
    static void checkForDefaultActionOverride(inja::json &tblJson, const TableConfig *tblConfig) {
        const auto *defaultOverrideObj = tblConfig->getProperty("overriden_default_action", false);
        if (defaultOverrideObj != nullptr) {
            const auto *defaultAction = defaultOverrideObj->checkedTo<ActionCall>();
            inja::json a;
            a["action_name"] = defaultAction->getActionName();
            auto const *actionArgs = defaultAction->getArgs();
            inja::json b = inja::json::array();
            for (const auto &actArg : *actionArgs) {
                inja::json j;
                j["param"] = actArg.getActionParamName().c_str();
                j["value"] = formatHexExpr(actArg.getEvaluatedValue());
                b.push_back(j);
            }
            a["act_args"] = b;
            tblJson["default_override"] = a;
        }
    }

    /// Collect all the action profile objects. These will have to be declared in the test.
    template <class ProfileType>
    static void collectActionProfileDeclarations(const TestSpec *testSpec,
                                                 inja::json &controlPlaneJson,
                                                 const std::map<cstring, cstring> &apAsMap) {
        auto actionProfiles = testSpec->getTestObjectCategory("action_profiles");
        if (!actionProfiles.empty()) {
            controlPlaneJson["action_profiles"] = inja::json::array();
        }
        for (auto const &testObject : actionProfiles) {
            const auto *const actionProfile = testObject.second->checkedTo<ProfileType>();
            const auto *actions = actionProfile->getActions();
            inja::json j;
            j["profile"] = actionProfile->getProfileDecl()->controlPlaneName();
            j["actions"] = inja::json::array();
            for (size_t idx = 0; idx < actions->size(); ++idx) {
                const auto &action = actions->at(idx);
                auto actionName = action.first;
                auto actionArgs = action.second;
                inja::json a;
                a["action_name"] = actionName;
                a["action_idx"] = std::to_string(idx);
                inja::json b = inja::json::array();
                for (const auto &actArg : actionArgs) {
                    inja::json c;
                    c["param"] = actArg.getActionParamName().c_str();
                    c["value"] = formatHexExpr(actArg.getEvaluatedValue()).c_str();
                    b.push_back(c);
                }
                a["act_args"] = b;
                j["actions"].push_back(a);
            }
            // Look up the selectors associated with the profile.
            if (apAsMap.find(actionProfile->getProfileDecl()->controlPlaneName()) !=
                apAsMap.end()) {
                j["selector"] = apAsMap.at(actionProfile->getProfileDecl()->controlPlaneName());
            }
            controlPlaneJson["action_profiles"].push_back(j);
        }
    }

    /// Returns the configuration options for the test back end.
    [[nodiscard]] const TestBackendConfiguration &getTestBackendConfiguration() const;

 public:
    virtual ~TestFramework() = default;

    /// The method used to output the test case to be implemented by
    /// all the test frameworks (eg. STF, PTF, etc.).
    /// @param spec the testcase specification to be outputted.
    /// @param selectedBranches string describing branches selected for this testcase.
    /// @param testIdx testcase unique number identifier. TODO: Make this a member?
    /// @param currentCoverage current coverage ratio (between 0.0 and 1.0)
    /// TODO (https://github.com/p4lang/p4c/issues/4403): This should not return void but instead a
    /// status.
    virtual void writeTestToFile(const TestSpec *spec, cstring selectedBranches, size_t testIdx,
                                 float currentCoverage) = 0;

    /// The method used to return the test case. This method is optional to each test framework.
    /// @param spec the testcase specification to be outputted.
    /// @param selectedBranches string describing branches selected for this testcase.
    /// @param testIdx testcase unique number identifier. TODO: Make this a member?
    /// @param currentCoverage current coverage ratio (between 0.0 and 1.0).
    virtual AbstractTestReferenceOrError produceTest(const TestSpec *spec, cstring selectedBranches,
                                                     size_t testIdx, float currentCoverage);

    /// @Returns true if the test framework is configured to write to a file.
    [[nodiscard]] bool isInFileMode() const;
};

}  // namespace P4Tools::P4Testgen

#endif /* BACKENDS_P4TOOLS_MODULES_TESTGEN_LIB_TEST_FRAMEWORK_H_ */
