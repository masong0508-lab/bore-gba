# BORE 1: casual skating / career opportunity and controls

New players start **unemployed**, not with an implied Pro Skater profession.
The optional skate-team opportunity becomes available on the existing job boards
only after skating level 1, five trick feats, a 250-point combo and a bronze-or-better medal earned in a real park jam. All other
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

### Small frame-time optimizations
- Wallride dismount heading uses eight precomputed exact results instead of a 128-angle trigonometric search. A general fallback remains for unrecognized directions.
- BORTUA FIGHTER BALLZ MODE shadows reuse the existing half-width cache for typical small projected radii instead of computing a square root for every scanline. Large shadows retain the original fallback. No additional buffers.

## Skating is a hobby, not a salary
`MY SIM > MORE > SKATE SESSION` now has unpaid free practice and PARK JAM.
A park jam requires your board and an equipped skate park; the contest ends
after two minutes and pays 0/120/300/600 for no/bronze/silver/gold medal.
Only one completed jam per in-game day is counted; practice may be repeated.
The existing 9-byte timed-run save block now uses its former spare byte
for a persistent jam-medal unlock (bit 7) and day tag modulo 127 (bits 0–6).
The checksum still covers the entire block; older saves read a zero spare byte.
No SRAM address, save chunk, ROM pack slot or texture allocation changed.

Collectibles remain accessible outside competitions, but do not give a
competitive score during a jam. Taking letters/tapes no longer increments
the lifetime "tricks landed" tally; they still feed the appropriate wants,
special meter, sound and goal systems. Existing Pro Skater employees remain
employed, including legacy saves.

### Boundary safety
The ordinary movement collision probe now treats off-map cells and full
walls as permanently solid even at extreme ollie heights. Upper-floor gaps
are similarly blocked. Falling-ramp tolerance and wallride checks use
guarded tile probes; the outside of a lot is not a wallride surface.
Manual testing should include maximum-height ramps by all four boundaries.

### Editor window preview
Window item previews now sample the 24-row window texture at indices
1, 3, ... 23 (not 3, 5, ... 25). GCC had diagnosed undefined
behaviour at the final iteration. No new pixels or assets are allocated.
