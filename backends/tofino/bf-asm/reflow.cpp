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
#include <string>
#include <vector>

void output_normal(std::ostream &out, std::vector<std::string> &lines) {
    for (auto &l : lines) out << l << '\n';
    lines.clear();
}

void strip_trail_ws(std::string &s) {
    auto end = s.find_last_not_of(" \t\r\n");
    if (end != std::string::npos) s.resize(end + 1);
}
void strip_lead_ws(std::string &s) {
    auto start = s.find_first_not_of(" \t\r\n");
    if (start != std::string::npos) s.erase(0, start);
}

void output_1line(std::ostream &out, std::vector<std::string> &lines) {
    bool first = true;
    for (auto &l : lines) {
        if (first) {
            strip_trail_ws(l);
            first = false;
        } else {
            strip_trail_ws(l);
            strip_lead_ws(l);
            out << ' ';
        }
        out << l;
    }
    out << '\n';
    lines.clear();
}

size_t output_len(std::vector<std::string> &lines) {
    size_t rv = 0;
    for (auto &l : lines) {
        size_t len = l.find_last_not_of(" \t\r\n"), plen;
        if (len == std::string::npos) len = l.size();
        if (rv == 0 && (plen = l.find_first_not_of(" \t\r\n")) != std::string::npos)
            len -= plen - 1;
        rv += len;
    }
    return rv;
}

void reflow(std::istream &in, std::ostream &out) {
    std::string line;
    char looking = 0;
    std::vector<std::string> save;
    const auto npos = std::string::npos;
    while (getline(in, line)) {
        if (line.find('{') != npos && line.find('}') == npos) {
            output_normal(out, save);
            looking = '}';
            save.push_back(line);
        } else if (line.find('[') != npos && line.find(']') == npos) {
            output_normal(out, save);
            looking = ']';
            save.push_back(line);
        } else if (looking) {
            save.push_back(line);
            if (line.find(looking) != std::string::npos) {
                output_1line(out, save);
                looking = 0;
            } else if (output_len(save) > 100) {
                output_normal(out, save);
                looking = 0;
            }
        } else {
            out << line << '\n';
        }
    }
    output_normal(out, save);
    out << std::flush;
}

int main(int ac, char **av) {
    if (ac == 2) {
        std::ifstream in(av[1]);
        if (in) {
            reflow(in, std::cout);
        } else {
            std::cerr << "Can't open " << av[1] << std::endl;
            return 1;
        }
    } else if (ac == 1) {
        reflow(std::cin, std::cout);
    } else {
        std::cerr << "usage: " << av[0] << " [file]" << std::endl;
        return 1;
    }
    return 0;
}
