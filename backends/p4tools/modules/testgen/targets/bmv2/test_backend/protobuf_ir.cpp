#include "backends/p4tools/modules/testgen/targets/bmv2/test_backend/protobuf_ir.h"

#include <filesystem>
#include <iomanip>
#include <optional>
#include <string>
#include <utility>

#include <inja/inja.hpp>

#include "backends/p4tools/common/lib/util.h"
#include "lib/log.h"
#include "nlohmann/json.hpp"

namespace P4Tools::P4Testgen::Bmv2 {

ProtobufIr::ProtobufIr(std::filesystem::path basePath, std::optional<unsigned int> seed)
    : Bmv2TestFramework(std::move(basePath), seed) {}

std::string ProtobufIr::getTestCaseTemplate() {
    static std::string TEST_CASE(
        R"""(# A P4TestGen-generated test case for {{test_name}}.p4
metadata: "p4testgen seed: {{ default(seed, "none") }}"
metadata: "Date generated: {{timestamp}}"
## if length(selected_branches) > 0
metadata: "{{selected_branches}}"
## endif
metadata: "Current node coverage: {{coverage}}"

## for trace_item in trace
traces: '{{trace_item}}'
## endfor

input_packet {
  packet: "{{send.pkt}}"
  port: {{send.ig_port}}
}

## if verify
expected_output_packet {
  packet: "{{verify.exp_pkt}}"
  port: {{verify.eg_port}}
  packet_mask: "{{verify.ignore_mask}}"
}
## endif

## if control_plane
entities : [
## for table in control_plane.tables
## for rule in table.rules
  # Table {{table.table_name}}
  {
    table_entry {
      table_name: "{{table.table_name}}"
## if rule.rules.needs_priority
      priority: {{rule.priority}}
## endif
## for r in rule.rules.single_exact_matches
      # Match field {{r.field_name}}
      matches {
        name: "{{r.field_name}}"
        exact: { hex_str: "{{r.value}}" }
      }
## endfor
## for r in rule.rules.optional_matches
    # Match field {{r.field_name}}
    matches {
      name: "{{r.field_name}}"
      optional {
        value: { hex_str: "{{r.value}}" }
      }
    }
## endfor
## for r in rule.rules.range_matches
      # Match field {{r.field_name}}
      matches {
        name: "{{r.field_name}}"
        range {
          low: { hex_str: "{{r.lo}}" }
          high: { hex_str:  "{{r.hi}}" }
        }
      }
## endfor
## for r in rule.rules.ternary_matches
      # Match field {{r.field_name}}
      matches {
        name: "{{r.field_name}}"
        ternary {
          value: { hex_str: "{{r.value}}" }
          mask: { hex_str: "{{r.mask}}" }
        }
      }
## endfor
## for r in rule.rules.lpm_matches
      # Match field {{r.field_name}}
      matches {
        name: "{{r.field_name}}"
        lpm {
          value: { hex_str: "{{r.value}}" }
          prefix_length: {{r.prefix_len}}
        }
      }
## endfor
      # Action {{rule.action_name}}
## if existsIn(table, "has_ap")
        action_set {
          actions {
            action {
              name: "{{rule.action_name}}"
## for act_param in rule.rules.act_args
              # Param {{act_param.param}}
              params {
                name: "{{act_param.param}}"
                value: { hex_str: "{{act_param.value}}" }
              }
## endfor
            }
          }
        }
## else
        action {
          name: "{{rule.action_name}}"
## for act_param in rule.rules.act_args
          # Param {{act_param.param}}
          params {
            name: "{{act_param.param}}"
            value: { hex_str: "{{act_param.value}}" }
          }
## endfor
        }
## endif
## endfor
    }
  }{% if not loop.is_last %},{% endif %}
## endfor
]
## endif
)""");
    return TEST_CASE;
}

void ProtobufIr::emitTestcase(const TestSpec *testSpec, cstring selectedBranches, size_t testId,
                              const std::string &testCase, float currentCoverage) {
    inja::json dataJson;
    if (selectedBranches != nullptr) {
        dataJson["selected_branches"] = selectedBranches.c_str();
    }
    if (seed) {
        dataJson["seed"] = *seed;
    }
    dataJson["test_name"] = basePath.stem();
    dataJson["test_id"] = testId;
    dataJson["trace"] = getTrace(testSpec);
    dataJson["control_plane"] = getControlPlane(testSpec);
    dataJson["send"] = getSend(testSpec);
    dataJson["verify"] = getExpectedPacket(testSpec);
    dataJson["timestamp"] = Utils::getTimeStamp();
    std::stringstream coverageStr;
    coverageStr << std::setprecision(2) << currentCoverage;
    dataJson["coverage"] = coverageStr.str();

    // Check whether this test has a clone configuration.
    // These are special because they require additional instrumentation and produce two output
    // packets.
    auto cloneSpecs = testSpec->getTestObjectCategory("clone_specs");
    if (!cloneSpecs.empty()) {
        dataJson["clone_specs"] = getClone(cloneSpecs);
    }
    auto meterValues = testSpec->getTestObjectCategory("meter_values");
    dataJson["meter_values"] = getMeter(meterValues);

    LOG5("ProtobufIR test back end: emitting testcase:" << std::setw(4) << dataJson);
    auto incrementedbasePath = basePath;
    incrementedbasePath.concat("_" + std::to_string(testId));
    incrementedbasePath.replace_extension(".txtpb");
    auto protobufFileStream = std::ofstream(incrementedbasePath);
    inja::render_to(protobufFileStream, testCase, dataJson);
    protobufFileStream.flush();
}

void ProtobufIr::outputTest(const TestSpec *testSpec, cstring selectedBranches, size_t testId,
                            float currentCoverage) {
    std::string testCase = getTestCaseTemplate();
    emitTestcase(testSpec, selectedBranches, testId, testCase, currentCoverage);
}

}  // namespace P4Tools::P4Testgen::Bmv2
