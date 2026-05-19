#include "backends/p4tools/modules/rtsmith/core/fuzzer.h"

#include "backends/p4tools/common/lib/util.h"
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
    protoLPM.set_value(produceBytes(bitwidth));
    protoLPM.set_prefix_len(Utils::getRandInt(0, bitwidth));
    return protoLPM;
}

p4::v1::FieldMatch_Ternary P4RuntimeFuzzer::produceFieldMatch_Ternary(int bitwidth) {
    p4::v1::FieldMatch_Ternary protoTernary;
    protoTernary.set_value(produceBytes(bitwidth));
    protoTernary.set_mask(produceBytes(bitwidth));
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

    auto action_index = Utils::getRandInt(action_refs.size() - 1);
    auto action_ref_id = action_refs[action_index].id();

    const auto *action =
        P4::ControlPlaneAPI::findP4InfoObject(actions.begin(), actions.end(), action_ref_id);
    auto action_id = action->preamble().id();
    auto params = action->params();

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
            auto value = Utils::getRandConstantForWidth(32)->value;
            auto priority = static_cast<int>(value);
            return priority;
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
        protoEntry.add_match()->CopyFrom(produceMatchField(match));
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

    const auto tables = p4Info->tables();
    const auto actions = p4Info->actions();

    auto tableCnt = tables.size();
    auto request = std::make_unique<p4::v1::WriteRequest>();
    for (auto tableId = 0; tableId < tableCnt; tableId++) {
        const auto &table = tables.Get(tableId);
        // NOTE: Temporary use a coin to decide if generating entries for the table.
        // Make this configurable.
        if (table.match_fields_size() == 0 || table.is_const_table() ||
            Utils::getRandInt(0, 4) == 0) {
            continue;
        }

        auto maxEntryGenCnt = getProgramInfo().getFuzzerConfig().getMaxEntryGenCnt();
        int attempts = 0;
        // Try to keep track of the entries we have generated so far.
        int count = 0;
        // Retrieve the current table configuration.
        auto &currentTableConfiguration = currentState[table.preamble().name()];
        while (count < maxEntryGenCnt) {
            if (attempts > getProgramInfo().getFuzzerConfig().getMaxAttempts()) {
                warning("Failed to generate %d entries for table %s", maxEntryGenCnt,
                        table.preamble().name());
                break;
            }
            auto entry = produceTableEntry(table, actions);
            // TODO: This is inefficient, but currently works.
            std::stringstream matchFieldStream;
            for (const auto &match : entry.match()) {
                matchFieldStream << match.DebugString();
            }
            auto matchFieldString = matchFieldStream.str();
            // Only insert unique entries that actually insert.
            if (currentTableConfiguration.find(matchFieldString) ==
                currentTableConfiguration.end()) {
                auto *update = request->add_updates();
                update->set_type(p4::v1::Update_Type::Update_Type_INSERT);
                update->mutable_entity()->mutable_table_entry()->CopyFrom(entry);
                count++;
                currentTableConfiguration.insert(matchFieldString);
            } else if (!isInitialConfig) {
                // In case of an initial config we may update or delete entries.
                auto *update = request->add_updates();
                // Whether we update or delete the entry is determined randomly.
                auto thresholdForDeletion =
                    getProgramInfo().getFuzzerConfig().getThresholdForDeletion();
                auto updateOrNot = Utils::getRandInt(100) >= thresholdForDeletion;
                if (updateOrNot) {
                    update->set_type(p4::v1::Update_Type::Update_Type_MODIFY);
                } else {
                    update->set_type(p4::v1::Update_Type::Update_Type_DELETE);
                    currentTableConfiguration.erase(matchFieldString);
                }
                update->mutable_entity()->mutable_table_entry()->CopyFrom(entry);
                count++;
            }
            attempts++;
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

bool RuntimeFuzzer::tableHasFieldType(const p4::config::v1::Table &table,
                                      const p4::config::v1::MatchField::MatchType type) {
    for (const auto &match : table.match_fields()) {
        if (match.match_type() == type) {
            return true;
        }
    }
    return false;
}

}  // namespace P4::P4Tools::RtSmith
