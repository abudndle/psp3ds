# PPSSPP upstream integration and platform targets

Upstream: https://github.com/hrydgard/ppsspp (GPL-2.0-or-later; verify each imported component's license).
This repository is **not** currently a full PPSSPP fork. Do not label its DS/3DS artifacts as PPSSPP-compatible without measured game tests.

## Target strategy

| Target | Integration strategy | Current claim |
| --- | --- | --- |
| Windows, Linux, macOS | Build and test upstream PPSSPP directly; use as behavioral reference | Upstream-supported; not built here |
| Android, iOS | Use upstream platform projects; avoid duplicating existing ports | Upstream-supported; not built here |
| Nintendo 3DS / New 3DS | Incrementally adapt PSP core CPU, HLE, GE and input to native homebrew; budget RAM and GPU features | Experimental scaffold and GE subset only |
| Nintendo DS / DSi | Separate constrained interpreter and rendering subset; no assumption of full PSP compatibility | Experimental scaffold only |
| Other Nintendo systems | Research toolchain, legal deployment and performance separately before claiming support | Not implemented |

## Upstream component mapping

- PSP CPU and guest execution: `Core/MIPS/` -> local CPU and guest memory modules; preserve instruction-level tests.
- PSP high-level emulation: `Core/HLE/` -> local import, syscall, thread, I/O and timing modules.
- PSP Graphics Engine: `GPU/` -> local bounded GE translator, native raster backends; start with opaque sprite commands.
- Filesystems and disc formats: `Core/FileSystems/` and `Core/Loaders/` -> local ISO/CSO, loader and I/O paths.
- Input and UI: `UI/` and platform-specific upstream frontends -> per-console controller and screen mappings.

## Integration rules

1. Pin upstream to a reviewed commit SHA for each actual code import. Record original path, upstream SHA, modifications, and license headers.
2. Port isolated components with explicit platform abstractions; never bulk-copy the entire upstream tree into memory-constrained targets.
3. Keep GPL notices and attribution for any upstream code used; review additional dependency licenses separately.
4. Require host tests, sanitizers, native builds and emulator smoke tests for every increment.
5. Validate with legally obtained PSP homebrew and redistributable test assets; do not ship commercial game content.
6. Track actual PSP game compatibility separately from compilation, emulator boot and synthetic graphics proofs.

## Next integration checkpoints

- Guest CPU-driven GE list submission with bounded fetch and failure isolation.
- More vertex formats, textures, clipping, depth, blending and PSP-to-native GPU translation.
- HLE call compatibility against upstream behavior and PSP autotests.
- Audio, savedata, scheduling and input timing.
- Hardware performance profiling and honest per-target compatibility reporting.

## Reproducible upstream source checkout (first migration milestone)

Run from a Linux/macOS shell with Git installed:

```sh
bash scripts/fetch-ppsspp-upstream.sh
# Optional custom destination:
# bash scripts/fetch-ppsspp-upstream.sh /tmp/ppsspp-source
```

The script checks out the **actual** PPSSPP repository at
`1aebad2c651ff9d0684ee335e43aceff8fc7e3ec` and sparsely populates
`Core/MIPS`, `Core/HLE`, `GPU`, `Core/FileSystems`, and `Core/FileLoaders`.
The default destination `vendor/ppsspp` is Git-ignored. It refuses to overwrite
an existing checkout. Upstream source is not yet linked to PSP3DS builds;
fetching source is a prerequisite, **not** evidence of a working PPSSPP port.

Before integrating an upstream source file, record its exact path, license,
required upstream dependencies, and the target abstraction replacing its
platform assumptions. Preserve copyright/license notices and test behavior
against upstream PSP homebrew test cases.

## Unsupported upstream code: porting policy

Do not compile upstream platform or JIT backends directly into a Nintendo target.
Classify each dependency before integration:

| PPSSPP subsystem | Nintendo 3DS strategy | DS/DSi strategy |
| --- | --- | --- |
| x86/x64, ARM64, RISC-V JIT | Exclude; begin with interpreter, investigate ARM11-specific backend separately | Exclude; interpreter subset only |
| Vulkan, D3D, desktop GL | Replace with 3DS GPU renderer and explicit PSP GE translation | Replace with bounded DS 2D/3D rendering subset |
| OS threads, timers, sockets | Platform-specific services and scheduler abstractions | Cooperative/limited scheduling and platform APIs |
| Host virtual memory and JIT executable pages | Explicit guest-address translation and bounded allocations | Small bounded guest-memory windows; reject unsupported mappings |
| Desktop file dialogs, paths, input, audio | Native filesystem, controller, audio adapters | DS/DSi filesystem, input and audio adapters |
| Third-party desktop libraries | Audit dependencies and licenses; port only required pieces | Prefer dependency-free subsets or documented unsupported status |

The upstream source checkout is **not** automatically buildable for either console.
For every imported source file, track (1) pinned upstream revision, (2) original
license notice, (3) transitive headers/libraries, (4) unsupported host APIs,
(5) console replacement, (6) executable tests. Do not silently stub essential
emulation behavior and report success. Fail compilation clearly when a required
platform adapter is absent. Maintain distinct 3DS and DS/DSi capability claims.
