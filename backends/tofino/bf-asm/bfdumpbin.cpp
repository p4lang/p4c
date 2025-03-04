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

#include <inttypes.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <fstream>
#include <string>

#include "bson.h"
#include "fdstream.h"

struct {
    bool oneLine;
    bool noHeader;
    bool noCtxtJson;
} options;

int dump_bin(std::istream &in) {
    uint32_t atom_typ = 0;
    while (in.read((char *)&atom_typ, 4)) {
        if ((atom_typ >> 24) == 'H') {
            json::map hdr;
            if (!(in >> json::binary(hdr))) return -1;
            if (!options.noHeader)
                for (auto &el : hdr) std::cout << el.first << " = " << el.second << std::endl;
        } else if ((atom_typ >> 24) == 'C') {
            // future context json embedding in binary
            std::unique_ptr<json::obj> ctxt_json;
            if (!(in >> json::binary(ctxt_json))) return -1;
            if (!options.noCtxtJson) std::cout << ctxt_json;
        } else if ((atom_typ >> 24) == 'P') {
            uint32_t prsr_hdl = 0;
            if (!in.read((char *)&prsr_hdl, 4)) return -1;
            printf("P: %08x (parser handle)\n", prsr_hdl);
        } else if ((atom_typ >> 24) == 'R') {
            // R block -- writing a single 32-bit register via 32-bit PCIe address
            uint32_t reg_addr = 0, reg_data = 0;
            if (!in.read((char *)&reg_addr, 4)) return -1;
            if (!in.read((char *)&reg_data, 4)) return -1;
            printf("R%08x: %08x\n", reg_addr, reg_data);
        } else if ((atom_typ >> 24) == 'B') {
            // B block -- write a range of 32-bit registers via 64-bit PCIe address
            // size of the range is specified as count * width (in bits), which must
            // always be a multiple of 32

            uint64_t addr = 0;
            uint32_t count = 0;
            uint32_t width = 0;

            if (!in.read((char *)&addr, 8)) return -1;
            if (!in.read((char *)&width, 4)) return -1;
            if (!in.read((char *)&count, 4)) return -1;
            printf("B%08" PRIx64 ": %xx%x", addr, width, count);
            if ((uint64_t)count * width % 32 != 0) printf("  (not a multiple of 32 bits!)");
            count = (uint64_t)count * width / 32;
            uint32_t data, prev;
            int repeat = 0, col = 0;
            for (unsigned i = 0; i < count; ++i) {
                if (!in.read((char *)&data, 4)) return -1;
                if (i != 0 && data == prev) {
                    repeat++;
                    continue;
                }
                if (repeat > 0) {
                    printf(" x%-7d", repeat + 1);
                    if (++col > 8) col = 0;
                }
                repeat = 0;
                if (!options.oneLine && col++ % 8 == 0) printf("\n   ");
                printf(" %08x", prev = data);
            }
            if (repeat > 0) printf(" x%d", repeat + 1);
            printf("\n");
        } else if ((atom_typ >> 24) == 'D') {
            // D block -- write a range of 128-bit memory via 64-bit chip address
            // size of the range is specified as count * width (in bits), which must
            // always be a multiple of 64

            uint64_t addr = 0;
            uint32_t count = 0;
            uint32_t width = 0;

            if (!in.read((char *)&addr, 8)) return -1;
            if (!in.read((char *)&width, 4)) return -1;
            if (!in.read((char *)&count, 4)) return -1;
            printf("D%011" PRIx64 ": %xx%x", addr, width, count);
            if ((uint64_t)count * width % 64 != 0) printf("  (not a multiple of 64 bits!)");

            width /= 8;

            uint64_t chunk[2], prev_chunk[2];
            int repeat = 0, col = 0;
            for (unsigned i = 0; i < count * width; i += 16) {
                if (!in.read((char *)chunk, 16)) return -1;
                if (i != 0 && chunk[0] == prev_chunk[0] && chunk[1] == prev_chunk[1]) {
                    repeat++;
                    continue;
                }
                if (repeat > 0) {
                    printf(" x%d", repeat + 1);
                    col = 0;
                }
                repeat = 0;
                if (!options.oneLine && col++ % 2 == 0) printf("\n   ");
                printf(" %016" PRIx64 "%016" PRIx64, prev_chunk[1] = chunk[1],
                       prev_chunk[0] = chunk[0]);
            }

            if (repeat > 0) {
                printf(" x%d", repeat + 1);
                col = 0;
            }

            if (count * width % 16 == 8) {
                if (!in.read((char *)chunk, 8)) return -1;
                if (!options.oneLine && col % 2 == 0) printf("\n   ");
                printf(" %016" PRIx64, chunk[0]);
            }
            printf("\n");
        } else if ((atom_typ >> 24) == 'S') {
            // S block -- 'scanset' writing multiple data to a single 32-bit PCIE address
            uint64_t sel_addr = 0, reg_addr = 0;
            uint32_t sel_data = 0, width = 0, count = 0;

            if (!in.read((char *)&sel_addr, 8)) return -1;
            if (!in.read((char *)&sel_data, 4)) return -1;
            if (!in.read((char *)&reg_addr, 8)) return -1;
            if (!in.read((char *)&width, 4)) return -1;
            if (!in.read((char *)&count, 4)) return -1;
            printf("S%011" PRIx64 ": %x, %011" PRIx64 ": %xx%x", sel_addr, sel_data, reg_addr,
                   width, count);
            if (width % 32 != 0) printf("  (not a multiple of 32 bits!)");
            count = (uint64_t)count * width / 32;
            uint32_t data, prev;
            int repeat = 0, col = 0;
            for (unsigned i = 0; i < count; ++i) {
                if (!in.read((char *)&data, 4)) return -1;
                if (i != 0 && data == prev) {
                    repeat++;
                    continue;
                }
                if (repeat > 0) {
                    printf(" x%-7d", repeat + 1);
                    if (++col > 8) col = 0;
                }
                repeat = 0;
                if (!options.oneLine && col++ % 8 == 0) printf("\n   ");
                printf(" %08x", prev = data);
            }
            if (repeat > 0) printf(" x%d", repeat + 1);
            printf("\n");
        } else {
            fprintf(stderr, "\n");
            fprintf(stderr, "Parse error: atom_typ=%x (%c)\n", atom_typ, atom_typ >> 24);
            fprintf(stderr, "fpos=%" PRIu64 " <%" PRIx64 "h>\n", (uint64_t)in.tellg(),
                    (uint64_t)in.tellg());
            fprintf(stderr, "\n");

            return -1;
        }
    }

    return in.eof() ? 0 : -1;
}

