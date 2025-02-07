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

#include "hashexpr.h"

#include "bitops.h"
#include "bitvec.h"
#include "input_xbar.h"

static bitvec crc(bitvec poly, bitvec val) {
    int poly_size = poly.max().index() + 1;
    if (!poly_size) return bitvec(0);
    val <<= poly_size - 1;
    for (auto i = val.max(); i.index() >= (poly_size - 1); --i) {
        BUG_CHECK(*i);
        val ^= poly << (i.index() - (poly_size - 1));
    }
    return val;
}

static bool check_ixbar(Phv::Ref &ref, InputXbar *ix, InputXbar::HashTable hash_table) {
    if (!ref.check()) return false;
    if (ref->reg.mau_id() < 0) {
        error(ref.lineno, "%s not accessable in mau", ref->reg.name);
        return false;
    }
    if (!hash_table) return true;
    for (auto in : ix->find_hash_inputs(*ref, hash_table)) {
        BUG_CHECK(in->lo >= 0, "invalid lo in IXBar::Input");
        return true;
    }
    error(ref.lineno, "%s not in %s input", ref.name(), hash_table.toString().c_str());
    return false;
}

/**
 * Generating a list of ixbar_input_t and hash_matrix_output_t to be sent to the
 * dynamic_hash library.  The vectors are part of the function call as they
 * must be on the stack to avoid using new and delete
 */
void HashExpr::gen_ixbar_init(ixbar_init_t *ixbar_init, std::vector<ixbar_input_t> &inputs,
                              std::vector<hash_matrix_output_t> &outputs, int logical_hash_bit,
                              InputXbar *ix, InputXbar::HashTable hash_table) {
    inputs.clear();
    outputs.clear();

    gen_ixbar_inputs(inputs, ix, hash_table);
    hash_matrix_output_t hmo;
    hmo.p4_hash_output_bit = logical_hash_bit;
    hmo.gfm_start_bit = 0;
    hmo.bit_size = 1;
    outputs.push_back(hmo);

    ixbar_init->ixbar_inputs = inputs.data();
    ixbar_init->inputs_sz = inputs.size();
    ixbar_init->hash_matrix_outputs = outputs.data();
    ixbar_init->outputs_sz = outputs.size();
}

/**
 * The function call for PhvRef, Random, Identity, and Crc functions.  The input xbar is
 * initialized, and the data returned writes out a vector of inputs.  For Stripe,
 * Slice, and others, they recursively will call this function
 */
void HashExpr::gen_data(bitvec &data, int logical_hash_bit, InputXbar *ix,
                        InputXbar::HashTable hash_table) {
    ixbar_init_t ixbar_init;
    hash_column_t hash_matrix[PARITY_GROUPS_DYN][HASH_MATRIX_WIDTH_DYN] = {};
    std::vector<ixbar_input_t> inputs;
    std::vector<hash_matrix_output_t> outputs;

    gen_ixbar_init(&ixbar_init, inputs, outputs, logical_hash_bit, ix, hash_table);

    bool non_zero = false;
    int loops = 0;
    // It is possible that a hash column can be genereated as all 0s if using RANDOM_DYN algo, so
    // regeneration is required if a hash column is all 0s and using RANDOM_DYN.
    while (!non_zero) {
        determine_hash_matrix(&ixbar_init, ixbar_init.ixbar_inputs, ixbar_init.inputs_sz,
                              &hash_algorithm, hash_matrix);
        if (hash_algorithm.hash_alg != RANDOM_DYN ||
            ix->global_column0_extract(hash_table, hash_matrix)) {
            non_zero = true;
        }
        BUG_CHECK(loops++ < 1000, "Looping trying to get a valid RANDOM_DYN matrix");
    }
    data |= ix->global_column0_extract(hash_table, hash_matrix);
}

