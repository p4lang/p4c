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

#include <cstring>
#include <fstream>
#include <iomanip>
#include <set>

#include "fdstream.h"
#include "json.h"
#include "ordered_map.h"

static bool show_deletion = true;
static bool show_addition = true;
static bool sort_map = true;
static std::vector<const char *> list_map_keys;
static std::set<std::string> ignore_keys;
static std::map<std::string, std::set<int64_t>> ignore_key_indexes;
static std::vector<std::pair<int64_t, int64_t>> ignore_intkeys;

bool is_list_map(json::vector *v, const char *key) {
    if (!key) return false;
    for (auto &e : *v)
        if (json::map *m = dynamic_cast<json::map *>(e.get())) {
            if (!m->count(key)) return false;
        } else
            return false;
    return true;
}

void add_ignore(const char *a) {
    while (isspace(*a)) a++;
    if (*a == '#' || *a == 0) return;
    if (*a == '&' || *a == '=' || *a == '|' || isdigit(*a)) {
        int64_t mask, val;
        int end = 0;
        if (sscanf(a, "%" PRIi64 " %n", &val, &end) >= 1)
            ignore_intkeys.emplace_back(-1, val);
        else if (sscanf(a, "== %" PRIi64 " %n", &val, &end) >= 1)
            ignore_intkeys.emplace_back(-1, val);
        else if (sscanf(a, "& %" PRIi64 " == %" PRIi64 " %n", &mask, &val, &end) >= 2)
            ignore_intkeys.emplace_back(mask, val);
        else if (sscanf(a, "| %" PRIi64 " == %" PRIi64 " %n", &mask, &val, &end) >= 2)
            ignore_intkeys.emplace_back(~mask, val ^ mask);
        else {
            std::cerr << "Unknown ignore expression " << a << std::endl;
            return;
        }
        if (a[end]) std::cerr << "extra text after ignore " << (a + end) << std::endl;
        return;
    }
    if (auto *idx = strchr(a, '[')) {
        int64_t val;
        int end = 0;
        if (sscanf(idx, "[%" PRIi64 " ] %n", &val, &end) >= 1 && end > 0) {
            end += idx - a;
            while (idx > a && isspace(idx[-1])) --idx;
            std::string key(a, idx - a);
            ignore_key_indexes[key].insert(val);
        } else {
            std::cerr << "Unknown ignore expression " << a << std::endl;
            return;
        }
        if (a[end]) std::cerr << "extra text after ignore " << (a + end) << std::endl;
        return;
    }
    ignore_keys.insert(a);
}
bool ignore(json::obj *o) {
    if (json::string *s = dynamic_cast<json::string *>(o)) {
        if (ignore_keys.count(*s)) return true;
    } else if (json::number *n = dynamic_cast<json::number *>(o)) {
        for (auto &k : ignore_intkeys)
            if ((n->val & k.first) == k.second) return true;
    }
    return false;
}
bool ignore(std::unique_ptr<json::obj> &o) { return ignore(o.get()); }

const std::set<int64_t> &ignore_indexes_for_key(json::obj *key) {
    if (key && key->as_string() && ignore_key_indexes.count(*key->as_string()))
        return ignore_key_indexes.at(*key->as_string());
    static std::set<int64_t> empty;
    return empty;
}

std::map<json::obj *, json::map *, json::obj::ptrless> build_list_map(json::vector *v,
                                                                      const char *key) {
    std::map<json::obj *, json::map *, json::obj::ptrless> rv;
    assert(key);
    for (auto &e : *v) {
        json::map *m = dynamic_cast<json::map *>(e.get());
        assert(m);
        rv[(*m)[key].get()] = m;
    }
    return rv;
}

void do_prefix(int indent, const char *prefix) {
    std::cout << '\n' << prefix;
    if (indent) std::cout << std::setw(indent) << ' ' << std::setw(0);
}

void do_output(json::obj *o, int indent, const char *prefix, const char *suffix = "") {
    do_prefix(indent, prefix);
    if (o)
        o->print_on(std::cout, indent, 80 - indent, prefix);
    else
        std::cout << "null";
    std::cout << suffix;
}

void do_output(int index, json::vector::iterator p, int indent, const char *prefix) {
    do_prefix(indent, prefix);
    std::cout << '[' << index << "] ";
    if (*p)
        (*p)->print_on(std::cout, indent, 80 - indent, prefix);
    else
        std::cout << "null";
}

void do_output(json::map::iterator p, int indent, const char *prefix) {
    do_prefix(indent, prefix);
    p->first->print_on(std::cout, indent, 80 - indent, prefix);
    std::cout << ": ";
    if (p->second)
        p->second->print_on(std::cout, indent, 80 - indent, prefix);
    else
        std::cout << "null";
}

