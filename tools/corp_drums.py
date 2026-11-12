#!/usr/bin/env python3
"""CORPORATIONS re-analysis, the DRUMS: the drum stem is a drum machine (the hits of one drum are the same waveform every time: kick 0.99, snare 0.94 correlation),
so the part is taken apart like one:

  1. KICK   matched filter in the kick's own band (25-150 Hz); the sample itself is recovered by averaging the aligned isolated hits (everything else averages away),
            and then subtracted from the stem (an isolated kick cancels to about -19 dB)
  2. SNARE  the same in 150-3000 Hz on what is left (ghost notes are the same sample, quieter: the gain becomes the note's volume)
  3. TOM    the 260 Hz drum of the fills, the same again
  4. HATS   what is left above 6 kHz: an onset per hat, long (the open-sounding eighth-note hat, about 150 ms) or short (closed)
Every hit keeps its exact time; build / eval turn that into tracker ticks (the off sixteenths are swung about 32 ms late).
The kit of the module is cut from the stem: the averaged kick, and the cleanest isolated hit of each other drum.

usage: python3 tools/corp_drums.py   -> tools/corporations/drums.json + tools/corporations/kit/*.wav (and prints the bar grid)"""
import os, sys, json
import numpy as np
from scipy import signal
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from corp_lib import *

L = int(0.224 * SR)        # a template is just under an eighth note long: in this pattern nothing else is ever inside it

def matched(y, d, min_gap, thr):
    """least-squares gain of template d at every lag of y, peaks above thr"""
    c = signal.fftconvolve(y, d[::-1], mode="full")[len(d) - 1:] / max(1e-12, (d ** 2).sum())
    pk, _ = signal.find_peaks(c, height=thr, distance=int(min_gap * SR)); return pk, c[pk]

def band_tpl(d, band): return bp(np.concatenate([np.zeros(3000), d, np.zeros(3000)]), *band)[3000:-3000]

