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

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#include <boost/algorithm/string/replace.hpp>
#include <boost/optional.hpp>
#include <google/protobuf/util/message_differencer.h>

#include <iterator>
#include <string>
#include <vector>

#include "control-plane/p4/config/v1/p4info.pb.h"
#include "control-plane/p4/config/v1/p4types.pb.h"
#include "control-plane/p4/v1/p4runtime.pb.h"
#include "gtest/gtest.h"
#pragma GCC diagnostic pop

#include "control-plane/p4RuntimeSerializer.h"
#include "control-plane/typeSpecConverter.h"
#include "frontends/common/parseInput.h"
#include "frontends/common/resolveReferences/referenceMap.h"
#include "frontends/p4/parseAnnotations.h"
#include "frontends/p4/typeChecking/typeChecker.h"
#include "frontends/p4/typeMap.h"
#include "helpers.h"
#include "ir/ir.h"
#include "lib/cstring.h"

namespace p4v1 = ::p4::v1;
namespace p4configv1 = ::p4::config::v1;

namespace Test {

namespace {

using P4Ids = p4configv1::P4Ids;

using google::protobuf::util::MessageDifferencer;

const cstring defaultArch = "v1model";

boost::optional<P4::P4RuntimeAPI>
createP4RuntimeTestCase(
    const std::string& source,
    CompilerOptions::FrontendVersion langVersion = FrontendTestCase::defaultVersion,
    const cstring arch = defaultArch,
    P4::ParseAnnotations parseAnnotations = P4::ParseAnnotations()) {
    auto frontendTestCase = FrontendTestCase::create(source, langVersion, parseAnnotations);
    if (!frontendTestCase) return boost::none;
    return P4::generateP4Runtime(frontendTestCase->program, arch);
}

boost::optional<P4::P4RuntimeAPI>
createP4RuntimeTestCase(
    const std::string& source,
    P4::ParseAnnotations parseAnnotations) {
    return createP4RuntimeTestCase(source, FrontendTestCase::defaultVersion,
                                   defaultArch, parseAnnotations);
}

/// Generic meta function which searches an object by @name in the given range
/// and @returns the P4Runtime representation, or null if none is found.
template <typename It>
auto findP4InfoObject(const It& first, const It& last, const std::string& name)
    -> const typename std::iterator_traits<It>::value_type* {
    using T = typename std::iterator_traits<It>::value_type;
    auto desiredObject = std::find_if(first, last,
                                      [&](const T& object) {
        return object.preamble().name() == name;
    });
    if (desiredObject == last) return nullptr;
    return &*desiredObject;
}

/// @return the P4Runtime representation of the table with the given name, or
/// null if none is found.
const p4configv1::Table* findTable(const P4::P4RuntimeAPI& analysis,
                                   const std::string& name) {
    auto& tables = analysis.p4Info->tables();
    return findP4InfoObject(tables.begin(), tables.end(), name);
}

/// @return the P4Runtime representation of the action with the given name, or
/// null if none is found.
const p4configv1::Action* findAction(const P4::P4RuntimeAPI& analysis,
                                     const std::string& name) {
    auto& actions = analysis.p4Info->actions();
    return findP4InfoObject(actions.begin(), actions.end(), name);
}

/// @return the P4Runtime representation of the value set with the given name,
/// or null if none is found.
const p4configv1::ValueSet* findValueSet(const P4::P4RuntimeAPI& analysis,
                                         const std::string& name) {
    auto& vsets = analysis.p4Info->value_sets();
    return findP4InfoObject(vsets.begin(), vsets.end(), name);
}

/// @return the P4Runtime representation of the register with the given name, or
/// null if none is found.
const p4configv1::Register* findRegister(const P4::P4RuntimeAPI& analysis,
                                         const std::string& name) {
    auto& registers = analysis.p4Info->registers();
    return findP4InfoObject(registers.begin(), registers.end(), name);
}

/// @return the P4Runtime representation of the counter with the given name, or
/// null if none is found.
const p4configv1::Counter* findCounter(const P4::P4RuntimeAPI& analysis,
                                       const std::string& name) {
    auto& counters = analysis.p4Info->counters();
    return findP4InfoObject(counters.begin(), counters.end(), name);
}

/// @return the P4Runtime representation of the direct counter with the given
/// name, or null if none is found.
const p4configv1::DirectCounter* findDirectCounter(const P4::P4RuntimeAPI& analysis,
                                                   const std::string& name) {
    auto& counters = analysis.p4Info->direct_counters();
    return findP4InfoObject(counters.begin(), counters.end(), name);
}

/// @return the P4Runtime representation of the digest with the given name, or
/// null if none is found.
const p4configv1::Digest* findDigest(const P4::P4RuntimeAPI& analysis,
                                     const std::string& name) {
    auto& digests = analysis.p4Info->digests();
    return findP4InfoObject(digests.begin(), digests.end(), name);
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
        EXPECT_EQ(unsigned(P4Ids::TABLE), igTable->preamble().id() >> 24);

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
        EXPECT_EQ(unsigned(P4Ids::TABLE), igTableWithName->preamble().id() >> 24);
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

TEST_F(P4Runtime, IdAssignmentCounters) {
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

            @name(".myDirectCounter1")
            direct_counter(CounterType.packets) myDirectCounter1;

            @name(".myTable1")
            table myTable1 {
                actions = { noop; }
                default_action = noop;
                counters = myDirectCounter1;
            }

            @name(".myTable2")
            table myTable2 {
                actions = { noop; }
                default_action = noop;
                @name(".myDirectCounter2") counters = direct_counter(CounterType.packets);
            }

            @name(".myCounter")
            counter(32w1024, CounterType.packets) myCounter;

            apply {
                myTable1.apply();
                myTable2.apply();
                myCounter.count(32w128);
            }
        }

        V1Switch(parse(), verifyChecksum(), ingress(), egress(),
                 computeChecksum(), deparse()) main;
    )"));

