/**
 * Copyright (C) 2024 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file
 * except in compliance with the License.  You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software distributed under the
 * License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
 * either express or implied.  See the License for the specific language governing permissions
 * and limitations under the License.
 *
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <fstream>
#include <iostream>

#include "bson.h"

int main(int ac, char **av) {
    if (ac != 3) {
        std::cerr << "usage " << av[0] << " <json in> <bson out>" << std::endl;
        return 1;
    }
    std::ifstream in(av[1]);
    if (!in) {
        std::cerr << "failed to open " << av[1] << std::endl;
        return 1;
    }
    json::obj *data = nullptr;
    if (!(in >> data)) {
        std::cerr << "failed to read json" << std::endl;
        return 1;
    }
    std::ofstream out(av[2]);
    if (!out) {
        std::cerr << "failed to open " << av[2] << std::endl;
        return 1;
    }
    if (!(out << json::binary(data))) {
        std::cerr << "failed to write bson" << std::endl;
        return 1;
    }
    return 0;
}
