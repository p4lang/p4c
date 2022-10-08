#include "backends/p4tools/testgen/targets/bmv2/backend/stf/stf.h"

#include <algorithm>
#include <iomanip>
#include <map>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include <boost/lexical_cast.hpp>
#include <boost/multiprecision/detail/et_ops.hpp>
#include <boost/multiprecision/number.hpp>
#include <boost/multiprecision/traits/explicit_conversion.hpp>
#include <boost/none.hpp>
#include <boost/optional/optional_io.hpp>
#include <boost/variant/apply_visitor.hpp>
#include <boost/variant/get.hpp>
#include <boost/variant/static_visitor.hpp>

#include "backends/p4tools/common/lib/format_int.h"
#include "backends/p4tools/common/lib/ir.h"
#include "backends/p4tools/common/lib/trace_events.h"
#include "backends/p4tools/common/lib/util.h"
#include "gsl/gsl-lite.hpp"
#include "lib/gmputil.h"
#include "lib/null.h"

namespace P4Tools {

namespace P4Testgen {

namespace Bmv2 {

/// MatchField objects contain information about one particular match field of an table entry.
struct MatchField {
    /// The range of the field, if it exists.
    std::pair<const IR::Constant*, const IR::Constant*> range;

    /// The value of the field.
    IR::Constant* val;

    /// The mask of the field. Often, this is zero.
    boost::optional<const IR::Constant*> mask;