    ASSERT_TRUE(test);
    EXPECT_EQ(0u, ::diagnosticCount());


    // checks that myDirectCounter1 with the right ID prefix
    {
        auto* myTable1 = findTable(*test, "myTable1");
        ASSERT_TRUE(myTable1 != nullptr);
        auto* myDirectCounter1 = findDirectCounter(*test, "myDirectCounter1");
        ASSERT_TRUE(myDirectCounter1 != nullptr);
        EXPECT_EQ(unsigned(P4Ids::DIRECT_COUNTER), myDirectCounter1->preamble().id() >> 24);
        EXPECT_EQ(myDirectCounter1->preamble().id(), myTable1->direct_resource_ids(0));
    }
    // checks that myDirectCounter2 with the right ID prefix
    {
        auto* myTable2 = findTable(*test, "myTable2");
        ASSERT_TRUE(myTable2 != nullptr);
        auto* myDirectCounter2 = findDirectCounter(*test, "myDirectCounter2");
        ASSERT_TRUE(myDirectCounter2 != nullptr);
        EXPECT_EQ(unsigned(P4Ids::DIRECT_COUNTER), myDirectCounter2->preamble().id() >> 24);
        EXPECT_EQ(myDirectCounter2->preamble().id(), myTable2->direct_resource_ids(0));
    }
    // checks that myCounter with the right ID prefix
    {
        auto* myCounter = findCounter(*test, "myCounter");
        ASSERT_TRUE(myCounter != nullptr);
        EXPECT_EQ(unsigned(P4Ids::COUNTER), myCounter->preamble().id() >> 24);
    }
}

namespace {

/// A helper for the match fields tests; represents metadata about a match field
/// that we expect to find in the generated P4Info.
struct ExpectedMatchField {
    unsigned id;
    std::string name;
    int bitWidth;
    p4configv1::MatchField::MatchType matchType;
};

}  // namespace

TEST_F(P4Runtime, P4_16_MatchFields) {
    using MatchField = p4configv1::MatchField;

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
    using MatchField = p4configv1::MatchField;

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
        { 15, "h.headerField", 2, MatchField::EXACT },
        { 16, "h.headerField", 2, MatchField::TERNARY },
        { 17, "h.headerField", 16, MatchField::EXACT },
        { 18, "h.headerField", 16, MatchField::TERNARY },
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
    // we expect one warning for the third digest, for which T is a tuple and we
    // have to auto-generate a name for the digest.
    EXPECT_EQ(1u, ::diagnosticCount());
    const auto &typeInfo = test->p4Info->type_info();

    // Verify that that the digest() instances match the ones we expect from the
    // program.

    // digest<T>() where T is a header.
    {
        auto digest = findDigest(*test, "Header");
        ASSERT_TRUE(digest != nullptr);
        EXPECT_EQ(unsigned(P4Ids::DIGEST), digest->preamble().id() >> 24);
        ASSERT_TRUE(digest->type_spec().has_header());
        EXPECT_EQ("Header", digest->type_spec().header().name());
        EXPECT_TRUE(typeInfo.headers().find("Header") != typeInfo.headers().end());
    }

    // digest<T>() where T is a struct.
    {
        auto digest = findDigest(*test, "Metadata");
        ASSERT_TRUE(digest != nullptr);
        EXPECT_EQ(unsigned(P4Ids::DIGEST), digest->preamble().id() >> 24);
        ASSERT_TRUE(digest->type_spec().has_struct_());
        EXPECT_EQ("Metadata", digest->type_spec().struct_().name());
        EXPECT_TRUE(typeInfo.structs().find("Metadata") != typeInfo.structs().end());
    }

    // digest<T>() where T is a tuple.
    {
        auto digest = findDigest(*test, "digest_0");
        ASSERT_TRUE(digest != nullptr);
        EXPECT_EQ(unsigned(P4Ids::DIGEST), digest->preamble().id() >> 24);
        ASSERT_TRUE(digest->type_spec().has_tuple());
        EXPECT_EQ(2, digest->type_spec().tuple().members_size());
    }
}

// TODO(antonin): there is a lot of boiler-plate code required for PSA so if we
// write more tests for the PSA P4Info generation, we will need to find a less
// verbose solution.
TEST_F(P4Runtime, PSADigests) {
    auto test = createP4RuntimeTestCase(P4_SOURCE(P4Headers::PSA, R"(
        header Header { bit<16> headerFieldA; bit<8> headerFieldB; }
        struct Headers { Header h; }
        struct Metadata { bit<3> metadataFieldA; bit<1> metadataFieldB; }
        struct EMPTY { };

        parser MyIP(
            packet_in buffer,
            out Headers h,
            inout Metadata m,
            in psa_ingress_parser_input_metadata_t c,
            in EMPTY d,
            in EMPTY e) {
            state start { transition accept; } }

        parser MyEP(
            packet_in buffer,
            out EMPTY a,
            inout EMPTY b,
            in psa_egress_parser_input_metadata_t c,
            in EMPTY d,
            in EMPTY e,
            in EMPTY f) {
            state start { transition accept; } }

        control MyIC(
            inout Headers h,
            inout Metadata m,
            in psa_ingress_input_metadata_t c,
            inout psa_ingress_output_metadata_t d) {
            // Digest<T> where T is a header.
            Digest<Header>() digest1;
            // Digest<T> where T is a struct.
            Digest<Metadata>() digest2;
            // Digest<T> where T is a tuple.
            Digest<tuple<bit<16>, bit<1> > >() digest3;
            action generateDigest() {
                digest1.pack(h.h);
                digest2.pack(m);
                digest3.pack({ h.h.headerFieldA, m.metadataFieldB });
            }
            apply {
                generateDigest();
            }
        }

        control MyEC(
            inout EMPTY a,
            inout EMPTY b,
            in psa_egress_input_metadata_t c,
            inout psa_egress_output_metadata_t d) {
            apply { } }

        control MyID(
            packet_out buffer,
            out EMPTY a,
            out EMPTY b,
            out EMPTY c,
            inout Headers h,
            in Metadata e,
            in psa_ingress_output_metadata_t f) {
            apply { } }

        control MyED(
            packet_out buffer,
            out EMPTY a,
            out EMPTY b,
            inout EMPTY c,
            in EMPTY d,
            in psa_egress_output_metadata_t e,
            in psa_egress_deparser_input_metadata_t f) {
        apply { } }

        IngressPipeline(MyIP(), MyIC(), MyID()) ip;
        EgressPipeline(MyEP(), MyEC(), MyED()) ep;

       PSA_Switch(ip, PacketReplicationEngine(), ep, BufferingQueueingEngine()) main;
    )"), CompilerOptions::FrontendVersion::P4_16, "psa");

    ASSERT_TRUE(test);
    EXPECT_EQ(0u, ::diagnosticCount());
    const auto &typeInfo = test->p4Info->type_info();

    // Verify that that the digest() instances match the ones we expect from the
    // program.

    // Digest<T> where T is a header.
    {
        auto digest = findDigest(*test, "MyIC.digest1");
        ASSERT_TRUE(digest != nullptr);
        EXPECT_EQ(unsigned(P4Ids::DIGEST), digest->preamble().id() >> 24);
        ASSERT_TRUE(digest->type_spec().has_header());
        EXPECT_EQ("Header", digest->type_spec().header().name());
        EXPECT_TRUE(typeInfo.headers().find("Header") != typeInfo.headers().end());
    }

    // Digest<T> where T is a struct.
    {
        auto digest = findDigest(*test, "MyIC.digest2");
        ASSERT_TRUE(digest != nullptr);
        EXPECT_EQ(unsigned(P4Ids::DIGEST), digest->preamble().id() >> 24);
        ASSERT_TRUE(digest->type_spec().has_struct_());
        EXPECT_EQ("Metadata", digest->type_spec().struct_().name());
        EXPECT_TRUE(typeInfo.structs().find("Metadata") != typeInfo.structs().end());
    }

    // Digest<T> where T is a tuple.
    {
        auto digest = findDigest(*test, "MyIC.digest3");
        ASSERT_TRUE(digest != nullptr);
        EXPECT_EQ(unsigned(P4Ids::DIGEST), digest->preamble().id() >> 24);
        ASSERT_TRUE(digest->type_spec().has_tuple());
        EXPECT_EQ(2, digest->type_spec().tuple().members_size());
    }
}

TEST_F(P4Runtime, StaticTableEntries) {
    auto test = createP4RuntimeTestCase(P4_SOURCE(P4Headers::V1MODEL, R"(
        header Header { bit<8> hfA; bit<16> hfB; }
        struct Headers { Header h; }
        struct Metadata { bool b; }

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

            action a_with_bool_param(bool x) { m.b = x; }

            table t_exact_valid {
                key = { h.h.isValid() : exact; }
                actions = { a; a_with_bool_param; }
                default_action = a;
                const entries = {
                    (true) : a_with_bool_param(true);
                    (false) : a_with_bool_param(false);
                }
            }
            apply { t_exact_ternary.apply(); t_exact_valid.apply(); }
        }
        V1Switch(parse(), verifyChecksum(), ingress(), egress(),
                 computeChecksum(), deparse()) main;
    )"));

    ASSERT_TRUE(test);
    EXPECT_EQ(0u, ::diagnosticCount());

    auto entries = test->entries;
    const auto& updates = entries->updates();
    ASSERT_EQ(6, updates.size());

    {
        auto table = findTable(*test, "ingress.t_exact_ternary");
        ASSERT_TRUE(table != nullptr);
        EXPECT_TRUE(table->is_const_table());
        unsigned int hfAId = 1;
        unsigned int hfBId = 2;
        unsigned int xId = 1;
        auto action = findAction(*test, "ingress.a_with_control_params");
        ASSERT_TRUE(action != nullptr);

        int priority = 0;
        auto check_entry = [&](const p4v1::Update& update,
                               const std::string& exact_v,
                               const std::string& ternary_v,
                               const std::string& ternary_mask,
                               const std::string& param_v) {
            EXPECT_EQ(p4v1::Update::INSERT, update.type());
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
        // We assume that the entries are generated in the same order as they
        // appear in the P4 program
        check_entry(updates.Get(0), "\x01", "\x11\x11", std::string("\x00\x0f", 2),
                    std::string("\x00\x01", 2));
        check_entry(updates.Get(1), "\x02", "\x11\x81", "\xff\xff",
                    std::string("\x00\x02", 2));
        check_entry(updates.Get(2), "\x03", "\x11\x11", std::string("\xf0\x00", 2),
                    std::string("\x00\x03", 2));
        check_entry(updates.Get(3), "\x04", std::string("\x00\x00", 2),
                    std::string("\x00\x00", 2), std::string("\x00\x04", 2));
    }

    {
        auto table = findTable(*test, "ingress.t_exact_valid");
        ASSERT_TRUE(table != nullptr);
        EXPECT_TRUE(table->is_const_table());
        auto action = findAction(*test, "ingress.a_with_bool_param");
        ASSERT_TRUE(action != nullptr);

        auto check_entry = [&](const p4v1::Update& update,
                               const std::string& exact_v,
                               const std::string& param_v) {
            EXPECT_EQ(p4v1::Update::INSERT, update.type());
            const auto& protoEntry = update.entity().table_entry();
            EXPECT_EQ(table->preamble().id(), protoEntry.table_id());

            ASSERT_EQ(1, protoEntry.match().size());
            const auto& mf = protoEntry.match().Get(0);
            EXPECT_EQ(exact_v, mf.exact().value());

            const auto& protoAction = protoEntry.action().action();
            EXPECT_EQ(action->preamble().id(), protoAction.action_id());
            ASSERT_EQ(1, protoAction.params().size());
            const auto& param = protoAction.params().Get(0);
            EXPECT_EQ(param_v, param.value());
        };
        // We assume that the entries are generated in the same order as they
        // appear in the P4 program
        check_entry(updates.Get(4), "\x01", "\x01");
        check_entry(updates.Get(5), std::string("\x00", 1), std::string("\x00", 1));
    }
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

TEST_F(P4Runtime, TableActionsAnnotations) {
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
            action b() { sm.egress_spec = 1; }
            action c() { sm.egress_spec = 2; }

            table t {
                key = { h.h.hfA : exact; }
                actions = { @defaultonly a; @tableonly b; c; }
                default_action = a();
            }
            apply { t.apply(); }
        }
        V1Switch(parse(), verifyChecksum(), ingress(), egress(),
                 computeChecksum(), deparse()) main;
    )"));

    ASSERT_TRUE(test);
    EXPECT_EQ(0u, ::diagnosticCount());

    auto table = findTable(*test, "ingress.t");
    ASSERT_TRUE(table != nullptr);

    // finds action reference based on the action name
    // returns nullptr if something goes wrong (e.g. not a valid action name for
    // this table)
    auto findActionRef = [test, table](const std::string& name) -> const p4configv1::ActionRef*{
        auto action = findAction(*test, name);
        if (!action) return nullptr;
        auto find_it = std::find_if(
            table->action_refs().begin(), table->action_refs().end(),
            [action](const p4configv1::ActionRef& ref) {
                return ref.id() == action->preamble().id(); });
        return (find_it != table->action_refs().end()) ? &*find_it : nullptr;
    };
    auto actionRefA = findActionRef("ingress.a");
    ASSERT_TRUE(actionRefA != nullptr);
    EXPECT_EQ(actionRefA->scope(), p4configv1::ActionRef::DEFAULT_ONLY);
    auto actionRefB = findActionRef("ingress.b");
    ASSERT_TRUE(actionRefB != nullptr);
    EXPECT_EQ(actionRefB->scope(), p4configv1::ActionRef::TABLE_ONLY);
    auto actionRefC = findActionRef("ingress.c");
    ASSERT_TRUE(actionRefC != nullptr);
    EXPECT_EQ(actionRefC->scope(), p4configv1::ActionRef::TABLE_AND_DEFAULT);
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
    EXPECT_EQ(unsigned(P4Ids::VALUE_SET), vset->preamble().id() >> 24);
    EXPECT_EQ(48, vset->bitwidth());
    EXPECT_EQ(16, vset->size());
}

class ParseAnnotations : public P4::ParseAnnotations {
 public:
    ParseAnnotations() : P4::ParseAnnotations("FrontendTest", true, {
                PARSE("my_anno", StringLiteral)
            }) { }
};

TEST_F(P4Runtime, Register) {
    auto test = createP4RuntimeTestCase(P4_SOURCE(P4Headers::V1MODEL, R"(
        header Header { bit<32> hfA; bit<16> hfB; }
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
            @my_anno("This is an annotation!")
            register<tuple<bit<16>, bit<8> > >(128) my_register_1;
            register<Header>(128) my_register_2;
            apply {
                my_register_1.write(32w10, {16w1, 8w2});
                my_register_2.write(32w10, h.h); } }
        V1Switch(parse(), verifyChecksum(), ingress(), egress(),
                 computeChecksum(), deparse()) main;
    )"), ParseAnnotations());

    ASSERT_TRUE(test);
    EXPECT_EQ(0u, ::diagnosticCount());

    {  // type parameter is tuple
        auto register_ = findRegister(*test, "ingress.my_register_1");
        ASSERT_TRUE(register_ != nullptr);
        EXPECT_EQ(unsigned(P4Ids::REGISTER), register_->preamble().id() >> 24);
        const auto& annotations = register_->preamble().annotations();
        ASSERT_EQ(1, annotations.size());
        EXPECT_EQ("@my_anno(\"This is an annotation!\")", annotations.Get(0));
        EXPECT_EQ(128, register_->size());
        const auto& typeSpec = register_->type_spec();
        ASSERT_TRUE(typeSpec.has_tuple());
        EXPECT_EQ(2, typeSpec.tuple().members_size());
    }
    {  // type parameter is header
        auto register_ = findRegister(*test, "ingress.my_register_2");
        ASSERT_TRUE(register_ != nullptr);
        EXPECT_EQ(unsigned(P4Ids::REGISTER), register_->preamble().id() >> 24);
        EXPECT_EQ(128, register_->size());
        const auto& typeSpec = register_->type_spec();
        ASSERT_TRUE(typeSpec.has_header());
        EXPECT_EQ("Header", typeSpec.header().name());
    }
}

TEST_F(P4Runtime, Documentation) {
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
            @brief("This action does nothing duh!")
            action noop() { }

            action drop() { mark_to_drop(); }

            // we cannot test a multi-line annotation here (with escaped new line)
            // because the preprocessor is not run
            @description("A table that matches on the ingress port and decides whether or not to drop the packet")
            table t {
                key = { sm.ingress_port : exact @brief("Ingress port"); }
                actions = { noop; drop; }
                default_action = noop;
            }

            apply {
                t.apply();
            }
        }

        V1Switch(parse(), verifyChecksum(), ingress(), egress(),
                 computeChecksum(), deparse()) main;
    )"));

