#!/usr/bin/env python3
"""Validate the authored 3DS campaign and its unlockable feature-length archive.

Source-data check, not a substitute for C compilation, 3DS emulator playback,
measured frame pacing or visual inspection of 3D character animation.
"""
from pathlib import Path
import re

BASE = Path(__file__).resolve().parents[1] / "source"
shared = (BASE / "story_reels.h").read_text()
feature = (BASE / "story_feature_3ds.h").read_text()

ARRAY = re.compile(r"static const FilmBeat (\w+)\[\]=\{(.*?)\};", re.S)
ENTRY = re.compile(r"^\s*(S|L|A|SHOT|FD|FA)\(([^\n]*)\)\s*,?", re.M)
EXPLICIT = re.compile(r"^\s*\{F_\w+,F_\w+,F_\w+,F_\w+,F_\w+,(\d+),", re.M)
STRING = re.compile(r'"([^"\n]*)"')
KEYWORDS = re.compile(r"\b(TBD|LOREM|PLACEHOLDER|FILLER)\b", re.I)


def fail(msg):
    raise SystemExit(f"STORY DURATION FAIL: {msg}")


def arrays(src):
    return {name: body for name, body in ARRAY.findall(src)}


def parts(body):
    items = []
    for kind, args in ENTRY.findall(body):
        seconds = {"S": 7, "L": 8, "A": 7, "FD": 8, "FA": 8}.get(kind)
        if seconds is None:
            fields = args.split(",")
            seconds = int(fields[5])
        items.append((kind, seconds, args))
    for seconds in EXPLICIT.findall(body):
        items.append(("explicit", int(seconds), ""))
    return items


def assert_captions(body, label):
    for line in STRING.findall(body):
        if len(line) > 41:
            fail(f"{label}: clipped subtitle {len(line)} characters: {line}")
        if KEYWORDS.search(line):
            fail(f"{label}: unfinished subtitle {line}")
    lines = [line for line in STRING.findall(body) if line]
    for previous, current in zip(lines, lines[1:]):
        if previous == current:
            fail(f"{label}: repeated adjacent line: {current}")


base = arrays(shared)
ext = arrays(feature)
main_names = [
    "firstLightBeats", "firstRainBeats", "roofBeats", "fordBeats",
    "shadowBeats", "forgeBeats", "hallBeats", "inkBeats",
    "hearthBeats", "siegeBeats", "cityBeats", "libraryBeats",
    "couchBeats", "godhoodBeats", "finaleBeats",
]
expanded_names = [
    "firstLightExtra", "folkExtra", "roofExtra", "fordExtra",
    "shadowExtra", "forgeExtra", "hallExtra", "inkExtra",
    "hearthExtra", "siegeExtra", "cityExtra", "libraryExtra",
    "couchExtra", "godhoodExtra", "finaleExtra",
]
feature_names = [
    "featureLight", "featureRain", "featureRoof", "featureFord",
    "featureShadow", "featureForge", "featureHall", "featureInk",
    "featureHearth", "featureSiege", "featureCity", "featureLibrary",
    "featureCouch", "featureGodhood", "featureFinale",
]
bonus_names = [
    "bonusFirstWinter", "bonusWinterBridge", "bonusCartographer",
    "bonusOpenRoom", "bonusNamesAtDawn", "bonusAfterCrown",
]

if len(feature_names) != 15 or len(bonus_names) != 6:
    fail("wrong number of films")

for name in main_names + expanded_names:
    if name not in base:
        fail(f"missing shared chapter {name}")

for name in feature_names + bonus_names:
    if name not in ext:
        fail(f"missing 3DS film {name}")
    assert_captions(ext[name], name)

for name in feature_names:
    beats = parts(ext[name])
    if len(beats) != 25 or any(sec != 8 for _, sec, _ in beats):
        fail(f"{name} needs 25 authored eight-second shots, got {len(beats)}")
for name in bonus_names:
    beats = parts(ext[name])
    if len(beats) != 50 or any(sec != 8 for _, sec, _ in beats):
        fail(f"{name} needs 50 authored eight-second shots, got {len(beats)}")

def duration(data, names):
    return sum(sec for name in names for _, sec, _ in parts(data[name]))


base_seconds = duration(base, main_names)
extra_seconds = duration(base, expanded_names)
feature_seconds = duration(ext, feature_names)
bonus_seconds = duration(ext, bonus_names)
campaign_seconds = base_seconds + extra_seconds + feature_seconds

if campaign_seconds < 80 * 60:
    fail(f"3DS campaign is only {campaign_seconds / 60:.1f} minutes")
if bonus_seconds != 40 * 60:
    fail(f"archive is {bonus_seconds / 60:.1f} minutes instead of 40")
if campaign_seconds + bonus_seconds < 120 * 60:
    fail("combined screenplay is shorter than two hours")

if '#include "story_feature_3ds.h"' not in shared:
    fail("feature reels are not included in cinematic runtime")
if "REEL(bonusFirstWinter)" not in feature or "REEL(featureFinale)" not in feature:
    fail("feature/archive reel tables are missing")
if "3,5,8,11,14,255" not in feature:
    fail("chapter unlock gates are missing")
for banned in ("malloc(", "calloc(", "realloc(", "free("):
    if banned in feature:
        fail("the screenplay must not allocate heap memory")

print(f"STORY DATA OK: {len(feature_names)} campaign films, "
      f"{feature_seconds // 8} new feature beats; "
      f"{campaign_seconds // 60}:{campaign_seconds % 60:02d} campaign, "
      f"{bonus_seconds // 60}:00 bonus, "
      f"{(campaign_seconds + bonus_seconds) // 60}:"
      f"{(campaign_seconds + bonus_seconds) % 60:02d} combined")
print("Additional baked movie footage is excluded from these durations.")