void do_output(std::map<json::obj *, json::map *, json::obj::ptrless>::iterator p, int indent,
               const char *prefix) {
    do_prefix(indent, prefix);
    p->first->print_on(std::cout, indent, 80 - indent, prefix);
    std::cout << ": ";
    if (p->second)
        p->second->print_on(std::cout, indent, 80 - indent, prefix);
    else
        std::cout << "null";
}

void do_output(std::map<json::obj *, json::obj *, json::obj::ptrless>::iterator p, int indent,
               const char *prefix) {
    do_prefix(indent, prefix);
    p->first->print_on(std::cout, indent, 80 - indent, prefix);
    std::cout << ": ";
    if (p->second)
        p->second->print_on(std::cout, indent, 80 - indent, prefix);
    else
        std::cout << "null";
}

bool equiv(json::obj *a, json::obj *b, json::obj *key = nullptr);
bool equiv(std::unique_ptr<json::obj> &a, json::obj *b, json::obj *key = nullptr) {
    return equiv(a.get(), b, key);
}
bool equiv(std::unique_ptr<json::obj> &a, std::unique_ptr<json::obj> &b, json::obj *key = nullptr) {
    return equiv(a.get(), b.get(), key);
}
void print_diff(json::obj *a, json::obj *b, int indent, json::obj *key = nullptr);
void print_diff(std::unique_ptr<json::obj> &a, std::unique_ptr<json::obj> &b, int indent,
                json::obj *key = nullptr) {
    print_diff(a.get(), b.get(), indent, key);
}

json::vector::iterator find(json::vector::iterator p, json::vector::iterator end, json::obj *m) {
    while (p < end && !equiv(*p, m)) ++p;
    return p;
}

