#ifndef P4C_UBPFARCHHANDLER_H
#define P4C_UBPFARCHHANDLER_H

#endif //P4C_UBPFARCHHANDLER_H

#include "p4RuntimeArchHandler.h"

/// The architecture handler builder implementation for ubpfFilter.
struct UbpfFilterArchHandlerBuilder : public P4RuntimeArchHandlerBuilderIface {
    P4RuntimeArchHandlerIface* operator()(
            ReferenceMap* refMap,
            TypeMap* typeMap,
            const IR::ToplevelBlock* evaluatedProgram) const override;
};