class HashExpr::PhvRef : HashExpr {
    Phv::Ref what;
    PhvRef(gress_t gr, int stg, const value_t &v) : HashExpr(v.lineno), what(gr, stg, v) {}
    friend class HashExpr;
    bool check_ixbar(InputXbar *ix, InputXbar::HashTable hash_table) override {
        return ::check_ixbar(what, ix, hash_table);
    }
    int width() override { return what.size(); }
    int input_size() override { return what.size(); }
    bool match_phvref(const Phv::Ref &ref) override {
        if (what->reg != ref->reg || what->lo != ref->lo) return false;
        return true;
    }
    bool operator==(const HashExpr &a_) const override {
        if (typeid(*this) != typeid(a_)) return false;
        auto &a = static_cast<const PhvRef &>(a_);
        return *what == *a.what;
    }
    void build_algorithm() override {
        hash_algorithm.hash_alg = IDENTITY_DYN;
        hash_algorithm.msb = false;
        hash_algorithm.extend = false;
        hash_algorithm.final_xor = 0ULL;
        hash_algorithm.poly = 0ULL;
        hash_algorithm.init = 0ULL;
        hash_algorithm.reverse = false;
    }

    void gen_ixbar_inputs(std::vector<ixbar_input_t> &inputs, InputXbar *ix,
                          InputXbar::HashTable hash_table) override;
    void get_sources(int bit, std::vector<Phv::Ref> &rv) const override {
        if (bit >= 0)
            rv.emplace_back(what, bit, bit);
        else
            rv.emplace_back(what);
    }
    Phv::Ref *get_ghost_slice() override { return &what; }
    void dbprint(std::ostream &out) const override {
        out << "HashExpr: PhvRef" << std::endl;
        out << "hash algorithm: [ algo : " << hash_algorithm.hash_alg
            << ", msb : " << hash_algorithm.msb << ", extend : " << hash_algorithm.extend
            << ", final_xor : " << hash_algorithm.final_xor << ", poly : " << hash_algorithm.poly
            << ", init : " << hash_algorithm.init << ", reverse : " << hash_algorithm.reverse
            << std::endl;
        if (what) out << "Phv: " << what << std::endl;
    }
};

class HashExpr::Random : HashExpr {
    std::vector<Phv::Ref> what;
    explicit Random(int lineno) : HashExpr(lineno) {}
    friend class HashExpr;
    bool check_ixbar(InputXbar *ix, InputXbar::HashTable hash_table) override {
        bool rv = true;
        for (auto &ref : what) rv &= ::check_ixbar(ref, ix, hash_table);
        return rv;
    }
    int width() override { return 0; }
    int input_size() override {
        int rv = 0;
        for (auto &ref : what) rv += ref->size();
        return rv;
    }
    bool operator==(const HashExpr &a_) const override {
        if (typeid(*this) != typeid(a_)) return false;
        auto &a = static_cast<const Random &>(a_);
        if (what.size() != a.what.size()) return false;
        auto it = a.what.begin();
        for (auto &el : what)
            if (*el != **it++) return false;
        return true;
    }
    void build_algorithm() override {
        hash_algorithm.hash_alg = RANDOM_DYN;
        hash_algorithm.msb = false;
        hash_algorithm.extend = false;
        hash_algorithm.final_xor = 0ULL;
        hash_algorithm.poly = 0ULL;
        hash_algorithm.init = 0ULL;
        hash_algorithm.reverse = false;
    }
    void gen_ixbar_inputs(std::vector<ixbar_input_t> &inputs, InputXbar *ix,
                          InputXbar::HashTable hash_table) override;
    void get_sources(int, std::vector<Phv::Ref> &rv) const override {
        rv.insert(rv.end(), what.begin(), what.end());
    }
    void dbprint(std::ostream &out) const override {
        out << "HashExpr: Random" << std::endl;
        out << "hash algorithm: [ algo : " << hash_algorithm.hash_alg
            << ", msb : " << hash_algorithm.msb << ", extend : " << hash_algorithm.extend
            << ", final_xor : " << hash_algorithm.final_xor << ", poly : " << hash_algorithm.poly
            << ", init : " << hash_algorithm.init << ", reverse : " << hash_algorithm.reverse
            << std::endl;
        for (auto &e : what) {
            out << "Phv: " << e << std::endl;
        }
    }
};

