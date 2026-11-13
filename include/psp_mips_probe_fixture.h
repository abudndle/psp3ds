#ifndef PSP_MIPS_PROBE_FIXTURE_H
#define PSP_MIPS_PROBE_FIXTURE_H

/* Original two-instruction MIPS32/Allegrex arithmetic fixture.
 * These opcodes are executed by the portable DS/3DS core and the
 * upstream PPSSPP interpreter probe. They are not a PSP game. */
#define PSP_PROBE_GUEST_BASE 0x08800000u
#define PSP_PROBE_ADDIU_T0 0x2408002au /* addiu t0, zero, 42 */
#define PSP_PROBE_ADDIU_T1 0x2509fffeu /* addiu t1, t0, -2 */
#define PSP_PROBE_EXPECT_T0 42u
#define PSP_PROBE_EXPECT_T1 40u
#define PSP_PROBE_EXPECT_PC (PSP_PROBE_GUEST_BASE + 8u)

#endif
