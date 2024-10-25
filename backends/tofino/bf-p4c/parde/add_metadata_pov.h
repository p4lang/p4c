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

#ifndef BF_P4C_PARDE_ADD_METADATA_POV_H_
#define BF_P4C_PARDE_ADD_METADATA_POV_H_

#include "bf-p4c/phv/phv_fields.h"
#include "ir/ir.h"

/**
 * \ingroup parde
 * @brief Create POV bits for output metadata (JBAY)
 *
 * JBay requires POV bits to control output metadata as implicit
 * PHV valid bits are gone.  We create a single POV bit for each metadata in use
 * and set the bit whenever the metadata is set.
 *
 */

class AddMetadataPOV : public Transform {
    const PhvInfo &phv;
    const IR::BFN::Deparser *dp = nullptr;

    bool equiv(const IR::Expression *a, const IR::Expression *b);
    static IR::MAU::Primitive *create_pov_write(const IR::Expression *povBit, bool validate);
    IR::Node *insert_deparser_param_pov_write(const IR::MAU::Primitive *p, bool validate);
    IR::Node *insert_deparser_digest_pov_write(const IR::MAU::Primitive *p, bool validate);
    IR::Node *insert_field_pov_read(const IR::MAU::Primitive *p);

    IR::BFN::Pipe *preorder(IR::BFN::Pipe *pipe) override;

    IR::BFN::DeparserParameter *postorder(IR::BFN::DeparserParameter *param) override;
    IR::BFN::Digest *postorder(IR::BFN::Digest *digest) override;
    IR::Node *postorder(IR::MAU::Primitive *p) override;
    IR::Node *postorder(IR::BFN::Extract *e) override;

 public:
    explicit AddMetadataPOV(PhvInfo &p) : phv(p) {}
};

#endif /* BF_P4C_PARDE_ADD_METADATA_POV_H_ */