class HashExpr::Crc : HashExpr {
    bitvec poly;
    bitvec init;
    bitvec final_xor;
    ///> It is a multimap to allow two fields to have the exact same hash matrix requirements
    std::multimap<unsigned, Phv::Ref> what;
    std::map<int, bitvec> constants;
    std::vector<Phv::Ref> vec_what;
    bool reverse = false;
    int total_input_bits = -1;
    explicit Crc(int lineno) : HashExpr(lineno) {}
    friend class HashExpr;
    bool check_ixbar(InputXbar *ix, InputXbar::HashTable hash_table) override;
    int width() override { return poly.max().index(); }
    int input_size() override {
        if (total_input_bits >= 0) return total_input_bits;
        if (what.empty()) {
            int rv = 0;
            for (auto &ref : vec_what) rv += ref->size();
            return rv;
        } else {
            return what.rbegin()->first + what.rbegin()->second->size();
        }
    }
    bool operator==(const HashExpr &a_) const override {
        if (typeid(*this) != typeid(a_)) return false;
        auto &a = static_cast<const Crc &>(a_);
        if (what.size() != a.what.size()) return false;
        if (vec_what.size() != a.vec_what.size()) return false;
        auto it = a.what.begin();
        for (auto &el : what)
            if (el.first != it->first || *el.second != *(it++)->second) return false;
        auto it2 = a.vec_what.begin();
        for (auto &el : vec_what)
            if (*el != **it2++) return false;
        return true;
    }
    void build_algorithm() override {
        hash_algorithm.hash_bit_width = poly.max().index();
        hash_algorithm.hash_alg = CRC_DYN;
        hash_algorithm.reverse = reverse;
        hash_algorithm.poly = poly.getrange(32, 32) << 32;
        hash_algorithm.poly |= poly.getrange(0, 32);
        hash_algorithm.init = init.getrange(32, 32) << 32;
        hash_algorithm.init |= init.getrange(0, 32);
        hash_algorithm.final_xor = final_xor.getrange(0, 32);
        hash_algorithm.final_xor |= final_xor.getrange(32, 32) << 32;
        hash_algorithm.extend = false;
        hash_algorithm.msb = false;
    }

    void gen_ixbar_inputs(std::vector<ixbar_input_t> &inputs, InputXbar *ix,
                          InputXbar::HashTable hash_table) override;
    void get_sources(int, std::vector<Phv::Ref> &rv) const override {
        rv.insert(rv.end(), vec_what.begin(), vec_what.end());
    }
};

/**
 * @brief XOR hashing algorithm implemented on the hashing matrix
 *
 * This expression implements XOR over the hasing matrix. The input
 * message is handled as a big integer number - the highest bit is
 * the begining, the zero-th bit is the end. The message is split
 * from the begining into blocks of length bit_width and these blocks
 * are bitwise XORed together.
 */
class HashExpr::XorHash : public HashExpr {
 private:
    std::multimap<unsigned, Phv::Ref> what;
    int bit_width;
    friend class HashExpr;

 public:
    explicit XorHash(int lineno, int bit_width_);

    /* -- avoid copying */
    XorHash &operator=(XorHash &&) = delete;

    bool check_ixbar(InputXbar *ix, InputXbar::HashTable hash_table) override;
    int width() override;
    int input_size() override;
    bool operator==(const HashExpr &a_) const override;
    void build_algorithm() override;
    void gen_ixbar_inputs(std::vector<ixbar_input_t> &inputs, InputXbar *ix,
                          InputXbar::HashTable hash_table) override;
    void get_sources(int, std::vector<Phv::Ref> &rv) const override;
};

