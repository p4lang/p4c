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

#include <exception>
#include "ir/ir.h"
#include "ir/json_loader.h"
#include "lib/hex.h"
#include "lib/big_int_util.h"
#include "bf-p4c/mau/hash_function.h"
#include "bf-utils/include/dynamic_hash/dynamic_hash.h"

using namespace P4::literals;

/**
 * The list of currently supported crc strings, and their corresponding conversion to the
 * dynamic_hash algorithms.  The strings need to be matched exactly from the P4 program
 *
 * FIXME: This whitelist approach is suboptimal.  This list needs to provided directly
 * to the programmer if a simple spelling mistake is found.
 */
static std::map<cstring, bfn_crc_alg_t> standard_crcs_t = {
    { "crc8"_cs, CRC_8 },
    { "crc_8"_cs, CRC_8 },
    { "crc_8_darc"_cs, CRC_8_DARC },
    { "crc_8_i_code"_cs, CRC_8_I_CODE },
    { "crc_8_itu"_cs, CRC_8_ITU },
    { "crc_8_maxim"_cs, CRC_8_MAXIM },
    { "crc_8_rohc"_cs, CRC_8_ROHC },
    { "crc_8_wcdma"_cs, CRC_8_WCDMA },
    { "crc_16"_cs, CRC_16 },
    { "crc16"_cs, CRC_16 },  // Note multiple entries
    { "crc_16_custom"_cs, CRC_16 },
    { "crc_16_bypass"_cs, CRC_16_BYPASS },
    { "crc_16_dds_110"_cs, CRC_16_DDS_110 },
    { "crc_16_dect"_cs, CRC_16_DECT },
    { "crc_16_dect_r"_cs, CRC_16_DECT_R },
    { "crc_16_dect_x"_cs, CRC_16_DECT_X },
    { "crc_16_dnp"_cs, CRC_16_DNP },
    { "crc_16_en_13757"_cs, CRC_16_EN_13757 },
    { "crc_16_genibus"_cs, CRC_16_GENIBUS },
    { "crc_16_maxim"_cs, CRC_16_MAXIM },
    { "crc_16_mcrf4xx"_cs, CRC_16_MCRF4XX },
    { "crc_16_riello"_cs, CRC_16_RIELLO },
    { "crc_16_t10_dif"_cs, CRC_16_T10_DIF },
    { "crc_16_teledisk"_cs, CRC_16_TELEDISK },
    { "crc_16_usb"_cs, CRC_16_USB },
    { "x_25"_cs, X_25 },
    { "xmodem"_cs, XMODEM },
    { "modbus"_cs, MODBUS },
    { "kermit"_cs, KERMIT },
    { "crc_ccitt_false"_cs, CRC_CCITT_FALSE },
    { "crc_aug_ccitt"_cs, CRC_AUG_CCITT },
    { "crc_32"_cs, CRC_32 },
    { "crc32"_cs, CRC_32 },  // Repeated value
    { "crc_32_custom"_cs, CRC_32 },
    { "crc_32_bzip2"_cs, CRC_32_BZIP2 },
    { "crc_32c"_cs, CRC_32C },
    { "crc_32d"_cs, CRC_32D },
    { "crc_32_mpeg"_cs, CRC_32_MPEG },
    { "posix"_cs, POSIX },
    { "crc_32q"_cs, CRC_32Q },
    { "jamcrc"_cs, JAMCRC },
    { "xfer"_cs, XFER },
    { "crc64"_cs, CRC_64 },
    { "crc_64"_cs, CRC_64 },
    { "crc_64"_cs, CRC_64_GO_ISO },
    { "crc_64_we"_cs, CRC_64_WE },
    { "crc_64_jones"_cs, CRC_64_JONES }
};

