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

#include "bf-p4c/parde/deparser_checksum_update.h"

#include <map>

#include "bf-p4c/bf-p4c-options.h"
#include "bf-p4c/common/utils.h"
#include "bf-p4c/device.h"
#include "bf-p4c/parde/create_pov_encoder.h"
#include "bf-p4c/parde/parser_info.h"
#include "ir/ir.h"
#include "ir/pattern.h"
#include "lib/error.h"
#include "logging/filelog.h"

using namespace P4;

namespace {

/**
 * \ingroup ExtractChecksum
 * Represents update condition
 */
struct UpdateConditionInfo {
    UpdateConditionInfo(const IR::Member *field, bool conditionNegated)
        : field(field), conditionNegated(conditionNegated) {}
    // single-bit update condition field
    const IR::Member *field = nullptr;

    // Indicated if condition is negated
    //     true  -> cond == 0
    //     false -> cond == 1
    bool conditionNegated = false;
};

/**
 * \ingroup ExtractChecksum
 * Represents a checksum field list
 */
struct FieldListInfo {
    explicit FieldListInfo(IR::Vector<IR::BFN::ChecksumEntry> *fields) : fields(fields) {}
    // List of fields that participate in the update calculation
    IR::Vector<IR::BFN::ChecksumEntry> *fields;

    // Checksum engines cannot compute a field that appears more than once in calculations
    // Hence, a metadata with the same value would be inserted in the duplicate field's
    // place.
    ordered_map<const IR::Member *, const IR::TempVar *> duplicatedFields;

    // Information regarding update conditions
    ordered_set<UpdateConditionInfo *> updateConditions;

    // To indicate zeros_as_ones feature is activated
    bool zerosAsOnes = false;

    // The compiler synthesized pov bit for this field list
    const IR::TempVar *deparseUpdated = nullptr;
    void add_field(const IR::Expression *field, const IR::Member *povBit, int offset) {
        auto checksumEntry = new IR::BFN::ChecksumEntry(new IR::BFN::FieldLVal(field),
                                                        new IR::BFN::FieldLVal(povBit), offset);
        fields->push_back(checksumEntry);
        return;
    }
};

/**
 * \ingroup ExtractChecksum
 * Represents checksum update info for a checksum field
 */
struct ChecksumUpdateInfo {
    explicit ChecksumUpdateInfo(cstring dest) : dest(dest) {}

    // This is the checksum field to be updated.
    cstring dest;

    // The header validity bit
    const IR::BFN::FieldLVal *povBit = nullptr;

    // The compiler synthesized pov bit to deparse the original header checksum.
    // This is true if none of the field lists' deparseUpdated bits are true.
    const IR::TempVar *deparseOriginal = nullptr;

    // Update is unconditional
    bool isUnconditional = false;

    // Contains all field lists
    ordered_set<FieldListInfo *> fieldListInfos;