class HashExpr::Xor : HashExpr {
    std::vector<HashExpr *> what;
    explicit Xor(int lineno) : HashExpr(lineno) {}
    friend class HashExpr;
    bool check_ixbar(InputXbar *ix, InputXbar::HashTable hash_table) override {
        bool rv = true;
        for (auto *e : what) rv |= e->check_ixbar(ix, hash_table);
        return rv;
    }
    void gen_data(bitvec &data, int logical_hash_bit, InputXbar *ix,
                  InputXbar::HashTable hash_table) override;
    int width() override {
        int rv = 0;
        for (auto *e : what) {
            int w = e->width();
            if (w > rv) rv = w;
        }
        return rv;
    }
    int input_size() override {
        int rv = 0;
        for (auto *e : what) rv += e->input_size();
        return rv;
    }
    bool operator==(const HashExpr &a_) const override {
        if (typeid(*this) != typeid(a_)) return false;
        auto &a = static_cast<const Xor &>(a_);
        if (what.size() != a.what.size()) return false;
        auto it = a.what.begin();
        for (auto &el : what)
            if (*el != **it++) return false;
        return true;
    }
    void build_algorithm() override {
        for (auto *e : what) {
            if (e) e->build_algorithm();
        }
    }

    void gen_ixbar_inputs(std::vector<ixbar_input_t> &inputs, InputXbar *ix,
                          InputXbar::HashTable hash_table) override {}
    void get_sources(int bit, std::vector<Phv::Ref> &rv) const override {
        for (auto *e : what) e->get_sources(bit, rv);
    }
    Phv::Ref *get_ghost_slice() override {
        for (auto *e : what) {
            auto g = e->get_ghost_slice();
            if (g) return g;
        }
        return nullptr;
    }
    void dbprint(std::ostream &out) const override {
        out << "HashExpr: Xor" << std::endl;
        for (auto *e : what) {
            e->dbprint(out);
        }
    }
};

class HashExpr::Mask : HashExpr {
    HashExpr *what;
    bitvec mask;
    Mask(int lineno, HashExpr *w, bitvec m) : HashExpr(lineno), what(w), mask(m) {}
    friend class HashExpr;
    bool check_ixbar(InputXbar *ix, InputXbar::HashTable hash_table) override {
        return what->check_ixbar(ix, hash_table);
    }
    void gen_data(bitvec &data, int bit, InputXbar *ix, InputXbar::HashTable hash_table) override {
        if (mask[bit]) what->gen_data(data, bit, ix, hash_table);
    }
    int width() override { return what->width(); }
    int input_size() override { return what->input_size(); }
    bool operator==(const HashExpr &a_) const override {
        if (typeid(*this) != typeid(a_)) return false;
        auto &a = static_cast<const Mask &>(a_);
        return mask == a.mask && *what == *a.what;
    }
    void build_algorithm() override { what->build_algorithm(); }

    void gen_ixbar_inputs(std::vector<ixbar_input_t> &inputs, InputXbar *ix,
                          InputXbar::HashTable hash_table) override {}
    void get_sources(int bit, std::vector<Phv::Ref> &rv) const override {
        if (mask[bit]) what->get_sources(bit, rv);
    }
    Phv::Ref *get_ghost_slice() override { return what->get_ghost_slice(); }
    void dbprint(std::ostream &out) const override {
        out << "HashExpr: Mask " << mask << std::endl;
        what->dbprint(out);
    }
};

class HashExpr::Stripe : HashExpr {
    std::vector<HashExpr *> what;
    bool supress_error_cascade = false;
    explicit Stripe(int lineno) : HashExpr(lineno) {}
    friend class HashExpr;
    bool check_ixbar(InputXbar *ix, InputXbar::HashTable hash_table) override {
        bool rv = true;
        for (auto *e : what) rv |= e->check_ixbar(ix, hash_table);
        return rv;
    }
    void gen_data(bitvec &data, int logical_hash_bit, InputXbar *ix,
                  InputXbar::HashTable hash_table) override;
    int width() override { return 0; }
    int input_size() override {
        int rv = 0;
        for (auto *e : what) rv += e->input_size();
        return rv;
    }
    bool operator==(const HashExpr &a_) const override {
        if (typeid(*this) != typeid(a_)) return false;
        auto &a = static_cast<const Stripe &>(a_);
        if (what.size() != a.what.size()) return false;
        auto it = a.what.begin();
        for (auto &el : what)
            if (*el != **it++) return false;
        return true;
    }
    void build_algorithm() override {
        for (auto *e : what) {
            e->build_algorithm();
        }
        // Does not set the extend algorithm, as the gen_data for extend does this
        // in the source
    }