/**
 * The purpose of this function is to directly convert a p4_14 algorithm string to a crc
 * polynomial if possible.  It uses predefined keywords such as poly_, not_rev,
 * init_, and final_xor_ to determine whether the allocation was valid.
 *
 * Again, string manipulation can be rigid, but the best placed to handle this would
 * be to move towards an extern in p4-16 so that this becomes unnecessary
 *
 * FIXME: In order to handle polynomials of 64 bit hashes, a potential poly_koopman_
 * node is needed.  64 bit hashes have 65 bit polynomials, and thus cannot be parsed
 * by stoll.  Any polynomial over 64 bits is currently unsupported by the bf-utils library
 */
static bool direct_crc_string_conversion(bfn_hash_algorithm_ *hash_alg,
        cstring alg_name_c, Util::SourceInfo srcInfo) {
    std::string alg_name = alg_name_c + "";
    size_t pos = 0;
    if ((pos = alg_name.find("poly_")) != std::string::npos) {
        try {
            hash_alg->poly = stoll(alg_name.substr(pos + 5), nullptr, 0);
        } catch (std::exception &e) {
            error("%s: String after poly_ cannot fit within a long long", srcInfo);
            return false;
        }
    } else {
        return false;
    }
    hash_alg->hash_bit_width = ceil_log2(hash_alg->poly) - 1;

    if ((pos = alg_name.find("not_rev")) != std::string::npos) {
        hash_alg->reverse = false;
    } else {
        hash_alg->reverse = true;
    }

    if ((pos = alg_name.find("init_")) != std::string::npos) {
        try {
            hash_alg->init = stoll(alg_name.substr(pos + 5), nullptr, 0);
        } catch (std::exception &e) {
            error("%s: String after init_ cannot fit within a long long", srcInfo);
            return false;
        }
    } else {
        hash_alg->init = 0ULL;
    }

    if ((pos = alg_name.find("final_xor_")) != std::string::npos) {
        try {
            hash_alg->final_xor = stoll(alg_name.substr(pos + 10), nullptr, 0);
        } catch (std::exception &e) {
            error("%s: String after final_xor_ cannot fit within a long long", srcInfo);
            return false;
        }
    } else if ((pos = alg_name.find("xout_")) != std::string::npos) {
        try {
            hash_alg->final_xor = stoll(alg_name.substr(pos + 5), nullptr, 0);
        } catch (std::exception &e) {
            error("%s: String after xout_ cannot fit within a long long", srcInfo);
            return false;
        }
    } else {
        hash_alg->final_xor = 0ULL;
    }
    return true;
}
/**
 * The purpose of this function is to convert a known ID into an expression that can
 * be intepreted as a polynomial.  The functions for identity, random, and crc have
 * specific method calls in order to pass all of their corresponding arguments to the
 * backend.
 *
 * The compiler can take a pre recognized string from the static map, or even convert
 * a crc polynomial directly, through the 3rd party library.
 *
 * FIXME: The conversion for this is currently fairly rigid, as the names have to
 * match directly with the static list above
 *
 * Order of arguments:
 *     msb
 *     extend
 *     ( The remaining are only for crc )
 *     poly
 *     init
 *     final_xor
 *     reverse
 *     name
 */
const IR::Expression *IR::MAU::HashFunction::convertHashAlgorithmBFN(Util::SourceInfo srcInfo,
                                                                     IR::ID algorithm) {
    bool msb = false;
    bool endianness_set = false;
    bool extend = false;
    bool extension_set = false;

    std::string alg_name = algorithm.name + "";
    std::transform(alg_name.begin(), alg_name.end(), alg_name.begin(), ::tolower);

    // Setting the msb or extend flags, and removing them from the name
    while (true) {
        size_t pos = 0;
        if ((pos = alg_name.find("_lsb")) != std::string::npos) {
            ERROR_CHECK(!endianness_set, "%s: Endianness set multiple times in algorithm %s",
                    srcInfo, algorithm.name);
            msb = false;
            endianness_set = true;
            alg_name = alg_name.substr(0, pos) + alg_name.substr(pos + 4);
        } else if ((pos = alg_name.find("_msb")) != std::string::npos) {
            ERROR_CHECK(!endianness_set, "%s: Endianness set multiple times in algorithm %s",
                    srcInfo, algorithm.name);
            msb = true;
            endianness_set = true;
            alg_name = alg_name.substr(0, pos) + alg_name.substr(pos + 4);
        } else if ((pos = alg_name.find("_extend")) != std::string::npos) {
            ERROR_CHECK(!extension_set, "%s: Extension set multiple times in algorithm %s",
                    srcInfo, algorithm.name);
            extend = true;
            extension_set = true;
            alg_name = alg_name.substr(0, pos) + alg_name.substr(pos + 7);
        } else {
            break;
        }
    }

    if (alg_name == "xor16" || alg_name == "csum16" || alg_name == "csum16_udp") {
        if (alg_name == "csum16_udp")
            return new IR::Member(new IR::TypeNameExpression("HashAlgorithm"), IR::ID("csum16"));
        return new IR::Member(new IR::TypeNameExpression("HashAlgorithm"), algorithm);
    }

    return convertHashAlgorithmInner(srcInfo, algorithm, msb, extend, extension_set, alg_name);
}

