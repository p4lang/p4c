#include "backends/p4tools/modules/smith/common/scope.h"

#include <cstddef>
#include <cstdio>
#include <iterator>
#include <map>
#include <optional>
#include <set>
#include <sstream>
#include <utility>
#include <vector>

#include "backends/p4tools/common/lib/logging.h"
#include "backends/p4tools/common/lib/util.h"
#include "ir/node.h"
#include "ir/vector.h"
#include "lib/cstring.h"
#include "lib/exceptions.h"

namespace P4Tools::P4Smith {

std::vector<IR::Vector<IR::Node> *> P4Scope::scope;
std::set<cstring> P4Scope::usedNames;
std::map<cstring, std::map<int, std::set<cstring>>> P4Scope::lvalMap;
std::map<cstring, std::map<int, std::set<cstring>>> P4Scope::lvalMapRw;
std::set<const IR::P4Table *> P4Scope::callableTables;

// TODO(fruffy): This should be set by the back end
std::set<cstring> P4Scope::notInitializedStructs;
Properties P4Scope::prop;
Requirements P4Scope::req;
Constraints P4Scope::constraints;
void P4Scope::addToScope(const IR::Node *node) {
    auto *lScope = P4Scope::scope.back();
    lScope->push_back(node);

    if (const auto *dv = node->to<IR::Declaration_Variable>()) {
        addLval(dv->type, dv->name.name);
    } else if (const auto *dc = node->to<IR::Declaration_Constant>()) {
        addLval(dc->type, dc->name.name, true);
    }
}

void P4Scope::startLocalScope() {
    auto *localScope = new IR::Vector<IR::Node>();
    scope.push_back(localScope);
}

void P4Scope::endLocalScope() {
    IR::Vector<IR::Node> *localScope = scope.back();

    for (const auto *node : *localScope) {
        if (const auto *decl = node->to<IR::Declaration_Variable>()) {
            deleteLval(decl->type, decl->name.name);
        } else if (const auto *dc = node->to<IR::Declaration_Constant>()) {
            deleteLval(dc->type, dc->name.name);
        } else if (const auto *param = node->to<IR::Parameter>()) {
            deleteLval(param->type, param->name.name);
        } else if (const auto *tbl = node->to<IR::P4Table>()) {
            callableTables.erase(tbl);
        }
    }

    delete localScope;
    scope.pop_back();
}

void addCompoundLvals(const IR::Type_StructLike *sl_type, cstring sl_name, bool read_only) {
    for (const auto *field : sl_type->fields) {
        std::stringstream ss;
        ss.str("");
        ss << sl_name << "." << field->name.name;
        cstring fieldName(ss.str());
        P4Scope::addLval(field->type, fieldName, read_only);
    }
}

void deleteCompoundLvals(const IR::Type_StructLike *sl_type, cstring sl_name) {
    for (const auto *field : sl_type->fields) {
        std::stringstream ss;
        ss.str("");
        ss << sl_name << "." << field->name.name;
        cstring fieldName(ss.str());
        P4Scope::deleteLval(field->type, fieldName);
    }
}

void P4Scope::deleteLval(const IR::Type *tp, cstring name) {
    cstring typeKey;
    int bitBucket = 0;

    if (const auto *tb = tp->to<IR::Type_Bits>()) {
        typeKey = IR::Type_Bits::static_type_name();
        bitBucket = tb->width_bits();
    } else if (const auto *tb = tp->to<IR::Type_Boolean>()) {
        typeKey = IR::Type_Boolean::static_type_name();
        bitBucket = tb->width_bits();
    } else if (tp->is<IR::Type_InfInt>()) {
        typeKey = IR::Type_InfInt::static_type_name();
        bitBucket = 1;
    } else if (const auto *ts = tp->to<IR::Type_Stack>()) {
        size_t stackSize = ts->getSize();
        typeKey = IR::Type_Stack::static_type_name();
        bitBucket = 1;
        if (const auto *tnType = ts->elementType->to<IR::Type_Name>()) {
            std::stringstream ss;
            ss.str("");
            ss << name << "[" << (stackSize - 1) << "]";
            cstring stackName(ss.str());
            deleteLval(tnType, stackName);
        } else {
            BUG("Type_Name %s not yet supported", ts->elementType->node_type_name());
        }
    } else if (const auto *tn = tp->to<IR::Type_Name>()) {
        auto tnName = tn->path->name.name;
        if (tnName == "packet_in") {
            return;
        }
        if (const auto *td = getTypeByName(tnName)) {
            if (const auto *tnType = td->to<IR::Type_StructLike>()) {
                typeKey = tnName;
                // width_bits should work here, do !know why not...
                // bit_bucket = P4Scope::compound_type[tn_name]->width_bits();
                bitBucket = 1;
                deleteCompoundLvals(tnType, name);
            } else {
                BUG("Type_Name %s !found!", td->node_type_name());
            }
        } else {
            printInfo("Type %s does not exist!\n", tnName.c_str());
            return;
        }
    } else {
        BUG("Type %s not yet supported", tp->node_type_name());
    }
    lvalMap[typeKey][bitBucket].erase(name);

    // delete values in the normal map
    if (lvalMap[typeKey][bitBucket].empty()) {
        lvalMap[typeKey].erase(bitBucket);
    }

    // delete values in the rw map
    if (lvalMapRw.count(typeKey) != 0) {
        if (lvalMapRw[typeKey].count(bitBucket) != 0) {
            lvalMapRw[typeKey][bitBucket].erase(name);
            if (lvalMapRw[typeKey][bitBucket].empty()) {
                lvalMapRw[typeKey].erase(bitBucket);
            }
        }
    }
    // delete empty type entries
    if (lvalMap[typeKey].empty()) {
        lvalMap.erase(typeKey);
    }
    if (lvalMapRw[typeKey].empty()) {
        lvalMapRw.erase(typeKey);
    }
}

void P4Scope::addLval(const IR::Type *tp, cstring name, bool read_only) {
    cstring typeKey;
    int bitBucket = 0;

    if (const auto *tb = tp->to<IR::Type_Bits>()) {
        typeKey = IR::Type_Bits::static_type_name();
        bitBucket = tb->width_bits();
    } else if (const auto *tb = tp->to<IR::Type_Boolean>()) {
        typeKey = IR::Type_Boolean::static_type_name();
        bitBucket = tb->width_bits();
    } else if (tp->is<IR::Type_InfInt>()) {
        typeKey = IR::Type_InfInt::static_type_name();
        bitBucket = 1;
    } else if (const auto *ts = tp->to<IR::Type_Stack>()) {
        size_t stackSize = ts->getSize();
        typeKey = IR::Type_Stack::static_type_name();
        bitBucket = 1;
        if (const auto *tnType = ts->elementType->to<IR::Type_Name>()) {
            std::stringstream ss;
            ss.str("");
            ss << name << "[" << (stackSize - 1) << "]";
            cstring stackName(ss.str());
            addLval(tnType, stackName, read_only);
        } else {
            BUG("Type_Name %s not yet supported", ts->elementType->node_type_name());
        }
    } else if (const auto *tn = tp->to<IR::Type_Name>()) {
        auto tnName = tn->path->name.name;
        // FIXME: this could be better
        if (tnName == "packet_in") {
            return;
        }
        if (const auto *td = getTypeByName(tnName)) {
            if (const auto *tnType = td->to<IR::Type_StructLike>()) {
                // width_bits should work here, do !know why not...
                typeKey = tnName;
                // does !work for some reason...
                // bit_bucket = P4Scope::compound_type[tn_name]->width_bits();
                bitBucket = 1;
                addCompoundLvals(tnType, name, read_only);
            } else {
                BUG("Type_Name %s not yet supported", td->node_type_name());
            }
        } else {
            BUG("Type %s does not exist", tnName);
        }
    } else {
        BUG("Type %s not yet supported", tp->node_type_name());
    }
    if (!read_only) {
        lvalMapRw[typeKey][bitBucket].insert(name);
    }
    lvalMap[typeKey][bitBucket].insert(name);
}

std::set<cstring> P4Scope::getCandidateLvals(const IR::Type *tp, bool must_write) {
    cstring typeKey;
    int bitBucket = 0;

    if (const auto *tb = tp->to<IR::Type_Bits>()) {
        typeKey = IR::Type_Bits::static_type_name();
        bitBucket = tb->width_bits();
    } else if (const auto *tb = tp->to<IR::Type_Boolean>()) {
        typeKey = IR::Type_Boolean::static_type_name();
        bitBucket = tb->width_bits();
    } else if (tp->is<IR::Type_InfInt>()) {
        typeKey = IR::Type_InfInt::static_type_name();
        bitBucket = 1;
    } else if (const auto *ts = tp->to<IR::Type_StructLike>()) {
        typeKey = ts->name.name;
        bitBucket = 1;
    } else if (const auto *tn = tp->to<IR::Type_Name>()) {
        auto tnName = tn->path->name.name;
        if (getTypeByName(tnName) != nullptr) {
            typeKey = tnName;
            // bit_bucket = td->width_bits();
            bitBucket = 1;
        } else {
            BUG("Type_name refers to unknown type %s", tnName);
        }
    } else {
        BUG("Type %s not yet supported", tp->node_type_name());
    }

    std::map<cstring, std::map<int, std::set<cstring>>> lookupMap;

    if (must_write) {
        lookupMap = lvalMapRw;
    } else {
        lookupMap = lvalMap;
    }

    if (lookupMap.count(typeKey) == 0) {
        return {};
    }
    auto keyTypes = lookupMap[typeKey];
    if (keyTypes.count(bitBucket) == 0) {
        return {};
    }

    return keyTypes[bitBucket];
}

std::optional<std::map<int, std::set<cstring>>> P4Scope::getWriteableLvalForTypeKey(
    cstring typeKey) {
    if (lvalMapRw.find(typeKey) == lvalMapRw.end()) {
        return std::nullopt;
    }
    return lvalMapRw.at(typeKey);
}
bool P4Scope::hasWriteableLval(cstring typeKey) {
    return lvalMapRw.find(typeKey) != lvalMapRw.end();
}

bool P4Scope::checkLval(const IR::Type *tp, bool must_write) {
    std::set<cstring> candidates = getCandidateLvals(tp, must_write);
    return !candidates.empty();
}

cstring P4Scope::pickLval(const IR::Type *tp, bool must_write) {
    std::set<cstring> candidates = getCandidateLvals(tp, must_write);

    if (candidates.empty()) {
        BUG("Invalid Type Query, %s not found", tp->toString());
    }
    size_t idx = Utils::getRandInt(0, candidates.size() - 1);
    auto lval = std::begin(candidates);
    // "advance" the iterator idx times
    std::advance(lval, idx);

    return *lval;
}

const IR::Type_Bits *P4Scope::pickDeclaredBitType(bool must_write) {
    std::map<cstring, std::map<int, std::set<cstring>>> lookupMap;

    if (must_write) {
        lookupMap = lvalMapRw;
    } else {
        lookupMap = lvalMap;
    }

    cstring bitKey = IR::Type_Bits::static_type_name();
    if (lookupMap.count(bitKey) == 0) {
        return nullptr;
    }
    auto keyTypes = lookupMap[bitKey];
    size_t idx = Utils::getRandInt(0, keyTypes.size() - 1);
    int bitWidth = next(keyTypes.begin(), idx)->first;
    return IR::Type_Bits::get(bitWidth, false);
}

std::vector<const IR::Type_Declaration *> P4Scope::getFilteredDecls(std::set<cstring> filter) {
    std::vector<const IR::Type_Declaration *> ret;
    for (auto *subScope : scope) {
        for (const auto *node : *subScope) {
            cstring name = node->node_type_name();
            if (filter.find(name) == filter.end()) {
                if (const auto *decl = node->to<IR::Type_Declaration>()) {
                    ret.push_back(decl);
                }
            }
        }
    }
    return ret;
}

std::set<const IR::P4Table *> *P4Scope::getCallableTables() { return &callableTables; }

const IR::Type_Declaration *P4Scope::getTypeByName(cstring name) {
    for (auto *subScope : scope) {
        for (const auto *node : *subScope) {
            if (const auto *decl = node->to<IR::Type_Declaration>()) {
                if (decl->name.name == name) {
                    return decl;
                }
            }
        }
    }
    return nullptr;
}
}  // namespace P4Tools::P4Smith
