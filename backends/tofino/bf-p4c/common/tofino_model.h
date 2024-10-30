/**
 * Copyright (C) 2024 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not
 * use this file except in compliance with the License.  You may obtain a copy
 * of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software distributed
 * under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
 * CONDITIONS OF ANY KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations under the License.
 *
 *
 * SPDX-License-Identifier: Apache-2.0
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
