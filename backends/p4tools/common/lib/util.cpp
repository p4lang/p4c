#include "backends/p4tools/common/lib/util.h"

#include <cxxabi.h>

#include <bits/types/struct_tm.h>
#include <lib/null.h>

#include <algorithm>
#include <chrono>  // NOLINT cpplint throws a warning because Google has a similar library...
#include <ctime>
#include <iomanip>
#include <map>
#include <sstream>
#include <tuple>

#include <boost/cstdint.hpp>
#include <boost/multiprecision/cpp_int.hpp>
#include <boost/multiprecision/cpp_int/add.hpp>
#include <boost/multiprecision/detail/et_ops.hpp>
#include <boost/multiprecision/number.hpp>
#include <boost/none.hpp>
#include <boost/random/uniform_int_distribution.hpp>

#include "backends/p4tools/common/lib/zombie.h"
#include "ir/id.h"
#include "ir/irutils.h"
#include "ir/vector.h"
#include "lib/safe_vector.h"
#include "p4tools/common/lib/formulae.h"

namespace P4Tools {

/* =========================================================================================
 *  Seeds, timestamps, randomness.
 * ========================================================================================= */

boost::optional<uint32_t> Utils::currentSeed = boost::none;

boost::random::mt19937 Utils::rng;

std::string Utils::getTimeStamp() {
    // get current time
    auto now = std::chrono::system_clock::now();
    // get number of milliseconds for the current second
    // (remainder after division into seconds)
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
    // convert to std::time_t in order to convert to std::tm (broken time)
    auto timer = std::chrono::system_clock::to_time_t(now);
    // convert to broken time
    std::tm* bt = std::localtime(&timer);
    CHECK_NULL(bt);
    std::stringstream oss;
    oss << std::put_time(bt, "%Y-%m-%d-%H:%M:%S");  // HH:MM:SS
    oss << '.' << std::setfill('0') << std::setw(3) << ms.count();
    return oss.str();
}

void Utils::setRandomSeed(int seed) {
    currentSeed = seed;
    rng.seed(seed);
}

boost::optional<uint32_t> Utils::getCurrentSeed() { return currentSeed; }

uint64_t Utils::getRandInt(uint64_t max) {
    if (!currentSeed) {
        return 0;
    }
    boost::random::uniform_int_distribution<uint64_t> dist(0, max);
    return dist(rng);
}

big_int Utils::getRandBigInt(big_int max) {
    if (!currentSeed) {
        return 0;
    }
    boost::random::uniform_int_distribution<big_int> dist(0, max);
    return dist(rng);
}

const IR::Constant* Utils::getRandConstantForWidth(int bitWidth) {
    auto maxVal = IR::getMaxBvVal(bitWidth);
    auto randInt = Utils::getRandBigInt(maxVal);
    const auto* constType = IR::getBitType(bitWidth);
    return IR::getConstant(constType, randInt);
}

const IR::Constant* Utils::getRandConstantForType(const IR::Type_Bits* type) {
    auto maxVal = IR::getMaxBvVal(type->width_bits());
    auto randInt = Utils::getRandBigInt(maxVal);
    return IR::getConstant(type, randInt);
}

/* =========================================================================================
 *  Variables and symbolic constants.
 * ========================================================================================= */

const cstring Utils::Valid = "*valid";

const StateVariable& Utils::getZombieTableVar(const IR::Type* type, const IR::P4Table* table,
                                              cstring name, boost::optional<int> idx1_opt,
                                              boost::optional<int> idx2_opt) {
    // Mash the table name, the given name, and the optional indices together.
    // XXX To be nice, we should probably build a PathExpression, but that's annoying to do, and we
    // XXX can probably get away with this.
    std::stringstream out;
    out << table->name.toString() << "." << name;
    if (idx1_opt) {
        out << "." << *idx1_opt;
    }
    if (idx2_opt) {
        out << "." << *idx2_opt;
    }

    return Zombie::getVar(type, 0, out.str());
}

const StateVariable& Utils::getZombieVar(const IR::Type* type, int incarnation, cstring name) {
    return Zombie::getVar(type, incarnation, name);
}

const StateVariable& Utils::getZombieConst(const IR::Type* type, int incarnation, cstring name) {
    return Zombie::getConst(type, incarnation, name);
}

StateVariable Utils::getHeaderValidity(const IR::Expression* headerRef) {
    return new IR::Member(IR::Type::Boolean::get(), headerRef, Valid);
}

StateVariable Utils::addZombiePostfix(const IR::Expression* paramPath,
                                      const IR::Type_Base* baseType) {
    return new IR::Member(baseType, paramPath, "*");
}

const IR::TaintExpression* Utils::getTaintExpression(const IR::Type* type) {
    // Do not cache varbits.
    if (type->is<IR::Extracted_Varbits>()) {
        return new IR::TaintExpression(type);
    }
    // Only cache bits with width lower than 16 bit to restrict the size of the cache.
    const auto* tb = type->to<IR::Type_Bits>();
    if (type->width_bits() > 16 || tb == nullptr) {
        return new IR::TaintExpression(type);
    }
    // Taint expressions are interned. Keys in the intern map is the signedness and width of the
    // type.
    using key_t = std::tuple<int, bool>;
    static std::map<key_t, const IR::TaintExpression*> taints;

    auto*& result = taints[{tb->width_bits(), tb->isSigned}];
    if (result == nullptr) {
        result = new IR::TaintExpression(type);
    }

    return result;
}

const StateVariable& Utils::getConcolicMember(const IR::ConcolicVariable* var, int concolicId) {
    const auto* const concolicMember = var->concolicMember;
    auto* clonedMember = concolicMember->clone();
    clonedMember->member = std::to_string(concolicId).c_str();
    return *(new StateVariable(clonedMember));
}

const IR::MethodCallExpression* Utils::generateInternalMethodCall(
    cstring methodName, const std::vector<const IR::Expression*>& argVector,
    const IR::Type* returnType) {
    auto* args = new IR::Vector<IR::Argument>();
    for (const auto* expr : argVector) {
        args->push_back(new IR::Argument(expr));
    }
    return new IR::MethodCallExpression(
        returnType,
        new IR::Member(new IR::Type_Method(new IR::ParameterList(), methodName),
                       new IR::PathExpression(new IR::Type_Extern("*"), new IR::Path("*")),
                       methodName),
        args);
}

}  // namespace P4Tools
