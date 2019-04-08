#ifndef P4C_UBPFCONTROL_H
#define P4C_UBPFCONTROL_H

namespace UBPF {

class UBPFControlBodyTranslator : EBPF::ControlBodyTranslator {
public:
    UBPFControlBodyTranslator(const EBPFControl* control) : EBPF::ControlBodyTranslator(control) {}


    bool preorder(const IR::PathExpression* expression) override;
    bool preorder(const IR::MethodCallExpression* expression) override;
    bool preorder(const IR::ExitStatement*) override;
    bool preorder(const IR::ReturnStatement*) override;
    bool preorder(const IR::IfStatement* statement) override;
    bool preorder(const IR::SwitchStatement* statement) override;
};

class UBPFControl : public EBPF::EBPFControl {
public:
    UBPFControl(const EBPFProgram* program, const IR::ControlBlock* block,
                const IR::Parameter* parserHeaders) : EBPF::EBPFControl(program, block, parserHeaders) {}

};

}

#endif //P4C_UBPFCONTROL_H