    void markAsUnconditional(const FieldListInfo *listInfo) {
        isUnconditional = true;
        ordered_set<FieldListInfo *> redundantListInfo;
        for (auto info : fieldListInfos) {
            if (listInfo != info) {
                redundantListInfo.insert(info);
            }
        }
        for (auto redundant : redundantListInfo) fieldListInfos.erase(redundant);
    }
};

using ChecksumUpdateInfoMap = ordered_map<cstring, ChecksumUpdateInfo *>;

using ParserStateChecksumMap =
    assoc::map<const IR::ParserState *, std::vector<const IR::MethodCallExpression *>>;

void checkDestHeader(const IR::Member *destField) {
    unsigned bitCount = 0;
    const IR::HeaderOrMetadata *headerRef = nullptr;
    if (auto curHeader = destField->expr->to<IR::ConcreteHeaderRef>()) {
        headerRef = curHeader->baseRef();
        for (auto f : headerRef->type->fields) {
            if (f->name == destField->member) {
                break;
            } else {
                bitCount += f->type->width_bits();
            }
        }
        if (bitCount % 8) {
            ::fatal_error(
                "Checksum destination field %1% is not byte-aligned in the header. "
                "Checksum engine is unable to update a field if it is not byte-aligned",
                destField);
        }
    }
    return;
}

static bool checksumUpdateSanityCheck(const IR::AssignmentStatement *assignment) {
    auto destField = assignment->left->to<IR::Member>();
    auto methodCall = assignment->right->to<IR::MethodCallExpression>();
    if (!destField || !methodCall || !methodCall->method) return false;
    auto member = methodCall->method->to<IR::Member>();
    if (!member || member->member != "update") return false;
    if (!destField || !destField->expr->is<IR::HeaderRef>()) {
        error("Destination of checksum calculation must be a header field: %1%", destField);
        return false;
    }

    if (destField->type->width_bits() != 16) {
        error("Output of checksum calculation can only be stored in a 16-bit field: %1%",
              destField);
        return false;
    }
    checkDestHeader(destField);
    LOG4("checksum update field: " << destField);
    if (methodCall->arguments->size() == 0) {
        error("Insufficient argumentst to method %1%", methodCall);
        return false;
    }

    const IR::Vector<IR::Expression> *components =
        getListExprComponents(*(*methodCall->arguments)[0]->expression);

    if (!components || components->empty()) {
        error("Invalid list of fields for checksum calculation: %1%", methodCall);
        return false;
    }

    for (auto *source : *components) {
        if (auto *member = source->to<IR::Member>()) {
            if (!member->expr->is<IR::HeaderRef>()) {
                error("Invalid field in the checksum calculation: %1%", source);
                return false;
            }
        } else if (source->is<IR::Constant>()) {
            // TODO can constant be any bit width? signed?
        }
    }

    if (components->empty()) {
        error("Expected at least one field in the checksum calculation: %1%", methodCall);
        return false;
    }

    return true;
}

bool checkIncorrectCsumFields(const IR::HeaderOrMetadata *last,
                              const IR::HeaderOrMetadata *current) {
    // if current field is from a different header, fields till current field should add up
    // to multiple of 8 bits
    if (last != current && last && last->is<IR::Header>()) {
        return true;
        // Only metadata are allowed after a constant which is not multiple of 8
    } else if (!last && current->is<IR::Header>()) {
        return true;
    } else {
        return false;
    }
}

using ChecksumHeaderToFieldInfo =
    ordered_map<cstring, std::pair<const IR::HeaderRef *, std::map<cstring, int>>>;

FieldListInfo *reorderFields(const ChecksumHeaderToFieldInfo &headerToFields,
                             const IR::Member *destField) {
    auto listInfo = new FieldListInfo(new IR::Vector<IR::BFN::ChecksumEntry>());
    for (auto &htf : headerToFields) {
        int bitCount = 0;
        int offset16 = 0;
        const IR::Member *prev;
        bool fieldsTillByte = false;
        const auto *headerRef = htf.second.first;
        const auto &fieldMap = htf.second.second;
        auto header = headerRef->baseRef();
        auto headerPovBit = new IR::Member(IR::Type::Bits::get(1), headerRef, "$valid");
        for (auto field : header->type->fields) {
            if (bitCount % 8 == 0) {
                // Next field does not need to be included in csum list
                fieldsTillByte = false;
            }
            if (fieldMap.count(field->name)) {
                auto member = new IR::Member(field->type, headerRef, field->name);
                prev = member;
                if (header->is<IR::Metadata>()) {
                    // Constraints are relaxed on metadata
                    auto destPov =
                        new IR::Member(IR::Type::Bits::get(1),
                                       destField->expr->to<IR::ConcreteHeaderRef>(), "$valid");
                    listInfo->add_field(member, destPov, fieldMap.at(field->name));
                    continue;
                }
                if (fieldsTillByte) {
                    if (offset16 != fieldMap.at(field->name) % 16) {
                        ::fatal_error(
                            "All fields within the same byte-size chunk of the header"
                            " must have the same 2 byte-alignment in the checksum list. Checksum"
                            " engine is unable to read %1% for %2% checksum update",
                            member, destField);
                    }
                }
                fieldsTillByte = true;
                // Setting the alignment for next field
                offset16 = (fieldMap.at(field->name) + field->type->width_bits()) % 16;
                if (bitCount % 8 != fieldMap.at(field->name) % 8) {
                    ::fatal_error(
                        "Each field's bit alignment in the packet should be"
                        " equal to that in the checksum list. Checksum"
                        " engine is unable to read %1% for %2% checksum update",
                        member, destField);
                }
                prev = member;
                listInfo->add_field(member, headerPovBit, fieldMap.at(field->name));
            } else if (fieldsTillByte) {
                ::fatal_error(
                    "All fields within same byte of header must participate in the "
                    "checksum list. Checksum engine is unable to read %1% for %2% checksum update",
                    prev, destField);
            }
            bitCount += field->type->width_bits();
        }
    }
    return listInfo;
}

FieldListInfo *analyzeUpdateChecksumStatement(const IR::AssignmentStatement *assignment,
                                              ChecksumUpdateInfo *csum) {
    auto destField = assignment->left->to<IR::Member>();
    auto methodCall = assignment->right->to<IR::MethodCallExpression>();
    bool zerosAsOnes = false;
    if ((*methodCall->arguments).size() == 2) {
        zerosAsOnes = (*methodCall->arguments)[1]->expression->to<IR::BoolLiteral>()->value;
    }
    const IR::HeaderOrMetadata *currentFieldHeaderRef = nullptr;
    const IR::HeaderOrMetadata *lastFieldHeaderRef = nullptr;
    ordered_map<const IR::Member *, int> duplicateMap;
    ChecksumHeaderToFieldInfo headerToFields;
    int offset = 0;
    std::stringstream msg;
    // Along with collecting checksum update fields, following checks are performed:
    // * Fields of a header should always be byte aligned. This rule is relaxed for metadata.
    // * Sum of all the bits in the checksum list should be equal to a multiple of 8.

    for (auto *source : *getListExprComponents(*(*methodCall->arguments)[0]->expression)) {
        if (source->is<IR::Member>() || source->is<IR::Constant>()) {
            if (auto *constant = source->to<IR::Constant>()) {
                if (constant->asInt() != 0) {
                    ::fatal_error(
                        "Non-zero constant entry in checksum calculation"
                        " not implemented yet: %1%",
                        source);
                }
                currentFieldHeaderRef = nullptr;
            } else if (auto *member = source->to<IR::Member>()) {
                if (auto curHeader = member->expr->to<IR::ConcreteHeaderRef>()) {
                    currentFieldHeaderRef = curHeader->baseRef();
                    cstring headerName = curHeader->toString();
                    if (headerToFields.count(headerName) &&
                        headerToFields[headerName].second.count(member->member)) {
                        duplicateMap[member] = offset;
                    } else {
                        headerToFields[headerName].first = curHeader;
                        headerToFields[headerName].second[member->member] = offset;
                    }
                } else if (auto curStack = member->expr->to<IR::HeaderStackItemRef>()) {
                    currentFieldHeaderRef = curStack->baseRef();
                    cstring headerName = curStack->toString();
                    if (headerToFields.count(headerName) &&
                        headerToFields[headerName].second.count(member->member)) {
                        duplicateMap[member] = offset;
                    } else {
                        headerToFields[headerName].first = curStack;
                        headerToFields[headerName].second[member->member] = offset;
                    }
                } else {
                    error("Unhandled checksum expression %1%", member);
                }
            }

            if (offset % 8 && checkIncorrectCsumFields(lastFieldHeaderRef, currentFieldHeaderRef)) {
                msg << "In checksum update list, fields before " << source
                    << " do not add up to a multiple of 8 bits. Total bits until " << source
                    << " : " << offset;
                ::fatal_error("%1% %2%", source->srcInfo, msg.str());
            }

            lastFieldHeaderRef = currentFieldHeaderRef;
            offset += source->type->width_bits();
            LOG4("checksum update includes field:" << source);

        } else {
            std::string entry_type = " ";
            if (source->is<IR::TempVar>()) entry_type = " local variable ";
            error(ErrorType::ERR_UNSUPPORTED,
                  "%1%: Invalid%2%entry in checksum calculation %3%\n"
                  "  Currently only zero-constants and metadata/headers or their "
                  "fields are supported as entries",
                  methodCall, entry_type, source);
        }
    }
    auto listInfo = reorderFields(headerToFields, destField);
    ordered_map<const IR::Member *, const IR::TempVar *> duplicatedFields;
    for (auto &dupField : duplicateMap) {
        auto fieldSize = dupField.first->type->width_bits();
        const IR::TempVar *dupf =
            new IR::TempVar(IR::Type::Bits::get(fieldSize), true,
                            +"$duplicate_" + dupField.first->toString() + "_" +
                                std::to_string(duplicatedFields.size()));
        duplicatedFields[dupField.first] = dupf;
        auto povBit = new IR::Member(IR::Type::Bits::get(1), dupField.first->expr, "$valid");
        listInfo->add_field(dupf, povBit, dupField.second);
    }
    listInfo->duplicatedFields = duplicatedFields;
    if (listInfo->fields->size() && offset % 8) {
        std::stringstream msg;
        msg << "Fields in checksum update list do not add up to a multiple of 8 bits."
            << " Total bits: " << offset;
        ::fatal_error("%1%", msg.str());
    }
    LOG2("Validated computed checksum for field: " << destField);
    listInfo->zerosAsOnes = zerosAsOnes;
    csum->fieldListInfos.insert(listInfo);
    return (listInfo);
}

static UpdateConditionInfo *getUpdateCondition(const IR::Expression *condition) {
    bool conditionNegated = false;
    if (auto neq = condition->to<IR::LNot>()) {
        condition = neq->expr;
        conditionNegated = true;
    }
    if (auto *condMember = condition->to<IR::Member>()) {
        if (condMember->type->is<IR::Type_Boolean>())
            return new UpdateConditionInfo(condMember, conditionNegated);
    }
    Pattern::Match<IR::Member> field;
    Pattern::Match<IR::Constant> constant;
    if ((field == constant).match(condition) && field->type->width_bits() == 1 &&
        constant->type->width_bits() == 1) {
        if (constant->value == 0) conditionNegated = !conditionNegated;
        return new UpdateConditionInfo(field->to<IR::Member>(), conditionNegated);
    }
    if ((field != constant).match(condition) && field->type->width_bits() == 1 &&
        constant->type->width_bits() == 1) {
        if (constant->value == 1) conditionNegated = !conditionNegated;
        return new UpdateConditionInfo(field->to<IR::Member>(), conditionNegated);
    }
    error(
        "Tofino only supports 1-bit checksum update condition in the deparser, "
        " in the form of \"cond == 1\", \"cond == 0\", \"cond != 1\", \"cond != 0\". "
        "Please move the update condition into the control flow: %1%",
        condition);
    return nullptr;
}

static ordered_set<UpdateConditionInfo *> analyzeUpdateChecksumCondition(
    const IR::IfStatement *ifstmt) {
    ordered_set<UpdateConditionInfo *> updateConditions;
    if (!ifstmt->ifTrue || ifstmt->ifFalse) {
        return updateConditions;
    }
    auto *condition = ifstmt->condition;
    while (auto andCondition = condition->to<IR::LAnd>()) {
        updateConditions.insert(getUpdateCondition(andCondition->right));
        if (auto leftEquation = andCondition->left->to<IR::Equ>()) {
            updateConditions.insert(getUpdateCondition(leftEquation));
        }
        condition = andCondition->left;
    }
    updateConditions.insert(getUpdateCondition(condition));
    return updateConditions;
}

struct ChecksumMethodCheck : public Inspector {
    bool preorder(const IR::MethodCallExpression *methodCall) {
        auto *member = methodCall->method->to<IR::Member>();
        if (member == nullptr) return false;
        auto *expr = member->expr->to<IR::PathExpression>();
        auto *type = expr->type->to<IR::Type_Extern>();
        if (type && type->name == "Checksum" && member->member != "update") {
            ::fatal_error(ErrorType::ERR_UNSUPPORTED,
                          "checksum operation."
                          " Only checksum update() operation is supported for checksum calculations"
                          " in the deparser. %1%",
                          member->srcInfo);
        }
        return false;
    }
};

struct CollectUpdateChecksums : public Inspector {
    ChecksumUpdateInfoMap checksums;
    ChecksumUpdateInfo *csum;
    bool preorder(const IR::AssignmentStatement *assignment) {
        if (!checksumUpdateSanityCheck(assignment)) {
            return false;
        }
        auto dest = assignment->left->to<IR::Member>();
        if (checksums.count(dest->toString())) {
            csum = checksums[dest->toString()];
        } else {
            csum = new ChecksumUpdateInfo(dest->toString());
        }
        auto listInfo = analyzeUpdateChecksumStatement(assignment, csum);

        if (listInfo) {
            auto ifStmt = findContext<IR::IfStatement>();
            if (ifStmt) {
                auto updateConditions = analyzeUpdateChecksumCondition(ifStmt);
                for (auto updateCondition : updateConditions) {
                    if (updateCondition && !csum->isUnconditional) {
                        bool updateCondExists = false;
                        for (auto cond : listInfo->updateConditions) {
                            if (cond->field->equiv(*updateCondition->field)) {
                                updateCondExists = true;
                                break;
                            }
                        }
                        if (!updateCondExists) {
                            listInfo->updateConditions.insert(updateCondition);
                        }

                    } else {
                        csum->fieldListInfos.erase(listInfo);
                    }
                }
            } else {
                // There exists an unconditional checksum. Delete the conditional checksum
                csum->markAsUnconditional(listInfo);

                if (csum->fieldListInfos.size() > 1) {
                    std::stringstream msg;
                    msg << dest << " has an unconditional update checksum operation."
                        << " All other conditional update checksums will be deleted.";
                    warning("%1%", msg.str());
                }
            }
            checksums[dest->toString()] = csum;
        }
        return false;
    }
};

struct GetChecksumPovBits : public Inspector {
    ChecksumUpdateInfoMap &checksums;
    explicit GetChecksumPovBits(ChecksumUpdateInfoMap &checksums) : checksums(checksums) {}