def seed(res, band, L=L, lo_pct=70, hi_pct=95):
    """a first guess of a drum's sample: an isolated, strong burst in its band"""
    yb = bp(res, *band); e = env(yb, 5.0); hop = 64; ee = e[::hop]; le = np.log(ee + 1e-4)
    d = np.concatenate([[0, 0, 0], le[3:] - le[:-3]]); w = int(0.06 * SR / hop); on = []
    i = 0
    while i < len(d):
        if d[i] > 0.8 and d[i] >= d[max(0, i - w):i + w].max() and ee[min(len(ee) - 1, i + 6)] > 0.01: on.append(i * hop); i += w
        else: i += 1
    on = np.array(on); lev = np.array([e[o:o + int(0.05 * SR)].max() for o in on])
    iso = [i for i in range(1, len(on) - 1) if on[i + 1] - on[i] > 0.226 * SR and on[i] - on[i - 1] > 0.15 * SR and np.percentile(lev, lo_pct) <= lev[i] <= np.percentile(lev, hi_pct)]
    i = iso[len(iso) // 2]; a = on[i] - int(0.006 * SR); return res[a:a + L].copy()

def learn(res, d0, band, thr, min_gap=0.05, rounds=3, iso_gap=0.2265, label=""):
    """events of one drum by matched filter in `band`; its full-band sample = the average of the isolated strong events; repeated"""
    d = d0 / np.abs(d0).max()
    for r in range(rounds):
        yb = bp(res, *band); pk, g = matched(yb, band_tpl(d, band), min_gap, thr)
        strong = np.percentile(g, 55)
        iso = [i for i in range(len(pk)) if (i + 1 == len(pk) or pk[i + 1] - pk[i] > iso_gap * SR) and g[i] >= strong and pk[i] + len(d) < len(res)]
        if len(iso) >= 8:
            d = np.mean([res[pk[i]:pk[i] + len(d)] / g[i] for i in iso], axis=0); d[-600:] *= np.linspace(1, 0, 600)
    return d

def analyse():
    x = load("drums").astype(np.float64); res = x.copy(); ev = []; tpl = {}; info = {}
    def take(name, d, band, thr_rel, min_gap=0.05):
        """final detection of one drum and its subtraction; thr_rel is relative to the median gain of the strong hits"""
        nonlocal res
        yb = bp(res, *band); pk, g = matched(yb, band_tpl(d, band), min_gap, 0.02)
        ref = np.median(g[g > 0.5 * np.percentile(g, 99.5)]); keep = g >= thr_rel * ref; pk, g = pk[keep], g[keep]        # the reference gain: the typical full-strength hit
        before = (yb ** 2).sum()
        for t, gg in zip(pk, g): n = min(len(d), len(res) - t); res[t:t + n] -= gg * d[:n]
        after = (bp(res, *band) ** 2).sum()
        att = int(np.argmax(np.abs(d) > 0.2 * np.abs(d).max()))                                                   # where the hit really starts inside the template
        for t, gg in zip(pk, g): ev.append(dict(s=int(t + att), k=name, g=float(gg / ref), a=int(att)))
        tpl[name] = d; info[name] = dict(events=int(len(pk)), band_hz=list(band), band_energy_left_db=round(10 * np.log10(after / before), 1), reference_gain=float(ref))
        print("  %-5s %4d hits, %.1f dB of the %d-%d Hz band left after taking it out" % (name, len(pk), 10 * np.log10(after / before), band[0], band[1]))
    # 1 kick (nothing of it lives above 5 kHz: what plays up there with a kick is the hat)
    dK = lp(learn(res, lp(seed(res, (25, 150)), 4000), (25, 150), 0.2), 5000)
    take("kick", dK, (25, 150), 0.45, 0.07)
    # 2 snare
    dS = learn(res, seed(res, (150, 3000)), (150, 3000), 0.15)
    take("snare", dS, (150, 3000), 0.22, 0.05)
    # 3 the other drum of the fills (a tom around 260 Hz): seeded from the strongest isolated bursts that are left
    try:
        dT = learn(res, seed(res, (180, 1500), lo_pct=85, hi_pct=99), (180, 1500), 0.3)
        take("tom", dT, (180, 1500), 0.5, 0.05)
    except Exception as e: print("  no tom class:", e)
    # 4 hats: the high band of what is left
    hf = bp(res, 6000, 16000); e = env(hf, 3.0); hop = 32; ee = e[::hop]; le = np.log(ee + 2e-4)
    d = np.concatenate([[0, 0], le[2:] - le[:-2]]); w = int(0.05 * SR / hop); on = []; i = 0
    while i < len(d):
        if d[i] > 0.55 and d[i] >= d[max(0, i - w):i + w].max() and ee[min(len(ee) - 1, i + 8)] > 0.004: on.append(i * hop); i += w
        else: i += 1
    sn = np.array(sorted(v["s"] for v in ev if v["k"] in ("snare", "tom") and v["g"] > 0.3)); hats = []
    for o in on:
        seg = e[o:o + int(0.30 * SR)]
        if len(seg) < int(0.30 * SR): continue
        pk = seg[:int(0.03 * SR)].max(); below = np.where(seg[int(0.02 * SR):] < 0.35 * pk)[0]; dur = (below[0] + int(0.02 * SR)) / SR if len(below) else 0.30
        if len(sn) and np.min(np.abs(sn - o)) < 0.02 * SR: continue            # the snare sample carries its own top end
        hats.append(dict(s=int(o), k="hat_open" if dur > 0.105 else "hat", g=float(pk), dur=float(dur)))
    for k in ("hat_open", "hat"):
        gs = np.array([h["g"] for h in hats if h["k"] == k]); ref = np.median(gs[gs > np.percentile(gs, 50)])
        for h in hats:
            if h["k"] == k: h["g"] = float(h["g"] / ref)
        info[k] = dict(events=int(len(gs)), reference_level=float(ref))
        print("  %-8s %4d hits" % (k, len(gs)))
    ev += [dict(s=h["s"], k=h["k"], g=h["g"]) for h in hats if h["g"] > 0.25]
    ev.sort(key=lambda v: v["s"])
    # the kit
    attK = int(np.argmax(np.abs(dK) > 0.2 * np.abs(dK).max())); cut = max(0, attK - 40)
    kit = {"kick": fade(dK[cut:], 0.0005, 0.02)}
    xk = x.copy()
    for v in ev:
        if v["k"] == "kick": t0 = v["s"] - v["a"]; n = min(len(dK), len(xk) - t0); xk[t0:t0 + n] -= v["g"] * info["kick"]["reference_gain"] * dK[:n]
    allon = np.array([v["s"] for v in ev])
    def cleanest(name, length, src, pre=0.002, glo=0.9, ghi=1.25, need_gap=None):
        need = int((need_gap or length) * SR); best = None
        for v in ev:
            if v["k"] != name or not (glo <= v["g"] <= ghi) or v["s"] < 60 * SR or v["s"] > 285 * SR: continue
            others = allon[(allon > v["s"] + int(0.012 * SR)) & (allon < v["s"] + need)]; before = allon[(allon < v["s"] - int(0.012 * SR)) & (allon > v["s"] - int(0.10 * SR))]
            same = [u for u in ev if abs(u["s"] - v["s"]) <= int(0.012 * SR) and u["k"] != name and u["k"] != "kick"]
            if len(others) or len(before) or same: continue
            a = v["s"] - int(pre * SR); seg = src[a:a + int(length * SR)]; score = abs(v["g"] - 1.0)
            if best is None or score < best[0]: best = (score, seg.copy(), v["s"] / SR)
        if best is None: raise RuntimeError("no isolated " + name)
        print("  kit %-8s from %.2f s" % (name, best[2])); return best[1]
    kit["snare"] = fade(cleanest("snare", 0.21, xk), 0.0005, 0.02)
    if "tom" in tpl:
        try: kit["tom"] = fade(cleanest("tom", 0.20, xk, glo=0.7, ghi=1.6, need_gap=0.10), 0.0005, 0.03)
        except RuntimeError: kit["tom"] = fade(tpl["tom"][:int(0.22 * SR)], 0.0005, 0.03)
    kit["hat_open"] = fade(hp(cleanest("hat_open", 0.21, xk, glo=0.9, ghi=1.4), 400), 0.0005, 0.02)
    kit["hat"] = fade(hp(cleanest("hat", 0.16, xk, glo=0.8, ghi=1.4, need_gap=0.16), 400), 0.0005, 0.02)
    # resynthesis from the kit (what the module's drum channels play)
    y = np.zeros(len(x)); peak = {}
    for name, smp in kit.items(): peak[name] = float(np.abs(smp).max())
    refs = {"kick": info["kick"]["reference_gain"] * np.abs(dK).max() / peak["kick"]}
    for v in ev:
        smp = kit[v["k"]]; t0 = v["s"] - (40 if v["k"] == "kick" else int(0.002 * SR)); n = min(len(smp), len(y) - t0)
        gain = v["g"] * refs["kick"] if v["k"] == "kick" else v["g"]
        y[t0:t0 + n] += gain * smp[:n]
    sel = slice(int(17 * SR), int(350 * SR)); rep = {}
    for nm, (lo, hi) in (("kick band 30-110", (30, 110)), ("body 150-400", (150, 400)), ("mid 1-4 kHz", (1000, 4000)), ("hats 7-16 kHz", (7000, 16000))):
        a = env(bp(x, lo, hi), 10)[sel][::441]; b = env(bp(y, lo, hi), 10)[sel][::441]; rep[nm] = round(float(np.corrcoef(a, b)[0, 1]), 3)
    print("  envelope correlation of the kit's resynthesis with the stem:", rep)
    return ev, kit, info, rep, y

def grid_text(ev, B0, B1):
    out = []
    for B in range(B0, B1):
        cells = {}
        for v in ev:
            b, tk = pos_of(v["s"] / SR)
            tk = int(round(tk))
            if tk >= TPB: b += 1; tk -= TPB
            if b == B: cells.setdefault(tk // 6, []).append((v["k"], v["g"], tk % 6))
        def cell(s, order, sym):
            for c in order:
                for (k, g, sw) in cells.get(s, []):
                    if k == c: return sym[c] if g > 0.6 else sym[c].lower()
            return "."
        l1 = "".join(cell(s, ("kick", "snare", "tom"), dict(kick="K", snare="S", tom="T")) for s in range(16)); l2 = "".join(cell(s, ("hat_open", "hat"), dict(hat_open="O", hat="H")) for s in range(16))
        out.append("%3d %6.1f  %s %s %s %s   %s %s %s %s" % (B, bar_time(B), l1[0:4], l1[4:8], l1[8:12], l1[12:16], l2[0:4], l2[4:8], l2[8:12], l2[12:16]))
    return "\n".join(out)

def main():
    import soundfile as sf
    ev, kit, info, rep, y = analyse()
    os.makedirs(OUT + "/kit", exist_ok=True)
    for name, smp in kit.items(): sf.write("%s/kit/%s.wav" % (OUT, name), smp / max(1e-9, np.abs(smp).max()) * 0.95, SR, subtype="PCM_16")
    json.dump(dict(info=info, envelope_correlation=rep, kit_peak={k: float(np.abs(v).max()) for k, v in kit.items()}, events=ev), open(OUT + "/drums.json", "w"))
    np.save(os.path.join(CACHE, "drums_resynth.npy"), y.astype(np.float32))
    print(grid_text(ev, int(sys.argv[1]) if len(sys.argv) > 1 else 9, int(sys.argv[2]) if len(sys.argv) > 2 else 60))

if __name__ == "__main__": main()
