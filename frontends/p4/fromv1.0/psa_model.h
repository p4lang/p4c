#ifndef BF_P4C_ARCH_PSA_MODEL_H_
#define BF_P4C_ARCH_PSA_MODEL_H_

#include "frontends/common/model.h"
#include "frontends/p4/coreLibrary.h"

namespace BFN {

namespace PSA {

// The following should be kept in sync with tna.p4
struct PacketPathType_Model : public ::Model::Enum_Model {
    PacketPathType_Model() : ::Model::Enum_Model("PSA_PacketPath_t"),
    normal("NORMAL"), normal_unicast("NORMAL_UNICAST"),
    normal_multicast("NORMAL_MULTICAST"),
    clone_i2e("CLONE_I2E"), clone_e2e("CLONE_E2E"), resubmit("RESUBMIT"),
    recirculate("RECIRCULATE") { }
    ::Model::Elem normal;
    ::Model::Elem normal_unicast;
    ::Model::Elem normal_multicast;
    ::Model::Elem clone_i2e;
    ::Model::Elem clone_e2e;
    ::Model::Elem resubmit;
    ::Model::Elem recirculate;
};

struct MeterType_Model : public ::Model::Enum_Model {
    MeterType_Model() : ::Model::Enum_Model("PSA_MeterType_t"),
                        packets("PACKETS"), bytes("PACKETS") {}
    ::Model::Elem packets;
    ::Model::Elem bytes;
};

struct MeterColor_Model : public ::Model::Enum_Model {
    MeterColor_Model() : ::Model::Enum_Model("PSA_MeterColor_t"),
                        green("GREEN"), yellow("YELLOW"), red("RED") {}
    ::Model::Elem green;
    ::Model::Elem yellow;
    ::Model::Elem red;
};

struct CounterType_Model : public ::Model::Enum_Model {
    CounterType_Model() : ::Model::Enum_Model("PSA_CounterType_t"),
                          packets("PACKETS"), bytes("BYTES"),
                          both("PACKETS_AND_BYTES") {}
    ::Model::Elem packets;
    ::Model::Elem bytes;
    ::Model::Elem both;
};

struct HashAlgorithmType_Model : public ::Model::Enum_Model {
    HashAlgorithmType_Model() : ::Model::Enum_Model("PSA_HashAlgorithm_t"),
                                identify("IDENTITY"),
                                crc16("CRC16"), crc16_custom("CRC16_CUSTOM"),
                                crc32("CRC32"), crc32_custom("CRC32_CUSTOM"),
                                ones_complement16("ONES_COMPLEMENT16"),
                                target_default("TARGET_DEFAULT") { }
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
        ::Model::Type_Model("psa_ingress_parser_input_metadata_t"),
        ingress_port("ingress_port"),
        packet_path("packet_path") {}
    ::Model::Elem ingress_port;
    ::Model::Elem packet_path;
};

struct EgressParserInputMetaType_Model : public ::Model::Type_Model {
    EgressParserInputMetaType_Model() :
        ::Model::Type_Model("psa_egress_parser_input_metadata_t"),
        egress_port("egress_port"),
        packet_path("packet_path") {}
    ::Model::Elem egress_port;
    ::Model::Elem packet_path;
};

struct IngressInputMetaType_Model : public ::Model::Type_Model {
    IngressInputMetaType_Model() :
        ::Model::Type_Model("psa_ingress_input_metadata_t"),
        ingress_port("ingress_port"),
        packet_path("packet_path"), ingress_timestamp("ingress_timestamp"),
        parser_error("parser_error") {}
    ::Model::Elem ingress_port;
    ::Model::Elem packet_path;
    ::Model::Elem ingress_timestamp;
    ::Model::Elem parser_error;
};

struct IngressOutputMetaType_Model : public ::Model::Type_Model {
    IngressOutputMetaType_Model() :
        ::Model::Type_Model("psa_ingress_output_metadata_t"),
        class_of_service("class_of_service"),
        clone("clone"), clone_session_id("clone_session_id"),
        drop("drop"), resubmit("resubmit"),
        multicast_group("multicast_group"), egress_port("egress_port") { }
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
        ::Model::Type_Model("psa_egress_input_metadata_t"),
        class_of_service("class_of_service"),
        egress_port("egress_port"), packet_path("packet_path"),
        instance("instance"), egress_timestamp("egress_timestamp"),
        parser_error("parser_error") { }
    ::Model::Elem class_of_service;
    ::Model::Elem egress_port;
    ::Model::Elem packet_path;
    ::Model::Elem instance;
    ::Model::Elem egress_timestamp;
    ::Model::Elem parser_error;
};

struct EgressDeparserInputMetaType_Model : public ::Model::Type_Model {
    EgressDeparserInputMetaType_Model() :
        ::Model::Type_Model("psa_egress_deparser_input_metadata_t"),
        egress_port("egress_port") { }
    ::Model::Elem egress_port;
};

struct EgressOutputMetaType_Model : public ::Model::Type_Model {
    EgressOutputMetaType_Model() :
        ::Model::Type_Model("psa_egress_output_metadata_t"),
        clone("clone"), clone_session_id("clone_session_id"), drop("drop") { }
    ::Model::Elem clone;
    ::Model::Elem clone_session_id;
    ::Model::Elem drop;
};

struct CompilerGeneratedMetaType_Model : public ::Model::Type_Model {
     explicit CompilerGeneratedMetaType_Model(cstring name) :
        ::Model::Type_Model("compiler_generated_metadata_t"),
        instance_name(name),
        mirror_id("mirror_id"), mirror_source("mirror_source"),
        clone_src("clone_src"), clone_digest_id("clone_digest_id") { }
    ::Model::Elem instance_name;
    ::Model::Elem mirror_id;
    ::Model::Elem mirror_source;
    ::Model::Elem clone_src;
    ::Model::Elem clone_digest_id;
};

struct Checksum_Model : public ::Model::Extern_Model {
    Checksum_Model() : Extern_Model("Checksum"),
                       clear("clear"), update("update"),
                       get("get") { }
    HashAlgorithmType_Model algorithm;
    ::Model::Elem clear;
    ::Model::Elem update;
    ::Model::Elem get;
};

struct InternetChecksum_Model : public ::Model::Extern_Model {
    InternetChecksum_Model() : Extern_Model("InternetChecksum"),
                               clear("clear"), add("add"),
                               subtract("subtract"), get("get"),
                               get_state("get_state"), set_state("set_state") { }
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
        Extern_Model("Hash"), get("get"),
        get_with_base("get") { }
    HashAlgorithmType_Model algorithm;
    ::Model::Elem get;
    ::Model::Elem get_with_base;
};

struct Random_Model : public ::Model::Extern_Model {
    Random_Model() :
        Extern_Model("Random"), get("get") {}
    ::Model::Elem get;
};

struct Counter_Model : public ::Model::Extern_Model {
    Counter_Model() :
        Extern_Model("Counter"), counterType(), count("count"), sizeParam("size") {}
    CounterType_Model counterType;
    ::Model::Elem count;
    ::Model::Elem sizeParam;
};

struct DirectCounter_Model : public ::Model::Extern_Model {
    DirectCounter_Model() :
        Extern_Model("DirectCounter"), counterType(), count("count") {}
    CounterType_Model counterType;
    ::Model::Elem count;
};

struct Meter_Model : public ::Model::Extern_Model {
    Meter_Model() :
        Extern_Model("Meter"), meterType(), execute("execute"), sizeParam("size"), typeParam("type") {}
    MeterType_Model meterType;
    ::Model::Elem execute;
    ::Model::Elem sizeParam;
    ::Model::Elem typeParam;
};

struct DirectMeter_Model : public ::Model::Extern_Model {
    DirectMeter_Model() :
        Extern_Model("DirectMeter"), meterType(), execute("execute"), typeParam("type") {}
    MeterType_Model meterType;
    ::Model::Elem execute;
    ::Model::Elem typeParam;
};

struct Register_Model : public ::Model::Extern_Model {
    Register_Model() :
        Extern_Model("Register"),
        sizeParam("size"), size_type(IR::Type_Bits::get(32)),
        index_type(IR::Type_Bits::get(32)),
        read("read"), write("write") {}
    ::Model::Elem sizeParam;
    const IR::Type* size_type;
    const IR::Type* index_type;
    ::Model::Elem read;
    ::Model::Elem write;
};

struct ActionProfile_Model : public ::Model::Extern_Model {
    ActionProfile_Model() : Extern_Model("ActionProfile"),
                            sizeType(IR::Type_Bits::get(32)), sizeParam("size") {}
    const IR::Type* sizeType;
    ::Model::Elem sizeParam;
};

struct ActionSelector_Model : public ::Model::Extern_Model {
    ActionSelector_Model() : Extern_Model("ActionSelector"),
                             hashType(),
                             sizeType(IR::Type_Bits::get(32)), sizeParam("size"),
                             outputWidthType(IR::Type_Bits::get(32)),
                             outputWidthParam("outputWidth"), algorithmParam("algorithm") { }
    HashAlgorithmType_Model hashType;
    const IR::Type* sizeType;
    ::Model::Elem sizeParam;
    const IR::Type* outputWidthType;
    ::Model::Elem outputWidthParam;
    ::Model::Elem algorithmParam;
};

struct Algorithm_Model : public ::Model::Enum_Model {
        Algorithm_Model() : ::Model::Enum_Model("HashAlgorithm"),
                            crc32("crc32"), crc32_custom("crc32_custom"),
                            crc16("crc16"), crc16_custom("crc16_custom"),
                            random("random"), identity("identity"), csum16("csum16"), xor16("xor16") {}
        ::Model::Elem crc32;
        ::Model::Elem crc32_custom;
        ::Model::Elem crc16;
        ::Model::Elem crc16_custom;
        ::Model::Elem random;
        ::Model::Elem identity;
        ::Model::Elem csum16;
        ::Model::Elem xor16;
    };

struct Digest_Model: public ::Model::Extern_Model {
    Digest_Model() : Extern_Model("Digest"), pack("pack") { }
    ::Model::Elem pack;
};

struct TableAttributes_Model {
    TableAttributes_Model() : tableImplementation("implementation"),
                              counters("counters"),
                              meters("meters"), size("size"),
                              supportTimeout("support_timeout") {}
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
        Model::Elem("IngressParser"),
        packetParam("buffer", P4::P4CoreLibrary::instance.packetIn, 0),
        headersParam("parsed_hdr", headersType, 1),
        metadataParam("user_meta", userMetaType, 2),
        istdParam("istd", istdType, 3),
        resubmitParam("resubmit_meta", resubmitMetaType, 4),
        recircParam("recirculate_meta", recircMetaType, 5) { /* empty */ }
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
        Model::Elem("Ingress"),
        headersParam("hdr", headersType, 0),
        metadataParam("user_meta", userMetaType, 1),
        istdParam("istd", istdType, 2),
        ostdParam("ostd", ostdType, 3)
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
        Model::Elem("IngressDeparser"),
        packetParam("buffer", P4::P4CoreLibrary::instance.packetIn, 0),
        cloneParam("clone_i2e_meta", cloneType, 1),
        resubmitParam("resubmit_meta", resubmitMetaType, 2),
        normalMetaParam("normal_meta", bridgeMetaType, 3),
        headersParam("hdr", headersType, 4),
        metadataParam("meta", userMetaType, 5),
        istdParam("istd", istdType, 6)
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
        Model::Elem("EgressParser"),
        packetParam("buffer", P4::P4CoreLibrary::instance.packetIn, 0),
        headersParam("parsed_hdr", headersType, 1),
        metadataParam("user_meta", userMetaType, 2),
        istdMetaParam("istd", istdMetaType, 3),
        normalMetaParam("normal_meta", bridgeMetaType, 4),
        cloneI2EMetaParam("clone_i2e_meta", cloneI2EMetaType, 5),
        cloneE2EMetaParam("clone_e2e_meta", cloneE2EMetaType, 6) { /* empty */ }
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
        Model::Elem("Egress"),
        headersParam("hdr", headersType, 0),
        metadataParam("user_meta", userMetaType, 1),
        istdParam("istd", istdType, 2),
        ostdParam("ostd", ostdType, 3)
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
        Model::Elem("EgressDeparser"),
        packetParam("buffer", P4::P4CoreLibrary::instance.packetIn, 0),
        cloneE2EMetaParam("clone_e2e_meta", cloneE2EMetaType, 1),
        recircMetaParam("recirculate_meta", recircMetaType, 2),
        headersParam("hdr", headersType, 3),
        metadataParam("meta", userMetaType, 4),
        istdParam("istd", istdType, 5),
        edstdParam("edstd", edstdType, 6)
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
        ingressParser("ingress_parser"),
        ingress("ingress"),
        ingressDeparser("ingress_deparser"),
        packetReplicationEngine("pre"),
        egressParser("egress_parser"),
        egress("egress"),
        egressDeparser("egress_deparser"),
        bufferingQueueingEngine("bqe") { /* empty */ }
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
 public:
    PsaModel() :
        Model::Model("1.0"), file("psa.p4"),
        headersType("headers"),
        metadataType("metadata"),
        resubmitMetaType("resubmit_meta"), recircMetaType("recirc_meta"),
        cloneI2EMetaType("clone_i2e_meta"),
        cloneE2EMetaType("clone_e2e_meta"), bridgeMetaType("bridge_meta"),
        compilerGeneratedType("compiler_generated_meta"),
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
        sw("Switch")
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
    Algorithm_Model     algorithm;
    Meter_Model          meter;
    Random_Model         random;
    Register_Model       registers;

    // tables
    TableAttributes_Model tableAttributes;

    static PsaModel instance;
};

}  // namespace PSA

}  // namespace BFN

#endif  /* BF_P4C_ARCH_PSA_MODEL_H_*/
