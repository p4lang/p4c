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

#ifndef MIDEND_FLATTENHEADERS_H_
#define MIDEND_FLATTENHEADERS_H_

#include "flattenInterfaceStructs.h"
#include "frontends/p4/typeChecking/typeChecker.h"
#include "ir/ir.h"

namespace P4 {

/**
Find the types to replace and insert them in the nested struct map.
 */
class FindHeaderTypesToReplace : public Inspector {
    P4::TypeMap *typeMap;
    AnnotationSelectionPolicy *policy;
    ordered_map<cstring, StructTypeReplacement<IR::Type_StructLike> *> replacement;

 public:
    explicit FindHeaderTypesToReplace(P4::TypeMap *typeMap, AnnotationSelectionPolicy *policy)
        : typeMap(typeMap), policy(policy) {
        setName("FindHeaderTypesToReplace");
        CHECK_NULL(typeMap);
    }
    bool preorder(const IR::Type_Header *type) override;
    void createReplacement(const IR::Type_Header *type, AnnotationSelectionPolicy *policy);
    StructTypeReplacement<IR::Type_StructLike> *getReplacement(const cstring name) const {
        return ::get(replacement, name);
    }
    bool empty() const { return replacement.empty(); }
};

/**
 * This pass flattens any nested struct inside a P4 header.
   This pass will fail if a header with nested fields is used as a left-value.

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
class ReplaceHeaders : public Transform, P4WriteContext {
    P4::TypeMap *typeMap;
    FindHeaderTypesToReplace *findHeaderTypesToReplace;

 public:
    explicit ReplaceHeaders(P4::TypeMap *typeMap,
                            FindHeaderTypesToReplace *findHeaderTypesToReplace)
        : typeMap(typeMap), findHeaderTypesToReplace(findHeaderTypesToReplace) {
        CHECK_NULL(typeMap);
        CHECK_NULL(findHeaderTypesToReplace);
        setName("ReplaceHeaders");
    }

    const IR::Node *preorder(IR::P4Program *program) override;
    const IR::Node *postorder(IR::Member *expression) override;
    const IR::Node *postorder(IR::Type_Header *type) override;
};

class FlattenHeaders final : public PassManager {
 public:
    FlattenHeaders(ReferenceMap *refMap, TypeMap *typeMap,
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

#endif /* MIDEND_FLATTENHEADERS_H_ */
