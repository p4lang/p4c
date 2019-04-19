# P4Runtime Specification Version 1

This directory contains the sources for generating the official P4Runtime
specification document.

# Markup version

The markup version uses Madoko (https://www.madoko.net) to produce
HTML and PDF versions of the documentation. Pre-built versions of the
documentation are available at **[TODO]**


Files:
- `P4Runtime-spec.mdk` is the main file. 
- assets: Figures
  - `*.odg` - OfficeLibre master drawing file used to export images. These are bulk-rendered at build time into .svg and .png images via `soffice` command-line (required in build environment)
- `Makefile` builds documentation in the build subdirectory
- `p4.json` is providing custom syntax highlighting for P4. It is a rip off from
  the cpp.json provided by Madoko (the "extend" clause does not work, my version
  of Madoko asks for a "tokenizer" to be defined). Style customization for each
  token can be done using CSS style attributes (see `token.keyword` in
  `P4Runtime-spec.mdk`).

## Building

The easiest way to render the Madoko specification documentation is to use the
`p4lang/p4rt-madoko:latest` Docker` image:

    docker run -v `pwd`/docs/v1:/usr/src/p4-spec p4lang/p4rt-madoko:latest make

### Linux
```
sudo apt-get install nodejs
sudo npm install madoko -g
sudo apt-get install libreoffice
make [all | html | pdf ]
```
In particular (on Ubuntu 16.04 at least), don't try `sudo apt-get install npm`
because `npm` is already included and this will yield a bunch of confusing error
messages from `apt-get`.

`dvipng` is used to render "math" inside BibTex (used for bibliography)
titles, so you will need to install it as well. On Debian-based Linux, it can be
done with `sudo apt-get install dvipng`.

### MacOS

We use the [local
installation](http://research.microsoft.com/en-us/um/people/daan/madoko/doc/reference.html#sec-installation-and-usage)
method. For Mac OS, I installed node.js using Homebrew and then Madoko using
npm:
```
brew install node.js
npm install madoko -g
```
Note that to build the PDF you need a functional TeX version installed.

### Windows

You need to install miktex [http://miktex.org/], madoko
[https://www.madoko.net/] and node.js [https://nodejs.org/en/].  To
build you can invoke the make.bat script.


# TODO
## Formating Fixups TODO

## Content TODO
Following are major content items which are missing or incomplete:
*  Section 17. P4Runtime Versioning
*  Section 18. Extending P4Runtime for non-PSA architectures
*  Section 19. Lifetime of a session