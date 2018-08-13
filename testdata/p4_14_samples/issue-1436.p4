header_type inboxes {
  fields {
    cornstarchs : 48;
    khans : 16 (signed);
  }
}

header inboxes extirpations;

parser start {
  return parse_extirpations;
}

parser parse_extirpations {
  extract(extirpations);
  return select (latest.khans) {
    1024 : ingress;
  }
}
control ingress { }
