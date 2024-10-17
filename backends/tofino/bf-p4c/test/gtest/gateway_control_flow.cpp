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

#include <optional>

#include "gtest/gtest.h"

#include "ir/ir.h"
#include "lib/cstring.h"
#include "lib/error.h"
#include "test/gtest/helpers.h"
#include "tofino_gtest_utils.h"
#include "bf-p4c/ir/gateway_control_flow.h"
#include "bf-p4c/mau/gateway.h"
namespace P4::Test {

class GatewayControlFlowTest : public TofinoBackendTest {};

namespace {

std::optional<TofinoPipeTestCase>
createGatewayControlFlowTestCase(const std::string &ingress_source) {
    auto pre_ingress = P4_SOURCE(tna_header(), R"(

extern void probe(int v);

header H1 { bit<8> f1; }
header H2 { bit<8> f2; }
header H3 { bit<8> f3; }
header H4 { bit<8> f4; }
header H5 { bit<8> f5; }
header H6 { bit<8> f6; }

struct Headers { H1 h1; H2 h2; H3 h3; H4 h4; H5 h5; H6 h6; }
struct Metadata { }

parser ig_parse(packet_in packet, out Headers headers, out Metadata meta,
    out ingress_intrinsic_metadata_t ig_intr_md) {
    state start {
        packet.extract(headers.h1);
        transition select(headers.h1.f1[0:0]) {
            0 : tryh3;
            1 : h2; } }
    state h2 {
        packet.extract(headers.h2);
        transition tryh3; }
    state tryh3 {
        transition select(headers.h1.f1[1:1]) {
            0 : tryh4;
            1 : h3; } }
    state h3 {
        packet.extract(headers.h3);
        transition tryh4; }
    state tryh4 {
        transition select(headers.h1.f1[2:2]) {
            0 : tryh5;
            1 : h4; } }
    state h4 {
        packet.extract(headers.h4);
        transition tryh5; }
    state tryh5 {
        transition select(headers.h1.f1[3:3]) {
            0 : tryh6;
            1 : h5; } }
    state h5 {
        packet.extract(headers.h5);
        transition tryh6; }
    state tryh6 {
        transition select(headers.h1.f1[4:4]) {
            0 : accept;
            1 : h6; } }
    state h6 {
        packet.extract(headers.h6);
        transition accept; }
}

control igrs(inout Headers headers, inout Metadata meta,
             in ingress_intrinsic_metadata_t ig_intr_md,
             in ingress_intrinsic_metadata_from_parser_t ig_intr_prsr_md,
             inout ingress_intrinsic_metadata_for_deparser_t ig_intr_dprs_md,
             inout ingress_intrinsic_metadata_for_tm_t ig_intr_tm_md) {
    )");

    auto post_ingress = P4_SOURCE(R"(
}

control ig_deparse(packet_out packet, inout Headers headers, in Metadata meta,
                   in ingress_intrinsic_metadata_for_deparser_t ig_intr_md_for_dprs) {
    apply { packet.emit(headers); }
}

parser eg_parse(packet_in packet, out Headers headers, out Metadata meta,
                    out egress_intrinsic_metadata_t eg_intr_md) {
    state start {
        packet.extract(eg_intr_md);
        transition accept;
    }
}

control egrs(inout Headers headers, inout Metadata meta,
             in egress_intrinsic_metadata_t eg_intr_md,
             in egress_intrinsic_metadata_from_parser_t eg_intr_md_from_prsr,
             inout egress_intrinsic_metadata_for_deparser_t eg_intr_md_for_dprs,
             inout egress_intrinsic_metadata_for_output_port_t eg_intr_md_for_oport) {
    apply {
    }
}

control eg_deparse(packet_out packet, inout Headers headers, in Metadata meta,
                   in egress_intrinsic_metadata_for_deparser_t eg_intr_md_for_dprs) {
    apply { }
}

Pipeline(ig_parse(), igrs(), ig_deparse(),
         eg_parse(), egrs(), eg_deparse()) pipe;
Switch(pipe) main;
    )");

    std::string source = pre_ingress + ingress_source + post_ingress;

    auto& options = BackendOptions();
    options.langVersion = CompilerOptions::FrontendVersion::P4_16;
    options.target = "tofino"_cs;
    options.arch = "tna"_cs;
    options.disable_parse_min_depth_limit = true;
    return TofinoPipeTestCase::createWithThreadLocalInstances(source);
}

class GCFTVisitor : public Inspector, public BFN::GatewayControlFlow, public TofinoWriteContext {
    struct global_t {
        std::map<cstring, int>  headers;
        std::vector<cstring>    idx2hdr;
        std::map<int, bitvec>   probes;
        int hdr2idx(cstring hdr) {
            if (headers.count(hdr)) return headers.at(hdr);
            headers[hdr] = idx2hdr.size();
            idx2hdr.push_back(hdr);
            return idx2hdr.size() - 1; }
    } *glob;
    bitvec live;  // headers possibly valid
    const IR::MAU::Table *visiting = nullptr;
    std::map<cstring, std::map<int, bool>>      tag_live_info;

