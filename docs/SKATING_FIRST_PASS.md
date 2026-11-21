undefined
### AI skateboarders and contest venues
NPC skaters now choose their route targets from a fixed 48-entry reservoir
of map skate objects. The reservoir is sampled uniformly and built on each
playable lot load; repeated two-pass full-map scans when landing are gone.
The cache costs 96 bytes of EWRAM plus one byte for its count, zero heap, no
assets, and no new work each frame. Parks with many objects still receive a
representative set of destinations. An existing in-flight NPC will not
immediately repeat its current destination unless there is only one object.
A PARK JAM requires at least three skateable pieces on the current map,
in addition to the appropriate community skate-park designation.
Regression testing checks reservoir behavior at and beyond capacity.
