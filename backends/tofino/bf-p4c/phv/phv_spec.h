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

#ifndef BACKENDS_TOFINO_BF_P4C_PHV_PHV_SPEC_H_
#define BACKENDS_TOFINO_BF_P4C_PHV_PHV_SPEC_H_

#include <optional>
#include <vector>

#include "bf-p4c/phv/phv.h"
#include "lib/bitvec.h"
#include "lib/json.h"
#include "lib/ordered_map.h"

namespace P4 {
class cstring;
}

namespace P4 {
namespace IR {
class Annotation;
}  // end namespace IR
}  // end namespace P4

using namespace P4;

class PhvSpec {
 public:
    // We should reuse the definition from arch/arch.h, however, because of circular includes
    // (arch.h, phv_spec.h, device.h) that is not currently workable
    enum ArchBlockType_t { PARSER, MAU, DEPARSER };

    /// Represents a range of container addresses
    /// Containers are organized by blocks of blockSize. The addresses start at 'start'.
    /// The next block address is current block start + incr
    /// The address of a regiser within a block is start + ((index << shl) >> shr)
    struct RangeSpec {
        unsigned start;
        unsigned blocks;
        unsigned blockSize;
        unsigned incr;
        unsigned shl;
        unsigned shr;
    };
    using AddressSpec = std::map<PHV::Type, RangeSpec>;

 protected:
    // All cache fields
    mutable bitvec physical_containers_i;
    mutable std::map<PHV::Size, std::vector<bitvec>> mau_groups_i;
    mutable bitvec ingress_only_containers_i;
    mutable bitvec egress_only_containers_i;
    mutable std::vector<bitvec> tagalong_collections_i;
    mutable bitvec individually_assigned_containers_i;

    /// All types of containers supported by the device.
    std::vector<PHV::Type> definedTypes;
    /// All sizes of containers supported by the device.
    std::set<PHV::Size> definedSizes;
    /// All kinds of containers supported by the device.
    std::set<PHV::Kind> definedKinds;

    ordered_map<PHV::Type, unsigned> typeIdMap;

    std::map<PHV::Size, std::set<PHV::Type>> sizeToTypeMap;

    /// A single MAU group description. Members include the number of these groups in a device and a
    /// map from the type of container to number of containers in each group.
    struct MauGroupType {
        unsigned numGroups;  // Number of MAU groups
        std::map<PHV::Type, unsigned> types;
        // Type and number of containers in each group

        explicit MauGroupType(unsigned n, std::map<PHV::Type, unsigned> t)
            : numGroups(n), types(t) {}

        explicit MauGroupType(unsigned n) : numGroups(n) {}

        MauGroupType() : numGroups(0) {}

        void addType(PHV::Type t, unsigned n) { types.emplace(t, n); }
    };

    /**
     * Describes PHV groups for MAU in the device.
     * E.g. if a device has two types of MAU groups, corresponding to 32-bit and 8-bit containers,
     * and each type of group has 16 normal PHV containers of a given size, the mauGroupSpec will
     * be:
     * { { PHV::Size::b8, MauGroupType(4, { PHV::Type::B, 16 }) },
     *   { PHV::Size::b32, MauGroupType(4, { PHV::Type::W, 16 }) } }.
     */
    std::map<PHV::Size, MauGroupType> mauGroupSpec;

    /** Number of containers in a single MAU group.
     */
    unsigned containersPerGroup = 0;

    std::map<PHV::Size, std::vector<unsigned>> ingressOnlyMauGroupIds;

    std::map<PHV::Size, std::vector<unsigned>> egressOnlyMauGroupIds;

    std::map<PHV::Type, unsigned> tagalongCollectionSpec;

    unsigned numTagalongCollections = 0;

    std::map<PHV::Type, unsigned> deparserGroupSize;

    std::map<PHV::Type, std::pair<unsigned, unsigned>> deparserGroupSpec;

    unsigned numPovBits = 0;

    /// Add a PHV container type to the set of types which are available on this
    /// device. This should only be called inside the constructor of subclasses
    /// of PhvSpec; after a PhvSpec instance is constructed, its list of
    /// container types is considered immutable.
    void addType(PHV::Type t);

    /// Return the number of containers in an MAU group.
    unsigned getContainersPerGroup(
        const std::map<PHV::Size, unsigned> &numContainersPerGroup) const;

 public:
    /// @return the PHV container types available on this device.
    const std::vector<PHV::Type> &containerTypes() const;

    /// @return the PHV container sizes available on this device.
    const std::set<PHV::Size> &containerSizes() const;

    /// @return the PHV container kinds available on this device.
    const std::set<PHV::Kind> &containerKinds() const;

    /// Determines whether the device has the given kind of PHV container.
    bool hasContainerKind(PHV::Kind kind) const;

    /// @return the map from PHV groups to the types supported by that group.
    const std::map<PHV::Size, std::set<PHV::Type>> groupsToTypes() const;

    /// @return the number of PHV container types available on this device.
    /// Behaves the same as `containerTypes().size()`.
    unsigned numContainerTypes() const;