    void gen_ixbar_inputs(std::vector<ixbar_input_t> &inputs, InputXbar *ix,
                          InputXbar::HashTable hash_table) override {}
    void get_sources(int bit, std::vector<Phv::Ref> &rv) const override {
        for (auto *e : what) {
            if (bit >= 0) {
                int width = e->width();
                if (bit < width) {
                    e->get_sources(bit, rv);
                    break;
                }
                bit -= width;
            } else {
                e->get_sources(bit, rv);
            }
        }
    }
    void dbprint(std::ostream &out) const override {
        out << "HashExpr: Stripe" << std::endl;
        for (auto *e : what) {
            e->dbprint(out);
        }
    }
};

class HashExpr::Slice : HashExpr {
    HashExpr *what = nullptr;
    int start = 0, _width = 0;
    explicit Slice(int lineno) : HashExpr(lineno) {}
    friend class HashExpr;
    bool check_ixbar(InputXbar *ix, InputXbar::HashTable hash_table) override {
        return what->check_ixbar(ix, hash_table);
    }
    void gen_data(bitvec &data, int logical_hash_bit, InputXbar *ix,
                  InputXbar::HashTable hash_table) override {
        what->gen_data(data, logical_hash_bit + start, ix, hash_table);
    }
    int width() override {
        if (_width == 0) {
            _width = what->width();
            if (_width > 0) {
                _width -= start;
                if (_width <= 0) _width = -1;
            }
        }
        return _width;
    }
    int input_size() override { return what->input_size(); }
    bool operator==(const HashExpr &a_) const override {
        if (typeid(*this) != typeid(a_)) return false;
        auto &a = static_cast<const Slice &>(a_);
        if (start != a.start || _width != a._width) return false;
        return *what == *a.what;
    }
    void build_algorithm() override { what->build_algorithm(); }
    void gen_ixbar_inputs(std::vector<ixbar_input_t> &inputs, InputXbar *ix,
                          InputXbar::HashTable hash_table) override {}
    void get_sources(int bit, std::vector<Phv::Ref> &rv) const override {
        if (bit >= start)
            what->get_sources(bit - start, rv);
        else if (bit < 0)
            what->get_sources(bit, rv);
    }
    void dbprint(std::ostream &out) const override {
        out << "HashExpr: Slice" << std::endl;
        if (what) out << what << std::endl;
        out << "start: " << start << " ,width: " << _width << std::endl;
    }
};

class HashExpr::SExtend : HashExpr {
    HashExpr *what;
    SExtend(int lineno, HashExpr *w) : HashExpr(lineno), what(w) {}
    friend class HashExpr;
    bool check_ixbar(InputXbar *ix, InputXbar::HashTable hash_table) override {
        return what->check_ixbar(ix, hash_table);
    }
    void gen_data(bitvec &data, int bit, InputXbar *ix, InputXbar::HashTable hash_table) override {
        int width = what->width();
        if (width > 0 && bit >= width) bit = width - 1;
        what->gen_data(data, bit, ix, hash_table);
    }
    int width() override { return 0; }
    int input_size() override { return what->input_size(); }
    bool operator==(const HashExpr &a_) const override {
        if (typeid(*this) != typeid(a_)) return false;
        auto &a = static_cast<const SExtend &>(a_);
        return *what == *a.what;
    }
    void build_algorithm() override { what->build_algorithm(); }
    void gen_ixbar_inputs(std::vector<ixbar_input_t> &inputs, InputXbar *ix,
                          InputXbar::HashTable hash_table) override {}
    void get_sources(int bit, std::vector<Phv::Ref> &rv) const override {
        int width = what->width();
        if (width > 0 && bit >= width) bit = width - 1;
        what->get_sources(bit, rv);
    }
    void dbprint(std::ostream &out) const override {
        out << "HashExpr: SExtend" << std::endl;
        if (what) out << what << std::endl;
    }
};

