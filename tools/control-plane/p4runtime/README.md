# P4Runtime Specification

This directory contains protobuf files, specifications and related artifacts for
all versions of the P4Runtime API. Documentation and protobuf definitions are
placed into two distinct top-level directories. In each of these directories,
files are organized based on the P4Runtime major version number (e.g. v1) as
follows:
```
.
├── docs
│   └── v1  # documentation for P4Runtime v1
├── proto
│   └── p4
│       ├── config
│       │   └── v1  # p4.config.v1 protobuf package (P4Info message definition)
│       └── v1  # p4.v1 protobuf package (P4Runtime service definition)
```

Git tags are used to mark minor and patch release versions.

## Reading the latest version of the documentation

The latest version of the P4Runtime v1 specification is available:
* [here](https://s3-us-west-2.amazonaws.com/p4runtime/docs/master/P4Runtime-Spec.html)
  in **HTML** format
* [here](https://s3-us-west-2.amazonaws.com/p4runtime/docs/master/P4Runtime-Spec.pdf)
  in **PDF** format

It is updated every time a new commit is pushed to the master branch.

## Overview
P4 is a language for programming the data plane of network devices. The
P4Runtime API is a control plane specification for controlling the data plane
elements of a device or program defined by a P4 program. This repository
provides a precise definition of the P4Runtime API via protobuf (.proto) files
and accompanying documentation. The target audience for this includes system
architects and developers who want to write controller applications for P4
devices or switches.

# Compiling P4Runtime Protobuf files

You can use Docker to run the protoc compiler on the P4Runtime Protobuf files
and generate the Protobuf & gRPC bindings for C++ and Python:

```
docker build -t p4runtime -f CI/Dockerfile .
docker run -v <OUT>:/out/ -t p4runtime /p4runtime/CI/compile_protos.sh /out/
```

This will generate the bindings in the local `<OUT>` directory. **You need to
provide the absolute path for `<OUT>`.** The default Docker user is root, so you
may need to change the permissions manually for the generated files after the
`docker run` command exits.

These commands are the ones used by our CI system to ensure that the Protobuf
files stay semantically valid.

# Modification Policy

We use the following processes when making changes to the P4Runtime
specification and associated documents. These processes are designed to be
lightweight, to encourage active participation by members of the P4.org
community, while also ensuring that all proposed changes are properly vetted
before they are incorporated into the repository and released to the community.

## Core Processes

* Only members of the P4.org community may propose changes to the P4Runtime
  specification, and all contributed changes will be governed by the
  Apache-style license specified in the P4.org membership agreement.

* We will use [semantic versioning](http://semver.org/) to track changes to the
  P4Runtime specification: major version numbers track API-incompatible changes;
  minor version numbers track backward-compatible changes; and patch versions
  make backward-compatible bug fixes. Generally speaking, the P4Runtime working
  group co-chairs will typically batch together multiple changes into a single
  release, as appropriate.

## Detailed Processes

We now identify detailed processes for three classes of changes. The text below
refers to [key
committers](https://github.com/orgs/p4lang/teams/p4lang-key-committers), a
GitHub team that is authorized to modify the specification according to these
processes.

1. **Non-Technical Changes:** Changes that do not affect the definition of the
   API can be incorporated via a simple, lightweight review process: the author
   creates a pull request against the specification that a key committer must
   review and approve. The P4Runtime Working Group does not need to be
   explicitly notified. Such changes include improvements to the wording of the
   specification document, the addition of examples or figures, typo fixes, and
   so on.

2. **Technical Bug Fixes:** Any changes that repair an ambiguity or flaw in the
   current API specification can also be incorporated via the same lightweight
   review process: the author creates a GitHub issue as well as a pull request
   against the specification that a key committer must review and approve. The
   key committer should use their judgment in deciding if the fix should be
   incorporated without broader discussion or if it should be escalated to the
   P4Runtime Working Group. In any event, the Working Group should be notified
   by email.

3. **API Changes** Any change that substantially modifies the definition of the
   API, or extends it with new features, must be reviewed by the P4Runtime
   Working Group, either in an email discussion or a meeting. We imagine that
   such proposals would go through three stages: (i) a preliminary proposal with
   text that gives the motivation for the change and examples; (ii) a more
   detailed proposal with a discussion of relevant issues including the impact
   on existing programs; (iii) a final proposal accompanied by a design
   document, a pull request against the specification, and prototype
   implementation on a branch of `p4runtime`, and example(s) that illustrate the
   change. After approval, the author would create a GitHub issue as well as a
   pull request against the specification that a key committer must review and
   approve.
