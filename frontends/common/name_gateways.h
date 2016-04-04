#ifndef _name_gateways_h_
#define _name_gateways_h_

class NameGateways : public Transform {
    const IR::Node *preorder(IR::If *n) override {
        return new IR::NamedCond(*n);
    }
    const IR::Node *preorder(IR::NamedCond *n) override {
        return n;
    }
};

#endif // _name_gateways_h_
