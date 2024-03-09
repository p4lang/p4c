/*
Copyright 2024 Intel Corp.

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

#include "ir/json_generator.h"
#include "ir/json_loader.h"

namespace BitRange {

void rangeToJSON(JSONGenerator &json, int lo, int hi) { json.toJSON(std::make_pair(lo, hi)); }

std::pair<int, int> rangeFromJSON(JSONLoader &json) {
    std::pair<int, int> endpoints;
    json >> endpoints;
    return endpoints;
}

}  // namespace BitRange
