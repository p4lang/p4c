parser start {
  return ingress;
}
control ingress {
  d();
}
control d {
    if(0 == 1) { }
