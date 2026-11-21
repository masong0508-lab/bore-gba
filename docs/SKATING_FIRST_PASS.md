undefined
### Boundary safety
The ordinary movement collision probe now treats off-map cells as permanently
solid even at very high ollie heights. Falling-ramp snap tolerance and
wallride checks use guarded tile probes; board edges are not walls to
wallride. This removes a class of out-of-bounds tile reads while preserving
ramp behavior inside the map. Manual testing should include maximum-height
ramps near all four lot boundaries.
