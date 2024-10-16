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

#ifndef BF_P4C_PARDE_ADD_METADATA_POV_H_
#define BF_P4C_PARDE_ADD_METADATA_POV_H_

#include "ir/ir.h"
#include "bf-p4c/phv/phv_fields.h"



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
