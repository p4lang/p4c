parser start {
  return select(current(0,1)) {
    default: loop;
  }
}

parser loop {
  return start;
}

control ingress {
}
