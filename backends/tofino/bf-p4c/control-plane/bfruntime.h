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

#ifndef BACKENDS_TOFINO_BF_P4C_CONTROL_PLANE_BFRUNTIME_H_
#define BACKENDS_TOFINO_BF_P4C_CONTROL_PLANE_BFRUNTIME_H_

#include <algorithm>
#include <iomanip>
#include <iosfwd>
#include <iterator>
#include <limits>
#include <optional>
#include <ostream>
#include <regex>
#include <sstream>
#include <string>

#include "control-plane/p4RuntimeSerializer.h"
#include "lib/big_int_util.h"
#include "lib/error.h"
#include "lib/exceptions.h"
#include "lib/json.h"
#include "lib/log.h"
#include "lib/null.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wpedantic"
#include "p4/config/v1/p4info.pb.h"
#include "p4/config/v1/p4types.pb.h"
#pragma GCC diagnostic pop

using namespace P4;
using namespace P4::literals;

namespace p4configv1 = ::p4::config::v1;

namespace P4 {
struct P4RuntimeAPI;
}  // namespace P4

namespace BFN {

namespace BFRT {

using P4Id = uint32_t;

// Helpers
template <typename T, typename R>
static constexpr P4Id makeBFRuntimeId(T base, R prefix) {
    return static_cast<P4Id>((base & 0xffffff) | (prefix << 24));
}

static constexpr P4Id getIdPrefix(P4Id id) { return ((id >> 24) & 0xff); }

Util::JsonObject *findJsonTable(Util::JsonArray *tablesJson, cstring tblName);

// See https://stackoverflow.com/a/33799784/4538702
static std::string escapeJson(const std::string &s) {
    std::ostringstream o;
    for (char c : s) {
        switch (c) {
            case '"':
                o << "\\\"";
                break;
            case '\\':
                o << "\\\\";
                break;
            case '\b':
                o << "\\b";
                break;
            case '\f':
                o << "\\f";
                break;
            case '\n':
                o << "\\n";
                break;
            case '\r':
                o << "\\r";
                break;
            case '\t':
                o << "\\t";
                break;
            default: {
                if ('\x00' <= c && c <= '\x1f') {
                    o << "\\u" << std::hex << std::setw(4) << std::setfill('0')
                      << static_cast<int>(c);
                } else {
                    o << c;
                }
            }
        }
    }
    return o.str();
}

static Util::JsonObject *transformAnnotation(const std::string &annotation) {
    auto *annotationJson = new Util::JsonObject();
    // TODO: annotation string will need to be parsed so we can have it
    // in key/value format here.
    annotationJson->emplace("name"_cs, escapeJson(annotation));
    return annotationJson;
}

template <typename It>
static Util::JsonArray *transformAnnotations(const It &first, const It &last) {
    auto *annotations = new Util::JsonArray();
    for (auto it = first; it != last; it++) annotations->append(transformAnnotation(*it));
    return annotations;
}

static Util::JsonArray *transformAnnotations(const p4configv1::Preamble &pre) {
    return transformAnnotations(pre.annotations().begin(), pre.annotations().end());
}

/// @returns true if @id's prefix matches the provided PSA @prefix value
template <typename T>
static bool isOfType(P4Id id, T prefix) {
    return getIdPrefix(id) == static_cast<P4Id>(prefix);
}

namespace Standard {

template <typename It>
static auto findP4InfoObject(const It &first, const It &last, P4Id objectId) -> const
    typename std::iterator_traits<It>::value_type * {
    using T = typename std::iterator_traits<It>::value_type;
    auto desiredObject = std::find_if(
        first, last, [&](const T &object) { return object.preamble().id() == objectId; });
    if (desiredObject == last) return nullptr;
    return &*desiredObject;
}

static const p4configv1::Table *findTable(const p4configv1::P4Info &p4info, P4Id tableId) {
    const auto &tables = p4info.tables();
    return findP4InfoObject(tables.begin(), tables.end(), tableId);
}

const p4configv1::ActionProfile *findActionProf(const p4configv1::P4Info &p4info,
                                                P4Id actionProfId);

const p4configv1::DirectCounter *findDirectCounter(const p4configv1::P4Info &p4info,
                                                   P4Id counterId);

const p4configv1::DirectMeter *findDirectMeter(const p4configv1::P4Info &p4info, P4Id meterId);

}  // namespace Standard

Util::JsonObject *makeTypeInt(cstring type, cstring mask = ""_cs);

template <typename T, typename std::enable_if<std::is_integral<T>::value, int>::type = 0>
static Util::JsonObject *makeTypeInt(cstring type, T defaultValue, cstring mask = ""_cs) {
    auto *typeObj = new Util::JsonObject();
    typeObj->emplace("type"_cs, type);
    typeObj->emplace("default_value"_cs, defaultValue);
    if (!mask.isNullOrEmpty()) typeObj->emplace("mask"_cs, mask);
    return typeObj;
}

Util::JsonObject *makeTypeBool(std::optional<bool> defaultValue = std::nullopt);

Util::JsonObject *makeTypeBytes(int width, std::optional<int64_t> defaultValue = std::nullopt,
                                cstring mask = ""_cs);

Util::JsonObject *makeTypeEnum(const std::vector<cstring> &choices,
                               std::optional<cstring> defaultValue = std::nullopt);

void addSingleton(Util::JsonArray *dataJson, Util::JsonObject *dataField, bool mandatory,
                  bool readOnly);

template <typename It>
static std::vector<P4Id> collectTableIds(const p4configv1::P4Info &p4info, const It &first,
                                         const It &last) {
    std::vector<P4Id> tableIds;
    for (auto it = first; it != last; it++) {
        auto *table = Standard::findTable(p4info, *it);
        if (table == nullptr) {
            error("Invalid table id '%1%'", *it);
            continue;
        }
        tableIds.push_back(*it);
    }
    return tableIds;
}

/// Takes a simple P4Info P4DataTypeSpec message in its factory method and
/// flattens it into a vector of BF-RT "fields" which can be used as key fields
/// or data fields. The class provides iterators.
class TypeSpecParser {
 public:
    struct Field {
        cstring name;
        P4Id id;
        Util::JsonObject *type;
    };