const IR::Expression *IR::MAU::HashFunction::convertHashAlgorithmExtern(Util::SourceInfo srcInfo,
                                                                        IR::ID algorithm) {
    std::string alg_name = algorithm.name + "";
    std::transform(alg_name.begin(), alg_name.end(), alg_name.begin(), ::tolower);
    return convertHashAlgorithmInner(srcInfo, algorithm, false, false, false, alg_name);
}

const IR::Expression *IR::MAU::HashFunction::convertHashAlgorithmInner(
    Util::SourceInfo srcInfo, IR::ID algorithm, bool msb, bool extend, bool extension_set,
    const std::string &alg_name) {
    cstring mc_name = "unknown"_cs;
    enum {
        CRC,
        XOR,
        OTHER,
    } detected_algorithm(OTHER);
    bfn_hash_algorithm_t hash_alg;
    hash_alg.hash_bit_width = 0;
    struct {
        uint32_t width;
    } xor_alg;

    // Determines the crc functions through the 3rd party library
    if (standard_crcs_t.find(alg_name) != standard_crcs_t.end()) {
        initialize_algorithm(&hash_alg, CRC_DYN, msb, extend, standard_crcs_t.at(alg_name));
        detected_algorithm = CRC;
    } else if (alg_name == "xor8") {
        detected_algorithm = XOR;
        xor_alg.width = 8;
    } else if (alg_name == "xor16") {
        detected_algorithm = XOR;
        xor_alg.width = 16;
    } else if (alg_name == "xor32") {
        detected_algorithm = XOR;
        xor_alg.width = 32;
    } else if (alg_name == "identity" || alg_name == "random") {
        detected_algorithm = OTHER;
        mc_name = alg_name + "_hash";
    } else if (direct_crc_string_conversion(&hash_alg, alg_name, srcInfo)) {
        char *error_message;
        if (!verify_algorithm(&hash_alg, &error_message)) {
            error("%s: Crc algorithm %s incorrect for the following reason : %s",
                   srcInfo, algorithm.name, error_message);
            return nullptr;
        } else {
            detected_algorithm = CRC;
        }
    } else {
        error("%s: Unrecognized algorithm for a hash expression: %s", srcInfo, algorithm.name);
        return nullptr;
    }

    auto args = new IR::Vector<IR::Argument>({
        new IR::Argument(new IR::BoolLiteral(msb)),
    });

    switch (detected_algorithm) {
        case CRC: {
            // Extend is true by default for CRC.
            if (!extension_set) {
                extend = true;
            }
            big_int poly = hash_alg.poly, init = hash_alg.init, final_xor = hash_alg.final_xor;
            auto typeT = IR::Type::Bits::get(hash_alg.hash_bit_width);
            args->push_back(new IR::Argument(new IR::BoolLiteral(extend)));
            args->push_back(new IR::Argument(new IR::Constant(typeT, poly)));
            args->push_back(new IR::Argument(new IR::Constant(typeT, init)));
            args->push_back(new IR::Argument(new IR::Constant(typeT, final_xor)));
            args->push_back(new IR::Argument(new IR::BoolLiteral(hash_alg.reverse)));
            mc_name = "crc_poly"_cs;
        } break;
        case XOR: {
            auto typeT(IR::Type::Bits::get(8));
            args->push_back(new IR::Argument(new IR::BoolLiteral(extend)));
            args->push_back(
                new IR::Argument(new IR::Constant(typeT, static_cast<big_int>(xor_alg.width))));
            mc_name = "xor_hash"_cs;
        } break;
        case OTHER:
            args->push_back(new IR::Argument(new IR::BoolLiteral(extend)));
            break;
    }

    return new IR::MethodCallExpression(srcInfo, new IR::PathExpression(mc_name), args);
}

