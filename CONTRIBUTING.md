# Contributing to the P4 Compiler Project (p4c)

Thank you for considering contributing to the P4 Compiler Project (p4c)! Your contributions are valuable and help improve the project for everyone. Before getting started, please take a moment to review the following guidelines.

## Coding Standard Philosophy

Please note that this project adheres to the [P4 Coding Standard Philosophy](https://github.com/p4lang/p4c/blob/main/docs/CodingStandardPhilosophy.md). By participating, you are expected to uphold this code. 

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

## P4 onboarding
For contributing to P4 Compiler, it’s good to know some P4. Here are some learning materials:

- General hands-on [tutorials](https://github.com/p4lang/tutorials).
- [Technical documentation on P4-related topics](https://github.com/jafingerhut/p4-guide?tab=readme-ov-file#introduction).
- Motivating P4: [IEEE ICC 2018 // Keynote: Nick McKeown, Programmable Forwarding Planes Are Here To Stay](https://www.youtube.com/watch?v=8ie0FcsN07U)
- Introducing P4-16 in detail: 
  - Part 1: [Introduction to P4_16. Part 1](https://www.youtube.com/watch?v=GslseT4hY1w)
  - Part 2: [Introduction to P4_16. Part 2](https://www.youtube.com/watch?v=yqxpypXIOtQ)
- Material on the official P4 compiler: 
  - [Understanding the Open-Soure P416 Compiler - February 15, 2022 - Mihai Budiu](https://www.youtube.com/watch?v=Rx5AQ0IF6eU)
  - [Understanding P416 Open-Source Compiler, Part 2 - March 1, 2022 - Mihai Budiu](https://www.youtube.com/watch?v=YnPHPaPSmpU)
  - https://github.com/p4lang/p4c/blob/main/docs/compiler-design.pdf
- Introduction to P4Runtime: [Next-Gen SDN Tutorial - Session 1: P4 and P4Runtime Basics](https://www.youtube.com/watch?v=KRx92qSLgo4)

## Contact
We appreciate your contributions and look forward to working with you to improve the P4 Compiler Project (p4c)!
- For further assistance or questions regarding contributions, reach out to us in our [community chat](https://p4-lang.slack.com/).  [Joining link](https://join.slack.com/t/p4-lang/shared_invite/zt-a9pe96br-Th73ueaBAwJw1ZbD_z1Rpg) .
- For general P4-related questions, use the [P4 forum](https://forum.p4.org/).
- For other communication channels click [here](https://p4.org/join/).

Happy coding!
