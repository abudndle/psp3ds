# Experimental PSP GE sprite-command subset

**Scope:** This is a deliberately narrow, fail-closed translation layer
for a *synthetic fixture* of predecoded, opaque, untextured rectangles.
It is not a PSP GE emulator, real game display-list decoder, generic
vertex fetcher, or evidence that Jetpack Joyride graphics can render.

The PSP Graphics Engine uses 32-bit command words (8-bit opcode and
24-bit argument). The implemented subset recognizes precisely:

| Opcode | Command | Accepted behavior |
| --- | --- | --- |
| 0x00 | NOP | Exact zero argument, before FINISH |
| 0x04 | PRIM | Sprite type 6 with exactly 2 vertices, encoded as 0x04060002 |
| 0x0F | FINISH | Zero argument, once, after any sprites |
| 0x0C | END | Zero argument, once, last command after FINISH |

The translator takes a **caller-supplied synthetic array** of
`PspGeDecodedSprite` rectangles. Each sprite is 2D BGR555 with positive
width/height wholly inside 256×192. These inputs are NOT decoded from
real PSP `VADDR`, `VTYPE`, `BASE`, or guest RAM. The decoder requires
at least one primitive; inputs are capped at 128 command words and
64 sprites. Unsupported commands (including memory addressing,
texture setup, matrix state, blending, JUMP/CALL/RET, vertices,
triangles and display-list branches) return an error. Invalid inputs
leave the public result empty, preventing partial output.

**Target integration:**
- DS/DS Lite, DS+Pak and DSi compile the same portable decoder and
  execute a one-sprite synthetic validation, while keeping their native
  hardware scrolling background and OAM sprite path.
- Original and New 3DS build the exact synthetic scrolling scene,
  encode it as synthetic PSP-style PRIM sprite words with explicitly
  predecoded rectangles, decode them, and then submit the *decoded*
  bounded rectangle batch through Citro2D/PICA200.
- Host regression tests check the decoded output against every logical
  pixel of the original scene across eight frames; unsupported PSP
  instructions and truncated/invalid sequences must fail closed.

**Next required milestone:** genuine PSP VTYPE, VADDR and vertex-memory
decoding, a small set of PSP vertex formats and GPU state, texture
decoding and proper display list semantics. Real PSP program rendering
must not be claimed until verified guest display lists are translated.
There is no memory, performance, or commercial gameplay breakthrough
from this test fixture alone.

Documentation references:
- https://www.ppsspp.org/docs/psp-hardware/gpu/display-lists/
- https://www.psdevwiki.com/psp/Graphics_Engine
