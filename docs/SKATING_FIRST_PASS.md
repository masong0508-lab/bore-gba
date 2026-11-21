# BORE 1: casual skating / career opportunity and controls

New players start **unemployed**, not with an implied Pro Skater profession.
The optional skate-team opportunity becomes available on the existing job boards
only after skating level 1, five landed tricks and a 250-point combo. All other
jobs remain accessible normally. Existing working skaters are not removed.
Career IDs, save layouts, SRAM offsets, expansion pack layouts and assets stay
unchanged.

The control pass increases push acceleration, steering authority, non-drift
traction, jump buffering and coyote time. It loosens the clean/sketchy landing
margins a little. Gravity and top speed remain unchanged.

### Validation
- Host regression: `cc -std=c11 -O2 -Wall -Wextra -Wno-unused-function tools/test_skatefeel.c -o /tmp/bore-skatefeel && /tmp/bore-skatefeel`
- CI: compile full GBA ROM and creator, check EWRAM/IWRAM/ROM limits,
  verify all expansion patch orders with the existing splitter.
- Emulator/manual: skate ramps, wallrides, grinding, flips, air spins and
  collisions; compare busy skate park draw cost and FPS before/after.
- Follow-on: contest interactions, better ramp landing cues and targeted
  renderer profiling in crowded lots and expansion modes.
