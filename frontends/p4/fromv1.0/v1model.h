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

#ifndef FRONTENDS_P4_FROMV1_0_V1MODEL_H_
#define FRONTENDS_P4_FROMV1_0_V1MODEL_H_

#include "frontends/common/model.h"
#include "frontends/p4/coreLibrary.h"
#include "frontends/p4/methodInstance.h"
#include "ir/ir.h"
#include "lib/cstring.h"
#include "lib/json.h"

namespace P4::P4V1 {

using namespace P4::literals;

// This should be kept in sync with p4includes/v1model.p4
// In a perfect world this would be generated automatically from
// p4includes/v1model.p4

struct Parser_Model : public ::P4::Model::Elem {
    Parser_Model(Model::Type_Model headersType, Model::Type_Model userMetaType,
                 Model::Type_Model standardMetadataType)
        : Model::Elem("ParserImpl"_cs),
          packetParam("packet"_cs, P4::P4CoreLibrary::instance().packetIn, 0),
          headersParam("hdr"_cs, headersType, 1),
          metadataParam("meta"_cs, userMetaType, 2),
          standardMetadataParam("standard_metadata"_cs, standardMetadataType, 3) {}
    ::P4::Model::Param_Model packetParam;
    ::P4::Model::Param_Model headersParam;
    ::P4::Model::Param_Model metadataParam;
    ::P4::Model::Param_Model standardMetadataParam;
};

struct Deparser_Model : public ::P4::Model::Elem {
    explicit Deparser_Model(Model::Type_Model headersType)
        : Model::Elem("DeparserImpl"_cs),
          packetParam("packet"_cs, P4::P4CoreLibrary::instance().packetOut, 0),
          headersParam("hdr"_cs, headersType, 1) {}
    ::P4::Model::Param_Model packetParam;
    ::P4::Model::Param_Model headersParam;
};

// Models ingress and egress
struct Control_Model : public ::P4::Model::Elem {
    Control_Model(cstring name, Model::Type_Model headersType, Model::Type_Model metadataType,
                  Model::Type_Model standardMetadataType)
        : Model::Elem(name),
          headersParam("hdr"_cs, headersType, 0),
          metadataParam("meta"_cs, metadataType, 1),
          standardMetadataParam("standard_metadata"_cs, standardMetadataType, 2) {}
    ::P4::Model::Param_Model headersParam;
    ::P4::Model::Param_Model metadataParam;
    ::P4::Model::Param_Model standardMetadataParam;
};

struct VerifyUpdate_Model : public ::P4::Model::Elem {
    VerifyUpdate_Model(cstring name, Model::Type_Model headersType)
        : Model::Elem(name), headersParam("hdr"_cs, headersType, 0) {}
    ::P4::Model::Param_Model headersParam;
};

struct CounterType_Model : public ::P4::Model::Enum_Model {
    CounterType_Model()
        : ::P4::Model::Enum_Model("CounterType"_cs),
          packets("packets"_cs),
          bytes("bytes"_cs),
          both("packets_and_bytes"_cs) {}
    ::P4::Model::Elem packets;
    ::P4::Model::Elem bytes;
    ::P4::Model::Elem both;
};

struct MeterType_Model : public ::P4::Model::Enum_Model {
    MeterType_Model()
        : ::P4::Model::Enum_Model("MeterType"_cs), packets("packets"_cs), bytes("bytes"_cs) {}
    ::P4::Model::Elem packets;
    ::P4::Model::Elem bytes;
};

struct ActionProfile_Model : public ::P4::Model::Extern_Model {
    ActionProfile_Model()
        : Extern_Model("action_profile"_cs),
          sizeType(IR::Type_Bits::get(32)),
          sizeParam("size"_cs) {}
    const IR::Type *sizeType;
    ::P4::Model::Elem sizeParam;
};

struct ActionSelector_Model : public ::P4::Model::Extern_Model {
    ActionSelector_Model()
        : Extern_Model("action_selector"_cs),
          sizeType(IR::Type_Bits::get(32)),
          sizeParam("size"_cs),
          widthType(IR::Type_Bits::get(32)),
          algorithmParam("algorithm"_cs) {}
    const IR::Type *sizeType;
    ::P4::Model::Elem sizeParam;
    const IR::Type *widthType;
    ::P4::Model::Elem algorithmParam;
};

struct Random_Model : public ::P4::Model::Elem {
    Random_Model() : Elem("random"_cs), modify_field_rng_uniform("modify_field_rng_uniform"_cs) {}
    ::P4::Model::Elem modify_field_rng_uniform;
};

class Truncate : public Model::Extern_Model {
 public:
    Truncate() : Extern_Model("truncate"_cs), length_type(IR::Type::Bits::get(32)) {}
    const IR::Type *length_type;
};

struct CounterOrMeter_Model : public ::P4::Model::Extern_Model {
    explicit CounterOrMeter_Model(cstring name)
        : Extern_Model(name),
          sizeParam("size"_cs),
          typeParam("type"_cs),
          size_type(IR::Type_Bits::get(32)) {}
    ::P4::Model::Elem sizeParam;
    ::P4::Model::Elem typeParam;
    const IR::Type *size_type;
    CounterType_Model counterType;
    MeterType_Model meterType;
};

struct Register_Model : public ::P4::Model::Extern_Model {
    Register_Model()
        : Extern_Model("register"_cs),
          sizeParam("size"_cs),
          read("read"_cs),
          write("write"_cs),
          size_type(IR::Type_Bits::get(32)) {}
    ::P4::Model::Elem sizeParam;
    ::P4::Model::Elem read;
    ::P4::Model::Elem write;
    const IR::Type *size_type;
};

struct DigestReceiver_Model : public ::P4::Model::Elem {
    DigestReceiver_Model() : Elem("digest"_cs), receiverType(IR::Type_Bits::get(32)) {}
    const IR::Type *receiverType;
};

struct Counter_Model : public CounterOrMeter_Model {
    Counter_Model() : CounterOrMeter_Model("counter"_cs), increment("count"_cs) {}
    ::P4::Model::Elem increment;
};

struct Meter_Model : public CounterOrMeter_Model {
    Meter_Model() : CounterOrMeter_Model("meter"_cs), executeMeter("execute_meter"_cs) {}
    ::P4::Model::Elem executeMeter;
};

struct DirectMeter_Model : public CounterOrMeter_Model {
    DirectMeter_Model() : CounterOrMeter_Model("direct_meter"_cs), read("read"_cs) {}
    ::P4::Model::Elem read;
};

struct DirectCounter_Model : public CounterOrMeter_Model {
    DirectCounter_Model() : CounterOrMeter_Model("direct_counter"_cs), count("count"_cs) {}
    ::P4::Model::Elem count;
};

struct StandardMetadataType_Model : public ::P4::Model::Type_Model {
    explicit StandardMetadataType_Model(cstring name)
        : ::P4::Model::Type_Model(name),
          dropBit("drop"_cs),
          recirculate("recirculate_port"_cs),
          egress_spec("egress_spec"_cs) {}
    ::P4::Model::Elem dropBit;
    ::P4::Model::Elem recirculate;
    ::P4::Model::Elem egress_spec;
};

struct CloneType_Model : public ::P4::Model::Enum_Model {
    CloneType_Model() : ::P4::Model::Enum_Model("CloneType"_cs), i2e("I2E"_cs), e2e("E2E"_cs) {}
    ::P4::Model::Elem i2e;
    ::P4::Model::Elem e2e;
};

struct Algorithm_Model : public ::P4::Model::Enum_Model {
    Algorithm_Model()
        : ::P4::Model::Enum_Model("HashAlgorithm"_cs),
          crc32("crc32"_cs),
          crc32_custom("crc32_custom"_cs),
          crc16("crc16"_cs),
          crc16_custom("crc16_custom"_cs),
          random("random"_cs),
          identity("identity"_cs),
          csum16("csum16"_cs),
          xor16("xor16"_cs) {}
    ::P4::Model::Elem crc32;
    ::P4::Model::Elem crc32_custom;
    ::P4::Model::Elem crc16;
    ::P4::Model::Elem crc16_custom;
    ::P4::Model::Elem random;
    ::P4::Model::Elem identity;
    ::P4::Model::Elem csum16;
    ::P4::Model::Elem xor16;
};

struct Hash_Model : public ::P4::Model::Elem {
    Hash_Model() : ::P4::Model::Elem("hash"_cs) {}
};

struct Cloner_Model : public ::P4::Model::Extern_Model {
    Cloner_Model()
        : Extern_Model("clone"_cs),
          clone3("clone_preserving_field_list"_cs),

