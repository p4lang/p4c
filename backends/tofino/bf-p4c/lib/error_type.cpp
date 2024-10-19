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
