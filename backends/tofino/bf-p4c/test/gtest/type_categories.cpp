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

#include "gtest/gtest.h"

#include "bf-p4c/midend/type_categories.h"
#include "ir/ir.h"

namespace BFN {

TEST(BFNTypeCategories, Types) {
    auto* imAnnotation = new IR::Annotations({
            new IR::Annotation(IR::ID("__intrinsic_metadata"), { })
        });
    auto* cgAnnotation = new IR::Annotations({
            new IR::Annotation(IR::ID("__compiler_generated"), { })
        });

    auto* headerType = new IR::Type_Header("hdr", IR::Annotations::empty,
                                           new IR::TypeParameters(), { });
    auto* headerImType = new IR::Type_Header("hdr", imAnnotation, new IR::TypeParameters(), { });
    auto* headerCgType = new IR::Type_Header("hdr", cgAnnotation, new IR::TypeParameters(), { });
    auto* structType = new IR::Type_Struct("meta", IR::Annotations::empty,
                                           new IR::TypeParameters(), { });
    auto* structImType = new IR::Type_Struct("meta", imAnnotation,
                                           new IR::TypeParameters(), { });
    auto* structCgType = new IR::Type_Struct("meta", cgAnnotation,
                                           new IR::TypeParameters(), { });
    auto* bitType = IR::Type::Bits::get(1);
    auto* boolType = IR::Type_Boolean::get();
    auto* infIntType = IR::Type_InfInt::get();
    auto* stringType = IR::Type_String::get();

    EXPECT_FALSE(isIntrinsicMetadataType(headerType));
    EXPECT_TRUE(isIntrinsicMetadataType(headerImType));
    EXPECT_FALSE(isIntrinsicMetadataType(headerCgType));
    EXPECT_FALSE(isIntrinsicMetadataType(structType));
    EXPECT_TRUE(isIntrinsicMetadataType(structImType));
    EXPECT_FALSE(isIntrinsicMetadataType(structCgType));
    EXPECT_FALSE(isIntrinsicMetadataType(bitType));
    EXPECT_FALSE(isIntrinsicMetadataType(boolType));
    EXPECT_FALSE(isIntrinsicMetadataType(infIntType));
    EXPECT_FALSE(isIntrinsicMetadataType(stringType));

    EXPECT_FALSE(isCompilerGeneratedType(headerType));
    EXPECT_FALSE(isCompilerGeneratedType(headerImType));
    EXPECT_TRUE(isCompilerGeneratedType(headerCgType));
    EXPECT_FALSE(isCompilerGeneratedType(structType));
    EXPECT_FALSE(isCompilerGeneratedType(structImType));
    EXPECT_TRUE(isCompilerGeneratedType(structCgType));
    EXPECT_FALSE(isCompilerGeneratedType(bitType));
    EXPECT_FALSE(isCompilerGeneratedType(boolType));
    EXPECT_FALSE(isCompilerGeneratedType(infIntType));
    EXPECT_FALSE(isCompilerGeneratedType(stringType));

    EXPECT_FALSE(isMetadataType(headerType));
    EXPECT_TRUE(isMetadataType(headerImType));
    EXPECT_FALSE(isMetadataType(headerCgType));
    EXPECT_TRUE(isMetadataType(structType));
    EXPECT_TRUE(isMetadataType(structImType));
    EXPECT_TRUE(isMetadataType(structCgType));
    EXPECT_FALSE(isMetadataType(bitType));
    EXPECT_FALSE(isMetadataType(boolType));
    EXPECT_FALSE(isMetadataType(infIntType));
    EXPECT_FALSE(isMetadataType(stringType));

    EXPECT_TRUE(isHeaderType(headerType));
    EXPECT_FALSE(isHeaderType(headerImType));
    EXPECT_TRUE(isHeaderType(headerCgType));
    EXPECT_FALSE(isHeaderType(structType));
    EXPECT_FALSE(isHeaderType(structImType));
    EXPECT_FALSE(isHeaderType(structCgType));
    EXPECT_FALSE(isHeaderType(bitType));
    EXPECT_FALSE(isHeaderType(boolType));
    EXPECT_FALSE(isHeaderType(infIntType));
    EXPECT_FALSE(isHeaderType(stringType));

    EXPECT_FALSE(isPrimitiveType(headerType));
    EXPECT_FALSE(isPrimitiveType(headerImType));
    EXPECT_FALSE(isPrimitiveType(headerCgType));
    EXPECT_FALSE(isPrimitiveType(structType));
    EXPECT_FALSE(isPrimitiveType(structImType));
    EXPECT_FALSE(isPrimitiveType(structCgType));
    EXPECT_TRUE(isPrimitiveType(bitType));
    EXPECT_TRUE(isPrimitiveType(boolType));
    EXPECT_TRUE(isPrimitiveType(infIntType));
    EXPECT_FALSE(isPrimitiveType(stringType));
}

}  // namespace BFN
