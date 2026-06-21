// It is an error to declare the same match_kind identifier multiple times,
// even when the duplicates appear in separate match_kind declarations.
// See https://github.com/p4lang/p4c/issues/5085

match_kind {
  foo,
  bar
}

match_kind {
  foo
}
