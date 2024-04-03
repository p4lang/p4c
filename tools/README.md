# Tools
Contains utility scripts for supporting the development of P4C.

## IWYU

[IWYU](https://github.com/include-what-you-use/include-what-you-use) checks that every symbol used in a C/C++ file has a corresponding include. Vice-versa IWYU also ensures that unnecessary includes are removed.

Since it depends on clang and LLVM, it is recommended to install IWYU from scratch.[Instructions to Build IWYU Standalone](https://github.com/include-what-you-use/include-what-you-use#how-to-build-standalone)

To use IWYU for P4Tools, enable it first in the build order with the option
```
cmake .. -DENABLE_IWYU=ON
```

Then build with the following command, which will collect IWYU's suggestions
```
make 2>&1 | tee iwyu_suggestions.txt
```

The suggestions can then be applied using the fix_includes script
```
fix_includes.py --nocomments --nosafe_headers < iwyu_suggestions.txt
```

For the cleanest results, this sequence of commands should be run multiple times.

### Mappings
Some includes removed or added by IWYU are undesirable. `.imp` include manual mappings that consolidate includes or prevent some includes from being added. Most of these helper files are from the main IWYU repository. To refresh these files run `make refresh` in the `iwyu_mappings folder`.

## clang-format
clang-format checks whether the C/C++ code in this repository is formatted correctly. The appropriate style file is labelled `.clang-format`.

To check whether the code confirms to the correct formatting standard you can run
```
make clang-format
```
in the build directory.
To automatically apply formatting to the entire configured code base run
```
make clang-format-fix-errors
```
in the build directory.

## cpplint
cpplint contains additional semantic checks that are enforced. These are configured in `CPPLINT.cfg`

To check whether the code confirms to the correct cpplint standard you can run
```
make cpplint
```
in the build directory.

## clang-tidy
This repository also contains configurations for clang-tidy, which are listed in `.clang-tidy` file. These recommendations are not enforced yet.

## check-git-submodules
A helper script that checks that each of the submodules in the repository remains on the branch that it is tracking. The intention is to avoid accidentally switching to a non-stable or outdated branch for a particular dependency.

To check whether all heads are on the correct branch run
```
./check-git-submodules.sh
```
