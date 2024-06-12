<!-- \page changelog Changelog -->

> [!NOTE]  
> # Semantic Versioning
> We follow a monthly release cadence. Our versioning scheme is as follows:
> - **Major.Minor.Patch** versions align with the P4 specification.
> - **Revision** (the last number) is incremented with each monthly release.

## Release v1.2.4.12 [[view](https://github.com/p4lang/p4c/pull/4699)]

### Breaking Changes 🛠
- Replace IR::getBitType with IR::Type_Bits::get. [[view](https://github.com/p4lang/p4c/pull/4669)]
- Make the new operator protected for some IR types. [[view](https://github.com/p4lang/p4c/pull/4670)]
  
### P4 Specification Implementation
- [#4656] Explicitly delay constant folding of only action enum `IR::SwitchCase` `label` expressions, instead of delaying constant folding of all `IR::Mux` expressions [[view](https://github.com/p4lang/p4c/pull/4657)]
- Changes for for loops [[view](https://github.com/p4lang/p4c/pull/4562)]

### Changes to the Compiler Core
- Use check_include_file_cxx instead of check_include_file to find mm_malloc.h [[view](https://github.com/p4lang/p4c/pull/4649)]
- Move IRUtils Literal get functions to the respective IR members. Add stringliteral get function.  [[view](https://github.com/p4lang/p4c/pull/4623)]
- Improve error message when shifting int by non-const [[view](https://github.com/p4lang/p4c/pull/4653)]
- Fixes to lib/hash and lib/big_int_util. [[view](https://github.com/p4lang/p4c/pull/4655)]
- Link against the Boehm-Demers-Weiser Garbage Collector using FetchContent. [[view](https://github.com/p4lang/p4c/pull/3930)]
- Explicitly include hash for Ubuntu 18.04. [[view](https://github.com/p4lang/p4c/pull/4664)]
- RemoveUnusedDeclarations - make helpers protected [[view](https://github.com/p4lang/p4c/pull/4668)]
- Generalization & minor refactoring in RenameMap [[view](https://github.com/p4lang/p4c/pull/4677)]
- Add string_view and string conversion operators and functions to cstring [[view](https://github.com/p4lang/p4c/pull/4676)]
- Facilitate inheritance from RenameSymbols pass, deduplicate code [[view](https://github.com/p4lang/p4c/pull/4682)]
- Set type for LAnd, LOr, LNot to be Type_Boolean if unknown. [[view](https://github.com/p4lang/p4c/pull/4612)]
- Improve `BUG_CHECK` internals [[view](https://github.com/p4lang/p4c/pull/4678)]
- Workaround for gcc-11.4/draft 2x spec flaw [[view](https://github.com/p4lang/p4c/pull/4679)]
- irgen: Generate explicit instantiations [[view](https://github.com/p4lang/p4c/pull/4681)]

### Changes to the Control Plane
- Add support for new platform property annotations for P4Runtime. [[view](https://github.com/p4lang/p4c/pull/4611)]

### Changes to the eBPF Back Ends
- Fix missing declaration of custom externs in the generated eBPF/uBPF header file. [[view](https://github.com/p4lang/p4c/pull/4644)]

### Changes to the TC Back End
- Support for Register Extern in P4TC [[view](https://github.com/p4lang/p4c/pull/4671)]
- Added changes for default hit actions for tc backend [[view](https://github.com/p4lang/p4c/pull/4673)]
- Fix extern pipeline name in template file [[view](https://github.com/p4lang/p4c/pull/4675)]

### Changes to the P4Tools Back End
- Merge TestgenCompilerTarget into TestgenTarget. [[view](https://github.com/p4lang/p4c/pull/4650)]
### Other Changes
- Change parameter for kfunc 'bpf_p4tc_entry_create_on_miss' [[view](https://github.com/p4lang/p4c/pull/4637)]
- Add support for a clang-tidy linter. Add a files utility function. [[view](https://github.com/p4lang/p4c/pull/4254)]
- Only emit the warning on EXPORT_COMPILE_COMMANDS when there are clang-tidy files to lint. [[view](https://github.com/p4lang/p4c/pull/4665)]
- Aggressively clean up the Protobuf CMake dependency. [[view](https://github.com/p4lang/p4c/pull/4543)]
- Remove no-longer-used *.p4info.txt files [[view](https://github.com/p4lang/p4c/pull/4687)]
- Add a release template to the compiler. [[view](https://github.com/p4lang/p4c/pull/4692)]

[Click here to find Full Changelog](https://github.com/p4lang/p4c/compare/v1.2.4.11...v1.2.4.12) 
