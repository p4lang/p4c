enum bit<4> e
{
  a = 1
}
control c(in e v)
{
  apply{
    switch(v) {
      4w1:
      default: {}
    }
  }
}