control C1()(int Size)
{
    apply {}
}

control C2()(int macSize, int macSizeI)
{
  C1(macSize) outerDst;
  C1(macSize) outerSrc;
  C1(macSizeI) innerDst;
  C1(macSizeI) innerSrc;

  apply {
    outerDst.apply();
    outerSrc.apply();
    innerDst.apply();
    innerSrc.apply();
  }
}

control ingress()
{
  C2(65536,65536) c2;
  apply {
      c2.apply();
  }
}

control Ingress();
package Switch(Ingress ingress);

Switch(ingress()) main;