// The ordering for crc expression is:
// crc(poly, @optional init, @optional input_bits, map)
HashExpr *HashExpr::create(gress_t gress, int stage, const value_t &what) {
    if (what.type == tCMD) {
        if (what[0] == "random") {
            Random *rv = new Random(what.lineno);
            for (int i = 1; i < what.vec.size; i++) rv->what.emplace_back(gress, stage, what[i]);
            return rv;
        } else if (what[0] == "xor") {
            if (what.vec.size != 3) {
                error(what[1].lineno,
                      "Syntax error, invalid number of parameters for 'xor' hash expression");
                return nullptr;
            }
            if (!CHECKTYPE(what[1], tINT)) {
                return nullptr;
            }
            if (!CHECKTYPE(what[2], tMAP)) {
                return nullptr;
            }
            std::unique_ptr<XorHash> rv(new XorHash(what.lineno, what[1].i));
            for (auto &kv : what[2].map) {
                if (CHECKTYPE(kv.key, tINT)) {
                    rv->what.emplace(kv.key.i, Phv::Ref(gress, stage, kv.value));
                } else {
                    return nullptr;
                }
            }

            return rv.release();
        } else if ((what[0] == "crc" || what[0] == "crc_rev" || what[0] == "crc_reverse") &&
                   CHECKTYPE2(what[1], tBIGINT, tINT)) {
            Crc *rv = new Crc(what.lineno);
            if (what[0] != "crc") rv->reverse = true;
            rv->poly = get_bitvec(what[1]);
            // Shift and set LSB to 1 to generate polynomial from Koopman number
            // provided in assembly
            rv->poly <<= 1;
            rv->poly[0] = 1;
            int i = 2;

            if (what.vec.size > i && (what[i].type == tINT || what[i].type == tBIGINT))
                rv->init = get_bitvec(what[i++]);
            if (what.vec.size > i && (what[i].type == tINT || what[i].type == tBIGINT))
                rv->final_xor = get_bitvec(what[i++]);
            if (what.vec.size > i && what[i].type == tINT) rv->total_input_bits = what[i++].i;

            if (what.vec.size > i && what[i].type == tMAP) {
                for (auto &kv : what[i].map) {
                    if (CHECKTYPE(kv.key, tINT)) {
                        rv->what.emplace(kv.key.i, Phv::Ref(gress, stage, kv.value));
                    }
                }
            } else {
                for (; i < what.vec.size; i++) {
                    rv->vec_what.emplace_back(gress, stage, what[i]);
                }
            }
            return rv;
        } else if (what[0] == "^") {
            Xor *rv = new Xor(what.lineno);
            for (int i = 1; i < what.vec.size; i++)
                rv->what.push_back(create(gress, stage, what[i]));
            return rv;
        } else if (what[0] == "&") {
            HashExpr *op = nullptr;
            bitvec mask;
            bool have_mask = false;
            for (int i = 1; i < what.vec.size; i++) {
                if (what[i].type == tINT || what[i].type == tBIGINT) {
                    if (have_mask) {
                        mask &= get_bitvec(what[i]);
                    } else {
                        mask = get_bitvec(what[i]);
                        have_mask = true;
                    }
                } else if (op) {
                    error(what.lineno, "Invalid mask operation");
                    return nullptr;
                } else {
                    op = create(gress, stage, what[i]);
                }
            }
            if (!op) {
                error(what.lineno, "Invalid mask operation");
                return nullptr;
            } else if (have_mask) {
                return new Mask(what.lineno, op, mask);
            } else {
                return op;
            }
        } else if (what[0] == "stripe") {
            Stripe *rv = new Stripe(what.lineno);
            for (int i = 1; i < what.vec.size; i++)
                rv->what.push_back(create(gress, stage, what[i]));
            return rv;
        } else if (what[0] == "slice") {
            if (what.vec.size < 3 || what[2].type == tRANGE
                    ? what.vec.size > 3 || what[2].hi < what[2].lo
                    : what[2].type != tINT || what.vec.size > 4 ||
                          (what.vec.size == 4 && what[3].type != tINT)) {
                error(what.lineno, "Invalid slice operation");
                return nullptr;
            }
            Slice *rv = new Slice(what.lineno);
            rv->what = create(gress, stage, what[1]);
            if (what[2].type == tRANGE) {
                rv->start = what[2].lo;
                rv->_width = what[2].hi - what[2].lo + 1;
            } else {
                rv->start = what[2].i;
                if (what.vec.size > 3) rv->_width = what[3].i;
            }
            return rv;
        } else if (what[0] == "sextend" || what[0] == "sign_extend") {
            if (what.vec.size != 2) {
                error(what.lineno, "Invalid sign extension");
                return nullptr;
            }
            return new SExtend(what.lineno, create(gress, stage, what[1]));
        } else if (what.vec.size == 2) {
            return new PhvRef(gress, stage, what);
        } else {
            error(what.lineno, "Unsupported hash operation '%s'", what[0].s);
        }
    } else if (what.type == tSTR) {
        return new PhvRef(gress, stage, what);
    } else {
        error(what.lineno, "Syntax error, expecting hash expression");
    }
    return nullptr;
}

