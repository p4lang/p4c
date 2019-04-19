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

#include "gtest/gtest.h"
#include "ir/ir.h"
#include "helpers.h"
#include "lib/log.h"

#include "frontends/common/parseInput.h"
#include "frontends/common/resolveReferences/referenceMap.h"
#include "frontends/common/resolveReferences/resolveReferences.h"
#include "frontends/p4/typeMap.h"

#include "p4/createBuiltins.h"
#include "p4/typeChecking/typeChecker.h"

using namespace P4;

namespace Test {

class P4CArchitecture : public P4CTest { };

TEST_F(P4CArchitecture, packet_out) {
    std::string program = P4_SOURCE(R"(
        // simplified core.p4
        extern packet_out {
            void emit<T> (in T hdr);
        }
        // simplified v1model.p4
        control Deparser<H> (packet_out b, in H hdr);
        package PSA<H> (Deparser<H> p);
        // user program
        header ParsedHeaders {
            bit<32> hdr;
        }
        control MyDeparser(packet_out b, in ParsedHeaders h){
            apply {
                b.emit(h);
            }
        }
        PSA(MyDeparser()) main;
    )");
    auto pgm = P4::parseP4String(program, CompilerOptions::FrontendVersion::P4_16);

    ReferenceMap refMap;
    TypeMap      typeMap;
    PassManager  passes({
        new TypeChecking(&refMap, &typeMap)
    });

    pgm = pgm->apply(passes);
    ASSERT_TRUE(pgm != nullptr && ::errorCount() == 0);
}

// Potential bug
TEST_F(P4CArchitecture, duplicatedDeclarationBug) {
    std::string program = P4_SOURCE(R"(
        // simplified core.p4
        extern packet_out {
            void emit<T> (in T hdr);
        }
        // simplified v1model.p4
        control Deparser<H> (packet_out b, in H hdr);
        package PSA<H> (Deparser<H> p);
        // user program
        struct ParsedHeaders {
            bit<32> hdr;
        }
        // BUG: duplicated name with 'Deparser'
        control Deparser(packet_out b, in ParsedHeaders h){
            apply {
                b.emit(h.hdr);
            }
        }
        PSA(Deparser()) main;
    )");
    auto pgm = P4::parseP4String(program, CompilerOptions::FrontendVersion::P4_16);

    ReferenceMap refMap;
    TypeMap      typeMap;
    PassManager  passes({
        new TypeChecking(&refMap, &typeMap)
    });

    pgm = pgm->apply(passes);
    ASSERT_GT(::errorCount(), 0U);
}

TEST_F(P4CArchitecture, instantiation) {
    std::string program = P4_SOURCE(R"(
        // simplified core.p4
        extern packet_in {
            void extract<T> (out T hdr);
        }
        extern packet_out {
            void emit<T> (in T hdr);
        }
        // simplified v1model.p4
        parser Parser<M> (packet_in b, out M meta);
        control Ingress<H, M> (inout H hdr, inout M meta);
        control Deparser<H> (packet_out b, in H hdr);
        package PSA<H, M> (Parser<M> p, Ingress<H, M> ig, Deparser<H> dp);
        // user program
        header ParsedHeaders {
            bit<32> hdr;
        }
        struct Metadata {
            bit<32> hdr;
        }
        parser MyParser(packet_in b, out Metadata m) {
            state start {}
        }
        control MyIngress(inout ParsedHeaders h, inout Metadata m) {
            apply {
            }
        }
        control MyDeparser(packet_out b, in ParsedHeaders h) {
            apply {
                b.emit(h);
            }
        }
        PSA(MyParser(), MyIngress(), MyDeparser()) main;
    )");
    auto pgm = P4::parseP4String(program, CompilerOptions::FrontendVersion::P4_16);

    ReferenceMap refMap;
    TypeMap      typeMap;
    PassManager  passes({
        new TypeChecking(&refMap, &typeMap)
    });

    pgm = pgm->apply(passes);
    ASSERT_TRUE(pgm != nullptr && ::errorCount() == 0);
}

TEST_F(P4CArchitecture, psa_package_with_body) {
    std::string program = P4_SOURCE(R"(
        // simplified core.p4
        // simplified v1model.p4
        control Ingress<H, M> (inout H hdr, inout M meta);
        package PSA<H, M> (Ingress<H, M> ig) {}
        // user program
        struct ParsedHeaders {
            bit<32> hdr;
        }
        struct Metadata {
            bit<32> hdr;
        }
        control MyIngress(inout ParsedHeaders h, inout Metadata m) (bit<32> x) {
            apply {
            }
        }
        PSA(MyIngress(2)) main;
    )");
    auto pgm = P4::parseP4String(program, CompilerOptions::FrontendVersion::P4_16);

    ReferenceMap refMap;
    TypeMap      typeMap;
    PassManager  passes({
        new TypeChecking(&refMap, &typeMap)
    });
    pgm = pgm->apply(passes);
    ASSERT_GT(::errorCount(), 0U);
}

TEST_F(P4CArchitecture, psa_control_in_control) {
    std::string program = P4_SOURCE(R"(
        // simplified core.p4
        // simplified v1model.p4
        control Ingress<H, M> (inout H hdr, inout M meta);
        control Egress<H, M> (inout M meta);
        package PSA<H, M> (Ingress<H, M> ig);
        // user program
        struct ParsedHeaders {
            bit<32> hdr;
        }
        struct Metadata {
            bit<32> hdr;
        }
        control E();
        control MyIngress(inout ParsedHeaders h, inout Metadata m) {
            apply {
            }
        }
        control MyEgress(inout Metadata m)(E ig) {
            apply {}
        }
        //MyEgress(ig) eg;
        PSA(MyIngress()) main;
    )");
    auto pgm = P4::parseP4String(program, CompilerOptions::FrontendVersion::P4_16);

    ReferenceMap refMap;
    TypeMap      typeMap;
    PassManager  passes({
        new TypeChecking(&refMap, &typeMap)
    });
    pgm = pgm->apply(passes);
    ASSERT_TRUE(pgm != nullptr && ::errorCount() == 0);
}

TEST_F(P4CArchitecture, psa_clone_as_param_to_package) {
    std::string program = P4_SOURCE(R"(
        extern clone {
            clone();
            void execute(in bit<32> sessions);
        }
        package PSA<H, M> (clone c);
        // user program
        struct ParsedHeaders {
            bit<32> hdr;
        }
        clone() c;
        PSA(c) main;
    )");
    auto pgm = P4::parseP4String(program, CompilerOptions::FrontendVersion::P4_16);

    ReferenceMap refMap;
    TypeMap      typeMap;
    PassManager  passes({
        new TypeChecking(&refMap, &typeMap)
    });
    pgm = pgm->apply(passes);
    ASSERT_TRUE(pgm != nullptr && ::errorCount() == 0);
}

TEST_F(P4CArchitecture, psa_clone_as_param_to_control) {
    std::string program = P4_SOURCE(R"(
        extern clone<T> {
            // constructor
            clone();
            // methods
            void clone(in bit<32> sessions);
            void clone(in bit<32> sessions, in T data);
        }
        control ingress<H, M> (inout H hdr, inout M meta);
        package PSA<H, M> (ingress<H,M> ig);
        // user program
        struct ParsedHeaders {
            bit<32> hdr;
        }
        struct Metadata {
            bit<32> md;
        }
        control MyIngress (inout ParsedHeaders p, inout Metadata m) (clone<bit<32>> c) {
            apply{}
        }
        PSA(MyIngress(clone<bit<32>>())) main;
    )");
    auto pgm = P4::parseP4String(program, CompilerOptions::FrontendVersion::P4_16);

    ReferenceMap refMap;
    TypeMap      typeMap;
    PassManager  passes({
        new TypeChecking(&refMap, &typeMap)
    });
    pgm = pgm->apply(passes);
    ASSERT_TRUE(pgm != nullptr && ::errorCount() == 0);
}

TEST_F(P4CArchitecture, psa_clone_as_param_to_extern) {
    std::string program = P4_SOURCE(R"(
        extern clone<T> {
            // constructor
            clone();
            // methods
            void clone(in bit<32> sessions);
            void clone(in bit<32> sessions, in T data);
        }
        extern PRE<T> {
            PRE();
            void clone(in bit<32> sessions); /* same as invoking c.clone(sessions); */
            void clone(in bit<32> sessions, in T data); /* same as invoking c.clone(sessions, data) */
            // ...
            // drop, multicast api here
        }
        control ingress<H, M> (inout H hdr, inout M meta);
        package PSA<H, M> (ingress<H,M> ig);
        // user program
        struct ParsedHeaders {
            bit<32> hdr;
        }
        struct Metadata {
            bit<32> md;
        }
        control MyIngress (inout ParsedHeaders p, inout Metadata m) (PRE<bit<32>> pre) {
            apply{}
        }
        PRE<bit<32>>() pre;
        PSA(MyIngress(pre)) main;
    )");
    auto pgm = P4::parseP4String(program, CompilerOptions::FrontendVersion::P4_16);

    ReferenceMap refMap;
    TypeMap      typeMap;
    PassManager  passes({
        new TypeChecking(&refMap, &typeMap)
    });
    pgm = pgm->apply(passes);
    ASSERT_TRUE(pgm != nullptr && ::errorCount() == 0);
}

TEST_F(P4CArchitecture, clone_as_extern_method) {
    std::string program = P4_SOURCE(R"(
        extern void clone(in bit<32> sessions);
        extern void clone3<T>(in bit<32> sessions, in T data);
        control ingress<H, M> (inout H hdr, inout M meta);
        package PSA<H, M> (ingress<H,M> ig);
        // user program
        struct ParsedHeaders {
            bit<32> hdr;
        }
        struct Metadata {
            bit<32> md;
        }
        control MyIngress (inout ParsedHeaders p, inout Metadata m) {
            apply{
                // invoke clone3 triggers a compiler bug.
            }
        }
        PSA(MyIngress()) main;
    )");
    auto pgm = P4::parseP4String(program, CompilerOptions::FrontendVersion::P4_16);

    ReferenceMap refMap;
    TypeMap      typeMap;
    PassManager  passes({
        new TypeChecking(&refMap, &typeMap)
    });
    pgm = pgm->apply(passes);
    ASSERT_TRUE(pgm != nullptr && ::errorCount() == 0);
}

}  // namespace Test
