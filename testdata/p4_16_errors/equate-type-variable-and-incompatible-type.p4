// generated from issue3324.p4

void f<t>(in t a) {}
void g<t>(in t b) {
  f<match_kind>(b);
}
