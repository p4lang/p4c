# Compatibility of BSD and Apache 2.0 licenses

Reference for what the SPDX-License-Identifer possible string values
are, including Apache-2.0 and BSD-3-Clause:

+ https://spdx.org/licenses

| Python program A  |  ... imports Python     |          |
| with license      |  package B with license | Allowed? |
| ----------------- | ----------------------- | -------- |
| Apache-2.0        | Apache-2.0              | yes, same licenses |
| BSD-3-Clause      | BSD-3-Clause            | yes, same licenses |
| Apache-2.0        | BSD-3-Clause            | yes, if released as Apache-2.0 |
| BSD-3-Clause      | Apache-2.0              | yes, if released as Apache-2.0 |


# Open Source Automation Development Lab (OSADL)

The OSADL [1] publishes a license compatibility matrix [2].  The
details in this section are for the version of this matrix downloaded
on 2025-Jan-23.

The matrix entry for the leading license being BSD-3-Clause and the
subordinate license being Apache-2.0 says:
```
    { "name": "Apache-2.0", "compatibility": "Yes", "explanation": "Non-copyleft licenses such as the Apache-2.0 license and the BSD-3-Clause license are generally considered mutually compatible."},
```

The matrix entry for the leading license being Apache-2.0 and the
subordinate license being BSD-3-Clause says:
```
    { "name": "BSD-3-Clause", "compatibility": "Yes", "explanation": "Non-copyleft licenses such as the BSD-3-Clause license and the Apache-2.0 license are generally considered mutually compatible."},
```


# David Wheeler's FLOSS License Slide

See [3].

The opinion of the author is that software released under BSD-3-Clause
and Apache-2.0 licenses can be combined into a single work.  The
combined work can be released under the Apache-2.0 license.


# References

[1] https://osadl.org

[2] "OASDL license compatibility matrix with explanations",
    https://www.osadl.org/html/CompatMatrix.html

[3] David A. Wheeler, "The Free-Libre / Open Source Software (FLOSS)
    License Slide", Jan 26, 2017 (was Sep 27, 2007),
    https://dwheeler.com/essays/floss-license-slide.html