    GCFTVisitor *clone() const override { return new GCFTVisitor(*this); }
    void flow_merge(Visitor &a_) override {
        auto &a = dynamic_cast<GCFTVisitor &>(a_);
        BUG_CHECK(glob == a.glob, "flow_merge of non-cloned GCFTVisitor");
        live |= a.live;
    }
    void flow_copy(::ControlFlowVisitor &a_) override {
        auto &a = dynamic_cast<GCFTVisitor &>(a_);
        BUG_CHECK(glob == a.glob, "flow_copy of non-cloned GCFTVisitor");
        live = a.live;
    }

    bool preorder(const IR::MAU::Primitive *prim) {
        if (prim->name == "probe") {
            int i = prim->operands.at(0)->to<IR::Constant>()->asInt();
            BUG_CHECK(glob->probes.count(i) == 0, "duplicate probe %d", i);
            glob->probes[i] = live; }
        return false; }

    int writeVal() const {
        if (auto *ext = findContext<IR::BFN::Extract>()) {
            auto *src = ext->source->to<IR::BFN::ConstantRVal>();
            BUG_CHECK(src, "extract to $valid is not a constant");
            return src->constant->to<IR::Constant>()->asInt();
        } else if (auto *p = getParent<IR::MAU::Primitive>()) {
            BUG_CHECK(p->name == "modify_field" || p->name == "set",
                      "write to $valid is not simpe set");
            return p->operands.at(1)->to<IR::Constant>()->asInt();
        } else {
            BUG("write unexpected context");
        }
    }
    cstring headerName(const IR::Node *n) {
        cstring rv;
        if (auto *hr = n->to<IR::ConcreteHeaderRef>()) {
            return headerName(hr->ref);
        } else if (auto *hdr = n->to<IR::Header>()) {
            rv = hdr->name;
        } else if (auto *mem = n->to<IR::Member>()) {
            rv = mem->member;
        } else {
            BUG("not a header? %s", n); }
        if (auto *suffix = rv.findlast('.'))
            rv = cstring(suffix+1);
        return rv;
    }
    bool compareWith(const IR::Expression *e) {
        if (auto *rel = getParent<IR::Operation::Relation>()) {
            auto *v = rel->left == e ? rel->right : rel->right == e ? rel->left : 0;
            BUG_CHECK(v, "parent(%s) is not parent of child(%s)?", rel, e);
            bool rv = v->to<IR::Constant>()->asInt() != 0;
            if (rel->is<IR::Equ>()) {
                return rv;
            } else if (rel->is<IR::Neq>()) {
                return !rv;
            }
        }
        BUG("(%s) is not == or !=", e);
    }

    bool preorder(const IR::Member *mem) override {
        if (mem->member != "$valid") return false;
        cstring header = headerName(mem->expr);
        int idx = glob->hdr2idx(header);
        if (isWrite()) {
            live[idx] = writeVal();
        } else if (isRead()) {
            cstring tag;
            if (auto *tbl = gateway_context(tag)) {
                if (visiting != tbl) {
                    visiting = tbl;
                    tag_live_info.clear(); }
                bool isLive = compareWith(mem);
                auto prev_tags = gateway_earlier_tags();
                if (!prev_tags.count(tag))
                    tag_live_info[tag][idx] = isLive;
                for (auto lt : gateway_later_tags()) {
                    if (lt != tag && !prev_tags.count(lt))
                        tag_live_info[lt][idx] = !isLive; }
            } else {
                // read not in a gateway -- perhaps a copy_header?
            }
        } else {
            BUG("$valid is neither read nor written?"); }
        return false;
    }

    void pre_visit_table_next(const IR::MAU::Table *tbl, cstring tag) override {
        if (tbl != visiting) {
            visiting = nullptr;
            tag_live_info.clear();
        } else if (tag_live_info.count(tag)) {
            for (auto h : tag_live_info.at(tag))
                live[h.first] = h.second; }
    }

 public:
    GCFTVisitor() : glob(new global_t) {}
    bool probe_live(int p, cstring hdr) {
        BUG_CHECK(glob->headers.count(hdr), "header %s not seen", hdr);
        BUG_CHECK(glob->probes.count(p), "probe %s not seen", p);
        return glob->probes.at(p)[glob->headers.at(hdr)]; }
};

}  // namespace

TEST_F(GatewayControlFlowTest, Basic) {
    auto program = createGatewayControlFlowTestCase(
        P4_SOURCE(R"(
    apply {
        probe(0);
        if (headers.h2.isValid()) {
            probe(1);
        } else if (!headers.h3.isValid()) {
            probe(2);
        }
    }

    )"));
    ASSERT_TRUE(program);
    auto *pipe = program->pipe->apply(CanonGatewayExpr());

    // dump_notype(pipe->thread[0].parsers[0]);
    // dump_notype(pipe->thread[0].mau);

    GCFTVisitor test;
    pipe->apply(test);

    EXPECT_TRUE(test.probe_live(0, "h1"_cs));
    EXPECT_TRUE(test.probe_live(0, "h2"_cs));
    EXPECT_TRUE(test.probe_live(0, "h3"_cs));
    EXPECT_TRUE(test.probe_live(0, "h4"_cs));
    EXPECT_TRUE(test.probe_live(0, "h5"_cs));
    EXPECT_TRUE(test.probe_live(0, "h6"_cs));
    EXPECT_FALSE(test.probe_live(2, "h2"_cs));
    EXPECT_FALSE(test.probe_live(2, "h3"_cs));
}

}  // namespace P4::Test
