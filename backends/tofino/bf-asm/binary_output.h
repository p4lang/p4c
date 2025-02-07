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

#ifndef _binary_output_h_
#define _binary_output_h_

#include <iomanip>
#include <iostream>

namespace binout {

class tag {
    char data[4] = {0, 0, 0, 0};

 public:
    tag(char ch) { data[3] = ch; }
    friend std::ostream &operator<<(std::ostream &out, const tag &e) {
        return out.write(e.data, 4);
    }
};

class byte4 {
    char data[4];

 public:
    byte4(uint32_t v) {
        data[0] = v & 0xff;
        data[1] = (v >> 8) & 0xff;
        data[2] = (v >> 16) & 0xff;
        data[3] = (v >> 24) & 0xff;
    }
    friend std::ostream &operator<<(std::ostream &out, const byte4 &e) {
        return out.write(e.data, 4);
    }
};

class byte8 {
    char data[8];

 public:
    byte8(uint64_t v) {
        data[0] = v & 0xff;
        data[1] = (v >> 8) & 0xff;
        data[2] = (v >> 16) & 0xff;
        data[3] = (v >> 24) & 0xff;
        data[4] = (v >> 32) & 0xff;
        data[5] = (v >> 40) & 0xff;
        data[6] = (v >> 48) & 0xff;
        data[7] = (v >> 56) & 0xff;
    }
    friend std::ostream &operator<<(std::ostream &out, const byte8 &e) {
        return out.write(e.data, 8);
    }
};

}  // end namespace binout

#endif /* _binary_output_h_ */
