#include "backends/p4tools/modules/rtsmith/targets/tofino/fuzzer.h"

#include <algorithm>

#include "backends/p4tools/common/lib/util.h"
#include "backends/p4tools/modules/rtsmith/core/fuzzer.h"
#include "control-plane/p4infoApi.h"

namespace P4::P4Tools::RtSmith::Tna {

TofinoTnaFuzzer::TofinoTnaFuzzer(const TofinoTnaProgramInfo &programInfo)
    : RuntimeFuzzer(programInfo) {}

const TofinoTnaProgramInfo &TofinoTnaFuzzer::getProgramInfo() const {
    return *RuntimeFuzzer::getProgramInfo().checkedTo<TofinoTnaProgramInfo>();
}

bfrt_proto::KeyField_Exact TofinoTnaFuzzer::produceKeyField_Exact(int bitwidth) {
    bfrt_proto::KeyField_Exact protoExact;
    protoExact.set_value(produceBytes(bitwidth));
    return protoExact;
}

bfrt_proto::KeyField_LPM TofinoTnaFuzzer::produceKeyField_LPM(int bitwidth) {
    bfrt_proto::KeyField_LPM protoLPM;
    protoLPM.set_value(produceBytes(bitwidth));
    protoLPM.set_prefix_len(Utils::getRandInt(0, bitwidth));
    return protoLPM;
}

bfrt_proto::KeyField_Ternary TofinoTnaFuzzer::produceKeyField_Ternary(int bitwidth) {
    bfrt_proto::KeyField_Ternary protoTernary;
    protoTernary.set_value(produceBytes(bitwidth));
    /// TODO: use 0 for mask for now because setting mask to random value may not make sense.
    protoTernary.set_mask(produceBytes(bitwidth, 0));
    return protoTernary;
}

bfrt_proto::KeyField_Range TofinoTnaFuzzer::produceKeyField_Range(int bitwidth) {
    bfrt_proto::KeyField_Range protoRange;
    const auto &highValue = Utils::getRandConstantForWidth(bitwidth)->value;
    auto low = produceBytes(bitwidth, /*min=*/0, /*max=*/highValue);
    auto high = checkBigIntToString(highValue, bitwidth);
    protoRange.set_low(low);
    protoRange.set_high(high);
    return protoRange;
}

bfrt_proto::KeyField_Optional TofinoTnaFuzzer::produceKeyField_Optional(int bitwidth) {
    bfrt_proto::KeyField_Optional protoOptional;
    protoOptional.set_value(produceBytes(bitwidth));
    return protoOptional;
}

bfrt_proto::DataField TofinoTnaFuzzer::produceDataField(const p4::config::v1::Action_Param &param) {
    bfrt_proto::DataField protoDataField;
    protoDataField.set_field_id(param.id());
    protoDataField.set_stream(produceBytes(param.bitwidth()));
    return protoDataField;
}

bfrt_proto::TableData TofinoTnaFuzzer::produceTableData(
    const google::protobuf::RepeatedPtrField<p4::config::v1::ActionRef> &action_refs,
    const google::protobuf::RepeatedPtrField<p4::config::v1::Action> &actions) {
    bfrt_proto::TableData protoTableData;

    auto action_index = Utils::getRandInt(action_refs.size() - 1);
    auto action_ref_id = action_refs[action_index].id();

    auto action =
        P4::ControlPlaneAPI::findP4InfoObject(actions.begin(), actions.end(), action_ref_id);
    protoTableData.set_action_id(action->preamble().id());
    for (auto &param : action->params()) {
        *protoTableData.add_fields() = produceDataField(param);
    }

    return protoTableData;
}

bfrt_proto::KeyField TofinoTnaFuzzer::produceKeyField(const p4::config::v1::MatchField &match) {
    bfrt_proto::KeyField protoKeyField;
    protoKeyField.set_field_id(match.id());
    auto matchType = match.match_type();
    auto bitwidth = match.bitwidth();

    switch (matchType) {
        case p4::config::v1::MatchField::EXACT:
            protoKeyField.mutable_exact()->CopyFrom(produceKeyField_Exact(bitwidth));
            break;
        case p4::config::v1::MatchField::LPM:
            protoKeyField.mutable_lpm()->CopyFrom(produceKeyField_LPM(bitwidth));
            break;
        case p4::config::v1::MatchField::TERNARY:
            protoKeyField.mutable_ternary()->CopyFrom(produceKeyField_Ternary(bitwidth));
            break;
        case p4::config::v1::MatchField::RANGE:
            protoKeyField.mutable_range()->CopyFrom(produceKeyField_Range(bitwidth));
            break;
        case p4::config::v1::MatchField::OPTIONAL:
            protoKeyField.mutable_optional()->CopyFrom(produceKeyField_Optional(bitwidth));
            break;
        default:
            P4C_UNIMPLEMENTED("Match type %1% not supported for TofinoTnaFuzzer yet",
                              p4::config::v1::MatchField::MatchType_Name(matchType));
    }
    return protoKeyField;
}

bfrt_proto::TableEntry TofinoTnaFuzzer::produceTableEntry(
    const p4::config::v1::Table &table,
    const google::protobuf::RepeatedPtrField<p4::config::v1::Action> &actions) {
    bfrt_proto::TableEntry protoEntry;

    // set table id
    const auto &pre_t = table.preamble();
    protoEntry.set_table_id(pre_t.id());

    // add matches
    const auto &matchFields = table.match_fields();
    for (auto i = 0; i < matchFields.size(); i++) {
        auto match = matchFields[i];
        protoEntry.mutable_key()->add_fields()->CopyFrom(produceKeyField(matchFields[i]));
    }

    // add action
    const auto &action_refs = table.action_refs();
    protoEntry.mutable_data()->CopyFrom(produceTableData(action_refs, actions));

    return protoEntry;
}

InitialConfig TofinoTnaFuzzer::produceInitialConfig() {
    auto request = std::make_unique<bfrt_proto::WriteRequest>();

    auto p4Info = getProgramInfo().getP4RuntimeApi().p4Info;

    /// TODO: for Tofino, we also need to look at externs instances for
    /// ActionSelector, ActionProfile and so on.
    const auto tables = p4Info->tables();
    const auto actions = p4Info->actions();

    auto tableCnt = tables.size();

    for (auto tableId = 0; tableId < tableCnt; tableId++) {
        /// NOTE: temporary use a coin to decide if generating entries for the table
        if (Utils::getRandInt(0, 1) == 0) {
            continue;
        }
        auto table = tables.Get(tableId);
        if (table.match_fields_size() == 0 || table.is_const_table()) {
            continue;
        }
        /// TODO: remove this `min`. It is for ease of debugging now.
        auto maxEntryGenCnt = std::min(table.size(), (int64_t)4);
        std::set<std::string> matchFields;
        for (auto i = 0; i < maxEntryGenCnt; i++) {
            auto entry = produceTableEntry(table, actions);
            std::string stringKey = entry.key().SerializeAsString();
            if (matchFields.find(stringKey) == matchFields.end()) {
                /// Only insert unique entries
                auto update = request->add_updates();
                /// TODO: add support for other types.
                update->set_type(bfrt_proto::Update_Type::Update_Type_INSERT);
                matchFields.insert(std::move(stringKey));
                update->mutable_entity()->mutable_table_entry()->CopyFrom(entry);
            }
        }
    }

    InitialConfig initialConfig;
    initialConfig.push_back(std::move(request));
    return initialConfig;
}

UpdateSeries TofinoTnaFuzzer::produceUpdateTimeSeries() { return {}; }

}  // namespace P4::P4Tools::RtSmith::Tna
