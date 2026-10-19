# Compatibility of GPL v2.0 and Apache 2.0 licenses

Please suggest additional references, whether they argue for
compatibility or incompatbility of these two software licenses.  They
need not _settle_ the question authoritatively.  The point of this
article as of 2025 is that the question of their compatibilty seems
not to be settled yet.

Reference for what the SPDX-License-Identifer possible string values
are, including Apache-2.0 and GPL-2.0-only:

+ https://spdx.org/licenses

| Python program A  |  ... imports Python     |          |
| with license      |  package B with license | Allowed? |
| ----------------- | ----------------------- | -------- |
| Apache-2.0        | Apache-2.0              | yes, same licenses |
| GPL-2.0-only      | GPL-2.0-only            | yes, same licenses |
| Apache-2.0        | GPL-2.0-only            | very questionable |
| GPL-2.0-only      | Apache-2.0              | very questionable |

There is no publicly available advice we have found so far on the last
two rows of the table above that makes the answer obviously
"compatible", or obviously "incompatible", hence the "very
questionable" statement on whether it is allowed.  See later sections
for examples arguing both ways.

Given that lack of clarity on whether it is allowed, it seems prudent
that the P4 Consortium should not risk its limited funds and available
volunteer time by using questionable combinations of software
licenses.  We have a way to avoid that risk: use combinations of
licenses that are well known to be compatible.

If someone proposes using other approaches, it seems wise that they
should provide advice backed by an organization with at least one
intellectual property lawyer, and willing to defend the approach in
court.


# Article published in a law journal

See [5].

Here is the text of the abstract of this 29-page article:

    License “incompatibility” in free and open source software
    licensing means that, when two differently licensed pieces of software
    are combined, one cannot comply with both licenses at the same time.
    It is commonly accepted that the GNU General Public License version
    2 is incompatible with the Apache License, version 2 because certain
    provisions of the Apache License would be considered “further
    restrictions” not permitted by the GPLv2. However, this article will
    explain why there is no legally cognizable claim for combining the two,
    either under a copyright infringement theory or a breach of contract
    theory.

The P4 Consortium has no legal advice to say whether the arguments
presented in that article would hold up in a court case.


# Open Source Automation Development Lab (OSADL)

The OSADL [1] publishes a license compatibility matrix [2].  The
details in this section are for the version of this matrix downloaded
on 2025-Jan-23.

The matrix entry for the leading license being GPL-2.0-only and the
subordinate license being Apache-2.0 says:
```
    { "name": "Apache-2.0", "compatibility": "No", "explanation": "Incompatibility of the Apache-2.0 license with the GPL-2.0-only license is explicitly stated in the GPL-2.0-only license checklist."},
```

The matrix entry for the leading license being Apache-2.0 and the
subordinate license being GPL-2.0-only says:
```
    { "name": "GPL-2.0-only", "compatibility": "No", "explanation": "Software under a copyleft license such as the GPL-2.0-only license normally cannot be redistributed under a non-copyleft license such as the Apache-2.0 license, except if it were explicitly permitted in the licenses."},
```


# Free Software Foundation (FSF)

"By the same token, lax licenses are usually compatible with any
copyleft license. In the combined program, the parts that came in
under lax licenses still carry them, and the combined program as a
whole carries the copyleft license. One lax license, Apache 2.0, has
patent clauses which are incompatible with GPL version 2; since I
think those patent clauses are good, I made GPL version 3 compatible
with them." [4]

The above was written by Richard Stallman, who often speaks for the
FSF, but I do not know whether the FSF specifically endorses the legal
interpretations in the quoted paragraph above.  It is published on the
gnu.org site quite prominently, though.


# Randomly found StackExchange discussion

See [3].  I don't claim anything in there as authoritative, but it
might have useful links that lead to more authoritiatve information.


# References

[1] https://osadl.org

[2] "OASDL license compatibility matrix with explanations",
    https://www.osadl.org/html/CompatMatrix.html

[3] https://opensource.stackexchange.com/questions/1357/can-i-link-a-apache-2-0-library-into-software-under-gplv2

[4] https://www.gnu.org/licenses/license-compatibility.html

[5] Pamela S. Chestek, "A promise without a remedy: The supposed
    incompatibility of the GPL v2 and Apache v2 licenses", Santa Clara
    High Technology Law Journal, Vol 40, Issue 3, Article 2,
    https://digitalcommons.law.scu.edu/cgi/viewcontent.cgi?article=1701&context=chtlj#:~:text=It%20is%20commonly%20accepted%20that,not%20permitted%20by%20the%20GPLv2.
