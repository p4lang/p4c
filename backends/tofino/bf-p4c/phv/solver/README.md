# Action Phv Constraint Solvers
Action constraint solvers are used to validate PHV packing. 
When a candidate allocation list is proposed and passed to can_pack function in 
action phv constrait, a solver will be created to collect all move-based instructions 
for all actions related to fields in the candidate container. 
If both source and destination has been allocated, the solver will 
check and generate instruction based on the PHV allocation.

## Example of using Z3
an example of using z3 to validate deposit field. Unfortunately it's about 20ms per invocation,
which is too slow for us.
``` c++
boost::optional<Error> ActionMoveSolver::run_deposit_field_z3_solver(
    const ContainerID dest, const std::vector<Assign>& src1,
    const std::vector<Assign>& src2) const {
    const int width = specs_i.at(dest).size;
    BUG_CHECK(width <= 32, "container larger than 32-bit is not supported");
    const unsigned int all_ones_value = ((unsigned long long)1 << width) - 1;
    // pre-compute rot
    int right_shift_val = right_rotate_offset(src1.front(), width);
    // pre-compute mask l and h
    int mask_l = width;
    int mask_h = -1;
    for (const auto& assign : src1) {
        mask_l = std::min(mask_l, assign.dst.range.lo);
        mask_h = std::max(mask_h, assign.dst.range.hi);
    }

    z3::context ctx;
    const auto rot = ctx.bv_val(right_shift_val, width);
    const auto ones = ctx.bv_val(all_ones_value, width);
    const auto s1 = ctx.bv_const("s1", width);
    const auto s2 = ctx.bv_const("s2", width);
    const auto mask = ctx.bv_const("mask", width);
    const auto l = ctx.bv_val(mask_l, width);
    const auto h = ctx.bv_val(mask_h, width);
    const auto bv_sort = ctx.bv_sort(width);
    const auto deposit_field = ctx.function("deposit_field", bv_sort, bv_sort, bv_sort);

    z3::solver s(ctx);
    s.add(h >= 0 && h < width && l >= 0 && l < width && h >= l);
    s.add(mask == z3::shl(z3::lshr(ones, (width - 1 - h + l)), l));
    s.add(
        z3::forall(s1, s2,
                   deposit_field(s1, s2) ==
                       (((z3::lshr(s1, rot) | z3::shl(s1, width - rot)) & mask) | (s2 & (~mask)))));

    // except for unused bits, all other bits must be either
    // (1) unchanged.
    // (2) set by a correct bit from one of the source.
    bitvec set_bits;
    for (const auto& assign : src1) {
        // add src1 expected assertions.
        const int dst_lo = assign.dst.range.lo;
        const int dst_hi = assign.dst.range.hi;
        set_bits.setrange(dst_lo, dst_hi - dst_lo + 1);
        // for ad_or_const slices are aligned with destination.
        int src_lo = assign.src.range.lo;
        int src_hi = assign.src.range.hi;
        if (assign.src.is_ad_or_const) {
            src_lo = dst_lo;
            src_hi = dst_hi;
        }
        s.add(z3::forall(
            s1, s2, deposit_field(s1, s2).extract(dst_hi, dst_lo) == s1.extract(src_hi, src_lo)));
    }
    for (const auto& assign : src2) {
        BUG_CHECK(!assign.src.is_ad_or_const, "deposit-field src2 cannot be ad_or_const");
        const int dst_lo = assign.dst.range.lo;
        const int dst_hi = assign.dst.range.hi;
        set_bits.setrange(dst_lo, dst_hi - dst_lo + 1);

        const int src_lo = assign.src.range.lo;
        const int src_hi = assign.src.range.hi;
        s.add(z3::forall(
            s1, s2, deposit_field(s1, s2).extract(dst_hi, dst_lo) == s2.extract(src_hi, src_lo)));
    }

    // In deposit-field, (src2 & ~mask) will be the background of destination. So, if
    // there is any bit that is not unused (occupied by some non-mutex fields) and not set
    // it cannot be changed. The only possible case that it will not be changed is
    // when src2 and dest are the same container.
    bool require_src2_be_dest = false;
    const auto& unused_bits = specs_i.at(dest).unused;
    for (int i = 0; i < width; i++) {
        // bits occupied by other field not involved in this action.
        // It's safe to assume that those bits are from s2, as long as
        // we called this function with both (s1, s2) and (s2, s1).
        if (!unused_bits[i] && !set_bits[i]) {
            require_src2_be_dest = true;
            s.add(z3::forall(s1, s2, deposit_field(s1, s2).extract(i, i) == s2.extract(i, i)));
        }
    }
    if (require_src2_be_dest && !src2.empty() && dest != src2.front().src.container) {
        std::stringstream ss;
        ss << "destination " << dest << " will be corrupted because src2 "
           << src2.front().src.container << " is not equal to dest";
        return Error(ErrorCode::deposit_src2_must_be_dest, ss.str());
    }

    LOG3("Z3 expr:\n" << s);
    switch (s.check()) {
        case z3::unsat: {
            return Error(ErrorCode::unsat, "unsat");
        }
        case z3::sat: {
            z3::model m = s.get_model();
            LOG3("deposit-field mask.l = " << m.eval(l).get_numeral_int());
            LOG3("deposit-field mask.h = " << m.eval(h).get_numeral_int());
            LOG3("deposit-field rot = " << right_shift_val);
            return boost::none;
        }
        case z3::unknown: {
            return Error(ErrorCode::smt_sovler_unknown, "smt solver cannot solve this in time");
        }
    }
    return boost::none;
}

```
