#include <core.p4>

control C1(in bit<48> address, inout bit<4> val, in bool valid)(int Size)
{
  action A(bit<4> v) {val = v;}
  action B() {val = 0;}
  table lookup {
    key = {address : exact;}
    actions = {A; B;}
    const default_action = A((((1<<4)-1)));
    size = Size;
  }
  apply {
    if(valid)
      lookup.apply();
    else A((((1<<4)-1)));
  }
}

control C2(
  in bit<48> srcMacO, in bit<48> dstMacO, in bit<48> srcMacI, in bit<48> dstMacI,
  in bool outerEthValid, in bool innerEthValid,
  inout bit<4> valFinal)
(int macSize, int macSizeI)
{
  bit<4> val1 = 0;
  bit<4> val2 = 0;
  bit<4> val3 = 0;
  bit<4> val4 = 0;

  C1(macSize) outerDst;
  C1(macSize) outerSrc;
  C1(macSizeI) innerDst;
  C1(macSizeI) innerSrc;

  apply {
    outerDst.apply(dstMacO,val1,outerEthValid);
    outerSrc.apply(srcMacO,val2,outerEthValid);
    innerDst.apply(dstMacI,val3,innerEthValid);
    innerSrc.apply(srcMacI,val4,innerEthValid);
  }
}

header eth_t {
  bit<48> dstAddr;
  bit<48> srcAddr;
  bit<16> etherType;
}

struct internal_t {
  bit<4>  val;
}

struct headers_t {
  eth_t baseEth;
  eth_t extEth;
  internal_t internal;
}

control ingress(inout headers_t hdr)
{
  C2(65536,65536) c2;
  apply {
      c2.apply(hdr.baseEth.srcAddr,hdr.baseEth.dstAddr,hdr.extEth.srcAddr,hdr.extEth.dstAddr,
               hdr.baseEth.isValid(), hdr.extEth.isValid(), hdr.internal.val);
  }
}

control Ingress<H>(inout H hdr);
package Pipeline<H>(Ingress<H> ingress);
package Switch<H>(Pipeline<H> pipe);

Pipeline(ingress()) ig;
Switch(ig) main;