    bool preorder(const IR::BFN::EmitField *emit) override {
        auto source = emit->source->field->to<IR::Member>();
        if (checksums.count(source->toString())) {
            auto csum = checksums.at(source->toString());
            csum->povBit = emit->povBit;
            FieldListInfo *uncondFieldList = nullptr;
            UpdateConditionInfo *uncond = nullptr;
            for (auto listInfo : csum->fieldListInfos) {
                // Header validity bit needs to be true for any kind of checksum update. If the only
                // condition is the valid bit then this checksum is same as an
                // unconditional checksum. Deleting all other conditional checksum.
                for (auto uc : listInfo->updateConditions) {
                    if (csum->povBit->field->equiv(*uc->field)) {
                        uncondFieldList = listInfo;
                        uncond = uc;
                    }
                }
            }
            if (uncondFieldList) {
                if (uncondFieldList->updateConditions.size() > 1) {
                    // If some other field is also used as condition along with header validity bit,
                    // then the checksum is not unconditional. Removing only valid bit condition
                    uncondFieldList->updateConditions.erase(uncond);
                } else {
                    csum->markAsUnconditional(uncondFieldList);
                }
            }
            // TODO If the condition bit dominates the header validity bit
            // in the parse graph, we can also safely ignore the condition.
        }
        return false;
    }