    ASSERT_TRUE(test);
    EXPECT_EQ(0u, ::diagnosticCount());

    {
      auto table = findTable(*test, "ingress.t");
      ASSERT_TRUE(table != nullptr);
      const auto& tDocumentation = table->preamble().doc();
      EXPECT_EQ(tDocumentation.brief(), "");
      // NOLINTNEXTLINE(whitespace/line_length)
      EXPECT_EQ(tDocumentation.description(), "A table that matches on the ingress port and decides whether or not to drop the packet");

      const auto& mf = table->match_fields(0);
      const auto& mfDocumentation = mf.doc();
      EXPECT_EQ(mfDocumentation.brief(), "Ingress port");
      EXPECT_EQ(mfDocumentation.description(), "");
    }

    {
      auto noop = findAction(*test, "ingress.noop");
      ASSERT_TRUE(noop != nullptr);
      const auto& aDocumentation = noop->preamble().doc();
      EXPECT_EQ(aDocumentation.brief(), "This action does nothing duh!");
      EXPECT_EQ(aDocumentation.description(), "");
    }

    {
      auto drop = findAction(*test, "ingress.drop");
      ASSERT_TRUE(drop != nullptr);
      EXPECT_FALSE(drop->preamble().has_doc());
    }
}

class P4RuntimePkgInfo : public P4CTest {
 protected:
    static boost::optional<P4::P4RuntimeAPI> createTestCase(const char* annotations);
};

