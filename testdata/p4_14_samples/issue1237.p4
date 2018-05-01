header_type affairs {
  fields {
    flaccidly : 48;
    sleeting : 16;
  }
}

header_type backs {
  fields {
    sharps : 16;
    peptides : 16;
    whirs : 16;
    breasted : 16;
    browning : 16;
    jews : 32;
  }
}

header affairs kilometer;

header backs expressivenesss;

parser start {
  extract(kilometer);
  extract(expressivenesss);
  return ingress;
}

action mooneys() {
  subtract_from_field(expressivenesss.breasted, expressivenesss.peptides);
  subtract_from_field(kilometer.sleeting, 7);
}

table conceptualization {
  reads {
    kilometer : valid;
    kilometer.flaccidly mask 0 : lpm;
    expressivenesss : valid;
  }
  actions {
    mooneys;
  }
}

control ingress {
  apply(conceptualization);
}
