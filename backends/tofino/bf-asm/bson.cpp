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

#include "bson.h"

#include <inttypes.h>

#include "lib/hex.h"

namespace {
uint8_t get8(std::istream &in) {
    char data;
    in.read(&data, sizeof(data));
    return data & 0xffU;
}

int32_t get32(std::istream &in) {
    char data[4];
    in.read(data, sizeof(data));
    return (data[0] & 0xffU) | ((data[1] & 0xffU) << 8) | ((data[2] & 0xffU) << 16) |
           ((data[3] & 0xffU) << 24);
}
int64_t get64(std::istream &in) {
    char data[8];
    in.read(data, sizeof(data));
    return (data[0] & 0xffULL) | ((data[1] & 0xffULL) << 8) | ((data[2] & 0xffULL) << 16) |
           ((data[3] & 0xffULL) << 24) | ((data[4] & 0xffULL) << 32) | ((data[5] & 0xffULL) << 40) |
           ((data[6] & 0xffULL) << 48) | ((data[7] & 0xffULL) << 56);
}

std::string out32(int32_t val) {
    char data[4];
    data[0] = val & 0xff;
    data[1] = (val >> 8) & 0xff;
    data[2] = (val >> 16) & 0xff;
    data[3] = (val >> 24) & 0xff;
    return std::string(data, sizeof(data));
}

std::string out64(int64_t val) {
    char data[8];
    data[0] = val & 0xff;
    data[1] = (val >> 8) & 0xff;
    data[2] = (val >> 16) & 0xff;
    data[3] = (val >> 24) & 0xff;
    data[4] = (val >> 32) & 0xff;
    data[5] = (val >> 40) & 0xff;
    data[6] = (val >> 48) & 0xff;
    data[7] = (val >> 56) & 0xff;
    return std::string(data, sizeof(data));
}

}  // end anonymous namespace