/* static */
boost::optional<P4::P4RuntimeAPI> P4RuntimePkgInfo::createTestCase(const char* annotations) {
    auto source = P4_SOURCE(P4Headers::V1MODEL, R"(
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
                        inout standard_metadata_t sm) { apply { } }
        %ANNOTATIONS%
        V1Switch(parse(), verifyChecksum(), ingress(), egress(),
                 computeChecksum(), deparse()) main;
    )");
    boost::replace_first(source, "%ANNOTATIONS%", annotations);
    return createP4RuntimeTestCase(source);
}

TEST_F(P4RuntimePkgInfo, NoAnnotations) {
    auto test = createTestCase("");
    ASSERT_TRUE(test);
    EXPECT_EQ(0u, ::diagnosticCount());
    const auto& pkgInfo = test->p4Info->pkg_info();
    EXPECT_EQ(pkgInfo.arch(), "v1model");
}

TEST_F(P4RuntimePkgInfo, GeneralCase) {
    auto test = createTestCase(R"(
        @pkginfo(name="prog.p4", version="1.0.0")
        @pkginfo(contact="p4-dev@lists.p4.org")
        @brief("This is a P4 program"))");
    ASSERT_TRUE(test);
    EXPECT_EQ(0u, ::diagnosticCount());
    const auto& pkgInfo = test->p4Info->pkg_info();
    EXPECT_EQ(pkgInfo.arch(), "v1model");
    EXPECT_EQ(pkgInfo.name(), "prog.p4");
    EXPECT_EQ(pkgInfo.version(), "1.0.0");
    EXPECT_EQ(pkgInfo.doc().brief(), "This is a P4 program");
}

