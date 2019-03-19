/*
    Copyright 2018 MNK Consulting, LLC.
    http://mnkcg.com

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

#ifndef _MIDEND_FLATTENHEADERS_H_
#define _MIDEND_FLATTENHEADERS_H_

#include "ir/ir.h"
#include "frontends/p4/typeChecking/typeChecker.h"

namespace P4 {

/**
 * Policy to select which annotations of the nested struct to attach
 * to the struct fields after the nest struct is flattened.
 */
class AnnotationSelectionPolicy {
 public:
    virtual ~AnnotationSelectionPolicy() {}

    /**
     * Call for each nested struct to check if the annotation of the nested
     * struct should be kept on its fields.
     *
     * @return
     *  true if the annotation should be kept on the field.
     *  false if the annotation should be discarded.
     */
    virtual bool keep(const IR::Annotation*) {
        return false;
    }
};

/**
Find the types to replace and insert them in the nested struct map.
 */
class FindHeaderTypesToReplace : public Inspector {
    P4::TypeMap* typeMap;
    AnnotationSelectionPolicy *policy;

    struct HeaderTypeReplacement : public IHasDbPrint {
        HeaderTypeReplacement(const P4::TypeMap* typeMap, const IR::Type_Header* type,
                AnnotationSelectionPolicy *policy);
        const P4::TypeMap *typeMap;

        // Maps nested field names to final field names.
        // In our example this could be:
        // .t.s.a -> _t_s_a0;
        // .t.s.b -> _t_s_b1;
        // .t.y -> _t_y2;
        // .x -> _x3;
        std::map<cstring, cstring> fieldNameRemap;
        // Holds a new flat type
        // struct M {
        //    bit _t_s_a0;
        //    bool _t_s_b1;
        //    bit<6> _t_y2;
        //    bit<3> _x3;
        // }
        const IR::Type* replacementType;
        virtual void dbprint(std::ostream& out) const {
            out << replacementType;
        }

        // Helper for constructor
        void flatten(cstring prefix,
                     const IR::Type* type,
                     const IR::Annotations* annos,
                     IR::IndexedVector<IR::StructField> *fields,
                     AnnotationSelectionPolicy *policy);
    };

    ordered_map<cstring, HeaderTypeReplacement*> replacement;

 public:
    explicit FindHeaderTypesToReplace(P4::TypeMap *typeMap,
            AnnotationSelectionPolicy *policy):
        typeMap(typeMap), policy(policy) {
        setName("FindHeaderTypesToReplace");
        CHECK_NULL(typeMap);
    }
    bool preorder(const IR::Type_Header* type) override;
    void createReplacement(const IR::Type_Header* type, AnnotationSelectionPolicy *policy);
    HeaderTypeReplacement* getReplacement(const cstring name) const {
        return ::get(replacement, name); }
    bool empty() const { return replacement.empty(); }
};

/**
 * This pass flattens any nested struct inside a P4 header

 * Should be run before the flattenInterfaceStructs pass.

For example, see the data structures below.

struct alt_t {
  bit<1> valid;
  bit<7> port;
};

struct row_t {
  alt_t alt0;
  alt_t alt1;
};

header bitvec_hdr {
  row_t row;
}

struct col_t {
  bitvec_hdr bvh;
}

struct local_metadata_t {
  row_t row0;
  row_t row1;
  col_t col;
  bitvec_hdr bvh0;
  bitvec_hdr bvh1;
};

The flattened data structures are shown below.  

struct alt_t {
    bit<1> valid;
    bit<7> port;
}

struct row_t {
    alt_t alt0;
    alt_t alt1;
}

header bitvec_hdr {
    bit<1> _row_alt0_valid0;
    bit<7> _row_alt0_port1;
    bit<1> _row_alt1_valid2;
    bit<7> _row_alt1_port3;
}

struct col_t {
    bitvec_hdr bvh;
}

struct local_metadata_t {
    bit<1>     _row0_alt0_valid0;
    bit<7>     _row0_alt0_port1;
    bit<1>     _row0_alt1_valid2;
    bit<7>     _row0_alt1_port3;
    bit<1>     _row1_alt0_valid4;
    bit<7>     _row1_alt0_port5;
    bit<1>     _row1_alt1_valid6;
    bit<7>     _row1_alt1_port7;
    bitvec_hdr _col_bvh8;
    bitvec_hdr _bvh09;
    bitvec_hdr _bvh110;
}
	
*/
class ReplaceHeaders : public Transform {
    P4::TypeMap *typeMap;
    FindHeaderTypesToReplace* findHeaderTypesToReplace;

 public:
    explicit ReplaceHeaders(P4::TypeMap *typeMap,
                            FindHeaderTypesToReplace* findHeaderTypesToReplace):
                            typeMap(typeMap),
                            findHeaderTypesToReplace(findHeaderTypesToReplace) {
        CHECK_NULL(typeMap); CHECK_NULL(findHeaderTypesToReplace);
        setName("ReplaceHeaders");
    }

    const IR::Node* preorder(IR::P4Program* program) override;
    const IR::Node* postorder(IR::Member* expression) override;
    const IR::Node* postorder(IR::Type_Header* type) override;
};

class FlattenHeaders final : public PassManager {
 public:
    FlattenHeaders(ReferenceMap* refMap, TypeMap* typeMap,
            AnnotationSelectionPolicy *policy = nullptr) {
        auto findHeadersToReplace = new FindHeaderTypesToReplace(typeMap, policy);
        passes.push_back(new TypeChecking(refMap, typeMap));
        passes.push_back(findHeadersToReplace);
        passes.push_back(new ReplaceHeaders(typeMap, findHeadersToReplace));
        passes.push_back(new ClearTypeMap(typeMap));
        setName("FlattenHeaders");
    }
};

}  // namespace P4

#endif /* _MIDEND_FLATTENHEADERS_H_ */
