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

#include "bf-p4c/midend/type_categories.h"

#include "gtest/gtest.h"
#include "ir/ir.h"

namespace BFN {

TEST(BFNTypeCategories, Types) {
    auto *imAnnotation =
        new IR::Annotations({new IR::Annotation(IR::ID("__intrinsic_metadata"), {})});
    auto *cgAnnotation =
        new IR::Annotations({new IR::Annotation(IR::ID("__compiler_generated"), {})});

    auto *headerType =
        new IR::Type_Header("hdr", IR::Annotations::empty, new IR::TypeParameters(), {});
    auto *headerImType = new IR::Type_Header("hdr", imAnnotation, new IR::TypeParameters(), {});
    auto *headerCgType = new IR::Type_Header("hdr", cgAnnotation, new IR::TypeParameters(), {});
    auto *structType =
        new IR::Type_Struct("meta", IR::Annotations::empty, new IR::TypeParameters(), {});
    auto *structImType = new IR::Type_Struct("meta", imAnnotation, new IR::TypeParameters(), {});
    auto *structCgType = new IR::Type_Struct("meta", cgAnnotation, new IR::TypeParameters(), {});
    auto *bitType = IR::Type::Bits::get(1);
    auto *boolType = IR::Type_Boolean::get();
    auto *infIntType = IR::Type_InfInt::get();
    auto *stringType = IR::Type_String::get();

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