bool list_map_equiv(json::vector *a, json::vector *b, const char *key) {
    auto bmap = build_list_map(b, key);
    for (auto &e : *a) {
        json::map *m = dynamic_cast<json::map *>(e.get());
        json::obj *ekey = (*m)[key].get();
        if (!bmap.count(ekey)) {
            if (show_deletion && !ignore(ekey)) return false;
            continue;
        }
        if (!ignore(ekey) && !equiv(m, bmap[ekey], ekey)) return false;
        bmap.erase(ekey);
    }
    if (show_addition)
        for (auto &e : bmap)
            if (!ignore(e.first)) return false;
    return true;
}
void list_map_print_diff(json::vector *a, json::vector *b, int indent, const char *key) {
    auto amap = build_list_map(a, key);
    auto bmap = build_list_map(b, key);
    auto p1 = amap.begin(), p2 = bmap.begin();
    std::cout << " [";
    indent += 2;
    while (p1 != amap.end() && p2 != bmap.end()) {
        if (*p1->first < *p2->first) {
            if (show_deletion && !ignore(p1->first)) do_output(p1, indent, "-");
            p1++;
            continue;
        }
        if (*p2->first < *p1->first) {
            if (show_addition && !ignore(p2->first)) do_output(p2, indent, "+");
            p2++;
            continue;
        }
        if (!ignore(p1->first) && !equiv(p1->second, p2->second, p1->first)) {
            int width = 80 - indent, copy;
            if (p1->first->test_width(width) && (copy = width) && p1->second &&
                p1->second->test_width(width) && p2->second && p2->second->test_width(copy)) {
                do_output(p1->first, indent, "-", ": ");
                std::cout << p1->second;
                do_output(p2->first, indent, "+", ": ");
                std::cout << p2->second;
            } else {
                do_output(p1->first, indent, " ", ":");
                print_diff(p1->second, p2->second, indent, p1->first);
            }
        }
        p1++;
        p2++;
    }
    if (show_deletion)
        while (p1 != amap.end()) {
            if (!ignore(p1->first)) do_output(p1, indent, "-");
            p1++;
        }
    if (show_addition)
        while (p2 != bmap.end()) {
            if (!ignore(p2->first)) do_output(p2, indent, "+");
            p2++;
        }
    indent -= 2;
    do_prefix(indent, " ");
    std::cout << ']';
}

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wpotentially-evaluated-expression"
bool equiv(json::vector *a, json::vector *b, const std::set<int64_t> &ignore_idx) {
    for (auto key : list_map_keys)
        if (is_list_map(a, key) && is_list_map(b, key)) return list_map_equiv(a, b, key);
    auto p1 = a->begin(), p2 = b->begin();
    while (p1 != a->end() && p2 != b->end()) {
        if (!ignore_idx.count(p1 - a->begin()) && !equiv(*p1, *p2)) {
            auto s1 = find(p1, a->end(), p2->get());
            auto s2 = find(p2, b->end(), p1->get());
            if (typeid(**p1) == typeid(**p2) && p1 - a->begin() == p2 - b->begin() &&
                (s1 - p1 == s2 - p2 || typeid(**p1) == typeid(json::vector) ||
                 typeid(**p1) == typeid(json::map)))
                return false;
            if (s1 - p1 <= s2 - p2) {
                if (show_deletion) return false;
                ++p1;
            } else {
                if (show_addition) return false;
                ++p2;
            }
        } else {
            ++p1;
            ++p2;
        }
    }
    while (p1 != a->end() && ignore_idx.count(p1 - a->begin())) ++p1;
    if (p1 != a->end() && show_deletion) return false;
    while (p2 != b->end() && ignore_idx.count(p2 - b->begin())) ++p2;
    if (p2 != b->end() && show_addition) return false;
    return true;
}
void print_diff(json::vector *a, json::vector *b, const std::set<int64_t> &ignore_idx, int indent) {
    for (auto key : list_map_keys)
        if (is_list_map(a, key) && is_list_map(b, key)) {
            list_map_print_diff(a, b, indent, key);
            return;
        }
    auto p1 = a->begin(), p2 = b->begin();
    std::cout << " [";
    indent += 2;
    while (p1 != a->end() && p2 != b->end()) {
        if (!ignore_idx.count(p1 - a->begin()) && !equiv(*p1, *p2)) {
            auto s1 = find(p1, a->end(), p2->get());
            auto s2 = find(p2, b->end(), p1->get());
            if ((p1 + 1 != a->end() && p2 + 1 != b->end() && equiv(p1[1], p2[1])) ||
                (typeid(**p1) == typeid(**p2) && p1 - a->begin() == p2 - b->begin() &&
                 (s1 - p1 == s2 - p2 || typeid(**p1) == typeid(json::vector) ||
                  typeid(**p1) == typeid(json::map)))) {
                do_prefix(indent, " ");
                std::cout << '[' << p1 - a->begin() << "]";
                print_diff(p1->get(), p2->get(), indent);
            } else {
                if (s1 - p1 <= s2 - p2) {
                    if (show_deletion) do_output(p1 - a->begin(), p1, indent, "-");
                    ++p1;
                } else {
                    if (show_addition) do_output(p2 - b->begin(), p2, indent, "+");
                    ++p2;
                }
                continue;
            }
        }

        ++p1;
        ++p2;
    }
    if (show_deletion)
        while (p1 != a->end()) {
            if (!ignore_idx.count(p1 - a->begin())) do_output(p1 - a->begin(), p1, indent, "-");
            ++p1;
        }
    if (show_addition)
        while (p2 != b->end()) {
            if (!ignore_idx.count(p2 - b->begin())) do_output(p2 - b->begin(), p2, indent, "+");
            ++p2;
        }
    indent -= 2;
    do_prefix(indent, " ");
    std::cout << ']';
}
#pragma clang diagnostic pop

std::map<json::obj *, json::obj *, json::obj::ptrless> build_sort_map(json::map *m) {
    std::map<json::obj *, json::obj *, json::obj::ptrless> rv;
    for (auto &e : *m) {
        rv[e.first] = e.second.get();
    }
    return rv;
}
bool sort_map_equiv(json::map *a, json::map *b) {
    auto bmap = build_sort_map(b);
    for (auto &e : *a) {
        json::obj *ekey = e.first;
        if (!bmap.count(ekey)) {
            if (show_deletion && !ignore(ekey)) return false;
            continue;
        }
        if (!ignore(ekey) && !equiv(e.second.get(), bmap[ekey], ekey)) return false;
        bmap.erase(ekey);
    }
    if (show_addition)
        for (auto &e : bmap)
            if (!ignore(e.first)) return false;
    return true;
}
void sort_map_print_diff(json::map *a, json::map *b, int indent) {
    auto amap = build_sort_map(a);
    auto bmap = build_sort_map(b);
    auto p1 = amap.begin(), p2 = bmap.begin();
    std::cout << " {";
    indent += 2;
    while (p1 != amap.end() && p2 != bmap.end()) {
        if (*p1->first < *p2->first) {
            if (show_deletion && !ignore(p1->first) && p1->second) do_output(p1, indent, "-");
            p1++;
            continue;
        }
        if (*p2->first < *p1->first) {
            if (show_addition && !ignore(p2->first) && p2->second) do_output(p2, indent, "+");
            p2++;
            continue;
        }
        if (!ignore(p1->first) && !equiv(p1->second, p2->second, p1->first)) {
            int width = 80 - indent, copy;
            if (p1->first->test_width(width) && (copy = width) && p1->second &&
                p1->second->test_width(width) && p2->second && p2->second->test_width(copy)) {
                do_output(p1->first, indent, "-", ": ");
                std::cout << p1->second;
                do_output(p2->first, indent, "+", ": ");
                std::cout << p2->second;
            } else {
                do_output(p1->first, indent, " ", ":");
                print_diff(p1->second, p2->second, indent, p1->first);
            }
        }
        p1++;
        p2++;
    }
    if (show_deletion)
        while (p1 != amap.end()) {
            if (!ignore(p1->first)) do_output(p1, indent, "-");
            p1++;
        }
    if (show_addition)
        while (p2 != bmap.end()) {
            if (!ignore(p2->first)) do_output(p2, indent, "+");
            p2++;
        }
    indent -= 2;
    do_prefix(indent, " ");
    std::cout << '}';
}