    /// @returns the number of containers in a single MAU group. Assumption currently is that each
    /// group has the same number of containers (applicable to both Tofino and Tofino2).
    unsigned numContainersInGroup() const { return containersPerGroup; }

    /// @return the PHV container type corresponding to the given numeric id.
    /// Different devices may map the same id to different types. Behaves the
    /// as `containerTypes()[id]`.
    PHV::Type idToContainerType(unsigned id) const;

    /// @return a numeric id that uniquely specifies the given PHV container
    /// type on this device. Different devices may map the same type to
    /// different ids. This behaves the same as using `std::find_if` to find the
    /// index of the given type in `containerTypes()`.
    unsigned containerTypeToId(PHV::Type type) const;

    /// @return the PHV container corresponding to the given numeric id on this
    /// device. Different devices have different PHV containers, so ids are not
    /// consistent between devices.
    PHV::Container idToContainer(unsigned id) const;

    /// @return a numeric id that uniquely specifies the given PHV container on
    /// this device. Different devices have different PHV containers, so ids are
    /// not consistent between devices.
    unsigned containerToId(PHV::Container container) const;

    /// Filters a set of containers for a single container kind or type.
    bitvec filterContainerSet(const bitvec &set, PHV::Kind kind) const;
    bitvec filterContainerSet(const bitvec &set, PHV::Type type) const;

    /// @return a string representation of the provided @p set of containers.
    cstring containerSetToString(const bitvec &set) const;

    /** The JBay parser treats the PHV as 256 x 16b containers, where each
     * extractor can write to the upper/lower/both 8b segments of each 16b
     * container.  MAU, on the other hand, views PHV as groups of 8b, 16b, and
     * 32b containers.
     *
     * As a result, if an even/odd pair of 8b PHV containers holds extracted
     * fields, then they need to be assigned to the same thread.
     *
     * @return the ids of every container in the same parser group as the
     * provided container.
     */
    virtual bitvec parserGroup(unsigned id) const = 0;

    /** Does the device have extract groups where all extracts to the group must be of the same
     * source type (e.g., packet vs non-packet)?
     */
    virtual bool hasParserExtractGroups() const = 0;

    /**
     *
     * @return the ids of every container in the same parser group as the
     * provided container.
     */
    virtual bitvec parserExtractGroup(unsigned id) const = 0;

    /// @return the Parser group id of the container
    virtual unsigned parserGroupId(const PHV::Container &c) const = 0;

    /// @return the ids of every container in the same deparser group as the
    /// provided container.
    bitvec deparserGroup(unsigned id) const;

    /// @return the Deparser group id of the container
    virtual unsigned deparserGroupId(const PHV::Container &c) const = 0;

    /**
     * Generates a bitvec containing a range of containers. This kind of bitvec
     * can be used to implement efficient set operations on large numbers of
     * containers.
     *
     * To generate the range [B10, B16), use `range(Kind::B, 10, 6)`.
     *
     * @param t The type of container.
     * @param start The index of first container in the range.
     * @param length The number of containers in the range. May be zero.
     */
    bitvec range(PHV::Type t, unsigned start, unsigned length) const;

    /// @return containers that constraint to either ingress or egress
    bitvec ingressOrEgressOnlyContainers(
        const std::map<PHV::Size, std::vector<unsigned>> &gressOnlyMauGroupIds) const;

    /// @return a bitvec of the containers which are hard-wired to ingress.
    const bitvec &ingressOnly() const;

    /// @return a bitvec of the containers which are hard-wired to egress.
    const bitvec &egressOnly() const;

    /// @return MAU groups of a given size @p sz.
    const std::vector<bitvec> &mauGroups(PHV::Size sz) const;

    /// @return MAU groups of all types
    const std::map<PHV::Size, std::vector<bitvec>> &mauGroups() const;

    /// @return the ids of every container in the MAU group of @p container_id, or
    /// std::nullopt if @p container_id is not part of any MAU group.
    bitvec mauGroup(unsigned container_id) const;

    /// @return the MAU group id for the container
    virtual unsigned mauGroupId(const PHV::Container &c) const = 0;

    /// @return a pair <em><\#groups, \#containers per group></em> corresponding to the
    /// PHV Type @p t
    const std::pair<int, int> mauGroupNumAndSize(const PHV::Type t) const;

    /// @return a pair <em><\#groups, \#containers per group></em> corresponding
    /// to the PHV Type @p t.
    const std::pair<int, int> deparserGroupNumAndSize(const PHV::Type t) const;

    /// @return the ID of tagalong collection that the container belongs to
    unsigned getTagalongCollectionId(PHV::Container c) const;

    const std::map<PHV::Type, unsigned> getTagalongCollectionSpec() const {
        return tagalongCollectionSpec;
    }

    unsigned getNumTagalongCollections() const { return numTagalongCollections; }

    /// @return a bitvec of available tagalong collections.
    const std::vector<bitvec> &tagalongCollections() const;