/**
 * This function will convert an Expression to a MethodCallExpression, in the same way
 * P4_14 converter.  This is necessary for any possible p4_16 hash expressions.  With this
 * conversion, the hash function setup can be identical, and just need to handle one type of
 * expression.
 */
const IR::MethodCallExpression *IR::MAU::HashFunction::hash_to_mce(const IR::Expression *e) {
    auto srcInfo = e->srcInfo;
    const IR::Expression *conv_e = nullptr;

    cstring error_alg_name;
    if (auto s = e->to<IR::StringLiteral>()) {
        conv_e = convertHashAlgorithmBFN(srcInfo, IR::ID(srcInfo, s->value));
        error_alg_name = s->value;
    } else if (auto m = e->to<IR::Member>()) {
        conv_e = convertHashAlgorithmExtern(srcInfo, m->member);
        error_alg_name = m->member.name;
    } else if (auto k = e->to<IR::Constant>()) {
        // Hash contructor calls will have the enum converted to an int const by
        // ConvertEnums in the midend.  Would be better if they didn't.
        // DANGER -- these values need to match up with the order of the hash tags
        // in the arch.p4 header file.
        switch (k->asInt()) {
        case 0:
            conv_e = convertHashAlgorithmBFN(srcInfo, IR::ID("identity"));
            error_alg_name = "identity"_cs;
            break;
        case 1:
            conv_e = convertHashAlgorithmBFN(srcInfo, IR::ID("random"));
            error_alg_name = "random"_cs;
            break;
        case 2:
            conv_e = convertHashAlgorithmBFN(srcInfo, IR::ID("xor8"));
            error_alg_name = "xor8"_cs;
            break;
        case 3:
            conv_e = convertHashAlgorithmBFN(srcInfo, IR::ID("xor16"));
            error_alg_name = "xor16"_cs;
            break;
        case 4:
            conv_e = convertHashAlgorithmBFN(srcInfo, IR::ID("xor32"));
            error_alg_name = "xor32"_cs;
            break;
        case 5:
            conv_e = convertHashAlgorithmBFN(srcInfo, IR::ID("crc_8"));
            error_alg_name = "crc_8"_cs;
            break;
        case 6:
            conv_e = convertHashAlgorithmBFN(srcInfo, IR::ID("crc_16"));
            error_alg_name = "crc_16"_cs;
            break;
        case 7:
            conv_e = convertHashAlgorithmBFN(srcInfo, IR::ID("crc_32"));
            error_alg_name = "crc_32"_cs;
            break;
        case 8:
            conv_e = convertHashAlgorithmBFN(srcInfo, IR::ID("crc_64"));
            error_alg_name = "crc_64"_cs;
            break;
        case 9:
            conv_e = convertHashAlgorithmBFN(srcInfo, IR::ID("csum16"));
            error_alg_name = "csum16"_cs;
            break;
        default:
            conv_e = nullptr;
            error_alg_name = "unknown"_cs;
        }
    // According to the current setup, if something was converted to a MethodCallExpression
    // or TypedPrimitive from the conversion, then it is an identity/random/crc function,
    // which can be saved on the matrix.
    // If other hash functions are converted to MethodCallExpressions, then this will have
    // to change
    } else if (auto mc = e->to<IR::MethodCallExpression>()) {
        conv_e = mc;
    } else if (auto tp = e->to<IR::MAU::TypedPrimitive>()) {
        auto ops = new IR::Vector<IR::Argument>();
        for (auto op : tp->operands) {
            ops->push_back(new IR::Argument(op));
        }
        conv_e = new IR::MethodCallExpression(srcInfo, new IR::PathExpression(tp->name), ops);
    }

    const IR::MethodCallExpression *mce = nullptr;
    if (conv_e == nullptr || (mce = conv_e->to<IR::MethodCallExpression>()) == nullptr) {
        error("%s: Cannot properly set up the hash function on the hash matrix : %s",
                e->srcInfo, error_alg_name);
        return nullptr;
    }

    return mce;
}

