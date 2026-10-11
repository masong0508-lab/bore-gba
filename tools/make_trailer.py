#!/usr/bin/env python3
"""THE BORE COMMERCIAL: an old-TV, late-night GBA spot cut from headless mGBA captures.
Needs: the scratchpad's emu (its `rec file.rgb` writes every frame as raw RGB24 240x160, `wav` the sound at 32768 Hz), ffmpeg, numpy, PIL.
Captures: tools/trailer/*.txt are the emulator scripts (one a scene); each leaves tr/<scene>.rgb + tr/<scene>.wav.
Run:  python3 tools/make_trailer.py <capture dir> <out.mp4>
The cut, the cards and the jokes are the SCENES table below (seconds in the finished spot, frames of the capture at 60 fps).
The look: a 4:3 tube (640x480) with the picture at 2.667x, scanlines, a little bleed, a rolling bar, jitter, noise, barrel curve, vignette,
rounded corners; channel-change static between scenes; the set clicks off at the end.
"""
import sys, os, wave, subprocess, math, random
import numpy as np
from PIL import Image, ImageDraw, ImageFont

CAP = sys.argv[1] if len(sys.argv) > 1 else 'tr'
OUT = sys.argv[2] if len(sys.argv) > 2 else 'bore_commercial.mp4'
FPS = 30; SR = 32768; GW, GH = 240, 160; TW, TH = 640, 480; PW, PH = 640, 426; PY0 = (TH - PH) // 2
FONT = '/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf'
FONT_R = '/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf'
random.seed(7); rng = np.random.default_rng(7)

# ---- the cut: (clip, first capture frame (60 fps), seconds on screen, [cards]) ; a card = (t0, t1, text, style) with t in scene seconds ----
# styles: big (yellow, centre), sub (white, lower third), fine (small white, bottom), price (starburst), top (small, top)
SCENES = [
 ('static', 0, 1.4, []),
 ('title', 300, 4.2, [(0.3, 2.2, 'ARE YOU... BORED?', 'big'), (2.4, 4.2, 'GET BORE.', 'big')]),
 ('menu', 30, 3.2, [(0.2, 3.2, 'A VOXEL LIFE SIM FOR THE GAME BOY ADVANCE', 'sub'), (1.4, 3.2, '(yes. the one in your drawer.)', 'fine')]),
 ('creator', 20, 3.6, [(0.2, 1.8, 'MAKE A BORE. ANY BORE.', 'sub'), (1.8, 3.6, 'ALL THE GENDERS. AND A BUTT SLIDER.', 'sub')]),
 ('play', 30, 4.6, [(0.3, 2.3, 'LIVE A LIFE.', 'big'), (2.3, 4.4, 'A SMALL ONE.', 'big')]),
 ('build', 10, 5.2, [(0.2, 2.4, '200 NEW THINGS TO BUY', 'sub'), (2.4, 5.2, 'GOLD TOILET $5000. BONG $150. PRIORITIES.', 'sub')]),
 ('weed', 140, 4.4, [(0.2, 2.0, 'GROW YOUR OWN TOMATOES*', 'sub'), (2.0, 4.4, '*NOT TOMATOES', 'sub'), (0.2, 4.4, 'GROWN-UPS ONLY', 'top')]),
 ('taco', 640, 6.5, [(0.2, 2.0, 'LOADING... GO GET SNACKS', 'sub'), (2.5, 6.5, 'TACO BORE: THE MUNCHIES\' NATURAL ENEMY', 'sub')]),
 ('skate', 30, 4.0, [(0.3, 4.0, 'SKATE. BAIL. REPEAT.', 'big')]),
 ('pets', 20, 3.6, [(0.2, 1.8, 'ADOPT A DOG.', 'big'), (1.8, 3.6, 'FORGET TO FEED IT.', 'big')]),
 ('jukebox', 40, 4.0, [(0.2, 2.0, '23 SONGS. ONE TINY SPEAKER.', 'sub'), (2.0, 4.0, 'NOW WITH 100% LESS CRACKLE', 'sub')]),
 ('fight_slap', 200, 6.0, [(0.3, 2.8, 'BORTUA FIGHTER: FIGHT YOUR FRIENDS', 'sub'), (3.4, 6.0, '...OR MOON THEM.', 'sub')]),
 ('fight_burst', 240, 7.5, [(6.5, 7.5, 'RATED M FOR MUNCHIES', 'sub')]),
 ('end', 0, 6.5, []),
]
# (the taco capture: the cab's loading screens run to frame 770, the arrival, the counter and the order follow; the fights: the cheat code lands at 281)

