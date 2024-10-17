/**
 * Copyright 2013-2024 Intel Corporation.
 *
 * This software and the related documents are Intel copyrighted materials, and your use of them
 * is governed by the express license under which they were provided to you ("License"). Unless
 * the License provides otherwise, you may not use, modify, copy, publish, distribute, disclose
 * or transmit this software or the related documents without Intel's prior written permission.
 *
 * This software and the related documents are provided as is, with no express or implied
 * warranties, other than those that are expressly stated in the License.
 */

#ifndef EXTENSIONS_BF_P4C_ARCH_PSA_PSA_MODEL_H_
#define EXTENSIONS_BF_P4C_ARCH_PSA_PSA_MODEL_H_

#include "frontends/common/model.h"
#include "frontends/p4/coreLibrary.h"

namespace BFN {

namespace PSA {

// The following should be kept in sync with tna.p4
struct PacketPathType_Model : public ::Model::Enum_Model {
    PacketPathType_Model() : ::Model::Enum_Model("PSA_PacketPath_t"_cs),
    normal("NORMAL"_cs), normal_unicast("NORMAL_UNICAST"_cs),
    normal_multicast("NORMAL_MULTICAST"_cs),
    clone_i2e("CLONE_I2E"_cs), clone_e2e("CLONE_E2E"_cs), resubmit("RESUBMIT"_cs),
    recirculate("RECIRCULATE"_cs) { }
    ::Model::Elem normal;
    ::Model::Elem normal_unicast;
    ::Model::Elem normal_multicast;
    ::Model::Elem clone_i2e;
    ::Model::Elem clone_e2e;
    ::Model::Elem resubmit;
    ::Model::Elem recirculate;
};

struct MeterType_Model : public ::Model::Enum_Model {
    MeterType_Model() : ::Model::Enum_Model("PSA_MeterType_t"_cs),
                        packets("PACKETS"_cs), bytes("PACKETS"_cs) {}
    ::Model::Elem packets;
    ::Model::Elem bytes;
};

struct MeterColor_Model : public ::Model::Enum_Model {
    MeterColor_Model() : ::Model::Enum_Model("PSA_MeterColor_t"_cs),
                        green("GREEN"_cs), yellow("YELLOW"_cs), red("RED"_cs) {}
    ::Model::Elem green;
    ::Model::Elem yellow;
    ::Model::Elem red;
};

struct CounterType_Model : public ::Model::Enum_Model {
    CounterType_Model() : ::Model::Enum_Model("PSA_CounterType_t"_cs),
                          packets("PACKETS"_cs), bytes("BYTES"_cs),
                          both("PACKETS_AND_BYTES"_cs) {}
    ::Model::Elem packets;
    ::Model::Elem bytes;
    ::Model::Elem both;
};

struct HashAlgorithmType_Model : public ::Model::Enum_Model {
    HashAlgorithmType_Model() : ::Model::Enum_Model("PSA_HashAlgorithm_t"_cs),
                                identify("IDENTITY"_cs),
                                crc16("CRC16"_cs), crc16_custom("CRC16_CUSTOM"_cs),
                                crc32("CRC32"_cs), crc32_custom("CRC32_CUSTOM"_cs),
                                ones_complement16("ONES_COMPLEMENT16"_cs),
                                target_default("TARGET_DEFAULT"_cs) { }
    ::Model::Elem identify;
    ::Model::Elem crc16;
    ::Model::Elem crc16_custom;
    ::Model::Elem crc32;
    ::Model::Elem crc32_custom;
    ::Model::Elem ones_complement16;
    ::Model::Elem target_default;
};

struct IngressParserInputMetaType_Model : public ::Model::Type_Model {
    IngressParserInputMetaType_Model() :
        ::Model::Type_Model("psa_ingress_parser_input_metadata_t"_cs),
        ingress_port("ingress_port"_cs),
        packet_path("packet_path"_cs) {}
    ::Model::Elem ingress_port;
    ::Model::Elem packet_path;
};

struct EgressParserInputMetaType_Model : public ::Model::Type_Model {
    EgressParserInputMetaType_Model() :
        ::Model::Type_Model("psa_egress_parser_input_metadata_t"_cs),
        egress_port("egress_port"_cs),
        packet_path("packet_path"_cs) {}
    ::Model::Elem egress_port;
    ::Model::Elem packet_path;
};

struct IngressInputMetaType_Model : public ::Model::Type_Model {
    IngressInputMetaType_Model() :
        ::Model::Type_Model("psa_ingress_input_metadata_t"_cs),
        ingress_port("ingress_port"_cs),
        packet_path("packet_path"_cs), ingress_timestamp("ingress_timestamp"_cs),
        parser_error("parser_error"_cs) {}
    ::Model::Elem ingress_port;
    ::Model::Elem packet_path;
    ::Model::Elem ingress_timestamp;
    ::Model::Elem parser_error;
};

struct IngressOutputMetaType_Model : public ::Model::Type_Model {
    IngressOutputMetaType_Model() :
        ::Model::Type_Model("psa_ingress_output_metadata_t"_cs),
        class_of_service("class_of_service"_cs),
        clone("clone"_cs), clone_session_id("clone_session_id"_cs),
        drop("drop"_cs), resubmit("resubmit"_cs),
        multicast_group("multicast_group"_cs), egress_port("egress_port"_cs) { }
    ::Model::Elem class_of_service;
    ::Model::Elem clone;
    ::Model::Elem clone_session_id;
    ::Model::Elem drop;
    ::Model::Elem resubmit;
    ::Model::Elem multicast_group;
    ::Model::Elem egress_port;
};

struct EgressInputMetaType_Model : public ::Model::Type_Model {
    EgressInputMetaType_Model() :
        ::Model::Type_Model("psa_egress_input_metadata_t"_cs),
        class_of_service("class_of_service"_cs),
        egress_port("egress_port"_cs), packet_path("packet_path"_cs),
        instance("instance"_cs), egress_timestamp("egress_timestamp"_cs),
        parser_error("parser_error"_cs) { }
    ::Model::Elem class_of_service;
    ::Model::Elem egress_port;
    ::Model::Elem packet_path;
    ::Model::Elem instance;
    ::Model::Elem egress_timestamp;
    ::Model::Elem parser_error;
};

struct EgressDeparserInputMetaType_Model : public ::Model::Type_Model {
    EgressDeparserInputMetaType_Model() :
        ::Model::Type_Model("psa_egress_deparser_input_metadata_t"_cs),
        egress_port("egress_port"_cs) { }
    ::Model::Elem egress_port;
};

struct EgressOutputMetaType_Model : public ::Model::Type_Model {
    EgressOutputMetaType_Model() :
        ::Model::Type_Model("psa_egress_output_metadata_t"_cs),
        clone("clone"_cs), clone_session_id("clone_session_id"_cs), drop("drop"_cs) { }
    ::Model::Elem clone;
    ::Model::Elem clone_session_id;
    ::Model::Elem drop;
};

struct CompilerGeneratedMetaType_Model : public ::Model::Type_Model {
     explicit CompilerGeneratedMetaType_Model(cstring name) :
        ::Model::Type_Model("compiler_generated_metadata_t"_cs),
        instance_name(name),
        mirror_id("mirror_id"_cs), mirror_source("mirror_source"_cs),
        clone_src("clone_src"_cs), clone_digest_id("clone_digest_id"_cs) { }
    ::Model::Elem instance_name;
    ::Model::Elem mirror_id;
    ::Model::Elem mirror_source;
    ::Model::Elem clone_src;
    ::Model::Elem clone_digest_id;
};

struct Checksum_Model : public ::Model::Extern_Model {
    Checksum_Model() : Extern_Model("Checksum"_cs),
                       clear("clear"_cs), update("update"_cs),
                       get("get"_cs) { }
    HashAlgorithmType_Model algorithm;
    ::Model::Elem clear;
    ::Model::Elem update;
    ::Model::Elem get;
};

struct InternetChecksum_Model : public ::Model::Extern_Model {
    InternetChecksum_Model() : Extern_Model("InternetChecksum"_cs),
                               clear("clear"_cs), add("add"_cs),
                               subtract("subtract"_cs), get("get"_cs),
                               get_state("get_state"_cs), set_state("set_state"_cs) { }
    HashAlgorithmType_Model algorithm;
    ::Model::Elem clear;
    ::Model::Elem add;
    ::Model::Elem subtract;
    ::Model::Elem get;
    ::Model::Elem get_state;
    ::Model::Elem set_state;
};

struct Hash_Model : public ::Model::Extern_Model {
    Hash_Model() :
        Extern_Model("Hash"_cs), get("get"_cs),
        get_with_base("get"_cs) { }
    HashAlgorithmType_Model algorithm;
    ::Model::Elem get;
    ::Model::Elem get_with_base;
};

struct Random_Model : public ::Model::Extern_Model {
    Random_Model() :
        Extern_Model("Random"_cs), get("get"_cs) {}
    ::Model::Elem get;
};

struct Counter_Model : public ::Model::Extern_Model {
    Counter_Model() :
        Extern_Model("Counter"_cs), counterType(), count("count"_cs) {}
    CounterType_Model counterType;
    ::Model::Elem count;
};

struct DirectCounter_Model : public ::Model::Extern_Model {
    DirectCounter_Model() :
        Extern_Model("DirectCounter"_cs), counterType(), count("count"_cs) {}
    CounterType_Model counterType;
    ::Model::Elem count;
};

struct Meter_Model : public ::Model::Extern_Model {
    Meter_Model() :
        Extern_Model("Meter"_cs), meterType(), execute("execute"_cs) {}
    MeterType_Model meterType;
    ::Model::Elem execute;
};

struct DirectMeter_Model : public ::Model::Extern_Model {
    DirectMeter_Model() :
        Extern_Model("DirectMeter"_cs), meterType(), execute("execute"_cs) {}
    MeterType_Model meterType;
    ::Model::Elem execute;
};

struct Register_Model : public ::Model::Extern_Model {
    Register_Model() :
        Extern_Model("Register"_cs),
        sizeParam("size"_cs), size_type(IR::Type_Bits::get(32)),
        read("read"_cs), write("write"_cs) { }
    ::Model::Elem sizeParam;
    const IR::Type* size_type;
    ::Model::Elem read;
    ::Model::Elem write;
};

struct ActionProfile_Model : public ::Model::Extern_Model {
    ActionProfile_Model() : Extern_Model("ActionProfile"_cs),
                            sizeType(IR::Type_Bits::get(32)), sizeParam("size"_cs) {}
    const IR::Type* sizeType;
    ::Model::Elem sizeParam;
};

struct ActionSelector_Model : public ::Model::Extern_Model {
    ActionSelector_Model() : Extern_Model("ActionSelector"_cs),
                             hashType(),
                             sizeType(IR::Type_Bits::get(32)), sizeParam("size"_cs),
                             outputWidthType(IR::Type_Bits::get(32)),
                             outputWidthParam("outputWidth"_cs) { }
    HashAlgorithmType_Model hashType;
    const IR::Type* sizeType;
    ::Model::Elem sizeParam;
    const IR::Type* outputWidthType;
    ::Model::Elem outputWidthParam;
};

struct Digest_Model: public ::Model::Extern_Model {
    Digest_Model() : Extern_Model("Digest"_cs), pack("pack"_cs) { }
    ::Model::Elem pack;
};

struct TableAttributes_Model {
    TableAttributes_Model() : tableImplementation("implementation"_cs),
                              counters("counters"_cs),
                              meters("meters"_cs), size("size"_cs),
                              supportTimeout("support_timeout"_cs) {}
    ::Model::Elem       tableImplementation;
    ::Model::Elem       counters;
    ::Model::Elem       meters;
    ::Model::Elem       size;
    ::Model::Elem       supportTimeout;
    const unsigned      defaultTableSize = 1024;
};

struct IngressParserModel : public ::Model::Elem {
    IngressParserModel(Model::Type_Model headersType, Model::Type_Model userMetaType,
                       Model::Type_Model istdType, Model::Type_Model resubmitMetaType,
                       Model::Type_Model recircMetaType) :
        Model::Elem("IngressParser"_cs),
        packetParam("buffer"_cs, P4::P4CoreLibrary::instance().packetIn, 0),
        headersParam("parsed_hdr"_cs, headersType, 1),
        metadataParam("user_meta"_cs, userMetaType, 2),
        istdParam("istd"_cs, istdType, 3),
        resubmitParam("resubmit_meta"_cs, resubmitMetaType, 4),
        recircParam("recirculate_meta"_cs, recircMetaType, 5) { /* empty */ }
    ::Model::Param_Model packetParam;
    ::Model::Param_Model headersParam;
    ::Model::Param_Model metadataParam;
    ::Model::Param_Model istdParam;
    ::Model::Param_Model resubmitParam;
    ::Model::Param_Model recircParam;
};

struct IngressModel : public ::Model::Elem {
    IngressModel(Model::Type_Model headersType, Model::Type_Model userMetaType,
                 Model::Type_Model istdType, Model::Type_Model ostdType):
        Model::Elem("Ingress"_cs),
        headersParam("hdr"_cs, headersType, 0),
        metadataParam("user_meta"_cs, userMetaType, 1),
        istdParam("istd"_cs, istdType, 2),
        ostdParam("ostd"_cs, ostdType, 3)
        { /* empty */ }
    ::Model::Param_Model headersParam;
    ::Model::Param_Model metadataParam;
    ::Model::Param_Model istdParam;
    ::Model::Param_Model ostdParam;
};

struct IngressDeparserModel : public ::Model::Elem {
    IngressDeparserModel(Model::Type_Model cloneType, Model::Type_Model resubmitMetaType,
                 Model::Type_Model bridgeMetaType, Model::Type_Model headersType,
                 Model::Type_Model userMetaType, Model::Type_Model istdType) :
        Model::Elem("IngressDeparser"_cs),
        packetParam("buffer"_cs, P4::P4CoreLibrary::instance().packetIn, 0),
        cloneParam("clone_i2e_meta"_cs, cloneType, 1),
        resubmitParam("resubmit_meta"_cs, resubmitMetaType, 2),
        normalMetaParam("normal_meta"_cs, bridgeMetaType, 3),
        headersParam("hdr"_cs, headersType, 4),
        metadataParam("meta"_cs, userMetaType, 5),
        istdParam("istd"_cs, istdType, 6)
    { /* empty */ }
    ::Model::Param_Model packetParam;
    ::Model::Param_Model cloneParam;
    ::Model::Param_Model resubmitParam;
    ::Model::Param_Model normalMetaParam;
    ::Model::Param_Model headersParam;
    ::Model::Param_Model metadataParam;
    ::Model::Param_Model istdParam;
};

struct EgressParserModel : public ::Model::Elem {
    EgressParserModel(Model::Type_Model headersType, Model::Type_Model userMetaType,
                      Model::Type_Model istdMetaType, Model::Type_Model bridgeMetaType,
                      Model::Type_Model cloneI2EMetaType, Model::Type_Model cloneE2EMetaType) :
        Model::Elem("EgressParser"_cs),
        packetParam("buffer"_cs, P4::P4CoreLibrary::instance().packetIn, 0),
        headersParam("parsed_hdr"_cs, headersType, 1),
        metadataParam("user_meta"_cs, userMetaType, 2),
        istdMetaParam("istd"_cs, istdMetaType, 3),
        normalMetaParam("normal_meta"_cs, bridgeMetaType, 4),
        cloneI2EMetaParam("clone_i2e_meta"_cs, cloneI2EMetaType, 5),
        cloneE2EMetaParam("clone_e2e_meta"_cs, cloneE2EMetaType, 6) { /* empty */ }
    ::Model::Param_Model packetParam;
    ::Model::Param_Model headersParam;
    ::Model::Param_Model metadataParam;
    ::Model::Param_Model istdMetaParam;
    ::Model::Param_Model normalMetaParam;
    ::Model::Param_Model cloneI2EMetaParam;
    ::Model::Param_Model cloneE2EMetaParam;
};

struct EgressModel : public ::Model::Elem {
    EgressModel(Model::Type_Model headersType, Model::Type_Model userMetaType,
                Model::Type_Model istdType, Model::Type_Model ostdType) :
        Model::Elem("Egress"_cs),
        headersParam("hdr"_cs, headersType, 0),
        metadataParam("user_meta"_cs, userMetaType, 1),
        istdParam("istd"_cs, istdType, 2),
        ostdParam("ostd"_cs, ostdType, 3)
    { /* empty */ }
    ::Model::Param_Model headersParam;
    ::Model::Param_Model metadataParam;
    ::Model::Param_Model istdParam;
    ::Model::Param_Model ostdParam;
};

struct EgressDeparserModel : public ::Model::Elem {
    EgressDeparserModel(Model::Type_Model cloneE2EMetaType,
                        Model::Type_Model recircMetaType,
                        Model::Type_Model headersType,
                        Model::Type_Model userMetaType,
                        Model::Type_Model istdType,
                        Model::Type_Model edstdType) :
        Model::Elem("EgressDeparser"_cs),
        packetParam("buffer"_cs, P4::P4CoreLibrary::instance().packetIn, 0),
        cloneE2EMetaParam("clone_e2e_meta"_cs, cloneE2EMetaType, 1),
        recircMetaParam("recirculate_meta"_cs, recircMetaType, 2),
        headersParam("hdr"_cs, headersType, 3),
        metadataParam("meta"_cs, userMetaType, 4),
        istdParam("istd"_cs, istdType, 5),
        edstdParam("edstd"_cs, edstdType, 6)
    { /* empty */ }
    ::Model::Param_Model packetParam;
    ::Model::Param_Model cloneE2EMetaParam;
    ::Model::Param_Model recircMetaParam;
    ::Model::Param_Model headersParam;
    ::Model::Param_Model metadataParam;
    ::Model::Param_Model istdParam;
    ::Model::Param_Model edstdParam;
};

struct Pipeline : public ::Model::Elem {
    explicit Pipeline(cstring name) :
        Model::Elem(name),
        ingressParser("ingress_parser"_cs),
        ingress("ingress"_cs),
        ingressDeparser("ingress_deparser"_cs),
        packetReplicationEngine("pre"_cs),
        egressParser("egress_parser"_cs),
        egress("egress"_cs),
        egressDeparser("egress_deparser"_cs),
        bufferingQueueingEngine("bqe"_cs) { /* empty */ }
    ::Model::Elem ingressParser;
    ::Model::Elem ingress;
    ::Model::Elem ingressDeparser;
    ::Model::Elem packetReplicationEngine;
    ::Model::Elem egressParser;
    ::Model::Elem egress;
    ::Model::Elem egressDeparser;
    ::Model::Elem bufferingQueueingEngine;
};

// must be kept consistent with psa.p4
class PsaModel : public ::Model::Model {
 protected:
    PsaModel() :
        Model::Model(), file("psa.p4"_cs),
        headersType("headers"_cs),
        metadataType("metadata"_cs),
        resubmitMetaType("resubmit_meta"_cs), recircMetaType("recirc_meta"_cs),
        cloneI2EMetaType("clone_i2e_meta"_cs),
        cloneE2EMetaType("clone_e2e_meta"_cs), bridgeMetaType("bridge_meta"_cs),
        compilerGeneratedType("compiler_generated_meta"_cs),
        ingress_parser(headersType, metadataType, igParserInputMetaType, resubmitMetaType,
                       recircMetaType),
        ingress(headersType, metadataType, igInputMetaType, igOutputMetaType),
        ingress_deparser(cloneI2EMetaType, resubmitMetaType, bridgeMetaType,
                         headersType, metadataType, igOutputMetaType),
        egress_parser(headersType, metadataType, egParserInputMetaType,
                      bridgeMetaType, cloneI2EMetaType, cloneE2EMetaType),
        egress(headersType, metadataType, egInputMetaType, egOutputMetaType),
        egress_deparser(cloneE2EMetaType, recircMetaType, headersType, metadataType,
                        egOutputMetaType, egDeparserInputMetaType),
        sw("Switch"_cs)
    { /* empty */ }