    using Fields = std::vector<Field>;
    using iterator = Fields::iterator;
    using const_iterator = Fields::const_iterator;

    static TypeSpecParser make(const p4configv1::P4Info &p4info,
                               const p4configv1::P4DataTypeSpec &typeSpec, cstring instanceType,
                               cstring instanceName,
                               const std::vector<cstring> *fieldNames = nullptr,
                               cstring prefix = ""_cs, cstring suffix = ""_cs, P4Id idOffset = 1);

    iterator begin() { return fields.begin(); }
    const_iterator cbegin() { return fields.cbegin(); }
    iterator end() { return fields.end(); }
    const_iterator cend() { return fields.cend(); }

 private:
    explicit TypeSpecParser(Fields &&fields) : fields(std::move(fields)) {}

    Fields fields;
};

class BFRuntimeGenerator {
 public:
    explicit BFRuntimeGenerator(const p4configv1::P4Info &p4info) : p4info(p4info) {}

    /// Generates the schema as a Json object for the provided P4Info instance.
    virtual const Util::JsonObject *genSchema() const;

    /// Generates BF-RT JSON schema from P4Info protobuf message and writes it to
    /// the @p destination stream.
    void serializeBFRuntimeSchema(std::ostream *destination);

 protected:
    // To avoid potential clashes with P4 names, we prefix the names of "fixed"
    // data field with a '$'. For example, for TD_DATA_ACTION_MEMBER_ID, we
    // use the name $ACTION_MEMBER_ID.
    enum TDDataFieldIds : P4Id {
        // ids for fixed data fields must not collide with the auto-generated
        // ids for P4 fields (e.g. match key fields).
        TD_DATA_START = (1 << 16),