TEST_F(P4RuntimePkgInfo, OverrideArch) {
    auto test = createTestCase(R"(@pkginfo(arch="v1"))");
    ASSERT_TRUE(test);
    // we expect 1 warning for overriding the architecture
    EXPECT_EQ(1u, ::diagnosticCount());
    const auto& pkgInfo = test->p4Info->pkg_info();
    EXPECT_EQ(pkgInfo.arch(), "v1");
}

TEST_F(P4RuntimePkgInfo, ValueNotAString) {
    auto test = createTestCase(R"(@pkginfo(name=77))");
    ASSERT_TRUE(test);
    // we expect 1 error message
    EXPECT_EQ(1u, ::diagnosticCount());
    const auto& pkgInfo = test->p4Info->pkg_info();
    EXPECT_EQ(pkgInfo.name(), "");
}

TEST_F(P4RuntimePkgInfo, DuplicateKey) {
    auto test = createTestCase(R"(@pkginfo(name="aaa", name="bbb"))");
    // kv annotations use an IndexedVector, which does not allow duplicate keys.
    ASSERT_FALSE(test);
}

TEST_F(P4RuntimePkgInfo, UnknownAnnotations) {
    auto test = createTestCase(R"(
        @my_annotation_1
        @my_annotation_2()
        @my_annotation_3(test)
        @my_annotation_4("test"))");
    ASSERT_TRUE(test);
    EXPECT_EQ(0u, ::diagnosticCount());
    const auto& pkgInfo = test->p4Info->pkg_info();
    const auto& annotations = pkgInfo.annotations();
    ASSERT_EQ(annotations.size(), 4);
    // P4 order is preserved when building the IR and generating P4Info
    EXPECT_EQ(annotations.Get(0), "@my_annotation_1");
    // The IR doesn't preserve the empty brackets
    // EXPECT_EQ(annotations.Get(1), "@my_annotation_2()");
    EXPECT_EQ(annotations.Get(1), "@my_annotation_2");
    EXPECT_EQ(annotations.Get(2), "@my_annotation_3(test)");
    EXPECT_EQ(annotations.Get(3), "@my_annotation_4(\"test\")");
}


