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

#include "gen/tofino/disas.regs.mau_addrmap.h"
#include "hex.h"
#include "json.h"

static Tofino::regs_mau_addrmap regs;

static void dump_hashtables(std::ostream &out);

int verbose = 0;
int get_file_log_level(const char *file, int *level) { return *level = verbose; }

int main(int ac, char **av) {
    for (int i = 1; i < ac; i++) {
        if (av[i][0] == '-' || av[i][0] == '+') {
            bool flag = av[i][0] == '+';
            for (char *arg = av[i] + 1; *arg;) switch (*arg++) {
                    case 'v':
                        verbose++;
                        break;
                    default:
                        std::cerr << "Unknown option " << (flag ? '+' : '-') << arg[-1]
                                  << std::endl;
                        std::cerr << "usage: " << av[0] << " file" << std::endl;
                }
        } else {
            std::ifstream in(av[i]);
            if (!in) {
                std::cerr << "Can't open " << av[i] << std::endl;
                continue;
            }
            std::unique_ptr<json::obj> data;
            in >> data;
            if (!in || regs.unpack_json(data.get())) {
                std::cerr << "Can't read/unpack json from " << av[i] << std::endl;
                continue;
            }
            dump_hashtables(std::cout);
        }
    }
}

static bool col_nonzero(int i, int col) {
    for (int word = i * 8; word < i * 8 + 8; word++) {
        auto &x = regs.dp.xbar_hash.hash.galois_field_matrix[word][col];
        if (x.byte0 || x.byte1) return true;
    }
    return false;
}

static bool col_valid_nonzero(int i, int col) {
    for (int word = i * 8; word < i * 8 + 8; word++) {
        auto &x = regs.dp.xbar_hash.hash.galois_field_matrix[word][col];
        if (x.valid0 || x.valid1) return true;
    }
    return false;
}

static bool ht_nonzero(int i) {
    for (int col = 0; col < 52; col++) {
        if ((regs.dp.xbar_hash.hash.hash_seed[col] >> i) & 1) return true;
        if (col_nonzero(i, col)) return true;
        if (col_valid_nonzero(i, col)) return true;
    }
    return false;
}

static void dump_ht(std::ostream &out, int i) {
    for (int col = 0; col < 52; col++) {
        if (col_nonzero(i, col)) {
            out << "    " << col << ": 0x";
            bool pfx = true;
            for (int word = 8 * i + 7; word >= 8 * i; word--) {
                auto &w = regs.dp.xbar_hash.hash.galois_field_matrix[word][col];
                if (!pfx || w.byte1) {
                    out << hex(w.byte1, pfx ? 0 : 2, '0');
                    pfx = false;
                }
                if (!pfx || w.byte0) {
                    out << hex(w.byte0, pfx ? 0 : 2, '0');
                    pfx = false;
                }
            }
            out << '\n';
        }
        if (col_valid_nonzero(i, col)) {
            out << "    valid " << col << ": 0b";
            bool pfx = true;
            for (int word = 8 * i + 7; word >= 8 * i; word--) {
                auto &w = regs.dp.xbar_hash.hash.galois_field_matrix[word][col];
                if (!pfx || w.valid1) {
                    out << (w.valid1 ? '1' : '0');
                    pfx = false;
                }
                if (!pfx || w.valid0) {
                    out << (w.valid0 ? '1' : '0');
                    pfx = false;
                }
            }
            out << '\n';
        }
        if ((regs.dp.xbar_hash.hash.hash_seed[col] >> i) & 1) out << "    seed " << col << ": 1\n";
    }
}

static void dump_hashtables(std::ostream &out) {
    for (int i = 0; i < 8; i++) {
        if (ht_nonzero(i)) {
            out << "hash " << i << ":\n";
            dump_ht(out, i);
        }
    }
}