void HashExpr::find_input(Phv::Ref what, std::vector<ixbar_input_t> &inputs, InputXbar *ix,
                          InputXbar::HashTable hash_table) {
    bool found = false;
    auto vec = ix->find_hash_inputs(*what, hash_table);
    for (auto *in : vec) {
        int group_bit_position = in->lo + (what->lo - in->what->lo);
        ixbar_input_t input;
        input.type = ixbar_input_type::tPHV;
        input.ixbar_bit_position = group_bit_position + ix->global_bit_position_adjust(hash_table);
        input.bit_size = what->size();
        input.u.valid = true;
        input.symmetric_info.is_symmetric = false;
        inputs.push_back(input);
        found = true;
        break;
    }
    if (!found) {
        error(ix->lineno, "Cannot find associated field %s[%d:%d] in %s", what->reg.name, what->hi,
              what->lo, hash_table.toString().c_str());
    }
}

void HashExpr::generate_ixbar_inputs_with_gaps(const std::multimap<unsigned, Phv::Ref> &what,
                                               std::vector<ixbar_input_t> &inputs, InputXbar *ix,
                                               InputXbar::HashTable hash_table) {
    unsigned previous_range_hi = 0;
    for (auto &entry : what) {
        if (previous_range_hi != entry.first) {
            ixbar_input_t invalid_input = {
                ixbar_input_type::tPHV,           // type
                0,                                // ixbar_bit_position
                entry.first - previous_range_hi,  // bit_size
                false                             // u.valid
            };
            inputs.push_back(invalid_input);
        }

        auto &ref = entry.second;
        find_input(ref, inputs, ix, hash_table);
        previous_range_hi = entry.first + ref->size();
    }
    if (previous_range_hi != input_size()) {
        ixbar_input_t invalid_input = {
            ixbar_input_type::tPHV,            // type
            0,                                 // ixbar_bit_position
            input_size() - previous_range_hi,  // bit_size
            false                              // u.valid
        };
        inputs.push_back(invalid_input);
    }
}

/**
 * Creates a vector with a single entry corresponding to the identity input
 */
void HashExpr::PhvRef::gen_ixbar_inputs(std::vector<ixbar_input_t> &inputs, InputXbar *ix,
                                        InputXbar::HashTable hash_table) {
    find_input(what, inputs, ix, hash_table);
}

/**
 * Iterates through the list of references to build a corresponding vector for the
 * dynamic hash library
 */
void HashExpr::Random::gen_ixbar_inputs(std::vector<ixbar_input_t> &inputs, InputXbar *ix,
                                        InputXbar::HashTable hash_table) {
    for (auto &ref : what) {
        find_input(ref, inputs, ix, hash_table);
    }
}

