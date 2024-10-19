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

#include "alpm.h"

#include <cmath>

#include "bf-p4c/arch/helpers.h"
#include "bf-p4c/common/pragma/all_pragmas.h"
#include "frontends/p4/coreLibrary.h"
#include "frontends/p4/methodInstance.h"
#include "frontends/p4/typeChecking/bindVariables.h"
#include "ir/ir.h"
#include "lib/bitops.h"
#include "lib/error_catalog.h"
#include "lib/log.h"

namespace BFN {

const std::set<unsigned> SplitAlpm::valid_partition_values = {1024, 2048, 4096, 8192};
const cstring SplitAlpm::ALGORITHMIC_LPM_PARTITIONS = cstring(PragmaAlpmPartitions::name);
const cstring SplitAlpm::ALGORITHMIC_LPM_SUBTREES_PER_PARTITION =
    cstring(PragmaAlpmSubtreePartitions::name);
const cstring SplitAlpm::ALGORITHMIC_LPM_ATCAM_EXCLUDE_FIELD_MSBS =
    cstring(PragmaAlpmAtcamExcludeFieldMsbs::name);

bool SplitAlpm::use_funnel_shift(int lpm_key_width) {
    return lpm_key_width > 32 && lpm_key_width <= 64;
}
/* syntheisze a funnel_shift_operation that takes two 32-bit fields
 * and right_shift by shift_amt and return a 32-bit output
 */
const IR::StatOrDecl *SplitAlpm::synth_funnel_shift_ops(const IR::P4Table *tbl,
                                                        const IR::Expression *lpm_key,
                                                        int shift_amt) {
    int lpm_key_width = lpm_key->type->width_bits();

    auto *shift_amt_const = new IR::Constant(IR::Type_Bits::get(lpm_key_width, true), shift_amt);
    if (shift_amt > 32) {
        shift_amt -= 32;
        return new IR::AssignmentStatement(
            new IR::PathExpression(IR::ID(tbl->name + "_partition_key")),
            new IR::Shr(new IR::Slice(lpm_key, lpm_key_width - 1, 32), shift_amt_const));
    } else if (shift_amt == 32) {
        return new IR::AssignmentStatement(
            new IR::PathExpression(IR::ID(tbl->name + "_partition_key")),
            new IR::Slice(lpm_key, lpm_key_width - 1, 32));
    } else {
        auto typeArguments = new IR::Vector<IR::Type>({IR::Type_Bits::get(32)});
        auto arguments = new IR::Vector<IR::Argument>();
        arguments->push_back(
            new IR::Argument(new IR::PathExpression(IR::ID(tbl->name + "_partition_key"))));

        // cast to bit<32> if the upper half is less than 32 bit wide
        IR::Expression *upper = new IR::Slice(lpm_key, lpm_key_width - 1, 32);
        if (lpm_key_width != 64) upper = new IR::Cast(IR::Type_Bits::get(32), upper);
        arguments->push_back(new IR::Argument(upper));
        arguments->push_back(new IR::Argument(new IR::Slice(lpm_key, 31, 0)));
        arguments->push_back(new IR::Argument(shift_amt_const));
        return new IR::MethodCallStatement(new IR::MethodCallExpression(
            new IR::PathExpression(new IR::Path(IR::ID("funnel_shift_right"))), typeArguments,
            arguments));
    }
}

const IR::IndexedVector<IR::Declaration> *SplitAlpm::create_temp_var(
    const IR::P4Table *tbl, unsigned number_actions, unsigned partition_index_bits,
    unsigned atcam_subset_width, unsigned subtrees_per_partition, const IR::Expression *lpm_key) {
    auto tempvar = new IR::IndexedVector<IR::Declaration>();

    tempvar->push_back(new IR::Declaration_Variable(IR::ID(tbl->name + "_partition_index"),
                                                    IR::Type_Bits::get(partition_index_bits)));

    if (number_actions > 1) {
        int partition_key_width = atcam_subset_width;
        if (use_funnel_shift(lpm_key->type->width_bits())) partition_key_width = 32;
        tempvar->push_back(new IR::Declaration_Variable(IR::ID(tbl->name + "_partition_key"),
                                                        IR::Type_Bits::get(partition_key_width)));
        if (subtrees_per_partition > 1) {
            auto subtree_id_bits = ::ceil_log2(subtrees_per_partition);
            tempvar->push_back(new IR::Declaration_Variable(IR::ID(tbl->name + "_subtree_id"),
                                                            IR::Type_Bits::get(subtree_id_bits)));
        }
    }

    return tempvar;
}

/**
 * create pre-classifier actions based on parameters in Alpm constructors.
 *
 * The Alpm constructors takes 4 arguments:
 * 1. number_partitions : only support 1024, 2048, 4096 and 8192, which corresponds to
 *                        partition_index of log2(number_partitions) bits.
 * 2. subtrees_per_partition : if more than 1, the action data would contain extra
 *                        bit to identify which subtree to search for during lookup
 *
 * A further optimization requires two more parameters: atcam_subset_width and
 * shift_granularity.
 *
 * 3. atcam_subset_width : further divide partition according to the subset_width
 *                        and shift_granularity into smaller chunks.
 * 4. shift_granularity : used in combination with atcam_subset_width
 *
 * Together, these two parameters allows the programmer to control the key size
 * used in the exact match portion of the atcam. The formulae for the
 * optimization is:
 *
 * for n in range 0 .. lpm_key_width / shift_granularity:
 *   tmp = (lpm_key >> n)
 *   partition_key = tmp[atcam_subset_width - 1, 0]
 *
 * The shift operation can be more optimally implemented with the
 * funnel_shift_right operation to reduce constraints on phv.
 *
 * At this stage, the funnel_shift_right operation is only synthesized for
 * lpm_key that is more than 32 bit. lpm_key that is <= 32 bit can be
 * efficiently implemented with a single PHV container.
 *
 */
const IR::IndexedVector<IR::Declaration> *SplitAlpm::create_preclassifer_actions(
    const IR::P4Table *tbl, unsigned number_actions, unsigned partition_index_bits,
    unsigned atcam_subset_width, unsigned shift_granularity, unsigned subtrees_per_partition,
    const IR::Expression *lpm_key) {
    auto actions = new IR::IndexedVector<IR::Declaration>();

    auto params = new IR::ParameterList();
    params->push_back(new IR::Parameter(IR::ID("index"), IR::Direction::None,
                                        IR::Type_Bits::get(partition_index_bits)));
    if (number_actions > 1 && subtrees_per_partition > 1) {
        auto subtree_id_bits = ::ceil_log2(subtrees_per_partition);
        params->push_back(new IR::Parameter(IR::ID("subtree_id"), IR::Direction::None,
                                            IR::Type_Bits::get(subtree_id_bits)));
    }

    for (unsigned n = 0; n < number_actions; n++) {
        auto body = new IR::BlockStatement;

        body->push_back(new IR::AssignmentStatement(
            new IR::PathExpression(IR::ID(tbl->name + "_partition_index")),
            new IR::PathExpression(IR::ID("index"))));

        if (number_actions > 1) {
            if (use_funnel_shift(lpm_key->type->width_bits())) {
                int shift_amt =
                    lpm_key->type->width_bits() - atcam_subset_width - n * shift_granularity;
                auto stmt = synth_funnel_shift_ops(tbl, lpm_key, shift_amt);
                body->push_back(stmt);
            } else {
                body->push_back(new IR::AssignmentStatement(
                    new IR::PathExpression(IR::ID(tbl->name + "_partition_key")),
                    new IR::Slice(
                        new IR::Shr(lpm_key,
                                    new IR::Constant(lpm_key->type, lpm_key->type->width_bits() -
                                                                        atcam_subset_width -
                                                                        n * shift_granularity)),
                        atcam_subset_width - 1, 0)));
            }
        }

        if (number_actions > 1 && subtrees_per_partition > 1) {
            body->push_back(new IR::AssignmentStatement(
                new IR::PathExpression(IR::ID(tbl->name + "_subtree_id")),
                new IR::PathExpression(IR::ID("subtree_id"))));
        }

        auto name = tbl->name + "_set_partition_index_" + cstring::to_cstring(n);
        actions->push_back(new IR::P4Action(IR::ID(name), params, body));
    }

    return actions;
}

void SplitAlpm::apply_pragma_exclude_msbs(const IR::P4Table *tbl,
                                          const ordered_map<cstring, int> *fields_to_exclude,
                                          IR::Vector<IR::KeyElement> &keys) {
    for (auto k : tbl->getKey()->keyElements) {
        cstring fname;
        if (auto annot = k->getAnnotation(IR::Annotation::nameAnnotation)) {
            fname = annot->getName();
        } else {
            fname = k->expression->toString();
        }
        if (fields_to_exclude->find(fname) != fields_to_exclude->end()) {
            auto msb = fields_to_exclude->at(fname);
            auto key_width = k->expression->type->width_bits();
            if (msb < key_width) {
                keys.push_back(new IR::KeyElement(
                    k->srcInfo, k->annotations,
                    new IR::Slice(k->expression, key_width - msb - 1, 0), k->matchType));
            }
        } else {
            keys.push_back(k);
        }
    }
}

const IR::P4Table *SplitAlpm::create_atcam_table(const IR::P4Table *tbl, unsigned partition_count,
                                                 unsigned subtrees_per_partition,
                                                 unsigned number_actions, int atcam_subset_width,
                                                 int shift_granularity,
                                                 ordered_map<cstring, int> &fields_to_exclude,
                                                 const IR::Expression *lpm_key) {
    auto properties = new IR::IndexedVector<IR::Property>;

    // create partition_index
    IR::Vector<IR::KeyElement> keys;
    if (number_actions > 1) {
        // context.json requires subtree_id to be first key
        if (subtrees_per_partition > 1) {
            keys.push_back(
                new IR::KeyElement(new IR::PathExpression(IR::ID(tbl->name + "_subtree_id")),
                                   new IR::PathExpression(IR::ID("exact"))));
        }
        IR::Expression *partition_key =
            new IR::PathExpression(IR::ID(tbl->name + "_partition_key"));
        if (use_funnel_shift(lpm_key->type->width_bits())) {
            partition_key = new IR::Slice(partition_key, atcam_subset_width - 1, 0);
        }
        keys.push_back(new IR::KeyElement(partition_key, new IR::PathExpression(IR::ID("lpm"))));
    } else {
        apply_pragma_exclude_msbs(tbl, &fields_to_exclude, keys);
    }
    keys.push_back(
        new IR::KeyElement(new IR::PathExpression(IR::ID(tbl->name + "_partition_index")),
                           new IR::PathExpression(IR::ID("atcam_partition_index"))));

    properties->push_back(new IR::Property("key"_cs, new IR::Key(keys), false));

    // create partition_key for further alpm optimization
    properties->push_back(new IR::Property("actions"_cs, tbl->getActionList(), false));

    // create size
    properties->push_back(
        new IR::Property("size"_cs, new IR::ExpressionValue(tbl->getSizeProperty()), false));

    // create default_action
    if (tbl->getDefaultAction()) {
        properties->push_back(new IR::Property(
            "default_action"_cs, new IR::ExpressionValue(tbl->getDefaultAction()), false));
    }

    if (auto prop = tbl->properties->getProperty("idle_timeout"_cs)) {
        properties->push_back(prop);
    }

    if (auto prop = tbl->properties->getProperty("implementation"_cs)) {
        properties->push_back(prop);
    }

    properties->push_back(new IR::Property(
        "as_atcam"_cs,
        new IR::Annotations({new IR::Annotation(IR::Annotation::hiddenAnnotation, {})}),
        new IR::ExpressionValue(new IR::BoolLiteral(true)), false));

    properties->push_back(new IR::Property(
        "as_alpm"_cs,
        new IR::Annotations({new IR::Annotation(IR::Annotation::hiddenAnnotation, {})}),
        new IR::ExpressionValue(new IR::BoolLiteral(true)), false));

    properties->push_back(new IR::Property(
        "atcam_partition_count"_cs,
        new IR::Annotations({new IR::Annotation(IR::Annotation::hiddenAnnotation, {})}),
        new IR::ExpressionValue(new IR::Constant(partition_count)), false));

    properties->push_back(new IR::Property(
        "atcam_subtrees_per_partition"_cs,
        new IR::Annotations({new IR::Annotation(IR::Annotation::hiddenAnnotation, {})}),
        new IR::ExpressionValue(new IR::Constant(subtrees_per_partition)), false));

    if (atcam_subset_width != -1) {
        properties->push_back(new IR::Property(
            "atcam_subset_width"_cs,
            new IR::Annotations({new IR::Annotation(IR::Annotation::hiddenAnnotation, {})}),
            new IR::ExpressionValue(new IR::Constant(atcam_subset_width)), false));
    }

    if (shift_granularity != -1) {
        properties->push_back(new IR::Property(
            "shift_granularity"_cs,
            new IR::Annotations({new IR::Annotation(IR::Annotation::hiddenAnnotation, {})}),
            new IR::ExpressionValue(new IR::Constant(shift_granularity)), false));
    }

    // Used a list of lists to save the excluded_field_msb_bits due to the limitation in
    // P4 frontend that does not support 'map' as table properties.
    if (fields_to_exclude.size() != 0) {
        IR::Vector<IR::Expression> property;
        for (auto f : fields_to_exclude) {
            IR::Vector<IR::Expression> entry;
            entry.push_back(new IR::StringLiteral(f.first));
            entry.push_back(new IR::Constant(f.second));
            auto elem = new IR::ListExpression(entry);
            property.push_back(elem);
        }
        properties->push_back(new IR::Property(
            "excluded_field_msb_bits",
            new IR::Annotations({new IR::Annotation(IR::Annotation::hiddenAnnotation, {})}),
            new IR::ExpressionValue(new IR::ListExpression(property)), false));
    }

    auto table_property = new IR::TableProperties(*properties);
    auto table = new IR::P4Table(tbl->srcInfo, IR::ID(tbl->name), tbl->annotations, table_property);
    return table;
}

const IR::P4Table *SplitAlpm::create_preclassifier_table(const IR::P4Table *tbl,
                                                         unsigned number_entries,
                                                         unsigned number_actions,
                                                         unsigned partition_index_bits,
                                                         unsigned subtrees_per_partition) {
    auto properties = new IR::IndexedVector<IR::Property>;

    // create key
    IR::Vector<IR::KeyElement> keys;
    for (auto f : tbl->getKey()->keyElements) {
        if (f->matchType->path->name == "ternary")
            error(ErrorType::ERR_UNSUPPORTED, "Ternary match key %1% not supported in table %2%",
                  f->expression, tbl->name);
        if (f->matchType->path->name == "lpm") {
            auto k =
                new IR::KeyElement(f->annotations, f->expression, new IR::PathExpression("lpm"));
            keys.push_back(k);
        } else {
            keys.push_back(f);
        }
    }
    properties->push_back(new IR::Property("key", new IR::Key(keys), false));

    // create action
    IR::IndexedVector<IR::ActionListElement> action_list;
    for (unsigned n = 0; n < number_actions; n++) {
        auto name = tbl->name + "_set_partition_index_" + cstring::to_cstring(n);
        action_list.push_back(new IR::ActionListElement(
            new IR::MethodCallExpression(new IR::PathExpression(IR::ID(name)), {})));
    }

    properties->push_back(new IR::Property("actions", new IR::ActionList(action_list), false));

    // Assume that the preclassifier always has an catch-all entry to set the
    // default partition index to 0.
    if (number_actions != 0) {
        auto act = new IR::PathExpression(IR::ID(tbl->name + "_set_partition_index_0"));
        auto args = new IR::Vector<IR::Argument>();
        args->push_back(
            new IR::Argument(new IR::Constant(IR::Type::Bits::get(partition_index_bits), 0)));
        if (number_actions > 1 && subtrees_per_partition > 1) {
            auto subtree_id_bits = ::ceil_log2(subtrees_per_partition);
            args->push_back(
                new IR::Argument(new IR::Constant(IR::Type::Bits::get(subtree_id_bits), 0)));
        }
        auto methodCall = new IR::MethodCallExpression(act, args);
        auto prop = new IR::Property(IR::ID(IR::TableProperties::defaultActionPropertyName),
                                     new IR::ExpressionValue(methodCall),
                                     /* isConstant = */ false);
        properties->push_back(prop);
    }

    // size field is not used
    properties->push_back(
        new IR::Property("size", new IR::ExpressionValue(tbl->getSizeProperty()), false));

    properties->push_back(new IR::Property(
        "alpm_preclassifier",
        new IR::Annotations({new IR::Annotation(IR::Annotation::hiddenAnnotation, {})}),
        new IR::ExpressionValue(new IR::BoolLiteral(true)), false));

    properties->push_back(new IR::Property(
        "alpm_preclassifier_number_entries",
        new IR::Annotations({new IR::Annotation(IR::Annotation::hiddenAnnotation, {})}),
        new IR::ExpressionValue(new IR::Constant(number_entries)), false));

    if (auto prop = tbl->properties->getProperty("requires_versioning"_cs)) {
        properties->push_back(prop);
    }

    auto table_property = new IR::TableProperties(*properties);
    auto name = tbl->name + "__alpm_preclassifier";
    auto table = new IR::P4Table(tbl->srcInfo, IR::ID(name), tbl->annotations, table_property);
    alpm_tables.insert(tbl->name);
    preclassifier_tables.emplace(tbl->name, name);

    return table;
}

bool SplitAlpm::values_through_pragmas(const IR::P4Table *tbl, int &number_partitions,
                                       int &number_subtrees_per_partition) {
    auto annot = tbl->getAnnotations();
    if (auto s = annot->getSingle(ALGORITHMIC_LPM_PARTITIONS)) {
        ERROR_CHECK(s->expr.size() > 0,
                    "%s: Please provide a valid %s "
                    "for table %s",
                    tbl->srcInfo, ALGORITHMIC_LPM_PARTITIONS, tbl->name);
        auto pragma_val = s->expr.at(0)->to<IR::Constant>();
        ERROR_CHECK(pragma_val != nullptr,
                    "%s: Please provide a valid %s "
                    "for table %s",
                    tbl->srcInfo, ALGORITHMIC_LPM_PARTITIONS, tbl->name);

        auto alg_lpm_partitions_value = static_cast<unsigned>(pragma_val->value);
        if (valid_partition_values.find(alg_lpm_partitions_value) != valid_partition_values.end()) {
            number_partitions = static_cast<int>(pragma_val->value);
        } else {
            error(
                "Unsupported %s value of %s for table %s."
                "\n  Allowed values are 1024, 2048, 4096, and 8192.",
                ALGORITHMIC_LPM_PARTITIONS, pragma_val->value, tbl->name);
        }
    }

    if (auto s = annot->getSingle(ALGORITHMIC_LPM_SUBTREES_PER_PARTITION)) {
        ERROR_CHECK(s->expr.size() > 0,
                    "%s: Please provide a valid %s "
                    "for table %s",
                    tbl->srcInfo, ALGORITHMIC_LPM_SUBTREES_PER_PARTITION, tbl->name);
        auto pragma_val = s->expr.at(0)->to<IR::Constant>();
        ERROR_CHECK(pragma_val != nullptr,
                    "%s: Please provide a valid %s "
                    "for table %s",
                    tbl->srcInfo, ALGORITHMIC_LPM_SUBTREES_PER_PARTITION, tbl->name);

        auto alg_lpm_subtrees_value = static_cast<int>(pragma_val->value);
        if (alg_lpm_subtrees_value <= 10) {
            number_subtrees_per_partition = alg_lpm_subtrees_value;
        } else {
            error(
                "Unsupported %s value of %s for table %s."
                "\n  Allowed values are in the range [1:10].",
                ALGORITHMIC_LPM_SUBTREES_PER_PARTITION, pragma_val->value, tbl->name);
        }
    }

    return errorCount() == 0;
}

bool SplitAlpm::values_through_impl(const IR::P4Table *tbl, int &number_partitions,
                                    int &number_subtrees_per_partition, int &atcam_subset_width,
                                    int &shift_granularity) {
    using BFN::getExternInstanceFromPropertyByTypeName;

    bool found_in_implementation = false;

    auto extract_alpm_config_from_property = [&](std::optional<P4::ExternInstance> &instance) {
        bool argHasName = false;
        for (auto arg : *instance->arguments) {
            cstring argName = arg->name.name;
            argHasName |= !argName.isNullOrEmpty();
        }
        // p4c requires all or none arguments to have names
        if (argHasName) {
            for (auto arg : *instance->arguments) {
                cstring argName = arg->name.name;
                if (argName == "number_partitions")
                    number_partitions = arg->expression->to<IR::Constant>()->asInt();
                else if (argName == "subtrees_per_partition")
                    number_subtrees_per_partition = arg->expression->to<IR::Constant>()->asInt();
                else if (argName == "atcam_subset_width")
                    atcam_subset_width = arg->expression->to<IR::Constant>()->asInt();
                else if (argName == "shift_granularity")
                    shift_granularity = arg->expression->to<IR::Constant>()->asInt();
            }
        } else {
            if (instance->arguments->size() > 0) {
                number_partitions =
                    instance->arguments->at(0)->expression->to<IR::Constant>()->asInt();
            } else if (instance->arguments->size() > 1) {
                number_subtrees_per_partition =
                    instance->arguments->at(1)->expression->to<IR::Constant>()->asInt();
            } else if (instance->arguments->size() > 2) {
                atcam_subset_width =
                    instance->arguments->at(2)->expression->to<IR::Constant>()->asInt();
            } else if (instance->arguments->size() > 3) {
                shift_granularity =
                    instance->arguments->at(3)->expression->to<IR::Constant>()->asInt();
            }
        }
    };

    // get Alpm instance from table implementation property
    auto instance = getExternInstanceFromPropertyByTypeName(tbl, "implementation"_cs, "Alpm"_cs,
                                                            refMap, typeMap);
    if (instance) {
        found_in_implementation = true;
        extract_alpm_config_from_property(instance);
        return true;
    }

    // for backward compatibility, also check the 'alpm' table property
    auto alpm = getExternInstanceFromProperty(tbl, "alpm"_cs, refMap, typeMap);
    if (alpm == std::nullopt) return false;
    if (alpm->type->name != "Alpm") {
        error("Unexpected extern %1% on 'alpm' property, only ALPM is allowed", alpm->type->name);
        return false;
    }
    if (found_in_implementation) {
        warning(
            "Alpm already found on 'implementation' table property,"
            " ignored the 'alpm' property");
        return false;
    }
    extract_alpm_config_from_property(alpm);
    return true;
}

bool SplitAlpm::pragma_exclude_msbs(const IR::P4Table *tbl,
                                    ordered_map<cstring, int> &fields_to_exclude) {
    ordered_map<cstring, int> field_name_to_width;
    ordered_set<cstring> field_name_is_lpm;
    ordered_set<cstring> field_name_is_partition_index;
    ordered_map<cstring, int> field_name_cnt;

    auto analyze_key_name_and_type = [&](const IR::KeyElement *k, cstring fname) {
        field_name_to_width.emplace(fname, k->expression->type->width_bits());
        if (k->matchType->path->name == "lpm") field_name_is_lpm.insert(fname);
        if (k->matchType->path->name == "atcam_partition_index")
            field_name_is_partition_index.insert(fname);
        field_name_cnt[fname] += 1;
    };

    for (auto k : tbl->getKey()->keyElements) {
        auto fname = k->expression->toString();
        analyze_key_name_and_type(k, fname);
        // alternatively, if user refers to the name in @name annotation
        // that also works. This is needed for programs translated from p4-14.
        auto fname_annot = k->getAnnotation("name"_cs);
        if (fname_annot != nullptr) {
            auto fname = fname_annot->expr.at(0)->to<IR::StringLiteral>()->value;
            // if annotation use a different name than the original field.
            if (field_name_to_width.count(fname) == 0) {
                analyze_key_name_and_type(k, fname);
            }
        }
    }
    auto annot = tbl->getAnnotations();
    for (auto an : annot->annotations) {
        if (an->name != ALGORITHMIC_LPM_ATCAM_EXCLUDE_FIELD_MSBS) continue;
        if (an->expr.size() != 1 && an->expr.size() != 2) {
            error(
                "Invalid %s pragma on table %s.\n Expected field name "
                "and optional msb bits %s",
                ALGORITHMIC_LPM_ATCAM_EXCLUDE_FIELD_MSBS, tbl->name, an->expr);
        }
        cstring fname;
        if (auto annot = an->expr.at(0)->to<IR::StringLiteral>()) {
            fname = annot->value;
        } else {
            fname = an->expr.at(0)->toString();
        }
        if (field_name_to_width.find(fname) == field_name_to_width.end()) {
            error("Invalid %s pragma on table %s.\n Field %s is not part of the table key.",
                  ALGORITHMIC_LPM_ATCAM_EXCLUDE_FIELD_MSBS, tbl->name, fname);
        } else if (field_name_is_partition_index.count(fname) != 0) {
            error(
                "Invalid %s pragma on table %s.\n Pragma cannot be used on "
                "the table's partition index '%s'.",
                ALGORITHMIC_LPM_ATCAM_EXCLUDE_FIELD_MSBS, tbl->name, fname);
        } else if (field_name_cnt.at(fname) != 1) {
            error(
                "Invalid %s pragma on table %s.\n Pragma cannot be used when "
                "different slices of field '%s' are used.",
                ALGORITHMIC_LPM_ATCAM_EXCLUDE_FIELD_MSBS, tbl->name, fname);
        }
        std::stringstream additional;
        bool msb_error = false;
        if (an->expr.size() == 2) {
            auto msb_bits_to_exclude = an->expr.at(1)->to<IR::Constant>()->value;
            if (msb_bits_to_exclude <= 0) {
                msb_error = true;
            } else if (msb_bits_to_exclude > field_name_to_width.at(fname)) {
                msb_error = true;
                additional << "Bits specified is larger than the field.";
            } else if (field_name_is_lpm.count(fname) > 0 &&
                       msb_bits_to_exclude >= field_name_to_width.at(fname)) {
                msb_error = true;
                additional << "The field with match type 'lpm' cannot be completely excluded.";
            }
            fields_to_exclude.emplace(fname, msb_bits_to_exclude);
        } else {
            auto msb_bits_to_exclude = field_name_to_width.at(fname);
            if (field_name_is_lpm.count(fname) > 0) {
                msb_error = true;
                additional << "The field with match type 'lpm' cannot be completely excluded.";
            }
            fields_to_exclude.emplace(fname, msb_bits_to_exclude);
        }
        if (msb_error) {
            error(
                "Invalid %s pragma on table %s.\n "
                "  Invalid most significant bits to exclude value of '%s'.\n"
                "%s",
                ALGORITHMIC_LPM_ATCAM_EXCLUDE_FIELD_MSBS, tbl->name, an->expr[0], additional.str());
        }
    }

    return errorCount() == 0;
}

const IR::Node *SplitAlpm::postorder(IR::P4Table *tbl) {
    if (alpm_info->alpm_table.count(tbl->name) == 0) return tbl;

    int number_partitions = 1024;
    int number_subtrees_per_partition = 2;
    int atcam_subset_width = -1;
    int shift_granularity = -1;
    int lpm_key_width = -1;

    auto lpm_cnt = 0;
    const IR::Expression *lpm_key = nullptr;
    for (auto f : tbl->getKey()->keyElements) {
        if (f->matchType->path->name == "lpm") {
            lpm_cnt += 1;
            lpm_key = f->expression;
            lpm_key_width = f->expression->type->width_bits();
        }
    }
    CHECK_NULL(lpm_key);
    ERROR_CHECK(lpm_cnt == 1,
                "To use algorithmic lpm, exactly one field in the match key "
                "must have a match type of lpm.  Table '%s' has %d.",
                tbl->name, lpm_cnt);

    if (!values_through_impl(tbl, number_partitions, number_subtrees_per_partition,
                             atcam_subset_width, shift_granularity) &&
        !values_through_pragmas(tbl, number_partitions, number_subtrees_per_partition))
        return tbl;

    auto partition_index_bits = ::ceil_log2(number_partitions);
    auto number_entries = number_partitions * number_subtrees_per_partition;

    ordered_map<cstring, int> fields_to_exclude;
    if (!pragma_exclude_msbs(tbl, fields_to_exclude)) return tbl;

    // @alpm_atcam_exclude_field_msbs pragma and
    // Alpm( ..., ..., atcam_subset_width, shift_granularity) are
    // mutually exclusive features.
    bool used_pragma_exclude_msb_bits = (fields_to_exclude.size() != 0);
    bool used_alpm_subset_width_opt = (atcam_subset_width != -1 || shift_granularity != -1);
    ERROR_CHECK(!(used_pragma_exclude_msb_bits && used_alpm_subset_width_opt),
                "@alpm_atcam_exclude_field_msbs cannot be applied to alpm table %s"
                " if 'atcam_subset_width' and 'shift_granularity' are specified on"
                " the Alpm extern",
                tbl->name);

    if (atcam_subset_width == -1) atcam_subset_width = lpm_key_width;

    if (shift_granularity == -1) shift_granularity = lpm_key_width;

    LOG3("atcam_subset_width " << atcam_subset_width << " shift_granularity " << shift_granularity);
    ERROR_CHECK(atcam_subset_width % shift_granularity == 0,
                "To use algorithmic lpm, "
                "atcam_subset_width must be an integer multiple of shift_granularity.");
    auto number_actions = (lpm_key_width - atcam_subset_width) / shift_granularity + 1;

    auto decls = new IR::IndexedVector<IR::Declaration>();
    decls->append(*create_temp_var(tbl, number_actions, partition_index_bits, atcam_subset_width,
                                   number_subtrees_per_partition, lpm_key));

    auto classifier_actions =
        create_preclassifer_actions(tbl, number_actions, partition_index_bits, atcam_subset_width,
                                    shift_granularity, number_subtrees_per_partition, lpm_key);
    decls->append(*classifier_actions);

    // create tcam table
    auto classifier_table = create_preclassifier_table(
        tbl, number_entries, number_actions, partition_index_bits, number_subtrees_per_partition);
    decls->push_back(classifier_table);

    // create atcam
    auto atcam_table =
        create_atcam_table(tbl, number_partitions, number_subtrees_per_partition, number_actions,
                           atcam_subset_width, shift_granularity, fields_to_exclude, lpm_key);
    decls->push_back(atcam_table);

    return decls;
}

const IR::Node *SplitAlpm::postorder(IR::MethodCallStatement *node) {
    auto mi = P4::MethodInstance::resolve(node, refMap, typeMap);
    if (!mi->is<P4::ApplyMethod>() || !mi->to<P4::ApplyMethod>()->isTableApply()) return node;
    if (auto tbl = mi->object->to<IR::P4Table>()) {
        if (alpm_info->alpm_table.count(tbl->name) == 0) return node;
        auto stmts = new IR::IndexedVector<IR::StatOrDecl>();
        stmts->push_back(new IR::MethodCallStatement(new IR::MethodCallExpression(
            new IR::Member(new IR::PathExpression(IR::ID(tbl->name + "__alpm_preclassifier")),
                           IR::ID("apply")),
            {})));
        stmts->push_back(node);
        auto result = new IR::BlockStatement(*stmts);
        return result;
    }
    return node;
}

const IR::Node *SplitAlpm::postorder(IR::IfStatement *c) {
    HasTableApply hta(refMap, typeMap);
    (void)c->condition->apply(hta);
    if (hta.table == nullptr) return c;
    if (alpm_info->alpm_table.count(hta.table->name) == 0) return c;
    auto stmts = new IR::BlockStatement();
    stmts->push_back(new IR::MethodCallStatement(new IR::MethodCallExpression(
        new IR::Member(new IR::PathExpression(IR::ID(hta.table->name + "__alpm_preclassifier")),
                       IR::ID("apply")),
        {})));
    stmts->push_back(c);
    return stmts;
}

const IR::Node *SplitAlpm::postorder(IR::SwitchStatement *statement) {
    auto expr = statement->expression;
    auto member = expr->to<IR::Member>();
    if (member->member != "action_run") return statement;
    if (!member->expr->is<IR::MethodCallExpression>()) return statement;
    auto mce = member->expr->to<IR::MethodCallExpression>();
    auto mi = P4::MethodInstance::resolve(mce, refMap, typeMap);
    auto gen_apply = [](cstring table) {
        IR::Statement *expr = new IR::MethodCallStatement(new IR::MethodCallExpression(
            new IR::Member(new IR::PathExpression(table), IR::ID(IR::IApply::applyMethodName))));
        return expr;
    };

    if (auto apply = mi->to<P4::ApplyMethod>()) {
        if (!apply->isTableApply()) return statement;
        auto table = apply->object->to<IR::P4Table>();
        if (alpm_tables.count(table->name) == 0) return statement;
        // switch(t0.apply()) {} is converted to
        //
        // t0_preclassifier.apply();
        // switch(t0.apply()) {}
        //
        auto tableName = apply->object->getName().name;
        if (preclassifier_tables.count(tableName) == 0) {
            error("Unable to find member table %1%", tableName);
            return statement;
        }
        auto preclassifier_table = preclassifier_tables.at(tableName);
        auto preclassifier_apply = gen_apply(preclassifier_table);

        auto decls = new IR::IndexedVector<IR::StatOrDecl>();
        decls->push_back(preclassifier_apply);
        decls->push_back(statement);
        return new IR::BlockStatement(*decls);
    }
    return statement;
}

AlpmImplementation::AlpmImplementation(P4::ReferenceMap *refMap, P4::TypeMap *typeMap) {
    auto collectAlpmInfo = new CollectAlpmInfo(refMap, typeMap);
    addPasses({
        collectAlpmInfo,
        new SplitAlpm(collectAlpmInfo, refMap, typeMap),
    });
}

void CollectAlpmInfo::postorder(const IR::P4Table *tbl) {
    // Alpm is identified with either extern or pragma
    using BFN::getExternInstanceFromProperty;
    using BFN::getExternInstanceFromPropertyByTypeName;

    auto instance = getExternInstanceFromPropertyByTypeName(tbl, "implementation"_cs, "Alpm"_cs,
                                                            refMap, typeMap);
    if (instance) alpm_table.insert(tbl->name);

    // for backward compatibility, also check the 'alpm' property
    auto alpm = getExternInstanceFromProperty(tbl, "alpm"_cs, refMap, typeMap);
    if (alpm != std::nullopt) {
        warning(ErrorType::WARN_DEPRECATED,
                "table property 'alpm' is deprecated,"
                " use 'implementation' instead.");
        alpm_table.insert(tbl->name);
    }

    // support @alpm(1) or @alpm(true)
    auto annot = tbl->getAnnotations();
    if (auto s = annot->getSingle(cstring(PragmaAlpm::name))) {
        ERROR_CHECK(s->expr.size() > 0,
                    "%s: Please provide a valid alpm "
                    "for table %s",
                    tbl->srcInfo, tbl->name);
        if (auto pragma_val = s->expr.at(0)->to<IR::Constant>()) {
            if (pragma_val->asInt()) alpm_table.insert(tbl->name);
        } else if (auto pragma_val = s->expr.at(0)->to<IR::BoolLiteral>()) {
            if (pragma_val->value) alpm_table.insert(tbl->name);
        } else {
            error("%s: Please provide a valid alpm for table %s", tbl->srcInfo, tbl->name);
        }
    }

    // check if alpm table contains ternary match, ternary is not supported.
    if (alpm_table.count(tbl->name) != 0) {
        for (auto key : tbl->getKey()->keyElements) {
            if (key->matchType->path->toString() == "ternary") {
                error(ErrorType::ERR_UNSUPPORTED,
                      "ternary match %s in ALPM table %s is not supported", key->expression,
                      tbl->name);
                break;
            }
        }
    }
}

}  // namespace BFN
