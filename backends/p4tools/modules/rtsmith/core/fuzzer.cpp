#include "backends/p4tools/modules/rtsmith/core/fuzzer.h"

#include <algorithm>
#include <limits>

#include "backends/p4tools/common/lib/util.h"
#include "backends/p4tools/modules/rtsmith/core/control_plane/protobuf_utils.h"
#include "control-plane/bytestrings.h"
#include "control-plane/p4infoApi.h"

namespace P4::P4Tools::RtSmith {

p4::v1::FieldMatch_Exact P4RuntimeFuzzer::produceFieldMatch_Exact(int bitwidth) {
    p4::v1::FieldMatch_Exact protoExact;
    protoExact.set_value(produceBytes(bitwidth));
    return protoExact;
}

p4::v1::FieldMatch_LPM P4RuntimeFuzzer::produceFieldMatch_LPM(int bitwidth) {
    p4::v1::FieldMatch_LPM protoLPM;
    const auto prefix = Utils::getRandInt(0, bitwidth);
    const auto shift = bitwidth - prefix;
    big_int value = Utils::getRandConstantForWidth(bitwidth)->value;
    value = (value >> shift) << shift;
    protoLPM.set_value(produceBytes(bitwidth, value));
    protoLPM.set_prefix_len(prefix);
    return protoLPM;
}

p4::v1::FieldMatch_Ternary P4RuntimeFuzzer::produceFieldMatch_Ternary(int bitwidth) {
    p4::v1::FieldMatch_Ternary protoTernary;
    const big_int mask = Utils::getRandConstantForWidth(bitwidth)->value;
    const big_int value = Utils::getRandConstantForWidth(bitwidth)->value & mask;
    protoTernary.set_value(produceBytes(bitwidth, value));
    protoTernary.set_mask(produceBytes(bitwidth, mask));
    return protoTernary;
}

p4::v1::FieldMatch_Range P4RuntimeFuzzer::produceFieldMatch_Range(int bitwidth) {
    p4::v1::FieldMatch_Range protoRange;
    const auto &highValue = Utils::getRandConstantForWidth(bitwidth)->value;
    auto low = produceBytes(bitwidth, /*min=*/0, /*max=*/highValue);
    auto high = checkBigIntToString(highValue, bitwidth);
    protoRange.set_low(low);
    protoRange.set_high(high);
    return protoRange;
}

p4::v1::FieldMatch_Optional P4RuntimeFuzzer::produceFieldMatch_Optional(int bitwidth) {
    p4::v1::FieldMatch_Optional protoOptional;
    protoOptional.set_value(produceBytes(bitwidth));
    return protoOptional;
}

p4::v1::Action_Param P4RuntimeFuzzer::produceActionParam(
    const p4::config::v1::Action_Param &param) {
    p4::v1::Action_Param protoParam;
    protoParam.set_param_id(param.id());
    protoParam.set_value(produceBytes(param.bitwidth()));
    return protoParam;
}

p4::v1::Action P4RuntimeFuzzer::produceTableAction(
    const google::protobuf::RepeatedPtrField<p4::config::v1::ActionRef> &action_refs,
    const google::protobuf::RepeatedPtrField<p4::config::v1::Action> &actions) {
    p4::v1::Action protoAction;

    std::vector<uint32_t> eligibleActions;
    for (const auto &ref : action_refs) {
        if (ref.scope() != p4::config::v1::ActionRef::DEFAULT_ONLY) {
            eligibleActions.push_back(ref.id());
        }
    }
    BUG_CHECK(!eligibleActions.empty(), "Table has no action available for entries");
    auto action_ref_id = eligibleActions.at(Utils::getRandInt(eligibleActions.size() - 1));

    const auto *action =
        P4::ControlPlaneAPI::findP4InfoObject(actions.begin(), actions.end(), action_ref_id);
    BUG_CHECK(action != nullptr, "Action %1% missing from P4Info", action_ref_id);
    auto action_id = action->preamble().id();

    protoAction.set_action_id(action_id);
    for (const auto &param : action->params()) {
        protoAction.add_params()->CopyFrom(produceActionParam(param));
    }

    return protoAction;
}

uint32_t P4RuntimeFuzzer::producePriority(
    const google::protobuf::RepeatedPtrField<p4::config::v1::MatchField> &matchFields) {
    for (const auto &match : matchFields) {
        auto matchType = match.match_type();
        if (matchType == p4::config::v1::MatchField::TERNARY ||
            matchType == p4::config::v1::MatchField::RANGE ||
            matchType == p4::config::v1::MatchField::OPTIONAL) {
            return Utils::getRandInt(1, std::numeric_limits<int32_t>::max());
        }
    }

    return 0;
}

p4::v1::FieldMatch P4RuntimeFuzzer::produceMatchField(p4::config::v1::MatchField &match) {
    p4::v1::FieldMatch protoMatch;
    protoMatch.set_field_id(match.id());

    auto matchType = match.match_type();
    auto bitwidth = match.bitwidth();

    switch (matchType) {
        case p4::config::v1::MatchField::EXACT:
            protoMatch.mutable_exact()->CopyFrom(produceFieldMatch_Exact(bitwidth));
            break;
        case p4::config::v1::MatchField::LPM:
            protoMatch.mutable_lpm()->CopyFrom(produceFieldMatch_LPM(bitwidth));
            break;
        case p4::config::v1::MatchField::TERNARY:
            protoMatch.mutable_ternary()->CopyFrom(produceFieldMatch_Ternary(bitwidth));
            break;
        case p4::config::v1::MatchField::RANGE:
            protoMatch.mutable_range()->CopyFrom(produceFieldMatch_Range(bitwidth));
            break;
        case p4::config::v1::MatchField::OPTIONAL:
            protoMatch.mutable_optional()->CopyFrom(produceFieldMatch_Optional(bitwidth));
            break;
        default:
            P4C_UNIMPLEMENTED("Match type %1% not supported for P4RuntimeFuzzer yet",
                              p4::config::v1::MatchField::MatchType_Name(matchType));
    }
    return protoMatch;
}

p4::v1::TableEntry P4RuntimeFuzzer::produceTableEntry(
    const p4::config::v1::Table &table,
    const google::protobuf::RepeatedPtrField<p4::config::v1::Action> &actions) {
    p4::v1::TableEntry protoEntry;

    // set table id
    const auto &pre_t = table.preamble();
    protoEntry.set_table_id(pre_t.id());

    // add matches
    const auto &matchFields = table.match_fields();
    for (auto match : matchFields) {
        auto field = produceMatchField(match);
        // P4Runtime represents wildcard matches by omitting the entire field.
        if ((field.has_lpm() && field.lpm().prefix_len() == 0) ||
            (field.has_ternary() && Protobuf::stringToBigInt(field.ternary().mask()) == 0) ||
            (field.has_range() && Protobuf::stringToBigInt(field.range().low()) == 0 &&
             Protobuf::stringToBigInt(field.range().high()) ==
                 (big_int(1) << match.bitwidth()) - 1)) {
            continue;
        }
        protoEntry.add_match()->CopyFrom(field);
    }

    // set priority
    auto priority = producePriority(matchFields);
    protoEntry.set_priority(priority);

    // add action
    const auto &action_refs = table.action_refs();
    auto protoAction = produceTableAction(action_refs, actions);
    *protoEntry.mutable_action()->mutable_action() = protoAction;
    return protoEntry;
}

std::unique_ptr<p4::v1::WriteRequest> P4RuntimeFuzzer::produceWriteRequest(bool isInitialConfig) {
    const auto *p4Info = getProgramInfo().getP4RuntimeApi().p4Info;

    const auto &config = getProgramInfo().getFuzzerConfig();
    const auto &skippedTables = config.getTablesToSkip();
    auto request = std::make_unique<p4::v1::WriteRequest>();
    int tableCount = 0;
    for (const auto &table : p4Info->tables()) {
        const auto &name = table.preamble().name();
        // Indirect and preinitialized tables need additional state we do not model yet.
        if (table.match_fields_size() == 0 || table.is_const_table() ||
            table.has_initial_entries() || table.implementation_id() != 0 ||
            std::find(skippedTables.begin(), skippedTables.end(), name) != skippedTables.end() ||
            std::none_of(table.action_refs().begin(), table.action_refs().end(),
                         [](const auto &ref) {
                             return ref.scope() != p4::config::v1::ActionRef::DEFAULT_ONLY;
                         })) {
            continue;
        }
        if (tableCount >= config.getMaxTables()) break;
        ++tableCount;
        auto &entries = currentState[name];
        int count = 0;
        for (int attempts = 0;
             count < config.getMaxEntryGenCnt() && attempts < config.getMaxAttempts(); ++attempts) {
            // Select existing entries explicitly: collisions are vanishingly rare for wide keys.
            if (!isInitialConfig && !entries.empty() &&
                (entries.size() >= static_cast<size_t>(table.size()) || Utils::getRandInt(0, 1))) {
                auto existing = entries.begin();
                std::advance(existing, Utils::getRandInt(entries.size() - 1));
                auto entry = existing->second;
                auto *update = request->add_updates();
                if (static_cast<uint64_t>(Utils::getRandInt(0, 99)) <
                    config.getThresholdForDeletion()) {
                    update->set_type(p4::v1::Update::DELETE);
                    entry.clear_action();
                    entries.erase(existing);
                } else {
                    update->set_type(p4::v1::Update::MODIFY);
                    *entry.mutable_action()->mutable_action() =
                        produceTableAction(table.action_refs(), p4Info->actions());
                    existing->second = entry;
                }
                *update->mutable_entity()->mutable_table_entry() = entry;
                ++count;
                continue;
            }
            if (entries.size() >= static_cast<size_t>(table.size())) break;
            auto entry = produceTableEntry(table, p4Info->actions());
            auto key = entry;
            key.clear_action();
            // Priority is part of a P4Runtime entry's identity.
            if (!entries.emplace(key.SerializeAsString(), entry).second) continue;
            auto *update = request->add_updates();
            update->set_type(p4::v1::Update::INSERT);
            *update->mutable_entity()->mutable_table_entry() = entry;
            ++count;
        }
    }
    return request;
}

/// Some Helper functions below

std::string RuntimeFuzzer::checkBigIntToString(const big_int &value, int bitwidth) {
    std::optional<std::string> valueStr = P4::ControlPlaneAPI::stringReprConstant(value, bitwidth);
    BUG_CHECK(valueStr.has_value(), "Failed to check %1% to string, maybe value < 0?", value.str());
    return valueStr.value();
}

std::string RuntimeFuzzer::produceBytes(int bitwidth) {
    auto value = Utils::getRandConstantForWidth(bitwidth)->value;
    return checkBigIntToString(value, bitwidth);
}

std::string RuntimeFuzzer::produceBytes(int bitwidth, const big_int &value) {
    return checkBigIntToString(value, bitwidth);
}

std::string RuntimeFuzzer::produceBytes(int bitwidth, const big_int &min, const big_int &max) {
    auto value = Utils::getRandBigInt(min, max);
    return checkBigIntToString(value, bitwidth);
}

bool P4RuntimeFuzzer::tableHasFieldType(const p4::config::v1::Table &table,
                                        const p4::config::v1::MatchField::MatchType type) {
    for (const auto &match : table.match_fields()) {
        if (match.match_type() == type) {
            return true;
        }
    }
    return false;
}

}  // namespace P4::P4Tools::RtSmith