class P4RuntimeDataTypeSpec : public P4Runtime {
 protected:
    const IR::P4Program* getProgram(const std::string& programStr) {
        auto pgm = P4::parseP4String(programStr, CompilerOptions::FrontendVersion::P4_16);
        if (pgm == nullptr) return nullptr;
        PassManager  passes({
            new P4::TypeChecking(&refMap, &typeMap)
        });
        pgm = pgm->apply(passes);
        return pgm;
    }

    template <typename T>
    const T* findExternTypeParameterName(
        const IR::P4Program* program, cstring externName) const {
        const T* type = nullptr;
        forAllMatching<IR::Type_Specialized>(program, [&](const IR::Type_Specialized* ts) {
            if (ts->baseType->toString() != externName) return;
            ASSERT_TRUE(type == nullptr);
            ASSERT_TRUE(ts->arguments->at(0)->is<T>());
            type = ts->arguments->at(0)->to<T>();
        });
        return type;
    }

    p4configv1::P4TypeInfo typeInfo;
    P4::ReferenceMap refMap;
    P4::TypeMap typeMap;
};

TEST_F(P4RuntimeDataTypeSpec, Bits) {
    int size(9);
    bool isSigned(true);
    auto type = new IR::Type_Bits(size, isSigned);
    auto typeSpec = P4::ControlPlaneAPI::TypeSpecConverter::convert(
        &refMap, &typeMap, type, &typeInfo);
    ASSERT_TRUE(typeSpec->has_bitstring());
    const auto& bitstringTypeSpec = typeSpec->bitstring();
    ASSERT_TRUE(bitstringTypeSpec.has_int_());  // signed type
    EXPECT_EQ(size, bitstringTypeSpec.int_().bitwidth());
}

TEST_F(P4RuntimeDataTypeSpec, Varbits) {
    int size(64);
    auto type = new IR::Type_Varbits(size);
    auto typeSpec = P4::ControlPlaneAPI::TypeSpecConverter::convert(
        &refMap, &typeMap, type, &typeInfo);
    ASSERT_TRUE(typeSpec->has_bitstring());
    const auto& bitstringTypeSpec = typeSpec->bitstring();
    ASSERT_TRUE(bitstringTypeSpec.has_varbit());
    EXPECT_EQ(size, bitstringTypeSpec.varbit().max_bitwidth());
}

TEST_F(P4RuntimeDataTypeSpec, Boolean) {
    auto type = new IR::Type_Boolean();
    auto typeSpec = P4::ControlPlaneAPI::TypeSpecConverter::convert(
        &refMap, &typeMap, type, &typeInfo);
    EXPECT_TRUE(typeSpec->has_bool_());
}

TEST_F(P4RuntimeDataTypeSpec, Tuple) {
    auto typeMember1 = new IR::Type_Bits(1, false);
    auto typeMember2 = new IR::Type_Bits(2, false);
    IR::Vector<IR::Type> components = {typeMember1, typeMember2};
    auto type = new IR::Type_Tuple(std::move(components));
    auto typeSpec = P4::ControlPlaneAPI::TypeSpecConverter::convert(
        &refMap, &typeMap, type, &typeInfo);
    ASSERT_TRUE(typeSpec->has_tuple());
    const auto& tupleTypeSpec = typeSpec->tuple();
    ASSERT_EQ(2, tupleTypeSpec.members_size());
    {
        auto typeSpec = P4::ControlPlaneAPI::TypeSpecConverter::convert(
            &refMap, &typeMap, typeMember1, nullptr);
        EXPECT_TRUE(MessageDifferencer::Equals(*typeSpec, tupleTypeSpec.members(0)));
    }
    {
        auto typeSpec = P4::ControlPlaneAPI::TypeSpecConverter::convert(
            &refMap, &typeMap, typeMember2, nullptr);
        EXPECT_TRUE(MessageDifferencer::Equals(*typeSpec, tupleTypeSpec.members(1)));
    }
}

TEST_F(P4RuntimeDataTypeSpec, Struct) {
    std::string program = P4_SOURCE(R"(
        struct my_struct { bit<8> f; }
        extern my_extern_t<T> { my_extern_t(bit<32> v); }
        my_extern_t<my_struct>(32w1024) my_extern;
    )");
    auto pgm = getProgram(program);
    ASSERT_TRUE(pgm != nullptr && ::errorCount() == 0);

    auto type = findExternTypeParameterName<IR::Type_Name>(pgm, "my_extern_t");
    ASSERT_TRUE(type != nullptr);
    auto typeSpec = P4::ControlPlaneAPI::TypeSpecConverter::convert(
        &refMap, &typeMap, type, &typeInfo);
    ASSERT_TRUE(typeSpec->has_struct_());
    EXPECT_EQ("my_struct", typeSpec->struct_().name());

    auto it = typeInfo.structs().find("my_struct");
    ASSERT_TRUE(it != typeInfo.structs().end());
    ASSERT_EQ(1, it->second.members_size());
    EXPECT_EQ("f", it->second.members(0).name());
    const auto &memberTypeSpec = it->second.members(0).type_spec();
    ASSERT_TRUE(memberTypeSpec.has_bitstring());
    ASSERT_TRUE(memberTypeSpec.bitstring().has_bit());
    EXPECT_EQ(8, memberTypeSpec.bitstring().bit().bitwidth());
}

