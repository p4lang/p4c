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

#ifndef _FRONTENDS_P4_FROMV1_0_V1MODEL_H_
#define _FRONTENDS_P4_FROMV1_0_V1MODEL_H_

#include "lib/cstring.h"
#include "frontends/common/model.h"
#include "frontends/p4/coreLibrary.h"
#include "frontends/p4/methodInstance.h"
#include "ir/ir.h"
#include "lib/json.h"

namespace P4V1 {

// This should be kept in sync with p4includes/v1model.p4
// In a perfect world this would be generated automatically from
// p4includes/v1model.p4

struct Parser_Model : public ::Model::Elem {
    Parser_Model(Model::Type_Model headersType, Model::Type_Model userMetaType,
                 Model::Type_Model standardMetadataType) :
            Model::Elem("ParserImpl"),
            packetParam("packet", P4::P4CoreLibrary::instance.packetIn, 0),
            headersParam("hdr", headersType, 1),
            metadataParam("meta", userMetaType, 2),
            standardMetadataParam("standard_metadata", standardMetadataType, 3)
    {}
    ::Model::Param_Model packetParam;
    ::Model::Param_Model headersParam;
    ::Model::Param_Model metadataParam;
    ::Model::Param_Model standardMetadataParam;
};

struct Deparser_Model : public ::Model::Elem {
    explicit Deparser_Model(Model::Type_Model headersType) :
            Model::Elem("DeparserImpl"),
            packetParam("packet", P4::P4CoreLibrary::instance.packetOut, 0),
            headersParam("hdr", headersType, 1)
    {}
    ::Model::Param_Model packetParam;
    ::Model::Param_Model headersParam;
};

// Models ingress and egress
struct Control_Model : public ::Model::Elem {
    Control_Model(cstring name, Model::Type_Model headersType, Model::Type_Model metadataType,
                 Model::Type_Model standardMetadataType) :
            Model::Elem(name),
            headersParam("hdr", headersType, 0),
            metadataParam("meta", metadataType, 1),
            standardMetadataParam("standard_metadata", standardMetadataType, 2) {}
    ::Model::Param_Model headersParam;
    ::Model::Param_Model metadataParam;
    ::Model::Param_Model standardMetadataParam;
};

struct VerifyUpdate_Model : public ::Model::Elem {
    VerifyUpdate_Model(cstring name, Model::Type_Model headersType) :
            Model::Elem(name),
            headersParam("hdr", headersType, 0) {}
    ::Model::Param_Model headersParam;
};

struct CounterType_Model : public ::Model::Enum_Model {
    CounterType_Model() : ::Model::Enum_Model("CounterType"),
        packets("packets"), bytes("bytes"), both("packets_and_bytes") {}
    ::Model::Elem packets;
    ::Model::Elem bytes;
    ::Model::Elem both;
};

struct MeterType_Model : public ::Model::Enum_Model {
    MeterType_Model() : ::Model::Enum_Model("MeterType"),
                        packets("packets"), bytes("bytes") {}
    ::Model::Elem packets;
    ::Model::Elem bytes;
};

struct ActionProfile_Model : public ::Model::Extern_Model {
    ActionProfile_Model() : Extern_Model("action_profile"),
                            sizeType(IR::Type_Bits::get(32)), sizeParam("size") {}
    const IR::Type* sizeType;
    ::Model::Elem sizeParam;
};

struct ActionSelector_Model : public ::Model::Extern_Model {
    ActionSelector_Model() : Extern_Model("action_selector"),
                             sizeType(IR::Type_Bits::get(32)), sizeParam("size"),
                             widthType(IR::Type_Bits::get(32)),
                             algorithmParam("algorithm") {}
    const IR::Type* sizeType;
    ::Model::Elem sizeParam;
    const IR::Type* widthType;
    ::Model::Elem algorithmParam;
};

struct Random_Model : public ::Model::Elem {
    Random_Model() : Elem("random"),
                     modify_field_rng_uniform("modify_field_rng_uniform") {}
    ::Model::Elem   modify_field_rng_uniform;
};

class Truncate : public Model::Extern_Model {
 public:
    Truncate() : Extern_Model("truncate"), length_type(IR::Type::Bits::get(32)) {}
    const IR::Type* length_type;
};

struct CounterOrMeter_Model : public ::Model::Extern_Model {
    explicit CounterOrMeter_Model(cstring name) : Extern_Model(name),
                      sizeParam("size"), typeParam("type"),
                      size_type(IR::Type_Bits::get(32)),
                      index_type(IR::Type_Bits::get(32)), counterType() {}
    ::Model::Elem sizeParam;
    ::Model::Elem typeParam;
    const IR::Type* size_type;
    const IR::Type* index_type;
    CounterType_Model counterType;
    MeterType_Model meterType;
};

struct Register_Model : public ::Model::Extern_Model {
    Register_Model() : Extern_Model("register"),
                       sizeParam("size"), read("read"), write("write"),
                       size_type(IR::Type_Bits::get(32)),
                       index_type(IR::Type_Bits::get(32)) {}
    ::Model::Elem sizeParam;
    ::Model::Elem read;
    ::Model::Elem write;
    const IR::Type* size_type;
    const IR::Type* index_type;
};

struct DigestReceiver_Model : public ::Model::Elem {
    DigestReceiver_Model() : Elem("digest"), receiverType(IR::Type_Bits::get(32)) {}
    const IR::Type* receiverType;
};

struct Counter_Model : public CounterOrMeter_Model {
    Counter_Model() : CounterOrMeter_Model("counter"), increment("count") {}
    ::Model::Elem increment;
};

struct Meter_Model : public CounterOrMeter_Model {
    Meter_Model() : CounterOrMeter_Model("meter"), executeMeter("execute_meter") {}
    ::Model::Elem executeMeter;
};

struct DirectMeter_Model : public CounterOrMeter_Model {
    DirectMeter_Model() : CounterOrMeter_Model("direct_meter"), read("read") {}
    ::Model::Elem read;
};

struct DirectCounter_Model : public CounterOrMeter_Model {
    DirectCounter_Model() : CounterOrMeter_Model("direct_counter"), count("count") {}
    ::Model::Elem count;
};

struct StandardMetadataType_Model : public ::Model::Type_Model {
    explicit StandardMetadataType_Model(cstring name) :
            ::Model::Type_Model(name), dropBit("drop"), recirculate("recirculate_port"),
            egress_spec("egress_spec") {}
    ::Model::Elem dropBit;
    ::Model::Elem recirculate;
    ::Model::Elem egress_spec;
};

struct CloneType_Model : public ::Model::Enum_Model {
    CloneType_Model() : ::Model::Enum_Model("CloneType"),
                        i2e("I2E"), e2e("E2E") {}
    ::Model::Elem i2e;
    ::Model::Elem e2e;
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

struct Hash_Model : public ::Model::Elem {
    Hash_Model() : ::Model::Elem("hash") {}
};

struct Cloner_Model : public ::Model::Extern_Model {
    Cloner_Model() : Extern_Model("clone"), clone3("clone3"), cloneType(),
                     sessionType(IR::Type_Bits::get(32)) {}
    ::Model::Elem   clone3;
    CloneType_Model cloneType;
    const IR::Type* sessionType;
};

struct Switch_Model : public ::Model::Elem {
    Switch_Model() : Model::Elem("V1Switch"),
                     parser("p"), verify("vr"), ingress("ig"),
                     egress("eg"), compute("ck"), deparser("dep") {}
    ::Model::Elem parser;  // names of the package arguments
    ::Model::Elem verify;
    ::Model::Elem ingress;
    ::Model::Elem egress;
    ::Model::Elem compute;
    ::Model::Elem deparser;
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
    const unsigned defaultTableSize = 1024;
};


class V1Model : public ::Model::Model {
 protected:
    V1Model() :
            Model::Model("0.1"), file("v1model.p4"),
            standardMetadata("standard_metadata"),
            // The following 2 are not really docmented in the P4-14 spec.
            intrinsicMetadata("intrinsic_metadata"),
            queueingMetadata("queueing_metadata"),
            headersType("headers"),
            metadataType("metadata"),
            standardMetadataType("standard_metadata_t"),
            parser(headersType, metadataType, standardMetadataType), deparser(headersType),
            egress("egress", headersType, metadataType, standardMetadataType),
            ingress("ingress", headersType, metadataType, standardMetadataType),
            sw(), counterOrMeter("$"), counter(), meter(), random(), action_profile(),
            action_selector(), clone(), resubmit("resubmit"),
            tableAttributes(), rangeMatchType("range"), selectorMatchType("selector"),
            verify("verifyChecksum", headersType), compute("computeChecksum", headersType),
            digest_receiver(), hash(), algorithm(),
            registers(), drop("mark_to_drop"),
            recirculate("recirculate"), verify_checksum("verify_checksum"),
            update_checksum("update_checksum"),
            verify_checksum_with_payload("verify_checksum_with_payload"),
            update_checksum_with_payload("update_checksum_with_payload"),
            directMeter(), directCounter()
    {}