    void end_apply() override {
        std::set<cstring> toElim;

        for (auto &csum : checksums) {
            if (csum.second->povBit == nullptr) toElim.insert(csum.first);
        }
        for (auto csum : toElim) {
            LOG4("eliminate unpredicated checksum");
            checksums.erase(csum);
        }
    }
};

/**
 * \ingroup ExtractChecksum
 *
 * Substitute computed checksums into the deparser code by replacing EmitFields for
 * the destination field with EmitChecksums.
 */
struct SubstituteUpdateChecksums : public Transform {
    explicit SubstituteUpdateChecksums(const ChecksumUpdateInfoMap &checksums)
        : checksums(checksums) {}

 private:
    IR::BFN::Deparser *preorder(IR::BFN::Deparser *deparser) override {
        IR::Vector<IR::BFN::Emit> newEmits;

        for (auto *p : deparser->emits) {
            bool rewrite = false;

            auto *emit = p->to<IR::BFN::EmitField>();
            if (emit->source->field->is<IR::Member>()) {
                auto *source = emit->source->field->to<IR::Member>();
                if (checksums.find(source->toString()) != checksums.end()) {
                    auto emitChecksums = rewriteEmitChecksum(emit);
                    for (auto *ec : emitChecksums) {
                        newEmits.push_back(ec);
                    }
                    rewrite = true;
                }
            }

            if (!rewrite) newEmits.push_back(emit);
        }

        deparser->emits = newEmits;
        return deparser;
    }

