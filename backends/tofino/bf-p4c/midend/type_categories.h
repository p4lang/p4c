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

#ifndef BF_P4C_MIDEND_TYPE_CATEGORIES_H_
#define BF_P4C_MIDEND_TYPE_CATEGORIES_H_

#include "ir/ir.h"

namespace P4 {
class ReferenceMap;
class TypeMap;
}  // namespace P4

namespace BFN {

struct LinearPath;

/**
 * @return true if the provided type has the `@__intrinsic_metadata` annotation;
 * the architecture tags types with this annotation if they're intrinsic
 * metadata.
 */
bool isIntrinsicMetadataType(const IR::Type *type);

/**
 * @return true if the provided type has the `@__compiler_generated` annotation;
 * the compiler tags types it generates with this annotation.
 */
bool isCompilerGeneratedType(const IR::Type *type);

/**
 * @return true if the provided type is logically a metadata type - that is, if
 * it's:
 *   - a struct type, or
 *   - a non-struct type with the annotation `@__intrinsic_metadata`.
 * The second case is required for TNA, which represents some types which are
 * logically metadata as headers.
 */
bool isMetadataType(const IR::Type *type);

/**
 * @return true if the provided type is logically a header type - that is, if
 * it's a header type for which `isMetadataType()` returns false.
 */
bool isHeaderType(const IR::Type *type);

/**
 * @return true if the provided type is a primitive type, which for the purposes
 * of this function is defined as:
 *   - a bit<> type, or
 *   - a Type_InfInt (i.e., a numeric constant of indeterminate width), or
 *   - a boolean type.
 */
bool isPrimitiveType(const IR::Type *type);

/**
 * @return true if the provided LinearPath refers to metadata - that is, if it
 * refers to one of:
 *  - an object of metadata type (i.e., `isMetadataType()` returns true; see
 *    that function for the full criteria), or
 *  - a field of primitive type on such an object.
 */
bool isMetadataReference(const LinearPath &path, P4::TypeMap *typeMap);

/**
 * @return true if the provided LinearPath refers to a header - that is, if it
 * refers to one of:
 *  - an object of header type (i.e., `isHeaderType()` returns true; see that
 *    function for the full criteria), or
 *  - a field of primitive type on such an object.
 */
bool isHeaderReference(const LinearPath &path, P4::TypeMap *typeMap);

/**
 * @return true if the provided LinearPath refers to an object of primitive
 * type - that is, if it refers to one of:
 *  - a simple object of primitive type, or
 *  - a field of primitive type on some structlike object.
 */
bool isPrimitiveReference(const LinearPath &path, P4::TypeMap *typeMap);

/**
 * @return true if the provided LinearPath refers to a field of primitive type
 * on some structlike object.
 */
bool isPrimitiveFieldReference(const LinearPath &path, P4::TypeMap *typeMap);

/**
 * @return true if the provided LinearPath refers to an object which is a
 * subobject of a parameter - that is, if it refers to one of:
 *  - a parameter directly, or
 *  - an object which is transitively a subobject of a parameter.
 */
bool isSubobjectOfParameter(const LinearPath &path, P4::ReferenceMap *refMap);

/**
 * @return the parameter which transitively contains this object if
 * `isSubobjectOfParameter()` would return true, or nullptr otherwise.
 */
const IR::Parameter *getContainingParameter(const LinearPath &path, P4::ReferenceMap *refMap);

}  // namespace BFN

#endif /* BF_P4C_MIDEND_TYPE_CATEGORIES_H_ */
