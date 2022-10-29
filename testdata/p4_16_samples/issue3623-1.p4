enum bit<4> e
{
  a = 1
}
control c(in e v)
{
  apply{
    switch(v) {
      e.a:
      default: {}
    }
  }
}
