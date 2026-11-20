# GodGuy frame stress benchmark

Run the benchmark from the repository root.

```sh
godguy/tools/bench_frame.sh
GG_STRESS_WIDE=1 godguy/tools/bench_frame.sh
GG_STRESS_FRAMES=2400 godguy/tools/bench_frame.sh ds dsi16 3ds
```

The benchmark starts a seeded battle between elves and dwarves. The shared 256 × 256 scene starts with 10,368 creatures and 48 war groups. The camera pans throughout the run, and the script starts tornadoes, black holes, fires, and dragons as hazard slots become available. The game warms up for 120 frames, then records 1,200 frames and 4,800 simulation ticks by default. Wide mode adds a 768 × 768 scene with 25,920 creatures on DSi and a 1,024 × 1,024 scene with 34,560 creatures on 3DS and the PC shim.

The benchmark drives the regular gameplay loop with a fixed input sequence. It checks the tick count, battle activity, camera movement, and that every frame reached the timing hooks. Each run reports an ending creature fingerprint so timing runs can be compared against the same simulation state. The script runs in a temporary directory to isolate the save path.

## Reading the report

- **Simulation** includes all four ticks in a frame. The creature, disaster, and maintenance rows are included in that total.
- **Game draw/update** covers terrain, sprites, lighting, and the HUD. The HUD and minimap row is included in the draw total.
- **Audio sequencer** covers the game's music and effects logic. The DS configurations advance the modeled sound channels outside the reported CPU total. The 3DS runs measure basic and HQ software mixing separately, at 32,768 samples per second.
- **Top 1%** is the average time each phase took in the same set of slowest total frames. It can show which subsystem was active during a spike. The p99 column gives the 99th percentile of each row independently.

All times are desktop thread CPU time. The DS and DSi numbers cover game work through the shim. The 3DS rows also cover software composition and mixing. Console clock rates, hardware video and audio, the 3DS framebuffer upload and DSP queue, and VBlank are outside these measurements.

## Baseline on commit `35159dd`

Measured with GCC 14.2.0 on an x86-64 Xeon Platinum 8573C. Two full runs agreed on the simulation fingerprint for all 11 scenarios. The timings below come from the second run. All values are in milliseconds per frame. The last four columns give separate phase averages across the 12 slowest total frames in each run.

| Configuration | p50 total | p99 total | Simulation | Game drawing | Composition | Mixer |
|---|---:|---:|---:|---:|---:|---:|
| DS cartridge | 0.108 | 0.554 | 0.568 | 0.124 | — | — |
| DS with SD saves | 0.108 | 0.556 | 0.604 | 0.156 | — | — |
| DSi in DS mode | 0.108 | 0.567 | 0.467 | 0.193 | — | — |
| DSi 16 MB mode | 0.154 | 0.716 | 0.844 | 0.122 | — | — |
| DSi 16 MB, 768 map | 0.279 | 1.487 | 2.431 | 0.309 | — | — |
| Old 3DS | 0.356 | 1.388 | 0.664 | 0.556 | 0.692 | 0.024 |
| New 3DS | 0.539 | 1.411 | 0.526 | 0.219 | 0.262 | 0.601 |
| Old 3DS, 1,024 map | 0.427 | 2.215 | 1.865 | 0.327 | 0.479 | 0.042 |
| New 3DS, 1,024 map | 0.611 | 2.688 | 1.988 | 0.806 | 0.704 | 0.490 |
| PC shim | 0.353 | 1.001 | 0.720 | 0.143 | 0.290 | 0.031 |
| PC shim, 1,024 map | 0.415 | 2.550 | 3.415 | 0.749 | 0.557 | 0.023 |

The music and effects sequencer took at most 0.009 ms on average across the slowest frames in all rows except Old 3DS, where a sequencer outlier raised the average to 0.159 ms.

- On DS cartridge settings, nine of the twelve slowest frames ran the 256-tick maintenance step. War group rebuilding cost 0.196 ms across those twelve frames, followed by notable bookkeeping at 0.087 ms and compaction at 0.030 ms. The HUD and minimap also cost 0.113 ms in those frames.
- On the 768-tile DSi map, maintenance was present in ten of the twelve slowest frames. Notable bookkeeping took 1.271 ms across the twelve frames. That makes the notable-character table the first path to profile on this larger workload.
- On the shared 3DS scene, the software compositor cost about 0.19 ms per frame on average. The HQ mixer averaged 0.212 ms, compared with 0.026 ms for the basic mixer on the same host. On the 1,024-tile scenes, simulation was the largest measured cost in the slowest frames.

The two existing 12,000-tick simulation fingerprint tests also passed with timing hooks disabled.