          sessionType(IR::Type_Bits::get(32)) {}
    ::P4::Model::Elem clone3;
    CloneType_Model cloneType;
    const IR::Type *sessionType;
};

struct Switch_Model : public ::P4::Model::Elem {
    Switch_Model()
        : Model::Elem("V1Switch"_cs),
          parser("p"_cs),
          verify("vr"_cs),
          ingress("ig"_cs),
          egress("eg"_cs),
          compute("ck"_cs),
          deparser("dep"_cs) {}
    ::P4::Model::Elem parser;  // names of the package arguments
    ::P4::Model::Elem verify;
    ::P4::Model::Elem ingress;
    ::P4::Model::Elem egress;
    ::P4::Model::Elem compute;
    ::P4::Model::Elem deparser;
};

struct TableAttributes_Model {
    TableAttributes_Model()
        : tableImplementation("implementation"_cs),
          counters("counters"_cs),
          meters("meters"_cs),
          size("size"_cs),
          supportTimeout("support_timeout"_cs) {}
    ::P4::Model::Elem tableImplementation;
    ::P4::Model::Elem counters;
    ::P4::Model::Elem meters;
    ::P4::Model::Elem size;
    ::P4::Model::Elem supportTimeout;
    const unsigned defaultTableSize = 1024;
};

class V1Model : public Model::Model {
 protected:
    V1Model()
        : file("v1model.p4"_cs),
          standardMetadata("standard_metadata"_cs),
          // The following 2 are not really docmented in the P4-14 spec.
          intrinsicMetadata("intrinsic_metadata"_cs),
          queueingMetadata("queueing_metadata"_cs),
          headersType("headers"_cs),
          metadataType("metadata"_cs),
          standardMetadataType("standard_metadata_t"_cs),
          parser(headersType, metadataType, standardMetadataType),
          deparser(headersType),
          egress("egress"_cs, headersType, metadataType, standardMetadataType),
          ingress("ingress"_cs, headersType, metadataType, standardMetadataType),
          sw(),
          counterOrMeter("$"_cs),
          counter(),
          meter(),
          random(),
          action_profile(),
          action_selector(),
          clone(),
          resubmit("resubmit_preserving_field_list"_cs),
          tableAttributes(),
          rangeMatchType("range"_cs),
          optionalMatchType("optional"_cs),
          selectorMatchType("selector"_cs),
          verify("verifyChecksum"_cs, headersType),
          compute("computeChecksum"_cs, headersType),
          digest_receiver(),
          hash(),
          algorithm(),
          registers(),
          drop("mark_to_drop"_cs),
          recirculate("recirculate_preserving_field_list"_cs),
          verify_checksum("verify_checksum"_cs),
          update_checksum("update_checksum"_cs),
          verify_checksum_with_payload("verify_checksum_with_payload"_cs),
          update_checksum_with_payload("update_checksum_with_payload"_cs),
          log_msg("log_msg"_cs),
          directMeter(),
          directCounter() {}