TEST_F(P4RuntimeDataTypeSpec, Header) {
    std::string program = P4_SOURCE(R"(
        header my_header { bit<8> f; }
        extern my_extern_t<T> { my_extern_t(bit<32> v); }
        my_extern_t<my_header>(32w1024) my_extern;
    )");
    auto pgm = getProgram(program);
    ASSERT_TRUE(pgm != nullptr && ::errorCount() == 0);

    auto type = findExternTypeParameterName<IR::Type_Name>(pgm, "my_extern_t");
    ASSERT_TRUE(type != nullptr);
    auto typeSpec = P4::ControlPlaneAPI::TypeSpecConverter::convert(
        &refMap, &typeMap, type, &typeInfo);
    ASSERT_TRUE(typeSpec->has_header());
    EXPECT_EQ("my_header", typeSpec->header().name());

    auto it = typeInfo.headers().find("my_header");
    ASSERT_TRUE(it != typeInfo.headers().end());
    ASSERT_EQ(1, it->second.members_size());
    EXPECT_EQ("f", it->second.members(0).name());
    const auto &memberBitstringTypeSpec = it->second.members(0).type_spec();
    ASSERT_TRUE(memberBitstringTypeSpec.has_bit());
    EXPECT_EQ(8, memberBitstringTypeSpec.bit().bitwidth());
}

TEST_F(P4RuntimeDataTypeSpec, HeaderWithFlattening) {
    std::string program = P4_SOURCE(R"(
        struct s_t { bit<8> f1; bit<8> f2; }
        header my_header { bit<8> f; s_t s; }
        extern my_extern_t<T> { my_extern_t(bit<32> v); }
        my_extern_t<my_header>(32w1024) my_extern;
    )");
    auto pgm = getProgram(program);
    ASSERT_TRUE(pgm != nullptr && ::errorCount() == 0);

    auto type = findExternTypeParameterName<IR::Type_Name>(pgm, "my_extern_t");
    ASSERT_TRUE(type != nullptr);
    auto typeSpec = P4::ControlPlaneAPI::TypeSpecConverter::convert(
        &refMap, &typeMap, type, &typeInfo);
    ASSERT_TRUE(typeSpec->has_header());
    EXPECT_EQ("my_header", typeSpec->header().name());

    auto it = typeInfo.headers().find("my_header");
    ASSERT_TRUE(it != typeInfo.headers().end());
    ASSERT_EQ(3, it->second.members_size());
    EXPECT_EQ("f", it->second.members(0).name());
    EXPECT_EQ("s.f1", it->second.members(1).name());
    EXPECT_EQ("s.f2", it->second.members(2).name());
    const auto &memberBitstringTypeSpec = it->second.members(1).type_spec();
    ASSERT_TRUE(memberBitstringTypeSpec.has_bit());
    EXPECT_EQ(8, memberBitstringTypeSpec.bit().bitwidth());
}

TEST_F(P4RuntimeDataTypeSpec, HeaderUnion) {
    std::string program = P4_SOURCE(R"(
        header my_header_1 { bit<8> f; }
        header my_header_2 { bit<16> f; }
        header_union my_header_union { my_header_1 h1; my_header_2 h2; }
        extern my_extern_t<T> { my_extern_t(bit<32> v); }
        my_extern_t<my_header_union>(32w1024) my_extern;
    )");
    auto pgm = getProgram(program);
    ASSERT_TRUE(pgm != nullptr && ::errorCount() == 0);

    auto type = findExternTypeParameterName<IR::Type_Name>(pgm, "my_extern_t");
    ASSERT_TRUE(type != nullptr);
    auto typeSpec = P4::ControlPlaneAPI::TypeSpecConverter::convert(
        &refMap, &typeMap, type, &typeInfo);
    ASSERT_TRUE(typeSpec->has_header_union());
    EXPECT_EQ("my_header_union", typeSpec->header_union().name());

    {
        auto it = typeInfo.header_unions().find("my_header_union");
        ASSERT_TRUE(it != typeInfo.header_unions().end());
        ASSERT_EQ(2, it->second.members_size());
        EXPECT_EQ("h1", it->second.members(0).name());
        EXPECT_EQ("my_header_1", it->second.members(0).header().name());
        EXPECT_EQ("h2", it->second.members(1).name());
        EXPECT_EQ("my_header_2", it->second.members(1).header().name());
    }

    EXPECT_TRUE(typeInfo.headers().find("my_header_1") != typeInfo.headers().end());
    EXPECT_TRUE(typeInfo.headers().find("my_header_2") != typeInfo.headers().end());
}

TEST_F(P4RuntimeDataTypeSpec, HeaderStack) {
    std::string program = P4_SOURCE(R"(
        header my_header { bit<8> f; }
        extern my_extern_t<T> { my_extern_t(bit<32> v); }
        my_extern_t<my_header[3]>(32w1024) my_extern;
    )");
    auto pgm = getProgram(program);
    ASSERT_TRUE(pgm != nullptr && ::errorCount() == 0);

    auto type = findExternTypeParameterName<IR::Type_Stack>(pgm, "my_extern_t");
    ASSERT_TRUE(type != nullptr);
    auto typeSpec = P4::ControlPlaneAPI::TypeSpecConverter::convert(
        &refMap, &typeMap, type, &typeInfo);
    ASSERT_TRUE(typeSpec->has_header_stack());
    EXPECT_EQ("my_header", typeSpec->header_stack().header().name());
    EXPECT_EQ(3, typeSpec->header_stack().size());

    EXPECT_TRUE(typeInfo.headers().find("my_header") != typeInfo.headers().end());
}

