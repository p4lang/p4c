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

#include "frontends/p4/typeMap.h"
#include "frontends/common/resolveReferences/referenceMap.h"
#include "frontends/common/resolveReferences/resolveReferences.h"
#include "frontends/p4/evaluator/evaluator.h"
#include "frontends/p4/inferArchitecture.h"
#include "frontends/p4/createBuiltins.h"
#include "backends/bmv2/jsonconverter.h"
#include "backends/bmv2/backend.h"

#include "p4/createBuiltins.h"
#include "p4/typeChecking/typeChecker.h"

#include "ir/json_generator.h"

using namespace P4;

TEST(arch, psa_infer) {
    Log::addDebugSpec("inferArchitecture:1");
    Log::addDebugSpec("jsonconverter:1");
    Log::addDebugSpec("psa_test:1");
    std::string program = P4_SOURCE(R"(
        typedef bit<9> PortId_t;
        typedef bit<3> InstanceType_t;
        typedef bit<5> ParserStatus_t;
        typedef bit<4> ParserErrorLocation_t;
        error {
            NoError
        }
        extern packet_in {
            void extract<T> (out T hdr);
        }
        extern packet_out {
            void emit<T> (in T hdr);
        }
        @metadata
        struct standard_metadata_t {
            PortId_t        ingress_port;
            InstanceType_t  instance_type;
            ParserStatus_t  parser_status;
            ParserErrorLocation_t parser_error_location;
        }
        parser Parser<H,M>(packet_in buffer, out H parsed_hdr, @metadata inout M user_meta, @metadata in standard_metadata_t std_meta);
        enum CounterType_t { packets, bytes, packets_and_bytes }
        extern Counter<W> {
            Counter(int<32> number, W size_in_bits, CounterType_t type);
            void count(in W index, in W increment);
        }
        control Ingress<H, M> (inout H hdr, @metadata inout M meta, @metadata inout standard_metadata_t stdmeta);
        @deparser
        control Deparser<H> (packet_out buffer, in H hdr);
        package PSA<H, M> (Parser<H,M> p, Ingress<H,M> i, Deparser<H> d);

        // user program
        struct IPv4 {
            bit<32> src;
            bit<32> dst;
            bit<8> type;
        }
        struct ParsedHeaders {
            bit<32> hdr;
            bit<32> hdr2;
            //IPv4 ipv4;
        }
        struct Metadata {
            bit<32> md;
        }
        parser MyParser (packet_in buffer, out ParsedHeaders p, inout Metadata m, in standard_metadata_t mt) {
            bit<32> var_parser;
            state start { transition accept; }
        }
        control MyIngress (inout ParsedHeaders p, inout Metadata m, inout standard_metadata_t mt) {
            action foo () {
                bit<32> var_action;
            }
            bit<32> var_control;
            apply{
                bit<32> var_apply;
            }
        }
        control MyDeparser (packet_out buffer, in ParsedHeaders hdr) {
            apply {
            }
        }
        MyParser() pr;
        MyIngress() ig;
        MyDeparser() dp;
        PSA(pr, ig, dp) main;
    )");
    const IR::P4Program* pgm = parse_string(program);
    ReferenceMap refMap;
    TypeMap      typeMap;
    // frontend passes
    PassManager  passes({
        new CreateBuiltins(),
        new TypeChecking(&refMap, &typeMap),
        new InferArchitecture(&typeMap)
    });
    pgm = pgm->apply(passes);
    // midend pass
    CompilerOptions options;
    BMV2::JsonConverter converter(options);
    const IR::ToplevelBlock *toplevel = nullptr;
    P4::ConvertEnums::EnumMapping enumMap;
    auto evaluator = new P4::EvaluatorPass(&refMap, &typeMap);
    auto convertEnums = new P4::ConvertEnums(&refMap, &typeMap, new EnumOn32Bits());
    PassManager midpass({
        convertEnums,
        new VisitFunctor([this, convertEnums, &enumMap]() { enumMap = convertEnums->getEnumMapping(); }),
        evaluator,
        new VisitFunctor([this, evaluator, &toplevel]() { toplevel = evaluator->getToplevelBlock(); })
    });
    pgm = pgm->apply(midpass);
    //dump(pgm);
    converter.convert(&refMap, &typeMap, toplevel, &enumMap);
    converter.serialize(std::cout);
}

TEST(arch, block_visitor) {
    Log::addDebugSpec("jsonconverter:1");
    Log::addDebugSpec("backend:1");
    Log::addDebugSpec("convertControl:1");
    std::string program = P4_SOURCE(R"(
        match_kind {
            exact
        }
        extern packet_in {
            void extract<T> (out T hdr);
        }
        extern packet_out {
            void emit<T> (in T hdr);
        }
        parser Parser<H,M>(packet_in buffer, out H parsed_hdr, inout M user_meta);
        control Ingress<H, M> (inout H hdr, inout M meta);
        control Deparser<H> (packet_out buffer, in H hdr);
        package PSA<H, M> (Parser<H,M> p, Ingress<H,M> i, Deparser<H> d);

        struct ParsedHeaders {
            bit<32> hdr;
            bit<32> hdr2;
        }
        struct Metadata {
            bit<32> md;
        }
        parser MyParser (packet_in buffer, out ParsedHeaders p, inout Metadata m) {
            state start { transition accept; }
        }
        control MyIngress (inout ParsedHeaders p, inout Metadata m) {
            table t1 {
                key = {
                    p.hdr : exact;
                }
            }
            apply{
                t1.apply();
            }
        }
        control MyDeparser (packet_out buffer, in ParsedHeaders hdr) {
            apply {}
        }
        MyParser() pr;
        MyIngress() ig;
        MyDeparser() dp;
        PSA(pr, ig, dp) main;
    )");
    const IR::P4Program* pgm = parse_string(program);
    ReferenceMap refMap;
    TypeMap      typeMap;
    // frontend passes
    PassManager  passes({
        new CreateBuiltins(),
        new TypeChecking(&refMap, &typeMap)
    });
    pgm = pgm->apply(passes);
    const IR::ToplevelBlock *toplevel = nullptr;
    auto evaluator = new P4::EvaluatorPass(&refMap, &typeMap);
    PassManager midpass({
        evaluator,
        new VisitFunctor([this, evaluator, &toplevel]() { toplevel = evaluator->getToplevelBlock(); })
    });
    pgm = pgm->apply(midpass);

    // backend passes
    BMV2::Backend backend;
    backend.run(toplevel);
    backend.serialize(std::cout);
}