 public:
    const ::P4::Model::Elem file;
    const ::P4::Model::Elem standardMetadata;
    const ::P4::Model::Elem intrinsicMetadata;
    const ::P4::Model::Elem queueingMetadata;
    const ::P4::Model::Type_Model headersType;
    const ::P4::Model::Type_Model metadataType;
    const StandardMetadataType_Model standardMetadataType;
    const Parser_Model parser;
    const Deparser_Model deparser;
    const Control_Model egress;
    const Control_Model ingress;
    const Switch_Model sw;
    const Truncate truncate;
    const CounterOrMeter_Model counterOrMeter;
    const Counter_Model counter;
    const Meter_Model meter;
    const Random_Model random;
    const ActionProfile_Model action_profile;
    const ActionSelector_Model action_selector;
    const Cloner_Model clone;
    const ::P4::Model::Elem resubmit;
    const TableAttributes_Model tableAttributes;
    const ::P4::Model::Elem rangeMatchType;
    const ::P4::Model::Elem optionalMatchType;
    const ::P4::Model::Elem selectorMatchType;
    const VerifyUpdate_Model verify;
    const VerifyUpdate_Model compute;
    const DigestReceiver_Model digest_receiver;
    const Hash_Model hash;
    const Algorithm_Model algorithm;
    const Register_Model registers;
    const ::P4::Model::Elem drop;
    const ::P4::Model::Elem recirculate;
    const ::P4::Model::Elem verify_checksum;
    const ::P4::Model::Elem update_checksum;
    const ::P4::Model::Elem verify_checksum_with_payload;
    const ::P4::Model::Elem update_checksum_with_payload;
    const ::P4::Model::Elem log_msg;
    const DirectMeter_Model directMeter;
    const DirectCounter_Model directCounter;

    static V1Model instance;
    // The following match constants appearing in v1model.p4
    static const char *versionInitial;  // 20180101
    static const char *versionCurrent;  // 20200408
};

/// Stores the version of the global constant __v1model_version used
/// in the 'version' public instance variable.
class getV1ModelVersion : public Inspector {
    bool preorder(const IR::Declaration_Constant *dc) override {
        if (dc->name == "__v1model_version") {
            auto val = dc->initializer->to<IR::Constant>();
            version = static_cast<unsigned>(val->value);
        }
        return false;
    }
    bool preorder(const IR::Declaration *) override { return false; }

 public:
    unsigned version = 0;
};

}  // namespace P4::P4V1

#endif /* FRONTENDS_P4_FROMV1_0_V1MODEL_H_ */
