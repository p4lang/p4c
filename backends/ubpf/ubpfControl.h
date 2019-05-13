#ifndef P4C_UBPFCONTROL_H
#define P4C_UBPFCONTROL_H

//#include "backends/ebpf/ebpfControl.h"
#include "backends/ebpf/ebpfObject.h"
//#include "backends/ebpf/ebpfTable.h"
#include "ubpfTable.h"
//#include "ir/ir.h"
//#include "backends/ebpf/codeGen.h"

//#include "ubpfProgram.h"

namespace UBPF {

    class UBPFControl;

//    class UBPFProgram;

    class UBPFControlBodyTranslator : public EBPF::CodeGenInspector {
    public:
        const UBPFControl *control;
        std::set<const IR::Parameter *> toDereference;
        std::vector<cstring> saveAction;
        P4::P4CoreLibrary &p4lib;

        explicit UBPFControlBodyTranslator(const UBPFControl *control);

        virtual void compileEmitField(const IR::Expression *expr, cstring field,
                                      unsigned alignment, EBPF::EBPFType *type);

        virtual void compileEmit(const IR::Vector<IR::Argument> *args);

        virtual void processMethod(const P4::ExternMethod *method);

        virtual void processApply(const P4::ApplyMethod *method);

        virtual void processFunction(const P4::ExternFunction *function);

        bool preorder(const IR::PathExpression *expression) override;

        bool preorder(const IR::MethodCallExpression *expression) override;

        bool preorder(const IR::ExitStatement *) override;

        bool preorder(const IR::ReturnStatement *) override;

        bool preorder(const IR::IfStatement *statement) override;

        bool preorder(const IR::SwitchStatement *statement) override;
    };

    class UBPFControl : public EBPF::EBPFObject {
    public:
        const UBPFProgram *program;
        const IR::ControlBlock *controlBlock;
        const IR::Parameter *headers;
        const IR::Parameter *accept;
        const IR::Parameter *parserHeaders;
        // replace references to headers with references to parserHeaders
        cstring hitVariable;
        UBPFControlBodyTranslator *codeGen;

        std::set<const IR::Parameter *> toDereference;
        std::map<cstring, UBPFTable *> tables;

        UBPFControl(const UBPFProgram *program, const IR::ControlBlock *block,
                    const IR::Parameter *parserHeaders);


        void emit(EBPF::CodeBuilder *builder);

        void emitDeclaration(EBPF::CodeBuilder *builder, const IR::Declaration *decl);

        void emitTableTypes(EBPF::CodeBuilder *builder);

        void emitTableInitializers(EBPF::CodeBuilder *builder);

        void emitTableInstances(EBPF::CodeBuilder *builder);

        bool build();

        UBPFTable *getTable(cstring name) const {
            auto result = ::get(tables, name);
            BUG_CHECK(result != nullptr, "No table named %1%", name);
            return result;
        }

    protected:
        void scanConstants();
    };

}

#endif //P4C_UBPFCONTROL_H
