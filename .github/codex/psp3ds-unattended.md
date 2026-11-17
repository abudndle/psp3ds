# PSP3DS unattended coding agent instructions

You are implementing **one** reviewable improvement in the assigned workstream of the experimental PSP3DS emulator at abudndle/psp3ds.

Rules:
- Read TODO.md, relevant code, and current tests before editing.
- Choose **one** bounded, unfinished task from the workstream, or fix a concrete bug blocking the next task.
- Work only in C/C++ source, headers, tests, and documentation relevant to the workstream.
- Do not edit .github/, privileged automation, secrets, vendored upstream PPSSPP code, or third-party binaries.
- Do not create proprietary PSP or Nintendo game assets. Synthetic tests only.
- Do not claim PPSSPP gameplay or commercial PSP compatibility from scaffolding, relocatable links, or synthetic CPU tests.
- Preserve existing tests. Add a focused regression for the behavior being changed.
- Keep the patch narrowly scoped and leave a concise explanation of what changed and what remains unverified.
- Do not commit, push, open PRs, modify workflows, access secrets, or configure credentials; a separate workflow job handles publication.
- Prefer correctness and safety over completing a milestone without evidence.

## Workstreams

**memory**: Audit and harden checked guest-memory accesses and fault classification. Work toward a genuine PPSSPP Memory:: backend, but do not wire unsafe unchecked host pointers.

**interpreter**: Investigate missing PPSSPP MIPS interpreter linkage, memory integration, and synthetic program execution. Do not represent custom CPU tests as upstream PPSSPP execution.

**platform**: Improve reproducible 3DS and DS/DSi tests, platform budgets, and hardware capability checks. Do not imply Slot-2 data RAM can execute PSP code.

The workflow supplies the workstream name. Select no more than one bounded task for each run.
