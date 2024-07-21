# Contribute to the P4 Compiler Project

Thank you for considering contributing to the P4 Compiler Project (P4C)! Your contributions are valuable and help improve the project for everyone. Before getting started, please take a moment to review the following guidelines.

## Contributing License
The P4 organizations uses [DCO](https://en.wikipedia.org/wiki/Developer_Certificate_of_Origin) for contributions. Please take a look at our [guidelines](https://github.com/p4lang/governance/wiki/P4-DCO-Guidelines).

To sign off the last commit quickly use the `git commit --amend --signoff` command. The failing check will also include instructions on how to sign off all commits in two steps (using `git rebase HEAD~$NUM_COMMITS --signoff`). The [Developer Community DCO guide](https://github.com/src-d/guide/blob/master/developer-community/fix-DCO.md#dco-is-missing) also provides helpful tips on fixing DCO inconveniences. Setting up a commit hook in the P4C repository will automate adding the DCO signoff.

## Coding Standard Philosophy

Please note that this project adheres to the [P4 Coding Standard Philosophy](docs/CodingStandardPhilosophy.md). By participating, you are expected to uphold this code. 

## How to Contribute
We welcome and appreciate new contributions. Check out [git usage](https://github.com/p4lang/p4c/tree/main/docs#git-usage) to get started.

### Guidelines 

* Writing unit test code [Guidelines](https://github.com/p4lang/p4c/tree/main/docs#adding-new-test-data).
* Write documentation [Guidelines](https://github.com/p4lang/p4c/tree/main/docs#writing-documentation).
* [Coding conventions](https://github.com/p4lang/p4c/tree/main/docs#coding-conventions).
* Opening pull requests and writing commit messages [Guidelines](https://github.com/p4lang/p4c/blob/main/docs/CodingStandardPhilosophy.md#Git-commits-and-pull-requests).
* Code has to be reviewed before it is merged.
* Make sure all tests pass when you send a pull request.
* Participate in the code review process and address any feedback or comments.
* Make sure `cmake --build . --target clang-format cpplint black isort
` produces no errors.

### Finding a Task
- Check out the issues that have specifically been [marked as being friendly to new contributors](https://github.com/p4lang/p4c/issues?q=is%3Aopen+is%3Aissue+label%3Agood-first-issue)
- Have a look at this README. Can it be improved? Do you see typos? You can open a PR.

## Reporting Issues
If you encounter any issues or have suggestions for improvement, please [open an issue](https://github.com/p4lang/p4c/issues) with a clear description and, if possible, steps to reproduce.

## Feature Requests
We welcome feature requests! Please open an issue and provide as much detail as possible about the requested feature and its use case.

Happy coding!