    /// @return the ids of every container in the tagalong collection of @p container_id, or
    /// std::nullopt if @p container_id is not part of any collection.
    bitvec tagalongCollection(unsigned container_id) const;

    /// @return the number of POV bits available
    unsigned getNumPovBits() const { return numPovBits; }

    /// @return the ids of containers that can be assigned to a thread
    /// individually.
    virtual const bitvec &individuallyAssignedContainers() const = 0;

    /// @return the ids of all containers which actually exist on the Tofino
    /// hardware - i.e., all non-overflow containers.
    const bitvec &physicalContainers() const;

    /// @return the target-specific address of @p container_id, for the specified interface
    /// in the pipeline: PARSER, MAU, DEPARSER.
    virtual unsigned physicalAddress(unsigned container_id, ArchBlockType_t interface) const = 0;

    /// @return the target-specific address of @p c, for the specified interface
    /// in the pipeline: PARSER, MAU, DEPARSER.
    unsigned physicalAddress(const PHV::Container &c, ArchBlockType_t interface) const;

    /// @return the target-specific address specification for the specified interface
    virtual AddressSpec &physicalAddressSpec(ArchBlockType_t interface) const = 0;

    /// @return the target-specific container of @p address, for the specified interface
    /// in the pipeline: PARSER, MAU, DEPARSER.
    std::optional<PHV::Container> physicalAddressToContainer(unsigned address,
                                                             ArchBlockType_t interface) const;

    /// apply global pragmas to cached info about available PHV containers
    void applyGlobalPragmas(const std::vector<const IR::Annotation *> &global_pragmas) const;
};

class TofinoPhvSpec : public PhvSpec {
 public:
    TofinoPhvSpec();

    /// @see PhvSpec::parserGroup(unsigned id).
    bitvec parserGroup(unsigned id) const override;

    /// @see PhvSpec::hasParserExtractGroups().
    bool hasParserExtractGroups() const override;

    /// @see PhvSpec::parserExtractGroup(unsigned id).
    bitvec parserExtractGroup(unsigned id) const override;

    /// @see PhvSpec::parserGroupId(const PHV::Container &)
    unsigned parserGroupId(const PHV::Container &c) const override;

    /// @see PhvSpec::mauGroupId(const PHV::Container &)
    unsigned mauGroupId(const PHV::Container &c) const override;

    /// @see PhvSpec::deparserGroupId(const PHV::Container &)
    unsigned deparserGroupId(const PHV::Container &c) const override;

    const bitvec &individuallyAssignedContainers() const override;

    /// @see PhvSpec::physicalAddress(unsigned container_id, BFN::ArchBlockType interface).
    /// For Tofino all interfaces are the same.
    unsigned physicalAddress(unsigned container_id, ArchBlockType_t /*interface*/) const override;

    /// @see PhvSpec::physicalAddressSpec(ArchBlockType_t)
    /// For Tofino all interfaces are the same
    AddressSpec &physicalAddressSpec(ArchBlockType_t /* interface */) const override {
        return _physicalAddresses;
    }

 private:
    static AddressSpec _physicalAddresses;
};

class JBayPhvSpec : public PhvSpec {
 public:
    JBayPhvSpec();

    /// @see PhvSpec::parserGroup(unsigned id).
    bitvec parserGroup(unsigned id) const override;

    /// @see PhvSpec::hasParserExtractGroups().
    bool hasParserExtractGroups() const override;

    /// @see PhvSpec::parserExtractGroup(unsigned id).
    bitvec parserExtractGroup(unsigned id) const override;

    /// @see PhvSpec::parserGroupId(const PHV::Container &)
    unsigned parserGroupId(const PHV::Container &c) const override;

    /// @see PhvSpec::mauGroupId(const PHV::Container &)
    unsigned mauGroupId(const PHV::Container &c) const override;

    /// @see PhvSpec::deparserGroupId(const PHV::Container &)
    unsigned deparserGroupId(const PHV::Container &c) const override;

    const bitvec &individuallyAssignedContainers() const override;

    /// @see PhvSpec::physicalAddress(unsigned container_id, BFN::ArchBlockType interface).
    /// For JBay, PARSER/DEPARSER have normal and mocha, MAU has additional darks
    unsigned physicalAddress(unsigned container_id, ArchBlockType_t interface) const override;

    /// @see PhvSpec::physicalAddressSpec(ArchBlockType_t)
    AddressSpec &physicalAddressSpec(ArchBlockType_t interface) const override {
        switch (interface) {
            case PhvSpec::MAU:
                return _physicalMauAddresses;
            case PhvSpec::PARSER:
                return _physicalParserAddresses;
            case PhvSpec::DEPARSER:
                return _physicalDeparserAddresses;
            default:
                BUG("Invalid interface");
        }
    }

 private:
    static AddressSpec _physicalMauAddresses;
    static AddressSpec _physicalParserAddresses;
    static AddressSpec _physicalDeparserAddresses;
};

#endif /* BACKENDS_TOFINO_BF_P4C_PHV_PHV_SPEC_H_ */