def font(sz, reg=False): return ImageFont.truetype(FONT_R if reg else FONT, sz)
def rgb_clip(name):
    p = os.path.join(CAP, name + '.rgb')
    if not os.path.exists(p): return None
    m = np.memmap(p, dtype=np.uint8, mode='r'); n = len(m) // (GW * GH * 3)
    return m.reshape(n, GH, GW, 3)
def wav_clip(name):
    p = os.path.join(CAP, name + '.wav')
    if not os.path.exists(p): return np.zeros((0, 2), np.int16)
    w = wave.open(p); d = np.frombuffer(w.readframes(w.getnframes()), np.int16).reshape(-1, 2); return d

# ---- the tube ----
yy, xx = np.mgrid[0:TH, 0:TW].astype(np.float32)
nx = (xx - TW / 2) / (TW / 2); ny = (yy - TH / 2) / (TH / 2); r2 = nx * nx + ny * ny
K = 0.06; sx = nx * (1 + K * r2); sy = ny * (1 + K * r2)   # barrel: where each tube pixel looks on the flat picture
MAPX = np.clip((sx * (TW / 2) + TW / 2).astype(np.int32), 0, TW - 1); MAPY = np.clip((sy * (TH / 2) + TH / 2).astype(np.int32), 0, TH - 1)
VIG = (1.0 - 0.55 * np.clip(r2 - 0.25, 0, 1)) ** 1.3
CORNER = ((np.abs(nx) ** 8 + np.abs(ny) ** 8) < 1.0).astype(np.float32)   # a rounded screen edge
SCAN = np.ones((TH, 1, 1), np.float32); SCAN[1::2, 0, 0] = 0.78
def tube(flat, t, static_amt=0.0):
    """flat: TH x TW x 3 uint8 (the picture already placed on the 4:3 canvas) -> the tube picture"""
    f = flat.astype(np.float32)
    if static_amt > 0: f = f * (1 - static_amt) + rng.random((TH, TW, 1), np.float32) * 255 * static_amt
    # bleed: red a pixel left, blue a pixel right, a little smear
    g = f.copy(); g[:, 1:, 0] = (f[:, 1:, 0] * 0.6 + f[:, :-1, 0] * 0.4); g[:, :-1, 2] = (f[:, :-1, 2] * 0.6 + f[:, 1:, 2] * 0.4)
    g[:, 1:, :] = g[:, 1:, :] * 0.85 + g[:, :-1, :] * 0.15
    g *= SCAN
    bar = (yy[:, 0:1] - ((t * 190.0) % (TH + 160)) + 80)   # a rolling bar every couple of seconds
    g *= (1 + 0.10 * np.exp(-(bar / 28.0) ** 2))[:, :, None]
    g *= (1 + 0.025 * math.sin(t * 37.0))   # flicker
    g = g[MAPY, MAPX]                        # barrel
    g *= VIG[:, :, None] * CORNER[:, :, None]
    g += rng.normal(0, 2.5, (TH, TW, 1)).astype(np.float32)
    return np.clip(g, 0, 255).astype(np.uint8)

def place(frame_rgb, jitter=0):
    im = Image.fromarray(np.ascontiguousarray(frame_rgb)).resize((PW, PH), Image.NEAREST)
    can = Image.new('RGB', (TW, TH), (0, 0, 0)); can.paste(im, (jitter, PY0)); return can

