header_type inboxes {
  fields {
    cornstarchs : 48;
    khans : 16;
  }
}

header_type whitesand_t {
  fields {
    contracts : 48;
  }
}

header inboxes extirpations;
metadata whitesand_t hesitation;

parser start {
  set_metadata(hesitation.contracts, latest.cornstarchs);
  extract(extirpations);
  return select (latest.khans) {
    1024 : ingress;
  }
}