    explicit MatchField(std::pair<const IR::Constant*, const IR::Constant*> range,
                        IR::Constant* val, boost::optional<const IR::Constant*> mask)
        : range(std::move(range)), val(val), mask(mask) {}
};

void STFTest::Command::print(std::ofstream& out) const { out << getCommandName(); }

STFTest::Add::Add(cstring table, const std::map<cstring, const FieldMatch>* matches, cstring action,
                  const std::vector<ActionArg>* args, int addr)
    : table(table), addr(addr), matches(matches), action(action), args(args) {}

cstring STFTest::Add::getCommandName() const { return "add"; }

void STFTest::Add::print(std::ofstream& out) const {
    // Contains the match fields.
    std::vector<cstring> fields;

    std::vector<MatchField> matchFields;

    // Contains the mask value to output.
    // If the match is ternary or LPM, the priority also needs to be emitted
    Command::print(out);
    out << " " << table + " ";
    for (const auto& matchTuple : *matches) {
        auto match = matchTuple.second;
        auto* ternary = boost::relaxed_get<Ternary>(&match);
        if (ternary != nullptr) {
            out << addr << " ";
            break;
        }
        auto* lpm = boost::relaxed_get<LPM>(&match);
        if (lpm != nullptr) {
            out << addr << " ";
            break;
        }
    }

    for (auto const& match : *matches) {
        auto const fieldName = match.first;
        auto const& fieldMatch = match.second;

        fields.push_back(fieldName);

        // Visit the fieldMatch to populate the dataRange and nextData
        // corresponding to each field.
        struct GetRange : public boost::static_visitor<void> {
            std::vector<MatchField>* matchFields;

            explicit GetRange(std::vector<MatchField>* matchFields) : matchFields(matchFields) {}
            void operator()(const Exact& elem) const {
                MatchField field({elem.getEvaluatedValue(), elem.getEvaluatedValue()},
                                 elem.getEvaluatedValue()->clone(), boost::none);
                matchFields->emplace_back(field);
            }
            void operator()(const Range& elem) const {
                MatchField field({elem.getEvaluatedLow(), elem.getEvaluatedHigh()},
                                 elem.getEvaluatedLow()->clone(), boost::none);
                matchFields->emplace_back(field);
            }
            void operator()(const Ternary& elem) const {
                MatchField field({elem.getEvaluatedValue(), elem.getEvaluatedValue()},
                                 elem.getEvaluatedValue()->clone(), elem.getEvaluatedMask());
                matchFields->emplace_back(field);
            }
            void operator()(const LPM& elem) const {
                const auto* value = elem.getEvaluatedValue();
                auto prefixLen = elem.getEvaluatedPrefixLength()->asInt();
                auto fieldWidth = value->type->width_bits();
                auto maxVal = IRUtils::getMaxBvVal(prefixLen);
                const auto* mask =
                    IRUtils::getConstant(value->type, maxVal << (fieldWidth - prefixLen));
                MatchField field({elem.getEvaluatedValue(), elem.getEvaluatedValue()},
                                 elem.getEvaluatedValue()->clone(), mask);
                matchFields->emplace_back(field);
            }
        };

        boost::apply_visitor(GetRange(&matchFields), fieldMatch);
    }

    while (true) {
        // Go through the vector of fields and emit the field:value pairs.
        for (size_t fieldIndex = 0; fieldIndex < fields.size(); ++fieldIndex) {
            if (fieldIndex > 0) {
                out << " ";
            }
            out << fields.at(fieldIndex) + ":";
            auto matchField = matchFields.at(fieldIndex);
            const auto& dataValue = matchField.val;
            const auto& maskFieldOpt = matchField.mask;
            if (maskFieldOpt != boost::none) {
                const auto* maskField = *maskFieldOpt;
                BUG_CHECK(dataValue->type->width_bits() == maskField->type->width_bits(),
                          "Data value and its mask should have the same bit width.");
                // Using the width from mask - should be same as data
                auto dataStr = formatBinExpr(dataValue, false, true, false);
                auto maskStr = formatBinExpr(maskField, false, true, false);
                std::string data;
                for (size_t dataPos = 0; dataPos < dataStr.size(); ++dataPos) {
                    if (maskStr.at(dataPos) == '0') {
                        data += "*";
                    } else {
                        data += dataStr.at(dataPos);
                    }
                }
                out << "0b" << data;
            } else {
                out << formatHexExpr(dataValue);
            }
        }

        // Output the action and action arguments.
        out << " " << action + "(";
        bool needComma = false;
        for (auto const& actArg : *args) {
            if (needComma) {
                out << ",";
            }
            needComma = true;
            out << actArg.getActionParamName() + ":";
            out << formatHexExpr(actArg.getEvaluatedValue());
        }

        // Setting nextData for the next iteration of the loop.
        for (int fieldIndex = static_cast<int>(fields.size()) - 1; fieldIndex >= 0; --fieldIndex) {
            auto matchField = matchFields.at(fieldIndex);
            const auto& dataRange = matchField.range;
            const auto& dataValue = matchField.val->value;

            // Reset the matchField.value when we are done outputting the entire range.
            if (dataValue == dataRange.second->value) {
                matchField.val->value = dataRange.first->value;
                // Continue iterating until all fields are done, when we return.
                if (fieldIndex == 0) {
                    out << ")" << std::endl;
                    return;
                }
            } else {
                matchField.val->value += 1;
                break;
            }
        }

        // Setting the output for the next entry.
        out << ")" << std::endl;
        Command::print(out);
        out << " " << table + " ";
        if (addr >= 0) {
            out << addr << " ";
        }
    }
}

STFTest::SetDefault::SetDefault(cstring table, cstring action,
                                const std::vector<const ActionArg>* args)
    : table(table), action(action), args(args) {}

cstring STFTest::SetDefault::getCommandName() const { return "setdefault"; }

void STFTest::SetDefault::print(std::ofstream& out) const {
    Command::print(out);
    out << " ";
    // TODO: Emit other arguments - <table> <action>({<name>:<value>})
}

STFTest::CheckStats::CheckStats(int pipe, cstring table, int index, const IR::Constant* value)
    : pipe(pipe), table(table), index(index), value(value) {}

void STFTest::CheckStats::print(std::ofstream& out) const {
    Command::print(out);
    out << " ";
    // TODO: Emit other arguments - [<pipe>:]<table>(<index>) <field> = <value>
}

cstring STFTest::CheckCounter::getCommandName() const { return "check_counter"; }

STFTest::CheckCounter::CheckCounter(int pipe, cstring table, int index, const IR::Constant* value)
    : CheckStats(pipe, table, index, value) {}

cstring STFTest::CheckRegister::getCommandName() const { return "check_register"; }

STFTest::CheckRegister::CheckRegister(int pipe, cstring table, int index, const IR::Constant* value)
    : CheckStats(pipe, table, index, value) {}

STFTest::Packet::Packet(int port, const IR::Constant* data, const IR::Constant* mask)
    : port(port), data(data), mask(mask) {}

void STFTest::Packet::print(std::ofstream& out) const {
    Command::print(out);
    out << " " << port << " ";
    auto dataStr = formatHexExpr(data, false, true, false);
    if (mask == nullptr) {
        out << dataStr;
        return;
    }
    // If a mask is present, construct the packet data  with wildcard `*` where there are
    // non zero nibbles
    auto maskStr = formatHexExpr(mask, false, true, false);
    std::string packetData;
    for (size_t dataPos = 0; dataPos < dataStr.size(); ++dataPos) {
        if (maskStr.at(dataPos) != 'F') {
            // TODO: We are being conservative here and adding a wildcard for any 0
            // in the 4b nibble
            packetData += "*";
        } else {
            packetData += dataStr[dataPos];
        }
    }
    out << packetData;
}

cstring STFTest::Ingress::getCommandName() const { return "packet"; }

STFTest::Ingress::Ingress(int port, const IR::Constant* data) : Packet(port, data, nullptr) {}

void STFTest::Ingress::print(std::ofstream& out) const {
    Packet::print(out);
    out << std::endl;
}

cstring STFTest::Egress::getCommandName() const { return "expect"; }

STFTest::Egress::Egress(int port, const IR::Constant* data, const IR::Constant* mask)
    : Packet(port, data, mask) {}

void STFTest::Egress::print(std::ofstream& out) const {
    Packet::print(out);
    out << "$" << std::endl;
}

cstring STFTest::Wait::getCommandName() const { return "wait"; }

void STFTest::Wait::print(std::ofstream& out) const {
    Command::print(out);
    out << std::endl;
}

STFTest::Comment::Comment(cstring message) : message(message) {}

cstring STFTest::Comment::getCommandName() const { return "#"; }

void STFTest::Comment::print(std::ofstream& out) const {
    Command::print(out);
    out << " " << message << std::endl;
}

STF::STF(cstring testName, boost::optional<unsigned int> seed) : TF(testName, seed) {}

void STF::comment(std::ofstream& out, cstring message) {
    auto commentConfig = STFTest::Comment(message);
    commentConfig.print(out);
}

void STF::trace(std::ofstream& out, const TestSpec* testSpec) {
    const auto* traces = testSpec->getTraces();
    if ((traces != nullptr) && !traces->empty()) {
        out << "\n";
        comment(out, "Traces");
        for (const auto& trace : *traces) {
            out << "# " << *trace << "\n";
        }
        out << "\n";
    }
}

void STF::add(std::ofstream& out, const TestSpec* testSpec) {
    auto tables = testSpec->getTestObjectCategory("tables");
    for (auto const& tbl : tables) {
        const auto* tblConfig = tbl.second->checkedTo<TableConfig>();
        auto const* tblRules = tblConfig->getRules();
        for (const auto& tblRules : *tblRules) {
            auto const* matches = tblRules.getMatches();
            auto const* actionCall = tblRules.getActionCall();
            auto const* actionArgs = actionCall->getArgs();
            const int priority = tblRules.getPriority();
            auto addConfig =
                STFTest::Add(tbl.first, matches, actionCall->getActionName(), actionArgs, priority);
            addConfig.print(out);
        }
    }
}

void STF::packet(std::ofstream& out, const TestSpec* testSpec) {
    auto const* iPacket = testSpec->getIngressPacket();
    CHECK_NULL(iPacket);
    const auto* payload = iPacket->getEvaluatedPayload();
    auto packetConfig = STFTest::Ingress(iPacket->getPort(), payload);
    packetConfig.print(out);
}

void STF::expect(std::ofstream& out, const TestSpec* testSpec) {
    const auto ePacket = testSpec->getEgressPacket();
    if (ePacket == boost::none) {
        return;
    }
    const auto* payload = (*ePacket)->getEvaluatedPayload();
    const auto* payloadMask = (*ePacket)->getEvaluatedPayloadMask();
    auto expectConfig = STFTest::Egress((*ePacket)->getPort(), payload, payloadMask);
    expectConfig.print(out);
}

void STF::wait(std::ofstream& out) {
    auto waitConfig = STFTest::Wait();
    waitConfig.print(out);
}

void STF::clone(std::ofstream& stfFile, const TestSpec* testSpec,
                const std::map<cstring, const TestObject*>& cloneInfos) {
    const auto ePacket = testSpec->getEgressPacket();
    for (auto cloneInfoTuple : cloneInfos) {
        const auto* cloneInfo = cloneInfoTuple.second->checkedTo<Bmv2_CloneInfo>();
        auto sessionId = cloneInfo->getEvaluatedSessionId()->asUint64();
        auto clonePort = cloneInfo->getEvaluatedClonePort()->asInt();
        // Add the mirroring configuration for this clone packet.
        stfFile << "mirroring_add " << sessionId << " " << clonePort << std::endl;
        // Insert the input packet.
        packet(stfFile, testSpec);
        if (cloneInfo->isClonedPacket()) {
            BUG_CHECK(ePacket != boost::none, "The cloned packet must not be empty!");
            // Expect only a dummy for the normal packet..
            stfFile << "expect " << (*ePacket)->getPort() << std::endl;
            // This packet is the cloned packet, so expect the cloned values as egress.
            // Note how we use the clonePort for egress.
            const auto* payload = (*ePacket)->getEvaluatedPayload();
            const auto* payloadMask = (*ePacket)->getEvaluatedPayloadMask();
            auto clonedConfig = STFTest::Egress(clonePort, payload, payloadMask);
            clonedConfig.print(stfFile);
            continue;
        }
        // If the packet is not the clone packet, insert a dummy entry for port where the cloned
        // packet is emitted.
        stfFile << "expect " << clonePort << std::endl;
        // Skip generation if the egress packet was dropped.
        if (ePacket == boost::none) {
            continue;
        }
        // This is the normal packet.
        const auto* payload = (*ePacket)->getEvaluatedPayload();
        const auto* payloadMask = (*ePacket)->getEvaluatedPayloadMask();
        auto expectConfig = STFTest::Egress((*ePacket)->getPort(), payload, payloadMask);
        expectConfig.print(stfFile);
    }
}

void STF::outputTest(const TestSpec* testSpec, cstring selectedBranches, size_t testIdx,
                     float currentCoverage) {
    std::ofstream stfFile(testName + "_" + std::to_string(testIdx) + ".stf");
    comment(stfFile, "STF test for " + testName);
    comment(stfFile, "p4testgen seed: " + boost::lexical_cast<std::string>(seed));
    if (selectedBranches != "") {
        comment(stfFile, selectedBranches);
    }
    comment(stfFile, "Date generated: " + TestgenUtils::getTimeStamp());
    std::stringstream coverageStr;
    coverageStr << "Current statement coverage: " << std::setprecision(2) << currentCoverage;
    comment(stfFile, coverageStr.str());
    trace(stfFile, testSpec);
    add(stfFile, testSpec);

    // Check whether this test has a clone configuration.
    // These are special because they require additional instrumentation and produce two output
    // packets.
    auto cloneInfos = testSpec->getTestObjectCategory("clone_infos");
    if (!cloneInfos.empty()) {
        clone(stfFile, testSpec, cloneInfos);
    } else {
        packet(stfFile, testSpec);
        expect(stfFile, testSpec);
    }

    // Temporarily disabled because of a bug in the test harness.
    // wait(stfFile);
    stfFile.flush();
    stfFile.close();
}

}  // namespace Bmv2

}  // namespace P4Testgen

}  // namespace P4Tools