/**
 * Iterates through the crc map, and will generate ixbar_input_t inputs for the holes.
 * These are marked as invalid, so that the hash calculation will be correct
 */
void HashExpr::Crc::gen_ixbar_inputs(std::vector<ixbar_input_t> &inputs, InputXbar *ix,
                                     InputXbar::HashTable hash_table) {
    generate_ixbar_inputs_with_gaps(what, inputs, ix, hash_table);
}

bool HashExpr::Crc::check_ixbar(InputXbar *ix, InputXbar::HashTable hash_table) {
    bool rv = true;
    if (!vec_what.empty()) {
        int off = 0;
        for (auto &ref : vec_what) {
            rv &= ::check_ixbar(ref, ix, InputXbar::HashTable());
            if (ref) {
                for (auto *in : ix->find_hash_inputs(*ref, hash_table)) {
                    if (in->lo >= 0) {
                        what.emplace(off, ref);
                        break;
                    }
                }
                off += ref.size();
            }
        }
        vec_what.clear();
    } else {
        int max = -1;
        for (auto &ref : what) {
            rv &= ::check_ixbar(ref.second, ix, hash_table);
        }
    }
    return rv;
}

HashExpr::XorHash::XorHash(int lineno, int bit_width_) : HashExpr(lineno), bit_width(bit_width_) {}

bool HashExpr::XorHash::check_ixbar(InputXbar *ix, InputXbar::HashTable hash_table) {
    bool rv(true);
    for (auto &ref : what) {
        rv = ::check_ixbar(ref.second, ix, hash_table) && rv;
    }
    return rv;
}

int HashExpr::XorHash::width() { return bit_width; }

int HashExpr::XorHash::input_size() {
    if (what.empty()) return 0;
    return what.rbegin()->first + what.rbegin()->second->size();
}

bool HashExpr::XorHash::operator==(const HashExpr &a_) const {
    if (typeid(*this) != typeid(a_)) return false;
    auto &a = static_cast<const XorHash &>(a_);

    if (what.size() != a.what.size()) return false;
    if (bit_width != a.bit_width) return false;

    auto iter1(what.begin());
    auto iter2(a.what.begin());
    while (iter1 != what.end()) {
        if (*iter1 != *iter2) return false;
        ++iter1;
        ++iter2;
    }
    return true;
}

void HashExpr::XorHash::build_algorithm() {
    memset(&hash_algorithm, 0, sizeof(hash_algorithm));
    hash_algorithm.hash_alg = XOR_DYN;
    hash_algorithm.extend = false;
    hash_algorithm.msb = false;
    hash_algorithm.hash_bit_width = bit_width;
}

void HashExpr::XorHash::gen_ixbar_inputs(std::vector<ixbar_input_t> &inputs, InputXbar *ix,
                                         InputXbar::HashTable hash_table) {
    generate_ixbar_inputs_with_gaps(what, inputs, ix, hash_table);
}

void HashExpr::XorHash::get_sources(int, std::vector<Phv::Ref> &rv) const {}

void HashExpr::Xor::gen_data(bitvec &data, int bit, InputXbar *ix,
                             InputXbar::HashTable hash_table) {
    for (auto *e : what) e->gen_data(data, bit, ix, hash_table);
}

void HashExpr::Stripe::gen_data(bitvec &data, int bit, InputXbar *ix,
                                InputXbar::HashTable hash_table) {
    while (1) {
        int total_size = 0;
        for (auto *e : what) {
            int sz = e->width();
            if (bit < total_size + sz) {
                e->gen_data(data, bit - total_size, ix, hash_table);
                return;
            }
            total_size += sz;
        }
        if (total_size == 0) {
            if (!supress_error_cascade) {
                error(lineno, "Can't stripe unsized data");
                supress_error_cascade = true;
            }
            break;
        }
        bit %= total_size;
    }
}

void dump(const HashExpr *h) {
    if (h)
        h->dbprint(std::cout);
    else
        std::cout << "(null)";
    std::cout << std::endl;
}
void dump(const HashExpr &h) {
    h.dbprint(std::cout);
    std::cout << std::endl;
}
