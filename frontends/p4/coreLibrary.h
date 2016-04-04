#ifndef _FRONTENDS_P4_CORELIBRARY_H_
#define _FRONTENDS_P4_CORELIBRARY_H_

#include "lib/cstring.h"
#include "frontends/common/model.h"

namespace P4 {

class PacketIn : public Model::Extern_Model {
 public:
    PacketIn() :
            Extern_Model("packet_in"),
            extract("extract"),
            lookahead("lookahead")
    {}
    Model::Elem extract;
    Model::Elem lookahead;
};

class PacketOut : public Model::Extern_Model {
 public:
    PacketOut() :
            Extern_Model("packet_out"),
            emit("emit")
    {}
    Model::Elem emit;
};

// Model of P4 standard library
// To be kept in sync with corelib.p4
class P4CoreLibrary : public ::Model::Model {
 protected:
    P4CoreLibrary() : Model("0.2"),
            exactMatch("exact"),
            ternaryMatch("ternary"),
            lpmMatch("lpm"),
            packetIn(PacketIn()),
            packetOut(PacketOut()),
            noError("NoError"),
            packetTooShort("PacketTooShort"),
            noMatch("NoMatch"),
            emptyStack("EmptyStack"),
            fullStack("FullStack"),
            overwritingHeader("OverwritingHeader")
    {}

 public:
    static P4CoreLibrary instance;

    ::Model::Elem exactMatch;
    ::Model::Elem ternaryMatch;
    ::Model::Elem lpmMatch;

    PacketIn    packetIn;
    PacketOut   packetOut;

    ::Model::Elem noError;
    ::Model::Elem packetTooShort;
    ::Model::Elem noMatch;
    ::Model::Elem emptyStack;
    ::Model::Elem fullStack;
    ::Model::Elem overwritingHeader;
};

}  // namespace P4

#endif /* _FRONTENDS_P4_CORELIBRARY_H_ */
