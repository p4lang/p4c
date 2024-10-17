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

#ifndef BF_P4C_MAU_HASH_FUNCTION_H_
#define BF_P4C_MAU_HASH_FUNCTION_H_

struct bfn_hash_algorithm_;

namespace P4 {
namespace IR {
namespace MAU {

struct HashFunction {
    Util::SourceInfo    srcInfo;
    enum { IDENTITY, CSUM, XOR, CRC, RANDOM }
                        type = IDENTITY;
    bool                msb = false;      // pull msbs for slice
    bool                extend = false;   // If not otherwise specified, extend is true for crc
    bool                reverse = false;  // crc reverse bits
    int                 size = 0;
    uint64_t            poly = 0;         // crc polynomial in koopman form (poly-1)/2
    uint64_t            init = 0;
    uint64_t            final_xor = 0;

    bool operator==(const HashFunction &a) const {
        return type == a.type && size == a.size && msb == a.msb && reverse == a.reverse &&
               poly == a.poly && init == a.init && final_xor == a.final_xor; }
    bool operator!=(const HashFunction &a) const { return !(*this == a); }
    void toJSON(JSONGenerator &json) const;
    static HashFunction *fromJSON(JSONLoader &);
    bool setup(const IR::Expression *exp);
    bool convertPolynomialExtern(const IR::GlobalRef *);
    static HashFunction identity() {
        HashFunction rv;
        rv.type = IDENTITY;
        return rv; }
    static HashFunction random() {
        HashFunction rv;
        rv.type = RANDOM;
        return rv; }
    bool size_from_algorithm() const {
        if (type == CRC)
            return true;
        return false;
    }
    bool ordered() const { return type != RANDOM; }

    std::string name() const {
        std::string algo_name = "";
        if (type == IDENTITY) {
            algo_name = "identity";
        } else if (type == CRC) {
            algo_name = "crc_" + std::to_string(size);
        } else if (type == RANDOM) {
            algo_name = "random";
        } else if (type == XOR) {
            algo_name = "xor";
        }
        return algo_name;
    }

    std::string algo_type() const {
        std::string algo_type = "";
        if (type == IDENTITY) {
            algo_type = "identity";
        } else if (type == CRC) {
            algo_type = "crc";
        } else if (type == RANDOM) {
            algo_type = "random";
        } else if (type == XOR) {
            algo_type = "xor";
        }
        return algo_type;
    }

 private:
    const IR::MethodCallExpression *hash_to_mce(const IR::Expression *);
    uint64_t toKoopman(uint64_t poly, uint32_t width);

 public:
    void build_algorithm_t(bfn_hash_algorithm_ *) const;
    static const IR::Expression *convertHashAlgorithmBFN(Util::SourceInfo srcInfo,
                                                         IR::ID algorithm);
    friend std::ostream &operator<<(std::ostream &, const HashFunction &);

 private:
    static const IR::Expression *convertHashAlgorithmExtern(Util::SourceInfo srcInfo,
                                                            IR::ID algorithm);

    static const IR::Expression *convertHashAlgorithmInner(Util::SourceInfo srcInfo,
                                                           IR::ID algorithm, bool msb, bool extend,
                                                           bool extension_set,
                                                           const std::string &alg_name);
};

inline std::ostream &operator<<(std::ostream &out, const IR::MAU::HashFunction &h) {
    switch (h.type) {
    case IR::MAU::HashFunction::IDENTITY:
        out << "identity";
        break;
    case IR::MAU::HashFunction::CSUM:
        out << "csum" << h.size;
        break;
    case IR::MAU::HashFunction::XOR:
        out << "xor" << h.size;
        break;
    case IR::MAU::HashFunction::CRC:
        out << "crc(0x" << std::hex << h.poly;
        if (h.init)
            out << " init=0x" << std::hex << h.init;
        out << ")";
        if (h.final_xor)
            out << "^" << std::hex << h.final_xor;
        break;
    case IR::MAU::HashFunction::RANDOM:
        out << "random";
        break;
    default:
        out << "invalid(0x" << std::hex << h.type << ")";
        break; }
    return out;
}

}  // end namespace MAU
}  // end namespace IR
}  // end namespace P4

#endif /* BF_P4C_MAU_HASH_FUNCTION_H_ */
