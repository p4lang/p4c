You can fork the repo and submit a pull request in Github. For more information
send us an email (p4-dev@lists.p4.org).

### Apache CLA

All developers must sign the [P4.org](http://p4.org) CLA and return it to
(membership@p4.org) before making contributions. The CLA is available
[here](https://p4.org/assets/P4_Language_Consortium_Membership_Agreement.pdf).

### Coding guidelines

Any contribution to the C++ core code (in particular the [bm_sim](src/bm_sim)
module) must respect the coding guidelines. We rely heavily on the [Google C++
Style Guide](https://google.github.io/styleguide/cppguide.html), with some
differences listed in this repository's
[wiki](https://github.com/p4lang/behavioral-model/wiki/Coding-guidelines). Every
submitted pull request will go through our Travis tests, which include running
`cpplint.py` to ensure correct style and formatting.

### Building locally

We recommend that you build with a recent C++ compiler and with `-Werror` since
our CI tests use this flag. In order to build with the same flags as the CI
tester, please run `configure` with the following flags:

    ./configure --with-pdfixed --with-pi --with-stress-tests --enable-debugger --enable-Werror