int main(int ac, char **av) {
    int error = 0;
    for (int i = 1; i < ac; ++i) {
        if (*av[i] == '-') {
            for (char *arg = av[i] + 1; *arg;) switch (*arg++) {
                    case 'C':
                        options.noCtxtJson = true;
                        break;
                    case 'H':
                        options.noHeader = true;
                        break;
                    case 'L':
                        options.oneLine = true;
                        break;
                    default:
                        fprintf(stderr, "ignoring argument -%c\n", *arg);
                        error = 1;
                }
        } else {
            std::ifstream in(av[i], std::ios::binary);
            if (!in) {
                fprintf(stderr, "failed to open %s\n", av[i]);
                error = 1;
                continue;
            }
            unsigned char magic[4] = {};
            in.read((char *)magic, 4);
            if (magic[0] == 0 && magic[3] && strchr("RDBH", magic[3])) {
                in.seekg(0);
                error |= dump_bin(in);
            } else if (magic[0] == 0x1f && magic[1] == 0x8b) {
                if (auto *pipe = popen((std::string("zcat < ") + av[i]).c_str(), "r")) {
                    fdstream in(fileno(pipe));
                    error |= dump_bin(in);
                    pclose(pipe);
                } else {
                    fprintf(stderr, "%s: Cannot open pipe to read\n", av[i]);
                }
            } else {
                fprintf(stderr, "%s: Unknown file format\n", av[i]);
            }
        }
    }
    if (error == 1) fprintf(stderr, "usage: %s <file>\n", av[0]);
    return error;
}