def draw_card(can, text, style, age, left):
    d = ImageDraw.Draw(can, 'RGBA'); w = TW
    pop = min(1.0, age / 0.18); ease = 1 - (1 - pop) ** 3
    if style == 'big':
        sz = int(44 * (0.6 + 0.4 * ease)); f = font(sz); tw = d.textlength(text, font=f)
        while tw > TW - 40 and sz > 20: sz -= 2; f = font(sz); tw = d.textlength(text, font=f)
        x = (w - tw) / 2 + random.randint(-1, 1); y = TH / 2 - sz / 2 + random.randint(-1, 1)
        for o in (4, 3): d.text((x + o, y + o), text, font=f, fill=(0, 0, 0, 220))
        d.text((x, y), text, font=f, fill=(255, 232, 40, 255))
    elif style == 'sub':
        f = font(24); tw = d.textlength(text, font=f)
        while tw > TW - 30: f = font(f.size - 1); tw = d.textlength(text, font=f)
        x = (w - tw) / 2; y = TH - 72 - (1 - ease) * 30
        d.rectangle((x - 14, y - 6, x + tw + 14, y + f.size + 8), fill=(0, 0, 0, 170))
        d.text((x + 2, y + 2), text, font=f, fill=(0, 0, 0, 255)); d.text((x, y), text, font=f, fill=(255, 255, 255, 255))
    elif style == 'top':
        f = font(18); tw = d.textlength(text, font=f); x = w - tw - 24; y = 36
        d.rectangle((x - 10, y - 4, x + tw + 10, y + f.size + 6), fill=(180, 20, 20, 210)); d.text((x, y), text, font=f, fill=(255, 255, 255, 255))
    elif style == 'fine':
        f = font(15, True); tw = d.textlength(text, font=f); x = (w - tw) / 2; y = TH - 42
        d.text((x + 1, y + 1), text, font=f, fill=(0, 0, 0, 255)); d.text((x, y), text, font=f, fill=(230, 230, 230, 255))
    return can

def starburst(d, cx, cy, r, text, f, fill=(255, 40, 40, 255), ink=(255, 255, 255, 255), spin=0.0):
    pts = []
    for i in range(24):
        a = spin + i * math.pi / 12; rr = r if i % 2 == 0 else r * 0.78; pts.append((cx + rr * math.cos(a), cy + rr * math.sin(a)))
    d.polygon(pts, fill=fill); tw = d.textlength(text, font=f); d.text((cx - tw / 2, cy - f.size / 2), text, font=f, fill=ink)

def end_card(t, T):
    can = Image.new('RGB', (TW, TH), (6, 6, 12)); d = ImageDraw.Draw(can, 'RGBA')
    pop = min(1.0, t / 0.4); ease = 1 - (1 - pop) ** 3
    f = font(int(132 * (0.5 + 0.5 * ease))); tw = d.textlength('BORE', font=f)
    x = (TW - tw) / 2; y = 92 - f.size / 2 + 40
    for o in (6, 4, 2): d.text((x + o, y + o), 'BORE', font=f, fill=(90, 20, 0, 255))
    d.text((x, y), 'BORE', font=f, fill=(255, 190, 30, 255))
    if t > 0.8:
        f2 = font(26); s = 'IT\'S LIKE LIFE. BUT WITH A SAVE BUTTON.'; tw = d.textlength(s, font=f2); d.text(((TW - tw) / 2, 232), s, font=f2, fill=(255, 255, 255, 255))
    if t > 1.6:
        starburst(d, 556, 138, 70, 'ONLY', font(20), spin=t * 0.6); f5 = font(24); tw5 = d.textlength('$69.99*', font=f5); d.text((556 - tw5 / 2, 146), '$69.99*', font=f5, fill=(255, 255, 255, 255))
    if t > 2.2 and int(t * 3) % 2 == 0:
        f3 = font(30); s = 'CALL NOW  1-800-GET-BORE'; tw = d.textlength(s, font=f3); d.text(((TW - tw) / 2, 300), s, font=f3, fill=(255, 232, 40, 255))
    if t > 2.6:
        f4 = font(13, True)
        for i, s in enumerate(['*WE DON\'T SELL IT. IT\'S FREE. IT\'S A GITHUB REPO.', 'RATED M: MUNCHIES, MOONING, MILD VOXEL NUDITY.', 'PLAY RESPONSIBLY. PASS THE CONTROLLER.', 'A DIPPINN PRODUCTION']):
            tw = d.textlength(s, font=f4); d.text(((TW - tw) / 2, 372 + i * 17), s, font=f4, fill=(200, 200, 200, 255))
    return can

def static_frame(): return (rng.random((TH, TW, 1), np.float32) * 255).astype(np.uint8).repeat(3, axis=2)

# ---- build the picture and the sound ----
total = sum(s[2] for s in SCENES); NF = int(total * FPS)
print('spot: %.1f s, %d frames' % (total, NF))
audio = []
frames_out = []
proc = subprocess.Popen(['ffmpeg', '-y', '-v', 'error', '-f', 'rawvideo', '-pix_fmt', 'rgb24', '-s', '%dx%d' % (TW, TH), '-r', str(FPS), '-i', '-',
                         '-c:v', 'libx264', '-preset', 'medium', '-crf', '23', '-pix_fmt', 'yuv420p', OUT + '.video.mp4'], stdin=subprocess.PIPE)