        TD_DATA_MATCH_PRIORITY,

        TD_DATA_ACTION,
        TD_DATA_ACTION_MEMBER_ID,
        TD_DATA_SELECTOR_GROUP_ID,
        TD_DATA_ACTION_MEMBER_STATUS,
        TD_DATA_MAX_GROUP_SIZE,

        TD_DATA_ENTRY_TTL,
        TD_DATA_ENTRY_HIT_STATE,

        TD_DATA_METER_SPEC_CIR_KBPS,
        TD_DATA_METER_SPEC_PIR_KBPS,
        TD_DATA_METER_SPEC_CBS_KBITS,
        TD_DATA_METER_SPEC_PBS_KBITS,

        TD_DATA_METER_SPEC_CIR_PPS,
        TD_DATA_METER_SPEC_PIR_PPS,
        TD_DATA_METER_SPEC_CBS_PKTS,
        TD_DATA_METER_SPEC_PBS_PKTS,

        TD_DATA_COUNTER_SPEC_BYTES,
        TD_DATA_COUNTER_SPEC_PKTS,

        TD_DATA_METER_INDEX,
        TD_DATA_COUNTER_INDEX,
        TD_DATA_REGISTER_INDEX,

        TD_DATA_END,
    };

    /// Common counter representation between PSA and other architectures
    struct Counter {
        enum Unit { UNSPECIFIED = 0, BYTES = 1, PACKETS = 2, BOTH = 3 };
        std::string name;
        P4Id id;
        int64_t size;
        Unit unit;
        Util::JsonArray *annotations;

        static std::optional<Counter> from(const p4configv1::Counter &counterInstance);
        static std::optional<Counter> fromDirect(const p4configv1::DirectCounter &counterInstance);
    };

    /// Common meter representation between PSA and other architectures
    struct Meter {
        enum Unit { UNSPECIFIED = 0, BYTES = 1, PACKETS = 2 };
        enum Type { COLOR_UNAWARE = 0, COLOR_AWARE = 1 };
        std::string name;
        P4Id id;
        int64_t size;
        Unit unit;
        Type type;
        Util::JsonArray *annotations;

        static std::optional<Meter> from(const p4configv1::Meter &meterInstance);
        static std::optional<Meter> fromDirect(const p4configv1::DirectMeter &meterInstance);
    };

    /// Common action profile / selector representation between PSA and other
    /// architectures
    struct ActionProf {
        std::string name;
        P4Id id;
        int64_t size;
        std::vector<P4Id> tableIds;
        Util::JsonArray *annotations;
        static P4Id makeActProfId(P4Id implementationId);
        static std::optional<ActionProf> from(const p4configv1::P4Info &p4info,
                                              const p4configv1::ActionProfile &actionProfile);
    };

    /// Common digest representation between PSA and other architectures
    struct Digest {
        std::string name;
        P4Id id;
        p4configv1::P4DataTypeSpec typeSpec;
        Util::JsonArray *annotations;

        static std::optional<Digest> from(const p4configv1::Digest &digest);
    };

    /// Common register representation between PSA and other architectures
    struct Register {
        std::string name;
        std::string dataFieldName;
        P4Id id;
        int64_t size;
        p4configv1::P4DataTypeSpec typeSpec;
        Util::JsonArray *annotations;

        // static std::optional<Register>
        // from(const p4configv1::ExternInstance& externInstance);
    };

    void addMatchTables(Util::JsonArray *tablesJson) const;
    virtual void addActionProfs(Util::JsonArray *tablesJson) const;
    virtual bool addActionProfIds(const p4configv1::Table &table,
                                  Util::JsonObject *tableJson) const;
    void addCounters(Util::JsonArray *tablesJson) const;
    void addMeters(Util::JsonArray *tablesJson) const;