    std::vector<IR::BFN::Emit *> rewriteEmitChecksum(const IR::BFN::EmitField *emit) {
        auto *source = emit->source->field->to<IR::Member>();
        auto *csumInfo = checksums.at(source->toString());

        std::vector<IR::BFN::Emit *> emitChecksums;

        if (!csumInfo->isUnconditional) {
            // If update condition is specified, we create two emits: one for
            // the updated checksum, and one for the original checksum from header.
            // The POV bits for these emits are inserted by the compiler (see
            // pass "InsertTablesForChecksums" below).
            for (auto listInfo : csumInfo->fieldListInfos) {
                auto *emitUpdatedChecksum = new IR::BFN::EmitChecksum(
                    new IR::BFN::FieldLVal(listInfo->deparseUpdated), *(listInfo->fields),
                    new IR::BFN::ChecksumLVal(source));
                emitUpdatedChecksum->zeros_as_ones = listInfo->zerosAsOnes;
                emitChecksums.push_back(emitUpdatedChecksum);
            }

            auto *emitOriginalChecksum = new IR::BFN::EmitField(source, csumInfo->deparseOriginal);

            emitChecksums.push_back(emitOriginalChecksum);
        } else {
            // If no user specified update condition, the semantic is
            // to deparse based on the header validity bit.
            for (auto listInfo : csumInfo->fieldListInfos) {
                auto *emitUpdatedChecksum = new IR::BFN::EmitChecksum(
                    emit->povBit, *(listInfo->fields), new IR::BFN::ChecksumLVal(source));
                emitUpdatedChecksum->zeros_as_ones = listInfo->zerosAsOnes;
                emitChecksums.push_back(emitUpdatedChecksum);
            }
        }

        // TODO if user specifies the update conditon, but never set it anywhere,
        // we should treat it as if no condition is specified.

        return emitChecksums;
    }

    assoc::map<const IR::Member *, const IR::Member *> negatedUpdateConditions;

    const ChecksumUpdateInfoMap &checksums;
};

/**
 * \ingroup ExtractChecksum
 *
 * If checksum update is unconditional, we don't need to allocate %PHV space to store
 * the original header checksum field, nor do we need to extract the header checksum
 * field (since we will deparse the checksum from the computed value in the deparser).
 * This pass replaces the unconditional checksum field with the "ChecksumLVal" so that
 * %PHV allocation doesn't allocate container for it.
 *
 * FIXME(zma) check if checksum is used in MAU
 */
struct SubstituteChecksumLVal : public Transform {
    explicit SubstituteChecksumLVal(const ChecksumUpdateInfoMap &checksums)
        : checksums(checksums) {}

    IR::BFN::ParserPrimitive *preorder(IR::BFN::Extract *extract) override {
        prune();
        if (checksums.find(extract->dest->toString()) != checksums.end()) {
            auto csumInfo = checksums.at(extract->dest->toString());
            for (auto listInfo : csumInfo->fieldListInfos) {
                if (listInfo->updateConditions.empty()) {
                    if (auto lval = extract->dest->to<IR::BFN::FieldLVal>()) {
                        return new IR::BFN::Extract(new IR::BFN::ChecksumLVal(lval->field),
                                                    extract->source);
                    }
                }
            }
        }
        return extract;
    }

