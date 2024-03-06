#include "backends/p4tools/modules/smith/backends/structFieldList.h"

#include <vector>

namespace P4Tools {

namespace P4Smith {

IR::IndexedVector<IR::StructField> get_sfs(std::vector<cstring> &fields, std::vector<int> &bit_size,
                                           size_t vec_size) {
    IR::IndexedVector<IR::StructField> ret_fields;

    for (size_t i = 0; i < vec_size; i++) {
        cstring name = fields.at(i);
        IR::Type *tp = new IR::Type_Bits(bit_size.at(i), false);
        ret_fields.push_back(new IR::StructField(name, tp));
    }

    return ret_fields;
}

IR::IndexedVector<IR::StructField> structFieldList::gen_sm() {
    IR::IndexedVector<IR::StructField> fields;

    // IR::ID   *name;
    // IR::Type *tp;

    /*
       name = new IR::ID("ingress_port");
       tp = new IR::Type_Bits(9, false);
       fields.push_back(new IR::StructField(*name, tp));
       name = new IR::ID("egress_spec");
       tp = new IR::Type_Bits(9, false);
       fields.push_back(new IR::StructField(*name, tp));
       name = new IR::ID("egress_port");
       tp = new IR::Type_Bits(9, false);
       fields.push_back(new IR::StructField(*name, tp));
       name = new IR::ID("instance_type");
       tp = new IR::Type_Bits(32, false);
       fields.push_back(new IR::StructField(*name, tp));
       name = new IR::ID("packet_length");
       tp = new IR::Type_Bits(32, false);
       fields.push_back(new IR::StructField(*name, tp));
       name = new IR::ID("enq_timestamp");
       tp = new IR::Type_Bits(32, false);
       fields.push_back(new IR::StructField(*name, tp));
       name = new IR::ID("enq_qdepth");
       tp = new IR::Type_Bits(19, false);
       fields.push_back(new IR::StructField(*name, tp));
       // name = new IR::ID("dep_timedelta");
       // tp = new IR::Type_Bits(32, false);
       // fields.push_back(new IR::StructField(*name, tp));
       name = new IR::ID("deq_qdepth");
       tp = new IR::Type_Bits(19, false);
       fields.push_back(new IR::StructField(*name, tp));
       name = new IR::ID("ingress_global_timestamp");
       tp = new IR::Type_Bits(48, false);
       fields.push_back(new IR::StructField(*name, tp));
       name = new IR::ID("egress_global_timestamp");
       tp = new IR::Type_Bits(48, false);
       fields.push_back(new IR::StructField(*name, tp));
       name = new IR::ID("egress_rid");
       tp = new IR::Type_Bits(16, false);
       fields.push_back(new IR::StructField(*name, tp));
       // Tao: error is omitted here
       name = new IR::ID("priority");
       tp = new IR::Type_Bits(3, false);
       fields.push_back(new IR::StructField(*name, tp));
     */

    return fields;
}

IR::IndexedVector<IR::StructField> structFieldList::gen_tf_ing_md_t() {
    std::vector<cstring> fields = {
        // "resubmit_flag",
    };

    std::vector<int> bit_size = {
        // 1,
    };

    return get_sfs(fields, bit_size, fields.size());
}

IR::IndexedVector<IR::StructField> structFieldList::gen_tf_ing_md_for_tm_t() {
    std::vector<cstring> fields = {
        // "ucast_egress_port",
        // "bypass_egress",
        // "deflect_on_drop",
        // "ingress_cos",
        // "qid",
        // "icos_for_copy_to_cpu",
        // "copy_to_cpu",
        // "packet_color",
        // "disable_ucast_cutthru",
        // "enable_mcast_cutthru",
        // "mcast_grp_a",
        // "mcast_grp_b",
        // "level1_mcast_hash",
        // "level2_mcast_hash",
        // "level1_exclusion_id",
        // "level2_exclusion_id",
        // "rid",
    };

    std::vector<int> bit_size = {
        // 9,
        // 1,
        // 1,
        // 3,
        // 5,
        // 3,
        // 1,
        // 2,
        // 1,
        // 1,
        // 16,
        // 16,
        // 13,
        // 13,
        // 16,
        // 9,
        // 16,
    };

    return get_sfs(fields, bit_size, fields.size());
}

IR::IndexedVector<IR::StructField> structFieldList::gen_tf_ing_intr_md_from_prsr() {
    std::vector<cstring> fields = {
        // "parser_err",
    };

    std::vector<int> bit_size = {
        // 16,
    };

    return get_sfs(fields, bit_size, fields.size());
}

IR::IndexedVector<IR::StructField> structFieldList::gen_tf_ing_intr_md_for_deprsr() {
    std::vector<cstring> fields = {
        // "drop_ctl",
        // "digest_type",
        // "resubmit_type",
        // "mirror_type",
    };

    std::vector<int> bit_size = {
        // 3,
        // 3,
        // 3,
        // 3,
    };

    return get_sfs(fields, bit_size, fields.size());
}

IR::IndexedVector<IR::StructField> structFieldList::gen_tf_eg_intr_md_t() {
    std::vector<cstring> fields = {
        // "egress_port",
    };

    std::vector<int> bit_size = {
        // 9,
    };

    return get_sfs(fields, bit_size, fields.size());
}

IR::IndexedVector<IR::StructField> structFieldList::gen_tf_eg_intr_md_from_prsr() {
    std::vector<cstring> fields = {
        // "parser_err",
    };

    std::vector<int> bit_size = {
        // 16,
    };

    return get_sfs(fields, bit_size, fields.size());
}

IR::IndexedVector<IR::StructField> structFieldList::gen_tf_eg_intr_md_for_deprsr() {
    std::vector<cstring> fields = {
        // "drop_ctl",
        // "mirror_type",
        // "coalesce_flush",
        // "coalesce_length",
    };

    std::vector<int> bit_size = {
        // 3,
        // 3,
        // 1,
        // 7,
    };

    return get_sfs(fields, bit_size, fields.size());
}

IR::IndexedVector<IR::StructField> structFieldList::gen_tf_eg_intr_md_for_output_port() {
    std::vector<cstring> fields = {
        // "force_tx_error",
    };

    std::vector<int> bit_size = {
        // 1,
    };

    return get_sfs(fields, bit_size, fields.size());
}

}  // namespace P4Smith

}  // namespace P4Tools
