/*
Copyright 2013-present Barefoot Networks, Inc.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#include "control-plane/p4/config/p4info.pb.h"
#include "control-plane/p4/config/v1model.pb.h"
#include "control-plane/p4/p4runtime.pb.h"
#include "gtest/gtest.h"

#include "control-plane/p4RuntimeSerializer.h"
#include "helpers.h"
#include "ir/ir.h"
#include "PI/pi_base.h"

namespace Test {

namespace {

boost::optional<P4::P4RuntimeAPI>
createP4RuntimeTestCase(const std::string& source,
                        CompilerOptions::FrontendVersion langVersion
                            = CompilerOptions::FrontendVersion::P4_16) {
    auto frontendTestCase = FrontendTestCase::create(source, langVersion);
    if (!frontendTestCase) return boost::none;
    return P4::generateP4Runtime(frontendTestCase->program);
}

/// @return the P4Runtime representation of the table with the given name, or
/// null if none is found.
const ::p4::config::Table* findTable(const P4::P4RuntimeAPI& analysis,
                                     const std::string& name) {
    auto& tables = analysis.p4Info->tables();
    auto desiredTable = std::find_if(tables.begin(), tables.end(),
                                     [&](const ::p4::config::Table& table) {
        return table.preamble().name() == name;
    });
    if (desiredTable == tables.end()) return nullptr;
    return &*desiredTable;
}

/// @return the P4Runtime representation of the extern with the given name, or
/// null if none is found.
const ::p4::config::Extern* findExtern(const P4::P4RuntimeAPI& analysis,
                                       const std::string& name) {
    auto& externs = analysis.p4Info->externs();
    auto desiredExtern = std::find_if(externs.begin(), externs.end(),
                                      [&](const ::p4::config::Extern& e) {
        return e.extern_type_name() == name;
    });
    if (desiredExtern == externs.end()) return nullptr;
    return &*desiredExtern;
}

/// @return the P4Runtime representation of the action with the given name, or
/// null if none is found.
const ::p4::config::Action* findAction(const P4::P4RuntimeAPI& analysis,
                                       const std::string& name) {
    auto& actions = analysis.p4Info->actions();
    auto desiredAction = std::find_if(actions.begin(), actions.end(),
                                     [&](const ::p4::config::Action& action) {
        return action.preamble().name() == name;
    });
    if (desiredAction == actions.end()) return nullptr;
    return &*desiredAction;
}

/// @return the P4Runtime representation of the value set with the given name,
/// or null if none is found.
const ::p4::config::ValueSet* findValueSet(const P4::P4RuntimeAPI& analysis,
                                           const std::string& name) {
    auto& vsets = analysis.p4Info->value_sets();
    auto desiredVSet = std::find_if(vsets.begin(), vsets.end(),
                                    [&](const ::p4::config::ValueSet& vset) {
        return vset.preamble().name() == name;
    });
    if (desiredVSet == vsets.end()) return nullptr;
    return &*desiredVSet;
}

}  // namespace

class P4Runtime : public P4CTest { };

TEST_F(P4Runtime, IdAssignment) {
    auto test = createP4RuntimeTestCase(P4_SOURCE(P4Headers::V1MODEL, R"(
        struct Headers { }
        struct Metadata { }
        parser parse(packet_in p, out Headers h, inout Metadata m,
                     inout standard_metadata_t sm) {
            state start { transition accept; } }
        control verifyChecksum(inout Headers h, inout Metadata m) { apply { } }
        control egress(inout Headers h, inout Metadata m,
                        inout standard_metadata_t sm) { apply { } }
        control computeChecksum(inout Headers h, inout Metadata m) { apply { } }
        control deparse(packet_out p, in Headers h) { apply { } }

        control ingress(inout Headers h, inout Metadata m,
                        inout standard_metadata_t sm) {
            action noop() { }

            table igTable {
                actions = { noop; }
                default_action = noop;
            }

            @name("igTableWithName")
            table igTableWithoutName {
                actions = { noop; }
                default_action = noop;
            }

            @id(1234)
            table igTableWithId {
                actions = { noop; }
                default_action = noop;
            }

            @id(5678)
            @name("igTableWithNameAndId")
            table igTableWithoutNameAndId {
                actions = { noop; }
                default_action = noop;
            }

            @id(4321)
            table conflictingTableA {
                actions = { noop; }
                default_action = noop;
            }

            @id(4321)
            table conflictingTableB {
                actions = { noop; }
                default_action = noop;
            }

            apply {
                igTable.apply();
                igTableWithoutName.apply();
                igTableWithId.apply();
                igTableWithoutNameAndId.apply();
                conflictingTableA.apply();
                conflictingTableB.apply();
            }
        }

        V1Switch(parse(), verifyChecksum(), ingress(), egress(),
                 computeChecksum(), deparse()) main;
    )"));

    ASSERT_TRUE(test);

    // We expect exactly one error:
    //   error: @id 4321 is assigned to multiple declarations
    EXPECT_EQ(1u, ::diagnosticCount());

    {
        // Check that 'igTable' ended up in the P4Info output.
        auto* igTable = findTable(*test, "ingress.igTable");
        ASSERT_TRUE(igTable != nullptr);

        // Check that the id indicates the correct resource type.
        EXPECT_EQ(unsigned(PI_TABLE_ID), igTable->preamble().id() >> 24);

        // Check that the rest of the id matches the hash value that we expect.
        // (If we were to ever change the hash algorithm we use when mapping P4
        // names to P4Runtime ids, we'd need to change this test.)
        EXPECT_EQ(16119u, igTable->preamble().id() & 0x00ffffff);
    }

    {
        // Check that 'igTableWithName' ended up in the P4Info output under that
        // name, which is determined by its @name annotation, and *not* under
        // 'igTableWithoutName'.
        EXPECT_TRUE(findTable(*test, "ingress.igTableWithoutName") == nullptr);
        auto* igTableWithName = findTable(*test, "ingress.igTableWithName");
        ASSERT_TRUE(igTableWithName != nullptr);

        // Check that the id of 'igTableWithName' was computed based on its
        // @name annotation. (See above for caveat re: the hash algorithm.)
        EXPECT_EQ(unsigned(PI_TABLE_ID), igTableWithName->preamble().id() >> 24);
        EXPECT_EQ(59806u, igTableWithName->preamble().id() & 0x00ffffff);
    }

    {
        // Check that 'igTableWithId' ended up in the P4Info output, and that
        // its id matches the one set by its @id annotation.
        auto* igTableWithId = findTable(*test, "ingress.igTableWithId");
        ASSERT_TRUE(igTableWithId != nullptr);
        EXPECT_EQ(1234u, igTableWithId->preamble().id());
    }

    {
        // Check that 'igTableWithNameAndId' ended up in the P4Info output under
        // that name, and that its id matches the one set by its @id annotation
        // - in other words, that @id takes precedence over @name.
        EXPECT_TRUE(findTable(*test, "ingress.igTableWithoutNameAndId") == nullptr);
        auto* igTableWithNameAndId = findTable(*test, "ingress.igTableWithNameAndId");
        ASSERT_TRUE(igTableWithNameAndId != nullptr);
        EXPECT_EQ(5678u, igTableWithNameAndId->preamble().id());
    }

    {
        // Check that the two tables with conflicting ids are both present, and
        // that they didn't end up with the same id in the P4Info output.
        auto* conflictingTableA = findTable(*test, "ingress.conflictingTableA");
        ASSERT_TRUE(conflictingTableA != nullptr);
        auto* conflictingTableB = findTable(*test, "ingress.conflictingTableB");
        ASSERT_TRUE(conflictingTableB != nullptr);
        EXPECT_TRUE(conflictingTableA->preamble().id() == 4321 ||
                    conflictingTableB->preamble().id() == 4321);
        EXPECT_NE(conflictingTableA->preamble().id(),
                  conflictingTableB->preamble().id());
    }
}

namespace {

/// A helper for the match fields tests; represents metadata about a match field
/// that we expect to find in the generated P4Info.
struct ExpectedMatchField {
    unsigned id;
    std::string name;
    int bitWidth;
    ::p4::config::MatchField::MatchType matchType;
};

}  // namespace

TEST_F(P4Runtime, P4_16_MatchFields) {
    using MatchField = ::p4::config::MatchField;

    auto test = createP4RuntimeTestCase(P4_SOURCE(P4Headers::V1MODEL, R"(
        header Header { bit<16> headerField; }
        header AnotherHeader { bit<8> anotherHeaderField; }
        header_union HeaderUnion { Header a; AnotherHeader b; }
        struct Headers { Header h; Header[4] hStack; HeaderUnion hUnion; }
        struct Metadata { bit<33> metadataField; }
        parser parse(packet_in p, out Headers h, inout Metadata m,
                     inout standard_metadata_t sm) {
            state start { transition accept; } }
        control verifyChecksum(inout Headers h, inout Metadata m) { apply { } }
        control egress(inout Headers h, inout Metadata m,
                        inout standard_metadata_t sm) { apply { } }
        control computeChecksum(inout Headers h, inout Metadata m) { apply { } }
        control deparse(packet_out p, in Headers h) { apply { } }

        control ingress(inout Headers h, inout Metadata m,
                        inout standard_metadata_t sm) {
            action noop() { }

            table igTable {
                key = {
                    // Standard match types.
                    h.h.headerField : exact;             // 1
                    m.metadataField : exact;             // 2
                    h.hStack[3].headerField : exact;     // 3
                    h.h.headerField : ternary;           // 4
                    m.metadataField : ternary;           // 5
                    h.hStack[3].headerField : ternary;   // 6
                    h.h.headerField : lpm;               // 7
                    m.metadataField : lpm;               // 8
                    h.hStack[3].headerField : lpm;       // 9
                    h.h.headerField : range;             // 10
                    m.metadataField : range;             // 11
                    h.hStack[3].headerField : range;     // 12

                    // Built-in methods.
                    h.h.isValid() : exact;            // 13
                    h.h.isValid() : ternary;          // 14
                    h.hStack[3].isValid() : exact;    // 15
                    h.hStack[3].isValid() : ternary;  // 16

                    // Simple bitwise operations.
                    h.h.headerField & 13 : exact;     // 17
                    h.h.headerField & 13 : ternary;   // 18
                    h.h.headerField[13:4] : exact;    // 19
                    h.h.headerField[13:4] : ternary;  // 20

                    // Header unions.
                    h.hUnion.a.headerField : exact;           // 21
                    h.hUnion.a.headerField : ternary;         // 22
                    h.hUnion.a.headerField : lpm;             // 23
                    h.hUnion.a.headerField : range;           // 24
                    h.hUnion.b.anotherHeaderField : exact;    // 25
                    h.hUnion.b.anotherHeaderField : ternary;  // 26
                    h.hUnion.b.anotherHeaderField : lpm;      // 27
                    h.hUnion.b.anotherHeaderField : range;    // 28
                    h.hUnion.a.isValid() : exact;             // 29
                    h.hUnion.a.isValid() : ternary;           // 30
                    h.hUnion.b.isValid() : exact;             // 31
                    h.hUnion.b.isValid() : ternary;           // 32
                    h.hUnion.isValid() : exact;               // 33
                    h.hUnion.isValid() : ternary;             // 34

                    // Complex operations which require an @name annotation.
                    h.h.headerField << 13 : exact @name("lShift");    // 35
                    h.h.headerField << 13 : ternary @name("lShift");  // 36
                    h.h.headerField + 6 : exact @name("plusSix");     // 37
                    h.h.headerField + 6 : ternary @name("plusSix");   // 38

                    // Action selectors. These won't be included in the list of
                    // match fields; they're just here as a sanity check.
                    h.h.headerField : selector;          // skipped
                    m.metadataField : selector;          // skipped
                    h.hStack[3].headerField : selector;  // skipped
                }

                actions = { noop; }
                default_action = noop;

                // This is needed for the `selector` match type to be valid.
                @name("action_profile") implementation = action_profile(32w128);
            }

            apply {
                igTable.apply();
            }
        }

        V1Switch(parse(), verifyChecksum(), ingress(), egress(),
                 computeChecksum(), deparse()) main;
    )"));

    ASSERT_TRUE(test);
    EXPECT_EQ(0u, ::diagnosticCount());

    auto* igTable = findTable(*test, "ingress.igTable");
    ASSERT_TRUE(igTable != nullptr);
    EXPECT_EQ(38, igTable->match_fields_size());

    std::vector<ExpectedMatchField> expected = {
        { 1, "h.h.headerField", 16, MatchField::EXACT },
        { 2, "m.metadataField", 33, MatchField::EXACT },
        { 3, "h.hStack[3].headerField", 16, MatchField::EXACT },
        { 4, "h.h.headerField", 16, MatchField::TERNARY },
        { 5, "m.metadataField", 33, MatchField::TERNARY },
        { 6, "h.hStack[3].headerField", 16, MatchField::TERNARY },
        { 7, "h.h.headerField", 16, MatchField::LPM },
        { 8, "m.metadataField", 33, MatchField::LPM },
        { 9, "h.hStack[3].headerField", 16, MatchField::LPM },
        { 10, "h.h.headerField", 16, MatchField::RANGE },
        { 11, "m.metadataField", 33, MatchField::RANGE },
        { 12, "h.hStack[3].headerField", 16, MatchField::RANGE },
        { 13, "h.h.$valid$", 1, MatchField::EXACT },
        { 14, "h.h.$valid$", 1, MatchField::TERNARY },
        { 15, "h.hStack[3].$valid$", 1, MatchField::EXACT },
        { 16, "h.hStack[3].$valid$", 1, MatchField::TERNARY },
        { 17, "h.h.headerField & 13", 16, MatchField::EXACT },
        { 18, "h.h.headerField & 13", 16, MatchField::TERNARY },
        { 19, "h.h.headerField[13:4]", 10, MatchField::EXACT },
        { 20, "h.h.headerField[13:4]", 10, MatchField::TERNARY },
        { 21, "h.hUnion.a.headerField", 16, MatchField::EXACT },
        { 22, "h.hUnion.a.headerField", 16, MatchField::TERNARY },
        { 23, "h.hUnion.a.headerField", 16, MatchField::LPM },
        { 24, "h.hUnion.a.headerField", 16, MatchField::RANGE },
        { 25, "h.hUnion.b.anotherHeaderField", 8, MatchField::EXACT },
        { 26, "h.hUnion.b.anotherHeaderField", 8, MatchField::TERNARY },
        { 27, "h.hUnion.b.anotherHeaderField", 8, MatchField::LPM },
        { 28, "h.hUnion.b.anotherHeaderField", 8, MatchField::RANGE },
        { 29, "h.hUnion.a.$valid$", 1, MatchField::EXACT },
        { 30, "h.hUnion.a.$valid$", 1, MatchField::TERNARY },
        { 31, "h.hUnion.b.$valid$", 1, MatchField::EXACT },
        { 32, "h.hUnion.b.$valid$", 1, MatchField::TERNARY },
        { 33, "h.hUnion.$valid$", 1, MatchField::EXACT },
        { 34, "h.hUnion.$valid$", 1, MatchField::TERNARY },
        { 35, "lShift", 16, MatchField::EXACT },
        { 36, "lShift", 16, MatchField::TERNARY },
        { 37, "plusSix", 16, MatchField::EXACT },
        { 38, "plusSix", 16, MatchField::TERNARY },
    };

    for (auto i = 0; i < igTable->match_fields_size(); i++) {
        SCOPED_TRACE(expected[i].id);
        EXPECT_EQ(expected[i].id, igTable->match_fields(i).id());
        EXPECT_EQ(expected[i].name, igTable->match_fields(i).name());
        EXPECT_EQ(expected[i].bitWidth, igTable->match_fields(i).bitwidth());
        EXPECT_EQ(expected[i].matchType, igTable->match_fields(i).match_type());
    }
}

TEST_F(P4Runtime, P4_14_MatchFields) {
    using MatchField = ::p4::config::MatchField;

    auto test = createP4RuntimeTestCase(P4_SOURCE(P4Headers::NONE, R"(
        header_type header_t { fields { headerField : 16; } }
        header header_t h;
        header header_t hStack[4];
        header_type metadata_t { fields { metadataField : 33; } }
        metadata metadata_t m;
        parser start { return ingress; }
        action noop() { }
        action_profile igTableProfile { actions { noop; } }

        table igTable {
            reads {
                // Standard match types.
                h.headerField : exact;            // 1
                m.metadataField : exact;          // 2
                hStack[3].headerField : exact;    // 3
                h.headerField : ternary;          // 4
                m.metadataField : ternary;        // 5
                hStack[3].headerField : ternary;  // 6
                h.headerField : lpm;              // 7
                m.metadataField : lpm;            // 8
                hStack[3].headerField : lpm;      // 9
                h.headerField : range;            // 10
                m.metadataField : range;          // 11
                hStack[3].headerField : range;    // 12
                h : valid;                        // 13
                hStack[3] : valid;                // 14

                // Masks. There are two variants here, because masks which have
                // a binary representation that is a single, contiguous sequence
                // of bits get converted to slices, while other masks get
                // converted to bitwise AND.
                h.headerField mask 12 : exact;     // 15 (will be a slice)
                h.headerField mask 12 : ternary;   // 16 (will be a slice)
                h.headerField mask 13 : exact;     // 17 (will be an AND)
                h.headerField mask 13 : ternary;   // 18 (will be an AND)
            }

            actions { noop; }
            default_action: noop;

            // This will generate match fields when converted to P4-16. They
            // won't be included in the list of match fields in P4Info; this is
            // just here as a sanity check.
            action_profile: igTableProfile;
        }

        control ingress { apply(igTable); }
    )"), CompilerOptions::FrontendVersion::P4_14);

    ASSERT_TRUE(test);
    EXPECT_EQ(0u, ::diagnosticCount());

    auto* igTable = findTable(*test, "igTable");
    ASSERT_TRUE(igTable != nullptr);
    EXPECT_EQ(18, igTable->match_fields_size());

    std::vector<ExpectedMatchField> expected = {
        { 1, "h.headerField", 16, MatchField::EXACT },
        { 2, "m.metadataField", 33, MatchField::EXACT },
        { 3, "hStack[3].headerField", 16, MatchField::EXACT },
        { 4, "h.headerField", 16, MatchField::TERNARY },
        { 5, "m.metadataField", 33, MatchField::TERNARY },
        { 6, "hStack[3].headerField", 16, MatchField::TERNARY },
        { 7, "h.headerField", 16, MatchField::LPM },
        { 8, "m.metadataField", 33, MatchField::LPM },
        { 9, "hStack[3].headerField", 16, MatchField::LPM },
        { 10, "h.headerField", 16, MatchField::RANGE },
        { 11, "m.metadataField", 33, MatchField::RANGE },
        { 12, "hStack[3].headerField", 16, MatchField::RANGE },
        { 13, "h.$valid$", 1, MatchField::EXACT },
        { 14, "hStack[3].$valid$", 1, MatchField::EXACT },
        { 15, "h.headerField[3:2]", 2, MatchField::EXACT },
        { 16, "h.headerField[3:2]", 2, MatchField::TERNARY },
        { 17, "h.headerField & 13", 16, MatchField::EXACT },
        { 18, "h.headerField & 13", 16, MatchField::TERNARY },
    };

    for (auto i = 0; i < igTable->match_fields_size(); i++) {
        SCOPED_TRACE(expected[i].id);
        EXPECT_EQ(expected[i].id, igTable->match_fields(i).id());
        EXPECT_EQ(expected[i].name, igTable->match_fields(i).name());
        EXPECT_EQ(expected[i].bitWidth, igTable->match_fields(i).bitwidth());
        EXPECT_EQ(expected[i].matchType, igTable->match_fields(i).match_type());
    }
}

TEST_F(P4Runtime, Digests) {
    auto test = createP4RuntimeTestCase(P4_SOURCE(P4Headers::V1MODEL, R"(
        header Header { bit<16> headerFieldA; bit<8> headerFieldB; }
        struct Headers { Header h; }
        struct Metadata { bit<3> metadataFieldA; bit<1> metadataFieldB; }

        parser parse(packet_in p, out Headers h, inout Metadata m,
                     inout standard_metadata_t sm) {
            state start { transition accept; } }
        control verifyChecksum(inout Headers h, inout Metadata m) { apply { } }
        control egress(inout Headers h, inout Metadata m,
                        inout standard_metadata_t sm) { apply { } }
        control computeChecksum(inout Headers h, inout Metadata m) { apply { } }
        control deparse(packet_out p, in Headers h) { apply { } }

        control ingress(inout Headers h, inout Metadata m,
                        inout standard_metadata_t sm) {
            action generateDigest() {
                // digest<T>() where T is a header.
                digest(1, h.h);
                // digest<T>() where T is a struct.
                digest(2, m);
                // digest<T>() where T is a tuple.
                // XXX(seth): Right now this requires that the EliminateTuples
                // pass has run; we may want to change that.
                digest(3, { h.h.headerFieldA, m.metadataFieldB });
            }
            apply {
                generateDigest();
            }
        }

        V1Switch(parse(), verifyChecksum(), ingress(), egress(),
                 computeChecksum(), deparse()) main;
    )"));

    ASSERT_TRUE(test);
    EXPECT_EQ(0u, ::diagnosticCount());

    auto* digestExtern = findExtern(*test, "digest");
    ASSERT_TRUE(digestExtern != nullptr);
    ASSERT_EQ(3, digestExtern->instances_size());

    // Verify that that the digest() instances match the ones we expect from the
    // program. Note that we don't check the resource type of the id (i.e., we
    // mask off the first 8 bits) because extern instances haven't yet been
    // assigned an "official" resource type.

    // digest<T>() where T is a header.
    {
        auto& instance = digestExtern->instances(0);
        EXPECT_EQ(std::string("Header"), instance.preamble().name());
        EXPECT_EQ(0x0000B33Fu, instance.preamble().id() & 0x00ffffff);

        ::p4::config::v1model::Digest digest;
        instance.info().UnpackTo(&digest);
        EXPECT_EQ(1u, digest.receiver());

        // XXX(seth): There are actually three fields in this digest because
        // `h.$valid$` is incorrectly included. We need to fix that.
        ASSERT_EQ(3, digest.fields_size())
          << "If you fixed the inclusion of $valid$ fields in digests, "
             "please update this test.";

        EXPECT_EQ(1u, digest.fields(0).id());
        EXPECT_EQ(std::string("h.headerFieldA"), digest.fields(0).name());
        EXPECT_EQ(16, digest.fields(0).bitwidth());

        EXPECT_EQ(2u, digest.fields(1).id());
        EXPECT_EQ(std::string("h.headerFieldB"), digest.fields(1).name());
        EXPECT_EQ(8, digest.fields(1).bitwidth());
    }

    // digest<T>() where T is a struct.
    {
        auto& instance = digestExtern->instances(1);
        EXPECT_EQ(std::string("Metadata"), instance.preamble().name());
        EXPECT_EQ(0x0000D0C6u, instance.preamble().id() & 0x00ffffff);

        ::p4::config::v1model::Digest digest;
        instance.info().UnpackTo(&digest);
        EXPECT_EQ(2u, digest.receiver());
        ASSERT_EQ(2, digest.fields_size());

        EXPECT_EQ(1u, digest.fields(0).id());
        EXPECT_EQ(std::string("metadataFieldA"), digest.fields(0).name());
        EXPECT_EQ(3, digest.fields(0).bitwidth());

        EXPECT_EQ(2u, digest.fields(1).id());
        EXPECT_EQ(std::string("metadataFieldB"), digest.fields(1).name());
        EXPECT_EQ(1, digest.fields(1).bitwidth());
    }

    // digest<T>() where T is a tuple.
    {
        // XXX(seth): The name is generated by EliminateTuples. We should do
        // something nicer than this.
        auto& instance = digestExtern->instances(2);
        EXPECT_EQ(std::string("tuple_0"), instance.preamble().name());
        EXPECT_EQ(0x00003620u, instance.preamble().id() & 0x00ffffff);

        ::p4::config::v1model::Digest digest;
        instance.info().UnpackTo(&digest);
        EXPECT_EQ(3u, digest.receiver());
        ASSERT_EQ(2, digest.fields_size());

        EXPECT_EQ(1u, digest.fields(0).id());
        EXPECT_EQ(std::string("h.headerFieldA"), digest.fields(0).name());
        EXPECT_EQ(16, digest.fields(0).bitwidth());

        EXPECT_EQ(2u, digest.fields(1).id());
        EXPECT_EQ(std::string("metadataFieldB"), digest.fields(1).name());
        EXPECT_EQ(1, digest.fields(1).bitwidth());
    }
}

TEST_F(P4Runtime, StaticTableEntries) {
    auto test = createP4RuntimeTestCase(P4_SOURCE(P4Headers::V1MODEL, R"(
        header Header { bit<8> hfA; bit<16> hfB; }
        struct Headers { Header h; }
        struct Metadata { }

        parser parse(packet_in p, out Headers h, inout Metadata m,
                     inout standard_metadata_t sm) {
            state start { transition accept; } }
        control verifyChecksum(inout Headers h, inout Metadata m) { apply { } }
        control egress(inout Headers h, inout Metadata m,
                        inout standard_metadata_t sm) { apply { } }
        control computeChecksum(inout Headers h, inout Metadata m) { apply { } }
        control deparse(packet_out p, in Headers h) { apply { } }

        control ingress(inout Headers h, inout Metadata m,
                        inout standard_metadata_t sm) {
            action a() { sm.egress_spec = 0; }
            action a_with_control_params(bit<9> x) { sm.egress_spec = x; }

            table t_exact_ternary {
                key = { h.h.hfA : exact; h.h.hfB : ternary; }
                actions = { a; a_with_control_params; }
                default_action = a;
                const entries = {
                    (0x01, 0x1111 &&& 0xF   ) : a_with_control_params(1);
                    (0x02, 0x1181           ) : a_with_control_params(2);
                    (0x03, 0x1111 &&& 0xF000) : a_with_control_params(3);
                    (0x04, _                ) : a_with_control_params(4);
                }
            }
            apply { t_exact_ternary.apply(); }
        }
        V1Switch(parse(), verifyChecksum(), ingress(), egress(),
                 computeChecksum(), deparse()) main;
    )"));

    ASSERT_TRUE(test);
    EXPECT_EQ(0u, ::diagnosticCount());

    auto table = findTable(*test, "ingress.t_exact_ternary");
    ASSERT_TRUE(table != nullptr);
    EXPECT_TRUE(table->is_const_table());
    auto action = findAction(*test, "ingress.a_with_control_params");
    ASSERT_TRUE(action != nullptr);
    unsigned int hfAId = 1;
    unsigned int hfBId = 2;
    unsigned int xId = 1;

    auto entries = test->entries;
    const auto& updates = entries->updates();
    ASSERT_EQ(4, updates.size());
    int priority = 0;
    auto check_entry = [&](const p4::Update& update,
                           const std::string& exact_v,
                           const std::string& ternary_v,
                           const std::string& ternary_mask,
                           const std::string& param_v) {
        EXPECT_EQ(p4::Update::INSERT, update.type());
        const auto& protoEntry = update.entity().table_entry();
        EXPECT_EQ(table->preamble().id(), protoEntry.table_id());

        ASSERT_EQ(2, protoEntry.match().size());
        const auto& mfA = protoEntry.match().Get(0);
        EXPECT_EQ(hfAId, mfA.field_id());
        EXPECT_EQ(exact_v, mfA.exact().value());
        const auto& mfB = protoEntry.match().Get(1);
        EXPECT_EQ(hfBId, mfB.field_id());
        EXPECT_EQ(ternary_v, mfB.ternary().value());
        EXPECT_EQ(ternary_mask, mfB.ternary().mask());

        const auto& protoAction = protoEntry.action().action();
        EXPECT_EQ(action->preamble().id(), protoAction.action_id());
        ASSERT_EQ(1, protoAction.params().size());
        const auto& param = protoAction.params().Get(0);
        EXPECT_EQ(xId, param.param_id());
        EXPECT_EQ(param_v, param.value());

        // increasing numerical priority, i.e. decreasing logical priority
        EXPECT_LT(priority, protoEntry.priority());
        priority = protoEntry.priority();
    };
    // We assume that the entries are generated in the same order as they appear
    // in the P4 program
    check_entry(updates.Get(0), "\x01", "\x11\x11", std::string("\x00\x0f", 2),
                std::string("\x00\x01", 2));
    check_entry(updates.Get(1), "\x02", "\x11\x81", "\xff\xff",
                std::string("\x00\x02", 2));
    check_entry(updates.Get(2), "\x03", "\x11\x11", std::string("\xf0\x00", 2),
                std::string("\x00\x03", 2));
    check_entry(updates.Get(3), "\x04", std::string("\x00\x00", 2),
                std::string("\x00\x00", 2), std::string("\x00\x04", 2));
}

TEST_F(P4Runtime, IsConstTable) {
    auto test = createP4RuntimeTestCase(P4_SOURCE(P4Headers::V1MODEL, R"(
        header Header { bit<8> hfA; }
        struct Headers { Header h; }
        struct Metadata { }

        parser parse(packet_in p, out Headers h, inout Metadata m,
                     inout standard_metadata_t sm) {
            state start { transition accept; } }
        control verifyChecksum(inout Headers h, inout Metadata m) { apply { } }
        control egress(inout Headers h, inout Metadata m,
                        inout standard_metadata_t sm) { apply { } }
        control computeChecksum(inout Headers h, inout Metadata m) { apply { } }
        control deparse(packet_out p, in Headers h) { apply { } }

        control ingress(inout Headers h, inout Metadata m,
                        inout standard_metadata_t sm) {
            action a() { sm.egress_spec = 0; }
            action a_with_control_params(bit<9> x) { sm.egress_spec = x; }

            table t_const {
                key = { h.h.hfA : exact; }
                actions = { a; a_with_control_params; }
                default_action = a;
                const entries = {
                    (0x01) : a_with_control_params(1);
                }
            }
            table t_non_const {
                key = { h.h.hfA : exact; }
                actions = { a; a_with_control_params; }
                default_action = a;
            }
            apply { t_const.apply(); t_non_const.apply(); }
        }
        V1Switch(parse(), verifyChecksum(), ingress(), egress(),
                 computeChecksum(), deparse()) main;
    )"));

    ASSERT_TRUE(test);
    EXPECT_EQ(0u, ::diagnosticCount());

    auto table_const = findTable(*test, "ingress.t_const");
    ASSERT_TRUE(table_const != nullptr);
    EXPECT_TRUE(table_const->is_const_table());
    auto table_non_const = findTable(*test, "ingress.t_non_const");
    ASSERT_TRUE(table_non_const != nullptr);
    EXPECT_FALSE(table_non_const->is_const_table());
}

TEST_F(P4Runtime, ValueSet) {
    auto test = createP4RuntimeTestCase(P4_SOURCE(P4Headers::V1MODEL, R"(
        header Header { bit<32> hfA; bit<16> hfB; }
        struct Headers { Header h; }
        struct Metadata { }

        parser parse(packet_in p, out Headers h, inout Metadata m,
                     inout standard_metadata_t sm) {
            value_set<tuple<bit<32>, bit<16>>>(16) pvs;
            state start {
                p.extract(h.h);
                transition select(h.h.hfA, h.h.hfB) {
                    pvs: accept;
                    default: reject; } } }
        control verifyChecksum(inout Headers h, inout Metadata m) { apply { } }
        control egress(inout Headers h, inout Metadata m,
                        inout standard_metadata_t sm) { apply { } }
        control computeChecksum(inout Headers h, inout Metadata m) { apply { } }
        control deparse(packet_out p, in Headers h) { apply { } }
        control ingress(inout Headers h, inout Metadata m,
                        inout standard_metadata_t sm) { apply { } }
        V1Switch(parse(), verifyChecksum(), ingress(), egress(),
                 computeChecksum(), deparse()) main;
    )"));

    ASSERT_TRUE(test);
    EXPECT_EQ(0u, ::diagnosticCount());

    auto vset = findValueSet(*test, "parse.pvs");
    ASSERT_TRUE(vset != nullptr);
    EXPECT_EQ(48, vset->bitwidth());
    EXPECT_EQ(16, vset->size());
}

}  // namespace Test