    const ChecksumUpdateInfoMap &checksums;
};

// Finds the key and return position. Will return -1 if key not found
int get_key_position(const std::vector<const IR::Expression *> &keys,
                     const IR::Expression *findKey) {
    int idx = 0;
    for (auto key : keys) {
        if (key->equiv(*findKey)) {
            return idx;
        }
        idx++;
    }
    return -1;
}

/**
 * \ingroup ExtractChecksum
 *
 * For each user checksum update condition bit, e.g.
 *
 *     calculated_field hdr.checksum  {
 *         update tcp_checksum if (meta.update_checksum == 1);
 *     }
 *
 * we need to create two bits to predicate two FD entries in the
 * deparser, one for the updated checksum value, the other for
 * the original checksum value from packet.
 * The logic for these two bits are:
 *
 *     $deparse_updated_csum = update_checksum & hdr.$valid
 *     $deparse_original_csum = !update_checksum & hdr.$valid
 *
 * At the end of the table sequence of each gress, we insert
 * a table that runs an action that contains the two instructions
 * for all user condition bits. This is the correct place to insert
 * the table since header maybe become validated/invalidated
 * anywhere in the parser or MAU. It is possible that we could
 * insert these instructions earlier in the table sequence (thus
 * saving a table) if the compiler can figure out an action
 * is predicated by the header validity bit, but this should be a
 * general table "fusing" optimization in the backend.
 */
void add_checksum_condition_table(IR::MAU::TableSeq *tableSeq, ChecksumUpdateInfo *csumInfo,
                                  gress_t gress) {
    if (csumInfo->isUnconditional) return;
    cstring tableName = csumInfo->dest + "_encode_update_condition_"_cs;
    cstring actionName = "_set_checksum_update_"_cs;
    // Get matchkeys
    std::vector<const IR::Expression *> keys;
    keys.push_back(csumInfo->povBit->field);
    for (auto fieldListInfo : csumInfo->fieldListInfos) {
        for (auto updateCond : fieldListInfo->updateConditions) {
            if (get_key_position(keys, updateCond->field) < 0) keys.push_back(updateCond->field);
        }
    }

    // Get Match Outputs
    csumInfo->deparseOriginal =
        new IR::TempVar(IR::Type::Bits::get(1), true, csumInfo->dest + ".$deparse_original_csum");
    ordered_set<const IR::TempVar *> outputs;
    outputs.insert(csumInfo->deparseOriginal);
    ordered_map<unsigned, unsigned> match_to_action_param;

    int id = 0;
    for (auto fieldListInfo : csumInfo->fieldListInfos) {
        unsigned action_param = 0;
        fieldListInfo->deparseUpdated =
            new IR::TempVar(IR::Type::Bits::get(1), true,
                            csumInfo->dest + ".$deparse_updated_csum_" + std::to_string(id++));
        outputs.insert(fieldListInfo->deparseUpdated);
        action_param |= 1 << (outputs.size() - 1);
        unsigned match = 1 << (keys.size() - 1);  // match for valid bit
        int dontCareBits = INT_MAX;
        for (auto updateCond : fieldListInfo->updateConditions) {
            int keyDiff = keys.size() - get_key_position(keys, updateCond->field) - 1;
            if (keyDiff < dontCareBits) dontCareBits = keyDiff;
            if (!updateCond->conditionNegated) {
                match |= (1 << (keyDiff));
            }
        }
        // Some tables might have more matchkeys than the ones required to calculate
        // checksum from the particular field list. "dontCareBits" is the number of
        // the excess keys we do not care about.
        // Map all possible combination of dont care bits to the same action parameter
        for (int i = 0; i < (1 << dontCareBits); i++) {
            match_to_action_param[match | i] = action_param;
        }
    }
    for (unsigned match = 1U << (keys.size() - 1); match < (1U << keys.size()); match++) {
        if (!match_to_action_param.count(match)) match_to_action_param[match] = 1;
    }
    auto ma = new MatchAction(keys, outputs, match_to_action_param);
    auto encoder = create_pov_encoder(gress, tableName, actionName, *ma);
    encoder->run_before_exit = true;
    tableSeq->tables.push_back(encoder);

    LOG3("Created table " << tableName << " in " << gress << " for match action:");
    LOG5(ma->print());
    return;
}

/**
 * \ingroup ExtractChecksum
 *
 * If a field is included in multiple times in checksum update, a mau table will be created
 * with an action to copy the repeated field into a  metadata. This metadata will be used instead of
 * the duplicate field. This is done because a checksum engine can add a %PHV only once.
 * If a field is added more than twice, that many
 * actions will be created to copy the field in different metadatas
 */
void add_entry_duplication_tables(IR::MAU::TableSeq *tableSeq, ChecksumUpdateInfo *csumInfo,
                                  gress_t gress) {
    cstring tableName = csumInfo->dest + "_duplication_"_cs + toString(gress);
    auto table = new IR::MAU::Table(tableName, gress);
    table->is_compiler_generated = true;
    auto p4Name = tableName + "_" + cstring::to_cstring(gress);
    table->match_table = new IR::P4Table(p4Name.c_str(), new IR::TableProperties());
    auto action = new IR::MAU::Action("__checksum_field_duplication__"_cs);
    for (auto &fieldInfo : csumInfo->fieldListInfos) {
        if (fieldInfo->duplicatedFields.empty()) {
            continue;
        } else {
            for (auto &dupf : fieldInfo->duplicatedFields) {
                action->default_allowed = action->init_default = true;
                auto setdup = new IR::MAU::Instruction("set"_cs, {dupf.second, dupf.first});
                action->action.push_back(setdup);
            }
            table->actions[action->name] = action;
        }
    }
    if (table->actions.size()) {
        tableSeq->tables.push_back(table);
        LOG3("Created table " << tableName << " to handle checksum entry duplication");
    }
    return;
}

/**
 * \ingroup ExtractChecksum
 *
 * Inserts possible extra tables into gress that determine if the checksum should be updated.
 *
 * @sa add_checksum_condition_table, add_entry_duplication_tables
 */
struct InsertTablesForChecksums : public Transform {
    explicit InsertTablesForChecksums(const ChecksumUpdateInfoMap &checksums, gress_t gress)
        : checksums(checksums), gress(gress) {}

 private:
    IR::MAU::TableSeq *preorder(IR::MAU::TableSeq *tableSeq) override {
        if (!checksums.empty()) {
            prune();
        }
        for (auto &csum : checksums) {
            add_checksum_condition_table(tableSeq, csum.second, gress);
            add_entry_duplication_tables(tableSeq, csum.second, gress);
        }
        return tableSeq;
    }

    const ChecksumUpdateInfoMap &checksums;
    const gress_t gress;
};

struct CollectFieldToState : public Inspector {
    std::map<cstring, ordered_set<const IR::BFN::ParserState *>> field_to_state;

    bool preorder(const IR::BFN::Extract *extract) override {
        auto state = findContext<IR::BFN::ParserState>();
        CHECK_NULL(state);

        field_to_state[extract->dest->field->toString()].insert(state);
        return false;
    }
};

struct CollectNestedChecksumInfo : public Inspector {
    const IR::BFN::ParserGraph &graph;
    const std::map<cstring, ordered_set<const IR::BFN::ParserState *>> &field_to_state;
    ChecksumUpdateInfoMap checksumsMap;
    std::map<cstring, ordered_set<const IR::BFN::EmitChecksum *>> dest_to_nested_csum;
    ordered_set<const IR::BFN::EmitChecksum *> visited;