bool equiv(json::map *a, json::map *b) {
    if (sort_map) return sort_map_equiv(a, b);
    auto p1 = a->begin(), p2 = b->begin();
    while (p1 != a->end() && p2 != b->end()) {
        if (*p1->first < *p2->first) {
            if (show_deletion && !ignore(p1->first)) return false;
            ++p1;
        } else if (*p2->first < *p1->first) {
            if (show_addition && !ignore(p2->first)) return false;
            ++p2;
        } else if (!ignore(p1->first) && !(equiv(p1->second, p2->second, p1->first))) {
            return false;
        } else {
            ++p1;
            ++p2;
        }
    }
    if (show_deletion)
        for (; p1 != a->end(); ++p1)
            if (!ignore(p1->first)) return false;
    if (show_addition)
        for (; p2 != b->end(); ++p2)
            if (!ignore(p2->first)) return false;
    return true;
}
void print_diff(json::map *a, json::map *b, int indent) {
    if (sort_map) {
        sort_map_print_diff(a, b, indent);
        return;
    }
    auto p1 = a->begin(), p2 = b->begin();
    std::cout << " {";
    indent += 2;
    while (p1 != a->end() && p2 != b->end()) {
        if (*p1->first < *p2->first) {
            if (show_deletion && !ignore(p1->first)) do_output(p1, indent, "-");
            p1++;
            continue;
        }
        if (*p2->first < *p1->first) {
            if (show_addition && !ignore(p2->first)) do_output(p2, indent, "+");
            p2++;
            continue;
        }
        if (!ignore(p1->first) && !equiv(p1->second, p2->second, p1->first)) {
            int width = 80 - indent, copy;
            if (p1->first->test_width(width) && (copy = width) && p1->second &&
                p1->second->test_width(width) && p2->second && p2->second->test_width(copy)) {
                do_output(p1->first, indent, "-", ": ");
                std::cout << p1->second;
                do_output(p2->first, indent, "+", ": ");
                std::cout << p2->second;
            } else {
                do_output(p1->first, indent, " ", ":");
                print_diff(p1->second, p2->second, indent, p1->first);
            }
        }
        p1++;
        p2++;
    }
    if (show_deletion)
        for (; p1 != a->end(); ++p1)
            if (!ignore(p1->first)) do_output(p1, indent, "-");
    if (show_addition)
        for (; p2 != b->end(); ++p2)
            if (!ignore(p2->first)) do_output(p2, indent, "+");
    indent -= 2;
    do_prefix(indent, " ");
    std::cout << '}';
}

bool equiv(json::obj *a, json::obj *b, json::obj *key) {
    if (a == b) return true;
    // Check true for map/vector with nullptr v/s with no elements "{}"
    if (!a) {
        if (auto m = b->as_map()) {
            if (m->empty()) return true;
        }
        if (auto v = b->as_vector()) {
            if (v->empty()) return true;
        }
    }
    if (!b) {
        if (auto m = a->as_map()) {
            if (m->empty()) return true;
        }
        if (auto v = a->as_vector()) {
            if (v->empty()) return true;
        }
    }
    if (!a || !b) return false;
    if (typeid(*a) != typeid(*b)) return false;
    if (typeid(*a) == typeid(json::vector))
        return equiv(static_cast<json::vector *>(a), static_cast<json::vector *>(b),
                     ignore_indexes_for_key(key));
    if (typeid(*a) == typeid(json::map))
        return equiv(static_cast<json::map *>(a), static_cast<json::map *>(b));
    return *a == *b;
}
void print_diff(json::obj *a, json::obj *b, int indent, json::obj *key) {
    if (equiv(a, b))
        return;
    else if (!a) {
        if (show_deletion) do_output(b, indent, "+");
        return;
    } else if (!b) {
        if (show_addition) do_output(a, indent, "-");
        return;
    } else if (typeid(*a) == typeid(*b)) {
        if (typeid(*a) == typeid(json::vector)) {
            print_diff(static_cast<json::vector *>(a), static_cast<json::vector *>(b),
                       ignore_indexes_for_key(key), indent);
            return;
        } else if (typeid(*a) == typeid(json::map)) {
            print_diff(static_cast<json::map *>(a), static_cast<json::map *>(b), indent);
            return;
        }
    }
    do_output(a, indent, "-");
    do_output(b, indent, "+");
}