 public:
    ::Model::Elem        file;
    ::Model::Type_Model  headersType;
    ::Model::Type_Model  metadataType;

    IngressParserInputMetaType_Model  igParserInputMetaType;
    IngressInputMetaType_Model        igInputMetaType;
    IngressOutputMetaType_Model       igOutputMetaType;
    EgressParserInputMetaType_Model   egParserInputMetaType;
    EgressInputMetaType_Model         egInputMetaType;
    EgressOutputMetaType_Model        egOutputMetaType;
    EgressDeparserInputMetaType_Model egDeparserInputMetaType;

    ::Model::Type_Model resubmitMetaType;
    ::Model::Type_Model recircMetaType;
    ::Model::Type_Model cloneI2EMetaType;
    ::Model::Type_Model cloneE2EMetaType;
    ::Model::Type_Model bridgeMetaType;

    CompilerGeneratedMetaType_Model compilerGeneratedType;

    // blocks
    IngressParserModel   ingress_parser;
    IngressModel         ingress;
    IngressDeparserModel ingress_deparser;
    EgressParserModel    egress_parser;
    EgressModel          egress;
    EgressDeparserModel  egress_deparser;

    // pipelines
    Pipeline             sw;

    // externs
    ActionProfile_Model  action_profile;
    ActionSelector_Model action_selector;
    Checksum_Model       checksum;
    Counter_Model        counter;
    DirectCounter_Model  directCounter;
    DirectMeter_Model    directMeter;
    Hash_Model           hash;
    Meter_Model          meter;
    Random_Model         random;
    Register_Model       registers;

    // tables
    TableAttributes_Model tableAttributes;

    static PsaModel instance;
};

}  // namespace PSA

}  // namespace BFN

#endif  /* EXTENSIONS_BF_P4C_ARCH_PSA_PSA_MODEL_H_ */
