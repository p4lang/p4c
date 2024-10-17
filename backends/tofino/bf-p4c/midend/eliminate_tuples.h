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

#ifndef BF_P4C_MIDEND_ELIMINATE_TUPLES_H_
#define BF_P4C_MIDEND_ELIMINATE_TUPLES_H_

#include "ir/ir.h"
#include "frontends/p4/typeChecking/typeChecker.h"
#include "frontends/common/resolveReferences/resolveReferences.h"
#include "midend/eliminateTuples.h"

namespace BFN {

// Finds all of the HashListExpressions and saves their data
// so it is not lost during TupleReplacement->TypeInference
// (which converts ListExpression to StructExpression,
// and since HashListExpression is also ListExpression
// it gets transformed and the extra data are lost)
class SaveHashListExpression: public Inspector {
 public:
    std::map<const IR::Expression*, const IR::HashListExpression*> update_hashes;

    bool preorder(const IR::HashListExpression *hle) override;
};

// Inserts back the information from HashListExpressions to new HashStructExpressions
class InsertHashStructExpression: public Transform {
    std::map<const IR::Expression*, const IR::HashListExpression*>* update_hashes;

 public:
    InsertHashStructExpression(
        std::map<const IR::Expression*, const IR::HashListExpression*>* update_hashes) :
            update_hashes(update_hashes) { };

    const IR::Node* preorder(IR::StructExpression *se) override;
};

/**
 * extended eliminate tuples pass used in barefoot's midend.
 */
class EliminateTuples final : public PassManager {
 public:
    EliminateTuples(P4::ReferenceMap* refMap, P4::TypeMap* typeMap,
            P4::TypeChecking* typeChecking = nullptr,
            P4::TypeInference* typeInference = nullptr) {
        if (!typeChecking)
            typeChecking = new P4::TypeChecking(refMap, typeMap);
        passes.push_back(typeChecking);
        passes.push_back(new P4::DoReplaceTuples(typeMap));
        // Save any HashListExpressions
        auto shle = new SaveHashListExpression();
        passes.push_back(shle);
        passes.push_back(new P4::ClearTypeMap(typeMap));
        // We do a round of type-checking which may mutate the program.
        // This will convert some ListExpressions
        // into StructExpression where tuples were converted
        // to structs.
        passes.push_back(new P4::ResolveReferences(refMap));
        if (!typeInference)
            typeInference = new P4::TypeInference(typeMap, false);
        passes.push_back(typeInference);
        // Reinsert HashListExpression information
        passes.push_back(new InsertHashStructExpression(&shle->update_hashes));
        setName("EliminateTuples");
    }
};

}  // namespace BFN

#endif  /* BF_P4C_MIDEND_ELIMINATE_TUPLES_H_ */