int do_diff(const char *a_name, json::obj *a, const char *b_name, json::obj *b) {
    if (equiv(a, b)) return 0;
    std::cout << "--- " << a_name << std::endl;
    std::cout << "+++ " << b_name << std::endl;
    print_diff(a, b, 0);
    std::cout << std::endl;
    return 1;
}
int do_diff(const char *a_name, std::unique_ptr<json::obj> &a, const char *b_name,
            std::unique_ptr<json::obj> &b) {
    return do_diff(a_name, a.get(), b_name, b.get());
}

int main(int ac, char **av) {
    int error = 0;
    std::unique_ptr<json::obj> file1;
    const char *file1_name = 0;
    for (int i = 1; i < ac; i++)
        if (av[i][0] == '-' && av[i][1] == 0) {
            if (file1) {
                std::unique_ptr<json::obj> file2;
                if (!(std::cin >> file2) || !file2) {
                    std::cerr << "Failed reading json from stdin" << std::endl;
                    error = 2;
                } else if (!(error & 2))
                    error |= do_diff(file1_name, file1, "<stdin>", file2);
            } else if (!(std::cin >> file1) || !file1) {
                std::cerr << "Failed reading json from stdin" << std::endl;
                error = 2;
            } else
                file1_name = "<stdin>";
        } else if (av[i][0] == '-' || av[i][0] == '+') {
            bool flag = av[i][0] == '+';
            for (char *arg = av[i] + 1; *arg;) switch (*arg++) {
                    case 'a':
                        show_addition = flag;
                        break;
                    case 'd':
                        show_deletion = flag;
                        break;
                    case 'i':
                        if (*av[++i] == '@') {
                            std::ifstream file(av[i] + 1);
                            std::string str;
                            if (!file)
                                std::cerr << "Can't read " << av[i] + 1 << std::endl;
                            else
                                while (getline(file, str)) add_ignore(str.c_str());
                        } else
                            add_ignore(av[i]);
                        break;
                    case 'l':
                        list_map_keys.push_back(av[++i]);
                        break;
                    case 's':
                        sort_map = flag;
                        break;
                    default:
                        std::cerr << "Unknown option " << (flag ? '+' : '-') << arg[-1]
                                  << std::endl;
                        error = 2;
                }
        } else {
            std::istream *in = nullptr;
            if (auto ext = strrchr(av[i], '.')) {
                std::string cmd;
                if (!strcmp(ext, ".gz") || !strcmp(ext, ".Z"))
                    cmd = "zcat ";
                else if (!strcmp(ext, ".bz") || !strcmp(ext, ".bz2"))
                    cmd = "bzcat ";
                if (!cmd.empty()) {
                    cmd += av[i];
                    cmd = "2>/dev/null; " + cmd;  // ignore errors (Broken Pipe in particular)
                    auto *pipe = popen(cmd.c_str(), "r");
                    if (pipe) {
                        auto *pstream = new fdstream(fileno(pipe));
                        pstream->setclose([pipe]() { pclose(pipe); });
                        in = pstream;
                    }
                }
            }
            if (!in) in = new std::ifstream(av[i]);
            if (!in || !*in) {
                std::cerr << "Can't open " << av[i] << " for reading" << std::endl;
                error = 2;
            } else if (file1) {
                std::unique_ptr<json::obj> file2;
                if (!(*in >> file2) || !file2) {
                    std::cerr << "Failed reading json from " << av[i] << std::endl;
                    error = 2;
                } else if (!(error & 2))
                    error |= do_diff(file1_name, file1, av[i], file2);
            } else if (!(*in >> file1) || !file1) {
                std::cerr << "Failed reading json from " << av[i] << std::endl;
                error = 2;
            } else
                file1_name = av[i];
            delete in;
        }
    if (error & 2) std::cerr << "usage: " << av[0] << " [-adi:l:] file1 file2" << std::endl;
    return error;
}
