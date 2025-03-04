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
#include "disasm.h"
#include "fdstream.h"

Disasm *disasm = nullptr;

int read_bin(std::istream &in) {
    uint32_t atom_typ = 0;
    while (in.read((char *)&atom_typ, 4)) {
        if ((atom_typ >> 24) == 'H') {
            json::map hdr;
            if (!(in >> json::binary(hdr))) return -1;
            if (auto target = hdr["target"]) {
                disasm = Disasm::create(target.to<json::string>());
            } else {
                std::cerr << "no target specified in the binary" << std::endl;
                delete disasm;
                disasm = nullptr;
            }
        } else if ((atom_typ >> 24) == 'C') {
            // future context json embedding in binary
            std::unique_ptr<json::obj> ctxt_json;
            if (!(in >> json::binary(ctxt_json))) return -1;
        } else if ((atom_typ >> 24) == 'P') {
            uint32_t prsr_hdl = 0;
            if (!in.read((char *)&prsr_hdl, 4)) return -1;
        } else if ((atom_typ >> 24) == 'R') {
            // R block -- writing a single 32-bit register via 32-bit PCIe address
            uint32_t reg_addr = 0, reg_data = 0;
            if (!in.read((char *)&reg_addr, 4)) return -1;
            if (!in.read((char *)&reg_data, 4)) return -1;
            if (disasm) disasm->input_binary(reg_addr, 'R', &reg_data, 1);
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
            // printf("B%08" PRIx64 ": %xx%x", addr, width, count);
            count = (uint64_t)count * width / 32;
            std::vector<uint32_t> data(count);
            if (!in.read((char *)&data[0], count * 4)) return -1;
            if (disasm) disasm->input_binary(addr, 'B', &data[0], count);
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
            // printf("D%011" PRIx64 ": %xx%x", addr, width, count);
            width /= 8;
            std::vector<uint32_t> data(count * width / 4);
            if (!in.read((char *)&data[0], count * width)) return -1;
            if (disasm) disasm->input_binary(addr, 'D', &data[0], count * width / 4);
        } else if ((atom_typ >> 24) == 'S') {
            // S block -- 'scanset' writing multiple data to a single 32-bit PCIE address
            uint64_t sel_addr = 0, reg_addr = 0;
            uint32_t sel_data = 0, width = 0, count = 0;

            if (!in.read((char *)&sel_addr, 8)) return -1;
            if (!in.read((char *)&sel_data, 4)) return -1;
            if (!in.read((char *)&reg_addr, 8)) return -1;
            if (!in.read((char *)&width, 4)) return -1;
            if (!in.read((char *)&count, 4)) return -1;
            count = (uint64_t)count * width / 32;
            std::vector<uint32_t> data(count);
            if (!in.read((char *)&data[0], count * 4)) return -1;
            if (disasm) disasm->input_binary(reg_addr, 'S', &data[0], count);
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
                    case 'l':
                        ++i;
                        if (!av[i]) {
                            std::cerr << "No log file specified '-l <log file>'" << std::endl;
                            error_count++;
                            break;
                        }
                        if (auto *tmp = new std::ofstream(av[i])) {
                            if (*tmp) {
                                /* FIXME -- tmp leaks, but if we delete it, the log
                                 * redirect fails, and we crash on exit */
                                std::clog.rdbuf(tmp->rdbuf());
                            } else {
                                std::cerr << "Can't open " << av[i] << " for writing" << std::endl;
                                delete tmp;
                            }
                        }
                        break;
                    case 'v':
                        Log::increaseVerbosity();
                        break;
                    case 'T':
                        if (*arg) {
                            Log::addDebugSpec(arg);
                            arg += strlen(arg);
                        } else if (++i < ac) {
                            Log::addDebugSpec(av[i]);
                        }
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
                error |= read_bin(in);
            } else if (magic[0] == 0x1f && magic[1] == 0x8b) {
                if (auto *pipe = popen((std::string("zcat < ") + av[i]).c_str(), "r")) {
                    fdstream in(fileno(pipe));
                    error |= read_bin(in);
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
