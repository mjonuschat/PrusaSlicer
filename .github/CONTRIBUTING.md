# Contribution guidelines

As an open source project, we appreciate pull requests from the community. On the other hand, every bit of code has to be reviewed and later maintained, which is a burden that will fall on us, not on the auther of the PR. Pulling unfinished, buggy or unservicable code would inevitably compromise stability of the software and make the future development difficult. For this reason, we cannot accept any pull request that comes. If you're a SW developer and consider contributing, this is a guideline to follow to increase the chance of merging your work.

## General guidelines for a good pull request

Click to see details:


<details>
<summary><b>No one-trick ponies</b></summary>
It is relatively simple to solve a corner case by adding a parameter that the user can tweak it with. It is also relatively simple to add an obscure feature that only works under some very specific conditions. However, we need PrusaSlicer to stay maintainable and usable by "normal" users, not flooded with half-baked features that are two people in the world use. Every feature is something that can conflict with something else, every workaround can stop working when another workaround triggers, and it is difficult to understand the dependency chains. Whatever you are trying to build, assess it by this optics first.
</details>

<details>
<summary><b>Don't do any architectural changes</b></summary>
You may see an opportunity to improve the architecture by rewriting it. You may even be right and competent enough to do it. However, such a pull request would be huge, virtually impossible to review or test completely and quite likely to break something. We will not risk merging it and then suffer the fallout.
</details>

<details>
<summary><b>Try to do the changes as locally as possible</b></summary>
We are hesitant to merge PRs that change 20+ files of well-tested code for the same reasons as mentioned above.
</details>

<details>
<summary><b>Keep the number of changes to a minimum</b></summary>
Have you found a bug? Fix it. Nothing else. Don't do anything not directly related to that. Do not fix comment-typos in all files that you see, don't unify whitespace usage, don't move several unrelated classes to a different file because they fit there better and don't refactor everything that you stumble accross, no matter how obvious you think it is. A large pull request takes longer to review, is difficult to review (reason of such changes is not obvious) and there is a risk of breaking something. File separate (and orthogonal !) pull requests if you feel there is more to improve. Consider squashing your branch before filing a PR.
</details>

<details>
<summary><b>Document your changes</b></summary>
Commit messages and code comments should make the reasons for your changes clear. Are you fixing a bug reported on github? Reference it in the commit message. Do you know when it was introduced? Reference the first bad commit. Have you added a non-trivial block of code to recognize and solve some corner case? Mention it in the comment. And when you post your PR on github, clearly describe what you were trying to achieve and why.
</details>

<details>
<summary><b>Test your changes</b></summary>
We have seen pull requests that did not do what they claimed at all. We have seen pull requests failing tests that they themselves provided. We don't like them. Properly test your improvement and anything you think could have been broken (no matter how unlikely you think it is). There is currently no CI build system for pull requests, so don't expect to iterate to the correct solution by trial and error by misusing it. If your code fails to compile on OSX because of a missing #include that you didn't need on Windows, that's fine. A missing semicolon indicates that nobody ever compiled and tested the code. That's not fine.
</details>


## Coding style

Coding style is described in [doc/CodeStyle.md](doc/CodeStyle.md) in more detail.

## Disclaimer

Maintainers of PrusaSlicer always have the final vote on what gets merged and what does not. No amount of likes on Facebook or hearts on the respective pull request can overrule it, and if a maintainer closes a PR for whatever reason, it is closed. As hard as it may sound, the responsibility and long-term maintenance is on us. Thanks for your understanding.