/**
 * Responsible for converting a frontend Expression to a IR::MAU::HashFunction structure.
 * This conversion using the global hash function conversion from the bf-utils function.
 *
 * The conversion is only done for hash functions set up through the galois matrix, and
 * therefore verifies that the hash function can be allocated on the galois matrix.
 * Currently unhandled is the possibility or XORing multiple functions together, which
 * would be a recursive hash function.
 */
bool IR::MAU::HashFunction::setup(const Expression *e) {
    if (!e) return false;

    /* -- set empty state (replacement of original memset) */
    *this = HashFunction{};

    srcInfo = e->srcInfo;
    size = 0;
    const IR::MethodCallExpression *mce = hash_to_mce(e);
    if (mce == nullptr) return false;

    cstring alg_name;
    if (auto meth = mce->method->to<IR::PathExpression>()) {
        alg_name = meth->path->name.name;
        // name = mce->getAnnotations()->getSingle("name"_cs);
    } else {
        BUG("%s: Cannot properly set up the hash function", mce->srcInfo);
    }

    // Supported Algorithms on the galois matrix
    const IR::Vector<IR::Argument> *args = mce->arguments;
    if (alg_name == "identity_hash")
        type = IDENTITY;
    else if (alg_name == "random_hash")
        type = RANDOM;
    else if (alg_name == "crc_poly")
        type = CRC;
    else if (alg_name == "xor_hash") {
        type = XOR;
    } else
        return false;

    BUG_CHECK(((type == IDENTITY || type == RANDOM) && args->size() == 2) ||
                  (type == XOR && args->size() == 3) || (type == CRC && args->size() == 6),
              "Hash function method call misconfigured");

    // This for loop is from the predefined crc_poly set up by the convertHashAlgorithmBFN
    for (size_t i = 0; i < args->size(); i++) {
        switch (i) {
        case 5:
            if (auto k = args->at(5)->expression->to<IR::BoolLiteral>())
                reverse = k->value;
            else
                BUG("Hash function method call misconfigured");
            break;
        case 4:
            if (auto k = args->at(4)->expression->to<IR::Constant>())
                final_xor = static_cast<uint64_t>(k->value);
            else
                BUG("Hash function method call misconfigured");
            break;
        case 3:
            if (auto k = args->at(3)->expression->to<IR::Constant>())
                init = static_cast<uint64_t>(k->value);
            else
                BUG("Hash function method call misconfigured");
            break;
        case 2: {
            switch (type) {
                case CRC:
                    if (auto k = args->at(2)->expression->to<IR::Constant>()) {
                        size = k->type->width_bits();
                        poly = toKoopman(static_cast<uint64_t>(k->value), size);
                    } else {
                        return false;
                    }
                    break;
                case XOR:
                    if (auto k = args->at(2)->expression->to<IR::Constant>()) {
                        size = static_cast<int>(k->value);
                    } else {
                        return false;
                    }
                    break;
                default:
                    /* -- shouldn't happen, already checked by the assertion above */
                    break;
            }
        } break;
        case 1:
            if (auto k = args->at(1)->expression->to<IR::BoolLiteral>())
                extend = k->value;
            else
                return false;
            break;
        case 0:
            if (auto k = args->at(0)->expression->to<IR::BoolLiteral>())
                msb = k->value;
            else
                return false;
            break;
            // fall through
        }
    }
    return true;
}

