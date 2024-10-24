# Compiler Interfaces

Definitions for compiler interfaces

This repo contains the schema definitions for the compiler outputs:

 - manifest.json: for the structure of the outputs
 - context.json: for the driver
 - resources.json: for p4i
 - structured logging
   - Summaries for: phv, mau, power, parser
   - Structured logging for: parser, phv allocation, table allocation

The intent of this repo is to be imported as a submodule in Glass and
Brig, and any other tools that need to be aware of the schema
definition.

## Tools

The tools/ directory contains a set of scripts that generate JSON and
text outputs from context.json and resources.json to match files that
are otherwise produced by the compiler.

These scripts run standalone or can be packaged with pyinstaller using
the following:
```
pyinstaller --onefile p4c-build-logs.spec
```
The resulting p4c-build-log executable will be located in dist.

## Contributions and change process

Schema changes are coordinated between multiple projects and teams
via the schema review board. Follow these steps to submit a change
for review:

  1. Make sure there is a Jira in the project driving the change (this is usually P4C project with various bugfixes). If there is no applicable project, create a Jira within the [CPIF project](https://jira.devtools.intel.com/projects/CPIF/issues). Make sure you know answers to following bulletpoints and can communicate them to the review board:
      - change justification
      - urgency
      - an initial proposal (e.g. pull request against this repo)
      - blast radius: which components might be affected by this
        change (Brig or Glass, driver, API generation, Harlyn model
        p4i, BF-Runtime)
      - testing strategy
      - any other information to support the change proposal
  2. Open a PR against [this repository](https://github.com/barefootnetworks/compiler-interfaces)
  3. If the change in the schema breaks the compatibility with the compiler, make sure there is also an integration PR in the bf-p4c-compilers repo
  4. Notify p4.schema.review.board@intel.com of your proposal. For
    large proposals we may arrange a review meeting.
  5. Once the changes are approved (both here and in compiler), follow this merge process:
      - Merge PR here in compiler interfaces
      - Rebase PR in bf-p4c-compilers to checkout compiler_interfaces at new tip of the master
      - Wait until the regression passes and merge ASAP (this is to prevent conflicts when multiple people are making changes to compiler interfaces)

## Schema review board members

As a schema review board member you are expected to:

  1. Provide code review to PRs against compiler-interfaces
  2. Consider stakeholder projects (p4c, driver, API generation,
    Harlyn model, p4i or BF-Runtime)
  3. Follow our schema versioning policy

## Schema versioning policy

1. Each schema must be versioned. 
2. This version must communicate backwards-compatibility between the
   new schema and existing data which adheres to earlier schema 
   versions.

Given a version MODEL-REVISION-ADDITION, increment:

  * MODEL when a schema change will prevent interaction with **any** 
    historical data
  * REVISION when a schema change which may break some interaction
    with historical data
  * ADDITION when a schema change is compatible with historical data

This policy is adopted from [SchemaVer](https://snowplowanalytics.com/blog/2014/05/13/introducing-schemaver-for-semantic-versioning-of-schemas/).

> What does this mean for compatibility?

For a component which uses schema version 1.2.3:
  * it is always compatible with 1.2.X
  * it may be compatible with 1.X.X if the changes were not pertinent
    to the concerned component
  * it will not be compatible with 2.X.X 
