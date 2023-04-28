#ifndef BACKENDS_P4TOOLS_MODULES_TESTGEN_LIB_TF_H_
#define BACKENDS_P4TOOLS_MODULES_TESTGEN_LIB_TF_H_

#include <cstddef>
#include <filesystem>
#include <functional>
#include <iosfwd>
#include <map>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include <inja/inja.hpp>

#include "backends/p4tools/common/lib/format_int.h"
#include "backends/p4tools/common/lib/trace_event.h"
#include "ir/ir.h"
#include "lib/cstring.h"

#include "backends/p4tools/modules/testgen/lib/test_spec.h"

namespace P4Tools::P4Testgen {

/// THe default base class for the various test frameworks (TF). Every test framework has a test
/// name and a seed associated with it. Also contains a variety of common utility functions.
class TF {
 protected:
    /// The @basePath to be used in test case generation.
    std::filesystem::path basePath;

    /// The seed used by the testgen.
    std::optional<unsigned int> seed;

    /// Creates a generic test framework.
    TF(std::filesystem::path basePath, std::optional<unsigned int> seed);

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

 public:
    /// The method used to output the test case to be implemented by
    /// all the test frameworks (eg. STF, PTF, etc.).
    /// @param spec the testcase specification to be outputted
    /// @param selectedBranches string describing branches selected for this testcase
    /// @param testIdx testcase unique number identifier
    /// @param currentCoverage current coverage ratio (between 0.0 and 1.0)
    // attaches arbitrary string data to the test preamble.
    virtual void outputTest(const TestSpec *spec, cstring selectedBranches, size_t testIdx,
                            float currentCoverage) = 0;

    /// Print out some performance numbers if logging feature "performance" is enabled.
    /// Also log performance numbers to a separate file in the test folder if @param write is
    /// enabled.
    void printPerformanceReport(bool write) const;
};

}  // namespace P4Tools::P4Testgen

#endif /* BACKENDS_P4TOOLS_MODULES_TESTGEN_LIB_TF_H_ */