TEST_F(P4RuntimeDataTypeSpec, HeaderUnionStack) {
    std::string program = P4_SOURCE(R"(
        header my_header_1 { bit<8> f; }
        header my_header_2 { bit<16> f; }
        header_union my_header_union { my_header_1 h1; my_header_2 h2; }
        extern my_extern_t<T> { my_extern_t(bit<32> v); }
        my_extern_t<my_header_union[3]>(32w1024) my_extern;
    )");
    auto pgm = getProgram(program);
    ASSERT_TRUE(pgm != nullptr && ::errorCount() == 0);

    auto type = findExternTypeParameterName<IR::Type_Stack>(pgm, "my_extern_t");
    ASSERT_TRUE(type != nullptr);
    auto typeSpec = P4::ControlPlaneAPI::TypeSpecConverter::convert(
        &refMap, &typeMap, type, &typeInfo);
    ASSERT_TRUE(typeSpec->has_header_union_stack());
    EXPECT_EQ("my_header_union", typeSpec->header_union_stack().header_union().name());
    EXPECT_EQ(3, typeSpec->header_union_stack().size());

    EXPECT_TRUE(typeInfo.header_unions().find("my_header_union") != typeInfo.header_unions().end());
    EXPECT_TRUE(typeInfo.headers().find("my_header_1") != typeInfo.headers().end());
    EXPECT_TRUE(typeInfo.headers().find("my_header_2") != typeInfo.headers().end());
}

TEST_F(P4RuntimeDataTypeSpec, Enum) {
    std::string program = P4_SOURCE(R"(
        enum my_enum { MBR_1, MBR_2 }
        extern my_extern_t<T> { my_extern_t(bit<32> v); }
        my_extern_t<my_enum>(32w1024) my_extern;
    )");
    auto pgm = getProgram(program);
    ASSERT_TRUE(pgm != nullptr && ::errorCount() == 0);

    auto type = findExternTypeParameterName<IR::Type_Name>(pgm, "my_extern_t");
    ASSERT_TRUE(type != nullptr);
    auto typeSpec = P4::ControlPlaneAPI::TypeSpecConverter::convert(
        &refMap, &typeMap, type, &typeInfo);
    ASSERT_TRUE(typeSpec->has_enum_());
    EXPECT_EQ("my_enum", typeSpec->enum_().name());

    auto it = typeInfo.enums().find("my_enum");
    ASSERT_TRUE(it != typeInfo.enums().end());
    ASSERT_EQ(2, it->second.members_size());
    EXPECT_EQ("MBR_1", it->second.members(0).name());
    EXPECT_EQ("MBR_2", it->second.members(1).name());
}

TEST_F(P4RuntimeDataTypeSpec, Error) {
    std::string program = P4_SOURCE(R"(
        error { MBR_1, MBR_2 }
        extern my_extern_t<T> { my_extern_t(bit<32> v); }
        my_extern_t<error>(32w1024) my_extern;
    )");
    auto pgm = getProgram(program);
    ASSERT_TRUE(pgm != nullptr && ::errorCount() == 0);

    auto type = findExternTypeParameterName<IR::Type_Name>(pgm, "my_extern_t");
    ASSERT_TRUE(type != nullptr);
    auto typeSpec = P4::ControlPlaneAPI::TypeSpecConverter::convert(
        &refMap, &typeMap, type, &typeInfo);
    ASSERT_TRUE(typeSpec->has_error());

    ASSERT_TRUE(typeInfo.has_error());
    EXPECT_EQ("MBR_1", typeInfo.error().members(0));
    EXPECT_EQ("MBR_2", typeInfo.error().members(1));
}

TEST_F(P4RuntimeDataTypeSpec, StructWithTypedef) {
    std::string program = P4_SOURCE(R"(
        typedef bit<8> my_type_t;
        typedef my_type_t my_type2_t;
        struct my_struct { my_type_t f; my_type2_t f2; }
        extern my_extern_t<T> { my_extern_t(bit<32> v); }
        my_extern_t<my_struct>(32w1024) my_extern;
    )");
    auto pgm = getProgram(program);
    ASSERT_TRUE(pgm != nullptr && ::errorCount() == 0);

    auto type = findExternTypeParameterName<IR::Type_Name>(pgm, "my_extern_t");
    ASSERT_TRUE(type != nullptr);
    auto typeSpec = P4::ControlPlaneAPI::TypeSpecConverter::convert(
        &refMap, &typeMap, type, &typeInfo);
    ASSERT_TRUE(typeSpec->has_struct_());
    EXPECT_EQ("my_struct", typeSpec->struct_().name());

    auto it = typeInfo.structs().find("my_struct");
    ASSERT_TRUE(it != typeInfo.structs().end());
    ASSERT_EQ(2, it->second.members_size());
    auto check_member = [&](cstring name, int index) {
      EXPECT_EQ(name, it->second.members(index).name());
      const auto &memberTypeSpec = it->second.members(0).type_spec();
      ASSERT_TRUE(memberTypeSpec.has_bitstring());
      ASSERT_TRUE(memberTypeSpec.bitstring().has_bit());
      EXPECT_EQ(8, memberTypeSpec.bitstring().bit().bitwidth());
    };
    check_member("f", 0);
    check_member("f2", 1);
}

}  // namespace Test
