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

#include "error_type.h"

#include <numeric>
#include <vector>

#include "lib/exceptions.h"

void BFN::ErrorType::printWarningsHelp(std::ostream &out) {
    std::vector<int> warningTypes;

    auto &catalog = ErrorCatalog::getCatalog();

    // p4c warnings types
    for (int i = LEGACY_WARNING; catalog.getName(i) != "--unknown--"; ++i)
        warningTypes.push_back(i);

    // bf-p4c warning types
    for (int i = FIRST_BACKEND_WARNING; catalog.getName(i) != "--unknown--"; ++i)
        warningTypes.push_back(i);

    out << "These are supported warning types for --Werror and --Wdisable: ";

    auto helptext = std::accumulate(warningTypes.begin() + 1, warningTypes.end(),
                                    std::string(catalog.getName(warningTypes.front()).c_str()),
                                    [&catalog](const std::string &r, int type) -> std::string {
                                        return r + ", " + catalog.getName(type);
                                    });

    out << helptext << std::endl << std::endl;

    out << "You can provide one or more of these types as a "
           "comma separated list to --Werror/--Wdisable options."
        << std::endl
        << std::endl;
}