    explicit CollectNestedChecksumInfo(
        const IR::BFN::ParserGraph &graph,
        const std::map<cstring, ordered_set<const IR::BFN::ParserState *>> &field_to_state,
        ChecksumUpdateInfoMap checksumsMap)
        : graph(graph), field_to_state(field_to_state), checksumsMap(checksumsMap) {}

    bool can_be_on_same_parse_path(const IR::Expression *a, const IR::Expression *b) {
        if (field_to_state.count(a->toString()) && field_to_state.count(b->toString())) {
            for (auto sa : field_to_state.at(a->toString())) {
                for (auto sb : field_to_state.at(b->toString())) {
                    if (graph.is_descendant(sa, sb) || graph.is_descendant(sb, sa)) return true;
                }
            }
        }

        return false;
    }

    ordered_set<UpdateConditionInfo *> findUpdateCondition(const IR::BFN::EmitChecksum *checksum) {
        auto checksumInfo = checksumsMap[checksum->dest->field->toString()];

        if (checksumInfo->isUnconditional) return {};
        for (auto fieldListInfo : checksumInfo->fieldListInfos) {
            if (checksum->sources.equiv(*fieldListInfo->fields))
                return fieldListInfo->updateConditions;
        }
        return {};
    }

    bool findIfConditionsMutex(const IR::BFN::EmitChecksum *checksum1,
                               const IR::BFN::EmitChecksum *checksum2) {
        auto updateCondition1 = findUpdateCondition(checksum1);
        auto updateCondition2 = findUpdateCondition(checksum2);

        if (updateCondition1.empty() || updateCondition2.empty()) return false;
        // Find if one update condition bit exists which should be true for one checksum
        // to allow update and false for the other or vice versa
        for (auto *uc1 : updateCondition1) {
            for (auto *uc2 : updateCondition2) {
                if (uc1->field->equiv(*uc2->field) &&
                    uc1->conditionNegated != uc2->conditionNegated) {
                    return true;
                }
            }
        }
        return false;
    }

    bool preorder(const IR::BFN::EmitChecksum *checksum) override {
        for (auto c : visited) {
            if (!can_be_on_same_parse_path(c->dest->field, checksum->dest->field)) continue;
            // Nested checkums are allowed if both the checksums are conditional and
            // have mutually exclusive conditions
            for (auto s : checksum->sources) {
                if (c->dest->field->equiv(*s->field->field)) {
                    if (!findIfConditionsMutex(checksum, c)) {
                        dest_to_nested_csum[checksum->dest->toString()].insert(c);
                    }
                }
            }

            for (auto s : c->sources) {
                if (checksum->dest->field->equiv(*s->field->field)) {
                    // Check if conditions are mutually exclusive
                    if (!findIfConditionsMutex(checksum, c)) {
                        dest_to_nested_csum[c->dest->toString()].insert(checksum);
                    }
                }
            }
        }

        visited.insert(checksum);

        return false;
    }
};

/**
 * \ingroup ExtractChecksum
 *
 * In nested checksums, if fieldlist of one checksum update is subset of
 * the other checksum update , then all the fields along with the checksum update destination
 * of the former checksum can be removed from the latter.
 * Consider two checksum updates
 *
 *      a.csum.update({ x, y, z})
 *      b.csum.update({x, y, z, a.csum, c, d}
 *
 * Note that b.csum contains a nested checksum a.csum.
 * According to the above statements,
 *
 *      a.csum = ~(x + y + z) where '~' means complement
 *      b.csum = ~(x + y + z + a.csum + c + d)
 *
 * If we substitute a.csum
 *
 *      b.csum = ~(x + y + z +  ~(x + y + z) + c + d) = ~(c + d)
 *
 * From the above equation, we see that a.csum, x, y, z has no effect on b.csum
 * This means we can safely remove the fields for a.csum from b.csum fieldlist
 * if fieldlist of a.csum are subset of b.csum fieldlist.
 */
struct AbsorbNestedChecksum : public Transform {
    ChecksumUpdateInfoMap checksumUpdateInfoMap;
    std::map<cstring, ordered_set<const IR::BFN::EmitChecksum *>> dest_to_nested_csum;
    std::map<cstring, std::set<cstring>> dest_to_delete_entries;
    AbsorbNestedChecksum(
        ChecksumUpdateInfoMap checksumUpdateInfoMap,
        std::map<cstring, ordered_set<const IR::BFN::EmitChecksum *>> dest_to_nested_csum)
        : checksumUpdateInfoMap(checksumUpdateInfoMap), dest_to_nested_csum(dest_to_nested_csum) {}

    void print_error(const IR::BFN::EmitChecksum *a, const IR::BFN::EmitChecksum *b) {
        std::stringstream hint;

        hint << "Consider using checksum units in both ingress and egress deparsers";

        if (BackendOptions().arch == "v1model") {
            hint << " (@pragma calculated_field_update_location/";
            hint << "residual_checksum_parser_update_location)";
        }

        hint << ".";

        ::fatal_error(
            "Tofino does not support nested checksum updates in the same deparser:"
            " %1%, %2%\n%3%",
            a->dest->field, b->dest->field, hint.str());
    }