/* convert polynomial in tna to koopman representation used by backend */
uint64_t IR::MAU::HashFunction::toKoopman(uint64_t poly, uint32_t width) {
    big_int koopman = poly;
    big_int upper_bit = 1;
    big_int mask = (upper_bit << width) - 1;
    koopman |= upper_bit << width;
    koopman >>= 1;
    koopman &= mask;
    LOG3("upper_bit : " << upper_bit << " mask " << mask << " koopman: " << koopman);
    return static_cast<uint64_t>(koopman);
}

/*
 * Convert CRCPolynomial extern to a IR::MAU::HashFunction structure.
 *
 * Tna defines polynomial as 1w1 ++ PV with a leading 1 omitted.
 *
 * Compiler stores polynomial in koopman form, which is PV ++ 1w1 with
 * the trailing 1 omitted.
 */
bool IR::MAU::HashFunction::convertPolynomialExtern(const IR::GlobalRef *ref) {
    auto decl = ref->obj->to<IR::Declaration_Instance>();
    auto it = decl->arguments->begin();
    size = (*it)->expression->type->width_bits();
    poly = (*it)->expression->to<IR::Constant>()->asUint64();
    LOG3("poly " << std::hex << poly << " size: " << size);
    // convert to koopman form.
    poly = toKoopman(poly, size);
    LOG3("poly " << poly);
    std::advance(it, 1);
    reverse = (*it)->expression->to<IR::BoolLiteral>()->value;
    std::advance(it, 1);
    msb = (*it)->expression->to<IR::BoolLiteral>()->value;
    std::advance(it, 1);
    extend = (*it)->expression->to<IR::BoolLiteral>()->value;
    std::advance(it, 1);
    init = (*it)->expression->to<IR::Constant>()->asUint64();
    std::advance(it, 1);
    final_xor = (*it)->expression->to<IR::Constant>()->asUint64();
    type = IR::MAU::HashFunction::CRC;
    return true;
}

void IR::MAU::HashFunction::toJSON(JSONGenerator &json) const {
    json << json.indent << "\"type\": " << static_cast<int>(type) << ",\n"
         << json.indent << "\"size\": " << size << ",\n"
         << json.indent << "\"msb\": " << msb << ",\n"
         << json.indent << "\"reverse\": " << reverse << ",\n"
         << json.indent << "\"poly\": " << poly << ",\n"
         << json.indent << "\"init\": " << init << ",\n"
         << json.indent << "\"xor\": " << final_xor << ",\n"
         << json.indent << "\"extend\": " << extend;
}

void IR::MAU::HashFunction::build_algorithm_t(bfn_hash_algorithm_ *alg) const {
    switch (type) {
        case RANDOM:
            alg->hash_alg = RANDOM_DYN; break;
        case IDENTITY:
            alg->hash_alg = IDENTITY_DYN; break;
        case CRC:
            alg->hash_alg = CRC_DYN; break;
        case XOR:
            alg->hash_alg = XOR_DYN;
            break;
        default:
            alg->hash_alg = IDENTITY_DYN;
    }

    alg->hash_bit_width = size;
    alg->msb = msb;
    alg->reverse = reverse;
    alg->poly = (poly << 1) | 1;
    alg->init = init;
    alg->final_xor = final_xor;
    alg->extend = extend;
}

IR::MAU::HashFunction *IR::MAU::HashFunction::fromJSON(JSONLoader &json) {
    if (!json.json) return nullptr;
    auto *rv = new HashFunction;
    int type = 0;
    json.load("type", type);
    rv->type = static_cast<decltype(rv->type)>(type);
    json.load("size", rv->size);
    json.load("msb", rv->msb);
    json.load("reverse", rv->reverse);
    json.load("poly", rv->poly);
    json.load("init", rv->init);
    json.load("xor", rv->final_xor);
    json.load("extend", rv->extend);
    return rv;
}
