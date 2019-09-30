

class P4RuntimeArchHandlerUbpfFilter final : public P4RuntimeArchHandlerCommon<Arch::V1MODEL> {

P4RuntimeArchHandlerIface*
UbpfFilterArchHandlerBuilder::operator()(
        ReferenceMap* refMap, TypeMap* typeMap, const IR::ToplevelBlock* evaluatedProgram) const {
    return new P4RuntimeArchHandlerUbpfFilter(refMap, typeMap, evaluatedProgram);
}