    void addCounterCommon(Util::JsonArray *tablesJson, const Counter &counter) const;
    void addMeterCommon(Util::JsonArray *tablesJson, const Meter &meter) const;
    void addRegisterCommon(Util::JsonArray *tablesJson, const Register &meter) const;
    void addActionProfCommon(Util::JsonArray *tablesJson, const ActionProf &actionProf) const;
    void addLearnFilters(Util::JsonArray *learnFiltersJson) const;
    void addLearnFilterCommon(Util::JsonArray *learnFiltersJson, const Digest &digest) const;
    virtual void addDirectResources(const p4configv1::Table &table, Util::JsonArray *dataJson,
                                    Util::JsonArray *operationsJson,
                                    Util::JsonArray *attributesJson,
                                    P4Id maxActionParamId = 0) const;

    virtual std::optional<bool> actProfHasSelector(P4Id actProfId) const;
    /// Generates the JSON array for table action specs. When the function
    /// returns normally and @p maxActionParamId is not NULL, @p maxActionParamId is
    /// set to the maximum assigned id for an action parameter across all
    /// actions. This is useful if other table data fields (e.g. direct register
    /// fields) need to receive a distinct id in the same space.
    Util::JsonArray *makeActionSpecs(const p4configv1::Table &table,
                                     P4Id *maxActionParamId = nullptr) const;
    virtual std::optional<Counter> getDirectCounter(P4Id counterId) const;
    virtual std::optional<Meter> getDirectMeter(P4Id meterId) const;

    /// Transforms a P4Info @p typeSpec to a list of JSON objects matching the
    /// BF-RT format. @p instanceType and @p instanceName are used for logging error
    /// messages. This method currenty only supports fixed-width bitstrings as
    /// well as non-nested structs and tuples. All field names are prefixed with
    /// @p prefix and suffixed with @p suffix. Field ids are assigned incrementally
    /// starting at @p idOffset. Field names are taken from the P4 program
    /// when possible; for tuples they are autogenerated (f1, f2, ...) unless
    /// supplied through @p fieldNames.
    void transformTypeSpecToDataFields(Util::JsonArray *fieldsJson,
                                       const p4configv1::P4DataTypeSpec &typeSpec,
                                       cstring instanceType, cstring instanceName,
                                       const std::vector<cstring> *fieldNames = nullptr,
                                       cstring prefix = ""_cs, cstring suffix = ""_cs,
                                       P4Id idOffset = 1) const;

    static void addMeterDataFields(Util::JsonArray *dataJson, const Meter &meter);
    static Util::JsonObject *makeCommonDataField(P4Id id, cstring name, Util::JsonObject *type,
                                                 bool repeated,
                                                 Util::JsonArray *annotations = nullptr);

    static Util::JsonObject *makeContainerDataField(P4Id id, cstring name, Util::JsonArray *items,
                                                    bool repeated,
                                                    Util::JsonArray *annotations = nullptr);

    static void addActionDataField(Util::JsonArray *dataJson, P4Id id, const std::string &name,
                                   bool mandatory, bool read_only, Util::JsonObject *type,
                                   Util::JsonArray *annotations = nullptr);

    static void addKeyField(Util::JsonArray *dataJson, P4Id id, cstring name, bool mandatory,
                            cstring matchType, Util::JsonObject *type,
                            Util::JsonArray *annotations = nullptr);

    static void addCounterDataFields(Util::JsonArray *dataJson, const Counter &counter);

    static Util::JsonObject *initTableJson(const std::string &name, P4Id id, cstring tableType,
                                           int64_t size, Util::JsonArray *annotations = nullptr);

    static void addToDependsOn(Util::JsonObject *tableJson, P4Id id);

    /// Add register data fields to the JSON data array for a BFRT table. Field
    /// ids are assigned incrementally starting at @p idOffset, which is 1 by
    /// default.
    void addRegisterDataFields(Util::JsonArray *dataJson, const Register &register_,
                               P4Id idOffset = 1) const;

    const p4configv1::P4Info &p4info;
};

}  // namespace BFRT
}  // namespace BFN

#endif /* BACKENDS_TOFINO_BF_P4C_CONTROL_PLANE_BFRUNTIME_H_ */
