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

#include <sstream>

#include "gtest/gtest.h"
#include "lib/bitvec.h"
#include "test/gtest/helpers.h"

#include "bf-p4c/device.h"
#include "bf-p4c/ir/gress.h"
#include "bf-p4c/phv/allocate_phv.h"
#include "bf-p4c/phv/slicing/phv_slicing_split.h"
#include "bf-p4c/phv/utils/utils.h"
#include "bf-p4c/test/gtest/tofino_gtest_utils.h"

namespace P4::Test {

class TofinoBitvec : public TofinoBackendTest {};

class TofinoPhvCrush : public TofinoBackendTest {};

/// Make a SuperCluster with slice lists (@lists), with each slice in each
/// list in its own RotationalCluster.
static PHV::SuperCluster* make_sc(ordered_set<PHV::SuperCluster::SliceList*> lists) {
    ordered_set<const PHV::RotationalCluster*> clusters;
    for (auto* list : lists) {
        for (auto slice : *list) {
            auto aligned = new PHV::AlignedCluster(PHV::Kind::normal,
                                                   std::vector<PHV::FieldSlice>({ slice }));
            auto rotational =
                new PHV::RotationalCluster(ordered_set<PHV::AlignedCluster*>({ aligned }));
            clusters.insert(rotational); } }
    return new PHV::SuperCluster(clusters, lists);
}

/// Make a SuperCluster with a single slice list (@list), with each slice in
/// @list in its own RotationalCluster.
static PHV::SuperCluster* make_sc(PHV::SuperCluster::SliceList* slices) {
    return make_sc(ordered_set<PHV::SuperCluster::SliceList*>({ slices }));
}

/// Make a SuperCluster with a single slice list holding @slice and a single
/// RotationalCluster holding @slice.
static PHV::SuperCluster* make_sc(PHV::FieldSlice slice) {
    return make_sc(new PHV::SuperCluster::SliceList({ slice }));
}

TEST_F(TofinoPhvCrush, sliceSuperCluster) {
    ordered_map<int, const PHV::FieldSlice*> slices_by_size;
    for (int i : { 7, 8, 9, 16, 24 }) {
        auto* f = new PHV::Field();
        std::stringstream ss;
        f->id = i;
        f->size = i;
        ss << "f" << f->id;
        f->name = ss.str();
        f->gress = INGRESS;
        f->offset = 0;
        f->metadata = false;
        f->bridged = false;
        f->pov = false;
        f->validContainerRange_i = ZeroToMax();
        f->alignment = std::nullopt;
        f->set_exact_containers(true);
        slices_by_size[i] = new PHV::FieldSlice(f, StartLen(0, f->size)); }
    auto* f16 = slices_by_size.at(16)->field();
    auto* f24 = slices_by_size.at(24)->field();
    ordered_map<PHV::SuperCluster::SliceList*, bitvec> schemas;
    auto* list = new PHV::SuperCluster::SliceList({ *slices_by_size.at(16) });

    // Test SuperCluster equality.
    auto* list2 = new PHV::SuperCluster::SliceList({ *slices_by_size.at(16) });
    EXPECT_EQ(*make_sc(list), *make_sc(list2));

    // Test 16b -> 8b, 8b slicing.
    schemas.clear();
    schemas[list] = bitvec(8, 1);
    auto res = PHV::Slicing::split(make_sc(list), schemas);
#if !(__GNUC__ == 4 && __GNUC_MINOR__ == 9)
    // Comparison with std::optional triggers an undefined reference
    // for basic_stream with GCC 4.9 !!!
    EXPECT_NE(std::nullopt, res);
#endif
    EXPECT_EQ(2U, res->size());
    EXPECT_EQ(*res->front(), *make_sc(PHV::FieldSlice(f16, StartLen(0, 8))));
    EXPECT_EQ(*res->back(), *make_sc(PHV::FieldSlice(f16, StartLen(8, 8))));

    // Test 24b -> 16b, 8b slicing.
    schemas.clear();
    list = new PHV::SuperCluster::SliceList({ *slices_by_size.at(24) });
    schemas[list] = bitvec(16, 1);
    res = PHV::Slicing::split(make_sc(list), schemas);
#if !(__GNUC__ == 4 && __GNUC_MINOR__ == 9)
    // Comparison with std::optional triggers an undefined reference
    // for basic_stream with GCC 4.9 !!!
    EXPECT_NE(std::nullopt, res);
#endif
    EXPECT_EQ(2U, res->size());
    EXPECT_EQ(*res->front(), *make_sc(PHV::FieldSlice(f24, StartLen(0, 16))));
    EXPECT_EQ(*res->back(), *make_sc(PHV::FieldSlice(f24, StartLen(16, 8))));

    // Test 24b, 16b -> 8b x 5
    schemas.clear();
    schemas[list] = bitvec();
    schemas[list].setbit(8);
    schemas[list].setbit(16);
    schemas[list2] = bitvec(8, 1);
    res = PHV::Slicing::split(make_sc({ list, list2 }), schemas);
#if !(__GNUC__ == 4 && __GNUC_MINOR__ == 9)
    // Comparison with std::optional triggers an undefined reference
    // for basic_stream with GCC 4.9 !!!
    EXPECT_NE(std::nullopt, res);
#endif
    EXPECT_EQ(5U, res->size());
    EXPECT_EQ(*res->front(), *make_sc(PHV::FieldSlice(f24, StartLen(0, 8))));
    EXPECT_EQ(*res->back(), *make_sc(PHV::FieldSlice(f16, StartLen(8, 8))));
}

TEST_F(TofinoPhvCrush, clusterAlignment) {
    // TODO: This just tests the first bit of the valid bits, not all
    // valid bits.
    using FieldData = struct {
        int field_size;
        std::optional<int> relativeAlignment;     // little Endian
        nw_bitrange validContainerRange;
    };
    using TestData = struct {
        // std::nullopt implies result is an empty bitvec
        std::optional<int> result;
        PHV::Size container_size;
        std::vector<FieldData> fields;
    };

    std::vector<TestData> tests = {
        // No constraints
        { 0, PHV::Size::b8, { { 8, std::nullopt, ZeroToMax() } } },
        { 0, PHV::Size::b8, { { 8, std::nullopt, ZeroToMax() },
                              { 8, std::nullopt, ZeroToMax() } } },

        // Relative alignment only
        { 0, PHV::Size::b8,  { { 8, 0, ZeroToMax() } } },
        { 0, PHV::Size::b8,  { { 8, 0, ZeroToMax() },
                               { 8, 0, ZeroToMax() } } },
        { 4, PHV::Size::b16, { { 8, 4, ZeroToMax() } } },
        { 4, PHV::Size::b16, { { 8, 4, ZeroToMax() },
                               { 8, 4, ZeroToMax() } } },
        { 4, PHV::Size::b16, { { 8, std::nullopt, ZeroToMax() },
                               { 8, 4, ZeroToMax() } } },

        // validContainerStartRange only
        { 0, PHV::Size::b16, { { 8, std::nullopt, StartLen(0, 16) } } },
        { 0, PHV::Size::b16, { { 8, std::nullopt, StartLen(0, 16) },
                               { 8, std::nullopt, StartLen(0, 16) } } },
        { 0, PHV::Size::b16, { { 8, std::nullopt, StartLen(0, 32) } } },
        { 0, PHV::Size::b16, { { 8, std::nullopt, StartLen(0, 32) },
                               { 8, std::nullopt, StartLen(0, 32) } } },
        { 4, PHV::Size::b16, { { 8, std::nullopt, StartLen(0, 12) } } },
        { 4, PHV::Size::b16, { { 8, std::nullopt, StartLen(0, 12) },
                               { 8, std::nullopt, StartLen(0, 13) } } },

        // Both relative alignment and validContainerStartRange
        { 0, PHV::Size::b16, { { 8, 0, StartLen(0, 16) } } },
        { 0, PHV::Size::b16, { { 8, 0, StartLen(0, 16) },
                               { 8, 0, StartLen(0, 16) } } },
        { 4, PHV::Size::b16, { { 8, 4, StartLen(0, 12) } } },
        { 4, PHV::Size::b16, { { 8, 4, StartLen(0, 12) },
                               { 8, 4, StartLen(0, 13) } } },
    };

    int field_id = 0;
    for (auto& test : tests) {
        std::vector<PHV::FieldSlice> slices;
        for (auto& fdata : test.fields) {
            auto* f = new PHV::Field();
            std::stringstream ss;
            f->id = field_id++;
            f->size = int(fdata.field_size);
            ss << "f" << f->id;
            f->name = ss.str();
            f->gress = INGRESS;
            f->offset = 0;
            f->metadata = true;
            f->bridged = false;
            f->pov = false;
            f->validContainerRange_i = fdata.validContainerRange;
            if (fdata.relativeAlignment)
                f->alignment =
                    FieldAlignment(le_bitrange(StartLen(*fdata.relativeAlignment,
                                                        int(test.container_size))));
            else
                f->alignment = std::nullopt;
            slices.push_back(PHV::FieldSlice(f));
        }

        PHV::AlignedCluster cl(PHV::Kind::normal, slices);
        if (test.result)
            EXPECT_EQ(*test.result, *(cl.validContainerStart(test.container_size).min()));
        else
            EXPECT_TRUE(cl.validContainerStart(test.container_size).empty());
    }
}

TEST_F(TofinoPhvCrush, makeDeviceAllocation) {
    const PhvSpec& phvSpec = Device::phvSpec();
    PhvInfo phv;
    PhvUse uses(phv);
    PHV::ConcreteAllocation alloc(phv, uses);

    // Check that all physical containers are accounted for and unallocated.
    for (auto cid : phvSpec.physicalContainers()) {
        auto c = phvSpec.idToContainer(cid);
        EXPECT_EQ(0U, alloc.slices(c).size()); }

    // Check that ONLY physical containers are present.
    for (auto kv : alloc) {
        auto cid = phvSpec.containerToId(kv.first);
        EXPECT_TRUE(phvSpec.physicalContainers()[cid]); }

    // Check that all hard-wired gress has been set.
    for (auto cid : phvSpec.ingressOnly()) {
        auto c = phvSpec.idToContainer(cid);
        EXPECT_EQ(INGRESS, alloc.gress(c)); }
    for (auto cid : phvSpec.egressOnly()) {
        auto c = phvSpec.idToContainer(cid);
        EXPECT_EQ(EGRESS, alloc.gress(c)); }
}

TEST_F(TofinoPhvCrush, Transaction) {
    const PhvSpec& phvSpec = Device::phvSpec();
    PhvInfo phv;
    PhvUse uses(phv);
    PHV::ConcreteAllocation alloc(phv, uses);

    EXPECT_NE(phvSpec.ingressOnly() | phvSpec.egressOnly(), phvSpec.physicalContainers());

    bitvec mauGroup0 = phvSpec.mauGroups(PHV::Size::b8)[2];
    bitvec depGroup0 = phvSpec.deparserGroup(*mauGroup0.min());
    EXPECT_NE(-1, depGroup0.min());
    EXPECT_NE(*depGroup0.min(), *depGroup0.max());
    EXPECT_EQ(bitvec(), (phvSpec.ingressOnly() | phvSpec.egressOnly()) & depGroup0);

    bitvec mauGroup1 = phvSpec.mauGroups(PHV::Size::b8)[3];
    bitvec depGroup1 = phvSpec.deparserGroup(*mauGroup1.min());
    EXPECT_NE(-1, depGroup1.min());
    EXPECT_NE(*depGroup1.min(), *depGroup1.max());
    EXPECT_EQ(bitvec(), (phvSpec.ingressOnly() | phvSpec.egressOnly()) & depGroup1);

    EXPECT_NE(mauGroup0, mauGroup1);
    EXPECT_NE(*mauGroup0.min(), *mauGroup1.min());

    std::vector<PHV::Container> containers;
    for (auto cid : depGroup0)
        containers.push_back(phvSpec.idToContainer(cid));
    EXPECT_LT(1U, containers.size());

    PHV::Container c0 = containers[0];
    PHV::Container c1 = containers[1];
    PHV::Container c2 = containers[2];
    PHV::Container c3 = phvSpec.idToContainer(*depGroup1.min());

    PHV::Field f0;
    f0.id = 0;
    f0.size = 8;
    f0.name = "foo.bar"_cs;
    f0.gress = EGRESS;
    f0.offset = 0;
    f0.metadata = true;
    f0.bridged = false;
    f0.pov = false;
    PHV::AllocSlice s0(&f0, c0, 0, 0, 8);

    PHV::Field f1;
    f1.id = 1;
    f1.size = 8;
    f1.name = "foo.bar"_cs;
    f1.gress = INGRESS;
    f1.offset = 0;
    f1.metadata = true;
    f1.bridged = false;
    f1.pov = false;
    PHV::AllocSlice s1(&f1, c1, 0, 0, 8);

    uses.deparser_i[INGRESS][f1.id] = true;

    // Allocation is empty.
    EXPECT_EQ(std::nullopt, alloc.gress(c3));
    EXPECT_EQ(0U, alloc.slices(c3).size());
    EXPECT_EQ(0U, alloc.slicesByLiveness(c3).size());

    // Allocate one slice, setting INGRESS to deparser group.
    alloc.allocate(s1);

    EXPECT_EQ(INGRESS, alloc.gress(c1));
    EXPECT_EQ(std::nullopt, alloc.gress(c2));
    EXPECT_EQ(INGRESS, alloc.deparserGroupGress(c1));
    EXPECT_EQ(INGRESS, alloc.deparserGroupGress(c2));
    EXPECT_EQ(std::nullopt, alloc.gress(c3));
    EXPECT_EQ(std::nullopt, alloc.deparserGroupGress(c3));
    EXPECT_EQ(ordered_set<PHV::AllocSlice>({s1}), alloc.slices(c1));
    EXPECT_EQ(1U, alloc.slicesByLiveness(c1).size());
    EXPECT_EQ(ordered_set<PHV::AllocSlice>({s1}), alloc.slicesByLiveness(c1).back());
    EXPECT_EQ(0U, alloc.slices(c3).size());

    // Make a transaction, which should initially match (but not change) alloc.
    auto alloc_attempt = alloc.makeTransaction();

    EXPECT_EQ(INGRESS, alloc.gress(c1));
    EXPECT_EQ(INGRESS, alloc_attempt.gress(c1));
    EXPECT_EQ(std::nullopt, alloc_attempt.gress(c2));
    EXPECT_EQ(INGRESS, alloc.deparserGroupGress(c1));
    EXPECT_EQ(INGRESS, alloc_attempt.deparserGroupGress(c1));
    EXPECT_EQ(INGRESS, alloc_attempt.deparserGroupGress(c2));
    EXPECT_EQ(ordered_set<PHV::AllocSlice>({s1}), alloc.slices(c1));
    EXPECT_EQ(ordered_set<PHV::AllocSlice>({s1}), alloc_attempt.slices(c1));

    // Other containers (out of deparser group range) don't change.
    EXPECT_EQ(std::nullopt, alloc.gress(c3));
    EXPECT_EQ(std::nullopt, alloc_attempt.gress(c3));
    EXPECT_EQ(0U, alloc.slices(c3).size());
    EXPECT_EQ(0U, alloc_attempt.slices(c3).size());

    // Allocate s0, which is not deparsed, and hence should not change the
    // gress nor deparserGroupGress of c1 or c2.
    alloc_attempt.allocate(s0);

    EXPECT_EQ(EGRESS, alloc_attempt.gress(c0));
    EXPECT_EQ(INGRESS, alloc_attempt.gress(c1));
    EXPECT_EQ(std::nullopt, alloc_attempt.gress(c2));
    EXPECT_EQ(INGRESS, alloc_attempt.deparserGroupGress(c0));
    EXPECT_EQ(INGRESS, alloc_attempt.deparserGroupGress(c1));
    EXPECT_EQ(INGRESS, alloc_attempt.deparserGroupGress(c2));

    PHV::Field f2;
    f2.id = 2;
    f2.size = 8;
    f2.name = "foo.baz"_cs;
    f2.gress = INGRESS;
    f2.offset = 0;
    f2.metadata = true;
    f2.bridged = false;
    f2.pov = false;
    PHV::AllocSlice s2(&f2, c1, 0, 4, 4);
    PHV::AllocSlice s3(&f2, c2, 4, 4, 4);

    // Allocate to c2.  No change to gress, nor to alloc.
    alloc_attempt.allocate(s3);

    EXPECT_EQ(INGRESS, alloc.gress(c1));
    EXPECT_EQ(INGRESS, alloc_attempt.gress(c1));
    EXPECT_EQ(ordered_set<PHV::AllocSlice>({s1}), alloc.slices(c1));
    EXPECT_EQ(ordered_set<PHV::AllocSlice>({s1}), alloc_attempt.slices(c1));

    EXPECT_EQ(std::nullopt, alloc.gress(c2));
    EXPECT_EQ(INGRESS, alloc_attempt.gress(c2));
    EXPECT_EQ(ordered_set<PHV::AllocSlice>({ }), alloc.slices(c2));
    EXPECT_EQ(ordered_set<PHV::AllocSlice>({s3}), alloc_attempt.slices(c2));

    // Other containers (out of deparser group range) don't change.
    EXPECT_EQ(std::nullopt, alloc.gress(c3));
    EXPECT_EQ(std::nullopt, alloc_attempt.gress(c3));
    EXPECT_EQ(0U, alloc.slices(c3).size());
    EXPECT_EQ(0U, alloc_attempt.slices(c3).size());

    // Allocate another slice to c1.  No change to gress, nor to alloc.
    alloc_attempt.allocate(s2);

    EXPECT_EQ(INGRESS, alloc.gress(c1));
    EXPECT_EQ(INGRESS, alloc_attempt.gress(c1));
    EXPECT_EQ(ordered_set<PHV::AllocSlice>({s1}), alloc.slices(c1));
    EXPECT_EQ(ordered_set<PHV::AllocSlice>({s1, s2}), alloc_attempt.slices(c1));

    EXPECT_EQ(std::nullopt, alloc.gress(c2));
    EXPECT_EQ(INGRESS, alloc_attempt.gress(c2));
    EXPECT_EQ(ordered_set<PHV::AllocSlice>({ }), alloc.slices(c2));
    EXPECT_EQ(ordered_set<PHV::AllocSlice>({s3}), alloc_attempt.slices(c2));

    // Other containers (out of deparser group range) don't change.
    EXPECT_EQ(std::nullopt, alloc.gress(c3));
    EXPECT_EQ(std::nullopt, alloc_attempt.gress(c3));
    EXPECT_EQ(0U, alloc.slices(c3).size());
    EXPECT_EQ(0U, alloc_attempt.slices(c3).size());

    // Commit, changing alloc.  Removes slices from alloc_attempt, but as
    // they're placed in alloc, the change is unobservable.
    alloc.commit(alloc_attempt);

    EXPECT_EQ(INGRESS, alloc.gress(c1));
    EXPECT_EQ(INGRESS, alloc_attempt.gress(c1));
    EXPECT_EQ(ordered_set<PHV::AllocSlice>({s1, s2}), alloc.slices(c1));
    EXPECT_EQ(ordered_set<PHV::AllocSlice>({s1, s2}), alloc_attempt.slices(c1));

    EXPECT_EQ(1U, alloc.slicesByLiveness(c1).size());
    EXPECT_EQ(ordered_set<PHV::AllocSlice>({s1, s2}), alloc.slicesByLiveness(c1).back());

    EXPECT_EQ(INGRESS, alloc.gress(c2));
    EXPECT_EQ(INGRESS, alloc_attempt.gress(c2));
    EXPECT_EQ(ordered_set<PHV::AllocSlice>({s3}), alloc.slices(c2));
    EXPECT_EQ(ordered_set<PHV::AllocSlice>({s3}), alloc_attempt.slices(c2));

    // Other containers (out of deparser group range) don't change.
    EXPECT_EQ(std::nullopt, alloc.gress(c3));
    EXPECT_EQ(std::nullopt, alloc_attempt.gress(c3));
    EXPECT_EQ(0U, alloc.slices(c3).size());
    EXPECT_EQ(0U, alloc_attempt.slices(c3).size());
}


TEST_F(TofinoPhvCrush, slicesByLiveness) {
    const PhvSpec& phvSpec = Device::phvSpec();

    PhvInfo phv;
    PhvUse uses(phv);
    PHV::ConcreteAllocation alloc(phv, uses);

    phv.field_mutex()[1][2] = true;

    std::vector<PHV::Container> containers;
    for (auto cid : phvSpec.physicalContainers()) {
        if (phvSpec.ingressOnly()[cid] || phvSpec.egressOnly()[cid])
            continue;
        containers.push_back(phvSpec.idToContainer(cid)); }

    PHV::Container c1 = containers[0];

    PHV::Field f1;
    f1.id = 1;
    f1.size = 8;
    f1.name = "foo.bar"_cs;
    f1.gress = INGRESS;
    PHV::AllocSlice s1(&f1, c1, 0, 0, 8);

    PHV::Field f2;
    f2.id = 2;
    f2.size = 8;
    f2.name = "foo.baz"_cs;
    f2.gress = INGRESS;
    PHV::AllocSlice s2(&f2, c1, 0, 0, 8);

    // Allocate one slice, setting INGRESS to deparser group.
    alloc.allocate(s1);
    alloc.allocate(s2);

    EXPECT_NE(ordered_set<PHV::AllocSlice>({ s1, s2 }), ordered_set<PHV::AllocSlice>({ s1 }));
    EXPECT_EQ(ordered_set<PHV::AllocSlice>({ s1, s2 }), alloc.slices(c1));

    EXPECT_EQ(2U, alloc.slicesByLiveness(c1).size());
    EXPECT_EQ(ordered_set<PHV::AllocSlice>({s1}), alloc.slicesByLiveness(c1).front());
    EXPECT_EQ(ordered_set<PHV::AllocSlice>({s2}), alloc.slicesByLiveness(c1).back());
    EXPECT_EQ(std::vector<ordered_set<PHV::AllocSlice>>({ { s1 }, { s2 } }),
              alloc.slicesByLiveness(c1));
}

class JBayPhvCrush : public JBayBackendTest {};

TEST_F(JBayPhvCrush, makeDeviceAllocation) {
    const PhvSpec& phvSpec = Device::phvSpec();

    PhvInfo phv;
    PhvUse uses(phv);
    PHV::ConcreteAllocation alloc(phv, uses);

    // Check that all physical containers are accounted for and unallocated.
    for (auto cid : phvSpec.physicalContainers()) {
        auto c = phvSpec.idToContainer(cid);
        EXPECT_EQ(0U, alloc.slices(c).size()); }

    // Check that ONLY physical containers are present.
    for (auto kv : alloc) {
        auto cid = phvSpec.containerToId(kv.first);
        EXPECT_TRUE(phvSpec.physicalContainers()[cid]); }

    // Check that all hard-wired gress has been set.
    for (auto cid : phvSpec.ingressOnly()) {
        auto c = phvSpec.idToContainer(cid);
        EXPECT_EQ(INGRESS, alloc.gress(c)); }
    for (auto cid : phvSpec.egressOnly()) {
        auto c = phvSpec.idToContainer(cid);
        EXPECT_EQ(EGRESS, alloc.gress(c)); }
}

}  // namespace P4::Test
