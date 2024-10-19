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

#ifndef BF_P4C_COMMON_TOFINO_MODEL_H_
#define BF_P4C_COMMON_TOFINO_MODEL_H_

#include "frontends/common/model.h"
#include "frontends/p4/coreLibrary.h"
#include "frontends/p4/methodInstance.h"
#include "ir/ir.h"
#include "lib/cstring.h"
#include "lib/json.h"

namespace P4 {

using ::Model::Elem;
using ::Model::Param_Model;
using ::Model::Type_Model;

struct IngressInMetadataType_Model : public ::Model::Type_Model {
    explicit IngressInMetadataType_Model(cstring name)
        : ::Model::Type_Model(name),
          dropBit("drop"),
          recirculate("recirculate_port"),
          egress_spec("egress_spec") {}
    ::Model::Elem dropBit;
    ::Model::Elem recirculate;
    ::Model::Elem egress_spec;
};

struct Ingress_Model : public ::Model::Elem {
    Ingress_Model(cstring name, Model::Type_Model headersType, Model::Type_Model metadataType,
                  Model::Type_Model ingressInMetadataType, Model::Type_Model ingressOutMetadataType)
        : Model::Elem(name),
          headersParam("hdr", headersType, 0),
          metadataParam("meta", metadataType, 1),
          ingressInMetadataParam("mi", ingressInMetadataType, 2),
          ingressOutMetadataParam("mo", ingressOutMetadataType, 3) {}
    ::Model::Param_Model headersParam;
    ::Model::Param_Model metadataParam;
    ::Model::Param_Model ingressInMetadataParam;
    ::Model::Param_Model ingressOutMetadataParam;
};

struct Egress_Model : public ::Model::Elem {
    Egress_Model(cstring name, Model::Type_Model headersType, Model::Type_Model metadataType,
                 Model::Type_Model egressInMetadataType, Model::Type_Model egressOutMetadataType)
        : Model::Elem(name),
          headersParam("hdr", headersType, 0),
          metadataParam("meta", metadataType, 1),
          egressInMetadataParam("mi", egressInMetadataType, 2),
          egressOutMetadataParam("mo", egressOutMetadataType, 3) {}
    ::Model::Param_Model headersParam;
    ::Model::Param_Model metadataParam;
    ::Model::Param_Model egressInMetadataParam;
    ::Model::Param_Model egressOutMetadataParam;
};

class TofinoModel : public ::Model::Model {
 protected:
    TofinoModel()
        : Model::Model("0.1"),
          headersType("headers"),
          metadataType("metadata"),
          ingressInMetadataType("ingress_in_metadata_t"),
          ingressOutMetadataType("ingress_out_metadata_t"),
          egressInMetadataType("egress_in_metadata_t"),
          egressOutMetadataType("egress_out_metadata_t"),
          ingress("ingress", headersType, metadataType, ingressInMetadataType,
                  ingressOutMetadataType),
          egress("egress", headersType, metadataType, egressInMetadataType, egressOutMetadataType) {
    }

 public:
    ::Model::Type_Model headersType;
    ::Model::Type_Model metadataType;
    IngressInMetadataType_Model ingressInMetadataType;
    ::Model::Type_Model ingressOutMetadataType;
    ::Model::Type_Model egressInMetadataType;
    ::Model::Type_Model egressOutMetadataType;
    Ingress_Model ingress;
    Egress_Model egress;

    static TofinoModel instance;
};

}  // namespace P4

#endif /* BF_P4C_COMMON_TOFINO_MODEL_H_ */
