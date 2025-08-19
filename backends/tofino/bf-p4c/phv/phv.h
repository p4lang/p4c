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

#ifndef BACKENDS_TOFINO_BF_P4C_PHV_PHV_H_
#define BACKENDS_TOFINO_BF_P4C_PHV_PHV_H_

#include "backends/tofino/bf-p4c/specs/phv.h"
#include "ir/json_generator.h"

namespace P4 {
class JSONGenerator;
class JSONLoader;
}  // namespace P4

namespace PHV {

class SerializableContainer : public Container {
    SerializableContainer(Kind k, Size s, unsigned i) : Container(k, s, i) {}

 public:
    SerializableContainer() = default;
    explicit SerializableContainer(const Container &container) : Container(container) {}

    /// Construct a container from @name - e.g., "B0" for container B0.
    /// NOLINTNEXTLINE(runtime/explicit)
    SerializableContainer(const char *name, bool abort_if_invalid = true)
        : Container(name, abort_if_invalid) {}

    /// Construct a container from @p kind and @p index - e.g., (Kind::B, 0) for
    /// container B0.
    SerializableContainer(PHV::Type t, unsigned index) : Container(t, index) {}

    /// JSON serialization/deserialization.
    void toJSON(P4::JSONGenerator &json) const;
    static SerializableContainer fromJSON(P4::JSONLoader &json);
};

}  // namespace PHV

#endif /* BACKENDS_TOFINO_BF_P4C_PHV_PHV_H_ */
