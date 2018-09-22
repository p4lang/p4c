/*
Copyright 2018-present Barefoot Networks, Inc.

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

#ifndef CONTROL_PLANE_P4RUNTIMEARCHSTANDARD_H_
#define CONTROL_PLANE_P4RUNTIMEARCHSTANDARD_H_

#include "frontends/common/resolveReferences/referenceMap.h"
#include "frontends/p4/typeMap.h"
#include "ir/ir.h"

#include "p4RuntimeArchHandler.h"

namespace P4 {

/** \addtogroup control_plane
 *  @{
 */
namespace ControlPlaneAPI {

/// Declarations specific to standard architectures (v1model & PSA).
namespace Standard {

/// The architecture handler builder implementation for v1model.
struct V1ModelArchHandlerBuilder : public P4RuntimeArchHandlerBuilderIface {
    P4RuntimeArchHandlerIface* operator()(
        ReferenceMap* refMap,
        TypeMap* typeMap,
        const IR::ToplevelBlock* evaluatedProgram) const override;
};

/// The architecture handler builder implementation for PSA.
struct PSAArchHandlerBuilder : public P4RuntimeArchHandlerBuilderIface {
    P4RuntimeArchHandlerIface* operator()(
        ReferenceMap* refMap,
        TypeMap* typeMap,
        const IR::ToplevelBlock* evaluatedProgram) const override;
};

}  // namespace Standard

}  // namespace ControlPlaneAPI

/** @} */  /* end group control_plane */
}  // namespace P4

#endif  /* CONTROL_PLANE_P4RUNTIMEARCHSTANDARD_H_ */
