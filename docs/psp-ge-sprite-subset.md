# Experimental PSP GE sprite-command subset

The portable decoder translates opaque, untextured sprite pairs from
a caller-supplied PSP memory window into the shared rectangle batch.
The DS validation and 3DS scrolling scene use synthetic fixtures.

The PSP Graphics Engine uses 32-bit command words (8-bit opcode and
24-bit argument). The implemented subset recognizes precisely:

| Opcode | Command | Accepted behavior |
| --- | --- | --- |
| 0x00 | NOP | Exact zero argument, before FINISH |
| 0x04 | PRIM | Sprite type 6 with exactly 2 vertices, encoded as 0x04060002 |
| 0x0F | FINISH | Zero argument, once, after any sprites |
| 0x0C | END | Zero argument, once, last command after FINISH |

The memory path also accepts BASE (0x10), VADDR (0x01) and one VTYPE
(0x12) value, 0x800114. Each vertex stores little-endian ABGR5551
followed by 16-bit through-mode X, Y and Z. Each sprite consumes two
vertices, advancing the current vertex address by 16 bytes. The pair
must have matching opaque colors and Z values and define a positive
rectangle wholly inside 256×192. The decoder checks alignment and
bounds on every fetch. Unsupported GPU state or malformed input
leaves the output empty. The predecoded fixture API remains available.

DS and DSi run a packed-vertex decoder check alongside the native
BG/OAM scene. Original and New 3DS encode the synthetic scrolling
scene using this layout and submit the decoded batch through Citro2D.
Host tests cover consecutive vertex fetches, truncated memory,
misalignment, unsupported VTYPE and legacy fixture pixel parity.

Texture sampling, additional vertex formats and guest display-list
execution are future work. Commercial-game rendering remains unverified.

Documentation references:
- https://www.ppsspp.org/docs/psp-hardware/gpu/display-lists/
- https://www.psdevwiki.com/psp/Graphics_Engine