 public:
    ::Model::Elem       file;
    ::Model::Elem       standardMetadata;
    ::Model::Elem       intrinsicMetadata;
    ::Model::Elem       queueingMetadata;
    ::Model::Type_Model headersType;
    ::Model::Type_Model metadataType;
    StandardMetadataType_Model standardMetadataType;
    Parser_Model        parser;
    Deparser_Model      deparser;
    Control_Model       egress;
    Control_Model       ingress;
    Switch_Model        sw;
    Truncate            truncate;
    CounterOrMeter_Model counterOrMeter;
    Counter_Model       counter;
    Meter_Model         meter;
    Random_Model        random;
    ActionProfile_Model action_profile;
    ActionSelector_Model action_selector;
    Cloner_Model        clone;
    ::Model::Elem       resubmit;
    TableAttributes_Model tableAttributes;
    ::Model::Elem       rangeMatchType;
    ::Model::Elem       selectorMatchType;
    VerifyUpdate_Model  verify;
    VerifyUpdate_Model  compute;
    DigestReceiver_Model digest_receiver;
    Hash_Model          hash;
    Algorithm_Model     algorithm;
    Register_Model      registers;
    ::Model::Elem       drop;
    ::Model::Elem       recirculate;
    ::Model::Elem       verify_checksum;
    ::Model::Elem       update_checksum;
    ::Model::Elem       verify_checksum_with_payload;
    ::Model::Elem       update_checksum_with_payload;
    DirectMeter_Model   directMeter;
    DirectCounter_Model directCounter;

    static V1Model instance;
};

}  // namespace P4V1

#endif /* _FRONTENDS_P4_FROMV1_0_V1MODEL_H_ */
