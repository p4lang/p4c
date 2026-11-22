// generated from issue1205-bmv2.p4

parser P();
package V1Switch(P p);
control MyC()() {
  apply {}
}
V1Switch(MyC()) main;