tglob = 0.0
for si, (name, f0, secs, cards) in enumerate(SCENES):
    n = int(secs * FPS); clip = None if name in ('static', 'end') else rgb_clip(name); snd = wav_clip(name) if clip is not None else None
    if clip is not None and f0 >= len(clip): f0 = max(0, len(clip) - 2 * n - 1)
    # sound of the scene: the capture's samples from frame f0, scene length; static and the end card: noise / a hum
    ns = int(secs * SR)
    if snd is not None and len(snd):
        a0 = int(f0 * SR / 60); seg = snd[a0:a0 + ns].copy()
        if len(seg) < ns: seg = np.concatenate([seg, np.zeros((ns - len(seg), 2), np.int16)])
    elif name == 'static': seg = (rng.normal(0, 2200, (ns, 2))).astype(np.int16)
    else:
        tt = np.arange(ns) / SR; hum = (np.sin(2 * np.pi * 55 * tt) * 500 * np.exp(-tt * 0.6))
        seg = np.stack([hum, hum], 1).astype(np.int16)
    # channel change: 4 frames of static over the first frames (picture) and a burst in the sound
    if si > 0 and name not in ('static',):
        burst = int(0.07 * SR); seg[:burst] = (seg[:burst] * 0.3 + rng.normal(0, 3000, (burst, 2))).astype(np.int16)
    ramp = np.linspace(0, 1, int(0.012 * SR))[:, None]; seg = seg.astype(np.float32); seg[:len(ramp)] *= ramp; seg[-len(ramp):] *= ramp[::-1]
    audio.append(seg)
    for i in range(n):
        t = i / FPS; tg = tglob + t
        if name == 'static':
            can = Image.fromarray(static_frame()); d = ImageDraw.Draw(can); d.text((40, 30), '\u25b6 PLAY', font=font(28), fill=(255, 255, 255)); d.text((TW - 110, 30), 'CH 4', font=font(28), fill=(255, 255, 255))
        elif name == 'end': can = end_card(t, secs)
        else:
            fi = min(len(clip) - 1, f0 + i * 2); jit = random.choice([0, 0, 0, 0, 0, 0, 0, 0, 1, -1]) if random.random() < 0.15 else 0
            can = place(clip[fi], jit)
            for (c0, c1, text, style) in cards:
                if c0 <= t < c1: can = draw_card(can, text, style, t - c0, c1 - t)
        sa = 0.0
        if si > 0 and name != 'static' and i < 4: sa = 0.85 - 0.2 * i      # channel change
        if name == 'end' and t > secs - 0.35: # the set clicks off: the picture collapses to a line
            k = (t - (secs - 0.35)) / 0.35; arr = np.array(can); h = max(2, int(TH * (1 - k) ** 2)); y0 = (TH - h) // 2
            small = np.array(Image.fromarray(arr).resize((TW, h))); arr[:] = 0; arr[y0:y0 + h] = np.clip(small.astype(np.float32) * (1 + 2 * k), 0, 255).astype(np.uint8); can = Image.fromarray(arr)
        pic = tube(np.array(can), tg, sa)
        proc.stdin.write(pic.tobytes())
    tglob += secs
proc.stdin.close(); proc.wait()
mix = np.concatenate(audio); peak = max(1.0, np.abs(mix).max()); mix = (mix / peak * 29000).astype(np.int16)
# the set's own noise floor and the click at the end
hiss = rng.normal(0, 90, mix.shape); mix = np.clip(mix.astype(np.float32) + hiss, -32767, 32767).astype(np.int16)
w = wave.open(OUT + '.audio.wav', 'wb'); w.setnchannels(2); w.setsampwidth(2); w.setframerate(SR); w.writeframes(mix.tobytes()); w.close()
subprocess.run(['ffmpeg', '-y', '-v', 'error', '-i', OUT + '.video.mp4', '-i', OUT + '.audio.wav', '-c:v', 'copy', '-c:a', 'aac', '-b:a', '160k', '-shortest', OUT], check=True)
os.remove(OUT + '.video.mp4'); os.remove(OUT + '.audio.wav')
print('wrote', OUT, os.path.getsize(OUT) // 1024, 'KB')