    const IR::Node *preorder(IR::BFN::EmitChecksum *emitChecksum) override {
        auto nestedVec = new IR::Vector<IR::BFN::EmitChecksum>();
        auto parentDest = emitChecksum->dest->toString();
        if (!dest_to_nested_csum.count(parentDest)) return emitChecksum;
        for (auto &nestedCsum : dest_to_nested_csum.at(parentDest)) {
            dest_to_delete_entries[parentDest].insert(nestedCsum->dest->toString());
            for (auto source1 : nestedCsum->sources) {
                bool match = false;
                for (auto source2 : emitChecksum->sources) {
                    if (source1->field->equiv(*source2->field)) {
                        match = true;
                        dest_to_delete_entries[parentDest].insert(source1->field->toString());
                        break;
                    }
                }
                if (!match) {
                    dest_to_delete_entries.erase(emitChecksum->dest->toString());
                    nestedVec->push_back(nestedCsum);
                    // Nesting of checksum supported only for JbayB0
                    if (Device::pardeSpec().numDeparserInvertChecksumUnits() == 0) {
                        print_error(emitChecksum, nestedCsum);
                    }
                    break;
                }
            }
        }
        emitChecksum->nested_checksum = *nestedVec;
        return emitChecksum;
    }

    const IR::Node *preorder(IR::BFN::ChecksumEntry *entry) override {
        auto emitChecksum = findContext<IR::BFN::EmitChecksum>();
        auto fieldString = entry->field->toString();
        cstring dest = emitChecksum->dest->toString();
        if (!dest_to_delete_entries.count(dest)) return entry;
        if (dest_to_delete_entries.at(dest).count(fieldString)) {
            return nullptr;
        }
        return entry;
    }
};

/**
 * \ingroup ExtractChecksum
 *
 * Remove the nested checksum destination from the parent checksum update field list
 * if the parent checksum is unconditional.
 * Use the "deparse_original" bit as gating pov bit for the nested checksum destination
 * in the parent checksum update if the parent checksum is conditional.
 */
struct DeleteChecksumField : public Transform {
    const ChecksumUpdateInfoMap &checksumInfo;
    explicit DeleteChecksumField(const ChecksumUpdateInfoMap &checksumInfo)
        : checksumInfo(checksumInfo) {}
    const IR::Node *preorder(IR::BFN::ChecksumEntry *entry) override {
        auto emitChecksum = findContext<IR::BFN::EmitChecksum>();
        for (auto nest : emitChecksum->nested_checksum) {
            if (nest->dest->field->equiv(*entry->field->field)) {
                auto nestCsum = checksumInfo.at(nest->dest->toString());
                if (nestCsum->isUnconditional) {
                    return nullptr;
                } else {
                    entry->povBit = new IR::BFN::FieldLVal(nestCsum->deparseOriginal);
                }
            }
        }
        return entry;
    }
};
}  // namespace

namespace BFN {

IR::BFN::Pipe *extractChecksumFromDeparser(const IR::BFN::TnaDeparser *deparser,
                                           IR::BFN::Pipe *pipe) {
    CHECK_NULL(pipe);

    if (!deparser) return pipe;

    if (BackendOptions().verbose > 0) Logging::FileLog parserLog(pipe->canon_id(), "parser.log"_cs);

    auto gress = deparser->thread;

    deparser->apply(ChecksumMethodCheck());
    CollectUpdateChecksums collectChecksums;
    deparser->apply(collectChecksums);
    auto checksums = collectChecksums.checksums;

    if (checksums.empty()) return pipe;

    GetChecksumPovBits getChecksumPovBits(checksums);
    pipe->thread[gress].deparser->apply(getChecksumPovBits);
    SubstituteUpdateChecksums substituteChecksums(checksums);
    SubstituteChecksumLVal substituteChecksumLVal(checksums);
    InsertTablesForChecksums inserttablesforchecksums(checksums, gress);
    pipe->thread[gress].mau = pipe->thread[gress].mau->apply(inserttablesforchecksums);
    pipe->thread[gress].deparser = pipe->thread[gress].deparser->apply(substituteChecksums);
    for (auto &parser : pipe->thread[gress].parsers) {
        parser = parser->apply(substituteChecksumLVal);
    }

    for (auto &parser : pipe->thread[gress].parsers) {
        CollectParserInfo cg;
        parser->apply(cg);

        CollectFieldToState cfs;
        parser->apply(cfs);

        auto graph = cg.graphs().at(parser->to<IR::BFN::Parser>());
        CollectNestedChecksumInfo collectNestedChecksumInfo(*graph, cfs.field_to_state, checksums);

        pipe->thread[gress].deparser->apply(collectNestedChecksumInfo);
        AbsorbNestedChecksum absorbNestedChecksum(checksums,
                                                  collectNestedChecksumInfo.dest_to_nested_csum);
        pipe->thread[gress].deparser = pipe->thread[gress].deparser->apply(absorbNestedChecksum);
        // Only for JbayB0
        if (Device::pardeSpec().numDeparserInvertChecksumUnits()) {
            DeleteChecksumField deleteChecksumField(checksums);
            pipe->thread[gress].deparser = pipe->thread[gress].deparser->apply(deleteChecksumField);
        }
    }

    return pipe;
}

}  // namespace BFN
