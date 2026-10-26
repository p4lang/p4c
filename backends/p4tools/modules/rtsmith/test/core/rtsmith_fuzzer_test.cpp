#include "backends/p4tools/modules/rtsmith/core/control_plane/protobuf_utils.h"
#include "backends/p4tools/modules/rtsmith/targets/bmv2/fuzzer.h"
#include "backends/p4tools/modules/rtsmith/test/core/rtsmith_test.h"

namespace P4::P4Tools::Test {
namespace {

class RtSmithFuzzerTest : public RtSmithTest {};

TEST_F(RtSmithFuzzerTest, ValidMatchesAndStatefulUpdates) {
    auto context = SetUp("bmv2", "v1model");
    ASSERT_TRUE(context);
    auto &options = RtSmith::RtSmithOptions::get();
    options.target = "bmv2"_cs;
    options.arch = "v1model"_cs;
    auto source = generateTestProgram("apply {}");
    auto compiler = RtSmith::RtSmith::generateCompilerResult(source, options);
    ASSERT_TRUE(compiler);
    if (!Utils::getCurrentSeed()) Utils::setRandomSeed(1);
    p4::config::v1::P4Info info;
    for (int id = 1; id <= 2; ++id) info.add_actions()->mutable_preamble()->set_id(id);
    auto *table = info.add_tables();
    table->mutable_preamble()->set_id(1);
    table->mutable_preamble()->set_name("ingress.t");
    table->set_size(3);
    table->add_action_refs()->set_id(1);
    auto *defaultAction = table->add_action_refs();
    defaultAction->set_id(2);
    defaultAction->set_scope(p4::config::v1::ActionRef::DEFAULT_ONLY);
    for (auto kind : {p4::config::v1::MatchField::EXACT, p4::config::v1::MatchField::LPM,
                      p4::config::v1::MatchField::TERNARY, p4::config::v1::MatchField::RANGE,
                      p4::config::v1::MatchField::OPTIONAL}) {
        auto *field = table->add_match_fields();
        field->set_id(table->match_fields_size());
        field->set_bitwidth(9);
        field->set_match_type(kind);
    }
    RtSmith::V1Model::Bmv2V1ModelProgramInfo program(*compiler, P4::P4RuntimeAPI(&info, nullptr));
    RtSmith::V1Model::Bmv2V1ModelFuzzer fuzzer(program);
    std::map<std::string, p4::v1::TableEntry> state;
    int inserts = 0, modifications = 0, deletions = 0;
    using RtSmith::Protobuf::stringToBigInt;
    for (int iteration = 0; iteration < 100; ++iteration) {
        auto request = fuzzer.produceWriteRequest(iteration == 0);
        for (const auto &update : request->updates()) {
            const auto &entry = update.entity().table_entry();
            EXPECT_GT(entry.priority(), 0);
            bool hasExact = false;
            for (const auto &match : entry.match()) {
                if (match.has_exact()) hasExact = true;
                if (match.has_lpm()) {
                    EXPECT_GT(match.lpm().prefix_len(), 0);
                    EXPECT_LE(match.lpm().prefix_len(), 9);
                    EXPECT_EQ(stringToBigInt(match.lpm().value()) &
                                  ((big_int(1) << (9 - match.lpm().prefix_len())) - 1),
                              0);
                }
                if (match.has_ternary()) {
                    auto mask = stringToBigInt(match.ternary().mask());
                    auto value = stringToBigInt(match.ternary().value());
                    EXPECT_NE(mask, 0);
                    EXPECT_EQ(value & mask, value);
                }
                if (match.has_range()) {
                    auto low = stringToBigInt(match.range().low());
                    auto high = stringToBigInt(match.range().high());
                    EXPECT_LE(low, high);
                    EXPECT_FALSE(low == 0 && high == 511);
                }
            }
            EXPECT_TRUE(hasExact);
            auto key = entry;
            key.clear_action();
            auto serialized = key.SerializeAsString();
            if (update.type() == p4::v1::Update::INSERT) {
                EXPECT_TRUE(state.emplace(serialized, entry).second);
                ++inserts;
            } else if (update.type() == p4::v1::Update::MODIFY) {
                EXPECT_EQ(state.count(serialized), 1U);
                ++modifications;
            } else {
                EXPECT_EQ(state.erase(serialized), 1U);
                ++deletions;
            }
            if (update.type() != p4::v1::Update::DELETE) {
                EXPECT_EQ(entry.action().action().action_id(), 1U);
            }
            EXPECT_LE(state.size(), 3U);
        }
    }
    EXPECT_GT(inserts, 0);
    EXPECT_GT(modifications, 0);
    EXPECT_GT(deletions, 0);
}

TEST_F(RtSmithFuzzerTest, HonorsTableSelection) {
    auto context = SetUp("bmv2", "v1model");
    ASSERT_TRUE(context);
    auto &options = RtSmith::RtSmithOptions::get();
    options.target = "bmv2"_cs;
    options.arch = "v1model"_cs;
    auto source = generateTestProgram(R"(
        action a() {}
        table t { key = { hdr.eth_hdr.dst_addr : exact; } actions = { a; } size = 4; }
        apply { t.apply(); }
    )");
    for (const auto *config : {"maxTables = 0", "tablesToSkip = ['ingress.t']"}) {
        options.setFuzzerConfigString(config);
        auto result = RtSmith::RtSmith::generateConfig(source, options);
        ASSERT_TRUE(result);
        for (const auto &message : result->config) {
            p4::v1::WriteRequest request;
            ASSERT_TRUE(request.ParseFromString(message->SerializeAsString()));
            EXPECT_EQ(request.updates_size(), 0);
        }
    }
}

}  // namespace
}  // namespace P4::P4Tools::Test