namespace json {

std::istream &operator>>(std::istream &in, bson_wrap<json::vector> o) {
    json::vector &out = o.o;
    std::streamoff start = in.tellg();
    std::streamoff end = start + get32(in);
    out.clear();
    while (uint8_t type = get8(in)) {
        if (!in) break;
        if (in.tellg() >= end) {
            std::cerr << "truncated array" << std::endl;
            in.setstate(std::ios::failbit);
            break;
        }
        std::string key;
        getline(in, key, '\0');
        if (key != std::to_string(out.size())) std::cerr << "incorrect key in array" << std::endl;
        switch (type) {
            case 0x02: {
                uint32_t len = get32(in) - 1;
                std::string val;
                val.resize(len);
                in.read(&val[0], len);
                out.push_back(val.c_str());
                if (in.get() != 0) {
                    std::cerr << "missing NUL in bson string" << std::endl;
                    in.setstate(std::ios::failbit);
                }
                break;
            }
            case 0x03: {
                json::map obj;
                in >> binary(obj);
                out.push_back(std::move(obj));
                break;
            }
            case 0x04: {
                json::vector obj;
                in >> binary(obj);
                out.push_back(std::move(obj));
                break;
            }
            case 0x08:
                switch (get8(in)) {
                    case 0:
                        out.push_back(false);
                        break;
                    case 1:
                        out.push_back(true);
                        break;
                    default:
                        std::cerr << "invalid boolean value" << std::endl;
                        in.setstate(std::ios::failbit);
                        break;
                }
                break;
            case 0x0a:
                out.push_back(nullptr);
                break;
            case 0x10:
                out.push_back(get32(in));
                break;
            case 0x12:
                out.push_back(get64(in));
                break;
            case 0x7f:
            case 0xff:
                break;
            default:
                std::cerr << "unhandled bson tag " << hex(type) << std::endl;
                break;
        }
    }
    if (start != -1 && in && in.tellg() != end) {
        std::cerr << "incorrect length for object" << std::endl;
    }
    return in;
}

std::istream &operator>>(std::istream &in, bson_wrap<json::map> o) {
    json::map &out = o.o;
    std::streamoff start = in.tellg();
    std::streamoff end = start + get32(in);
    out.clear();
    while (uint8_t type = get8(in)) {
        if (!in) break;
        if (in.tellg() >= end) {
            std::cerr << "truncated object" << std::endl;
            in.setstate(std::ios::failbit);
            break;
        }
        std::string key;
        getline(in, key, '\0');
        if (out.count(key.c_str())) std::cerr << "duplicate key in map" << std::endl;
        switch (type) {
            case 0x02: {
                uint32_t len = get32(in) - 1;
                std::string val;
                val.resize(len);
                in.read(&val[0], len);
                out[key] = val;
                if (in.get() != 0) {
                    std::cerr << "missing NUL in bson string" << std::endl;
                    in.setstate(std::ios::failbit);
                }
                break;
            }
            case 0x03: {
                json::map obj;
                in >> binary(obj);
                out[key] = mkuniq<json::map>(std::move(obj));
                break;
            }
            case 0x04: {
                json::vector obj;
                in >> binary(obj);
                out[key] = mkuniq<json::vector>(std::move(obj));
                break;
            }
            case 0x08:
                switch (get8(in)) {
                    case 0:
                        out[key] = mkuniq<False>(False());
                        break;
                    case 1:
                        out[key] = mkuniq<True>(True());
                        break;
                    default:
                        std::cerr << "invalid boolean value" << std::endl;
                        in.setstate(std::ios::failbit);
                        break;
                }
                break;
            case 0x0a:
                out[key] = std::unique_ptr<json::obj>();
                break;
            case 0x10:
                out[key] = get32(in);
                break;
            case 0x12:
                out[key] = get64(in);
                break;
            case 0x7f:
            case 0xff:
                break;
            default:
                std::cerr << "unhandled bson tag " << hex(type) << std::endl;
                break;
        }
    }
    if (start != -1 && in && in.tellg() != end) {
        std::cerr << "incorrect length for object" << std::endl;
    }
    return in;
}

static std::unique_ptr<json::vector> map_is_vector(json::map &m) {
    int idx = 0;
    for (auto &el : m) {
        if (*el.first != std::to_string(idx).c_str()) return nullptr;
        ++idx;
    }
    if (idx == 0) return nullptr;
    auto rv = mkuniq<json::vector>();
    for (auto &el : m) rv->push_back(std::move(el.second));
    // return std::move(rv);
    return rv;
}

std::istream &operator>>(std::istream &in, bson_wrap<std::unique_ptr<obj>> json) {
    json::map rv;
    in >> binary(rv);
    if (auto asvec = map_is_vector(rv))
        json.o = std::move(asvec);
    else
        json.o = mkuniq<json::map>(std::move(rv));
    return in;
}

std::string bson_encode(const json::vector &v);
std::string bson_encode(const json::map &m);

std::string bson_encode_element(const std::string &key, const json::obj *o) {
    if (!o) return '\x0A' + key + '\0';
    if (o->is<json::True>()) return '\x08' + key + '\0' + '\1';
    if (o->is<json::False>()) return '\x08' + key + '\0' + '\0';
    if (o->is<json::number>()) {
        auto &n = o->to<json::number>();
        if (static_cast<int32_t>(n.val) == n.val)
            return '\x10' + key + '\0' + out32(n.val);
        else
            return '\x12' + key + '\0' + out64(n.val);
    }
    if (o->is<json::string>()) {
        auto &s = o->to<json::string>();
        return '\x02' + key + '\0' + out32(s.size() + 1) + s + '\0';
    }
    if (o->is<json::vector>()) {
        auto doc = bson_encode(o->to<json::vector>());
        return '\x04' + key + '\0' + out32(doc.size() + 4) + doc;
    }
    if (o->is<json::map>()) {
        auto doc = bson_encode(o->to<json::map>());
        return '\x03' + key + '\0' + out32(doc.size() + 4) + doc;
    }
    assert(0);
    return "";  // quiet warning
}

std::string bson_encode(const json::vector &v) {
    std::string rv;
    int idx = 0;
    for (auto &el : v) {
        rv += bson_encode_element(std::to_string(idx), el.get());
        ++idx;
    }
    rv += '\0';
    return rv;
}
std::string bson_encode(const json::map &m) {
    std::string rv;
    for (auto &el : m) {
        if (auto key = el.first->as_string())
            rv += bson_encode_element(*key, el.second.get());
        else
            std::cerr << "Can't encode non-string key in bson object" << std::endl;
    }
    rv += '\0';
    return rv;
}

std::ostream &operator<<(std::ostream &out, bson_wrap<const vector> v) {
    auto data = bson_encode(v.o);
    out.write(out32(data.size() + 4).c_str(), 4);
    out.write(data.data(), data.size());
    return out;
}
std::ostream &operator<<(std::ostream &out, bson_wrap<const map> m) {
    auto data = bson_encode(m.o);
    out.write(out32(data.size() + 4).c_str(), 4);
    out.write(data.data(), data.size());
    return out;
}

std::ostream &operator<<(std::ostream &out, bson_wrap<const obj> json) {
    if (auto m = json.o.as_map()) return out << binary(*m);
    if (auto v = json.o.as_vector()) return out << binary(*v);
    std::cerr << "object not map or vector can't be output as bson" << std::endl;
    return out;
}

}  // end namespace json
