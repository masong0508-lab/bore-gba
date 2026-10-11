#!/usr/bin/env python3
# make_pets.py - the PETS' art (source/petart.h): dogs and cats as little 3D models (spheres, ellipsoids and capsules), ray-marched in the room's
# 2:1 isometric view and cut into 32 x 32 hardware sprites (4bpp tiles, 1D order), like the Sims' baked voxel bodies but made offline.
#   Two views (FRONT = facing screen left-down, BACK = facing screen left-up; the game flips them for the other two) x PET_FR poses:
#   STAND, WALK1, WALK2, SIT, LIE, SLEEP, EAT, HAPPY (a dog begs on its hind legs, a cat does a play bow), PEE (a leg up / a squat).
#   Colour indices: 0 clear, 1 outline, 2 eyes and nose, 3 red (collar, tongue, a cat's nose), 4..7 the coat: dark, mid, light, accent.
#   The game gives each pet on screen its own four coat colours in one shared palette bank (pets.h), so the coat is not in this file.
#   Variants: DOG PLAIN (accent muzzle, chest, paws), DOG SPOTS, CAT PLAIN (accent belly, muzzle, paws), CAT TABBY (stripes), CAT POINTS (Siamese).
# Needs numpy and Pillow. Writes source/petart.h, and tools/pets_preview.png with every coat (pets.h petCoat) to look at.
import numpy as np, os, sys
from PIL import Image

SIZES = (1.06, 1.3, 1.55)   # screen pixels per model unit: SMALL, MEDIUM, LARGE
S = SIZES[1]
AX, AY = 16, 27     # where the pet's feet (model origin) land in the 32 x 32 sprite
SS = 4              # samples per pixel along each axis
TH = np.radians(30.0)
F = np.array([-np.cos(TH)/np.sqrt(2), -np.cos(TH)/np.sqrt(2), -np.sin(TH)])   # the camera looks down at 30 degrees, from +X +Y
R = np.array([1, -1, 0])/np.sqrt(2)                                          # screen right
D = np.cross(R, F)                                                           # screen down
LIGHT = np.array([-0.35, 0.45, 0.85]); LIGHT /= np.linalg.norm(LIGHT)       # top lit, the left-down face mid, the right-down face dark (like the furniture)
COAT, ACC, EYE, RED = 0, 1, 2, 3

def rotm(axis, deg):
    a = np.radians(deg); c, s = np.cos(a), np.sin(a)
    if axis == 'y': return np.array([[c, 0, -s], [0, 1, 0], [s, 0, c]])   # pitch: +deg lifts the front (+x) up
    if axis == 'z': return np.array([[c, -s, 0], [s, c, 0], [0, 0, 1]])   # yaw: +deg turns the front to the left (+y)
    return np.array([[1, 0, 0], [0, c, -s], [0, s, c]])

class Xf:   # p' = M p + t
    def __init__(s, M=None, t=None): s.M = np.eye(3) if M is None else M; s.t = np.zeros(3) if t is None else np.asarray(t, float)
    def __mul__(s, o): return Xf(s.M @ o.M, s.M @ o.t + s.t)
def about(axis, deg, piv, move=(0, 0, 0)):   # turn about a pivot, then move
    M = rotm(axis, deg); piv = np.asarray(piv, float); return Xf(M, piv - M @ piv + np.asarray(move, float))

class Prim:
    def __init__(s, kind, a, b=None, r=None, lab=COAT, pat=None, xf=None):
        s.kind, s.a, s.b, s.r, s.lab, s.pat, s.xf = kind, np.asarray(a, float), (None if b is None else np.asarray(b, float)), r, lab, pat, xf or Xf()

def sd_prim(p, q):   # p: N x 3 in the primitive's own (rest) frame
    if q.kind == 'sph': return np.linalg.norm(p - q.a, axis=1) - q.r
    if q.kind == 'ell':
        rr = np.asarray(q.r, float); x = (p - q.a)/rr; k0 = np.linalg.norm(x, axis=1); k1 = np.linalg.norm(x/rr, axis=1)
        return k0*(k0 - 1)/np.maximum(k1, 1e-6)
    if q.kind == 'cap':   # a capsule that may taper: radius r[0] at a, r[1] at b
        ab = q.b - q.a; ap = p - q.a; h = np.clip((ap @ ab)/(ab @ ab), 0, 1)
        r0, r1 = (q.r, q.r) if np.isscalar(q.r) else q.r
        return np.linalg.norm(ap - np.outer(h, ab), axis=1) - (r0 + (r1 - r0)*h)
    raise ValueError(q.kind)

def to_rest(p, xf):   # world (pet) -> the primitive's rest frame
    return (p - xf.t) @ xf.M

def scene(prims, p):
    ds = np.stack([sd_prim(to_rest(p, q.xf), q) for q in prims])
    return ds

def smin_all(ds, k=0.9):   # a soft union (body parts melt together a little)
    d = ds[0]
    for e in ds[1:]:
        h = np.clip(0.5 + 0.5*(e - d)/k, 0, 1); d = e*(1 - h) + d*h - k*h*(1 - h)
    return d

def hash3(c):
    c = np.floor(c).astype(np.int64)
    h = (c[:, 0]*73856093) ^ (c[:, 1]*19349663) ^ (c[:, 2]*83492791)
    return ((h & 0xFFFF)/65535.0)

def pattern(q, prest):   # which coat parts turn to the accent colour
    if q.pat is None: return np.zeros(len(prest), bool)
    kind, arg = q.pat
    if kind == 'below': return prest[:, 2] < arg                      # a light belly / chest
    if kind == 'all': return np.ones(len(prest), bool)
    if kind == 'spots':                                               # round spots on a jittered grid
        g = prest/arg; out = np.zeros(len(prest), bool)
        for dx in (0, 1):
            for dy in (0, 1):
                for dz in (0, 1):
                    cell = np.floor(g) + np.array([dx, dy, dz]) - 0.5
                    h = hash3(cell + 0.5); c = cell + 0.25 + 0.5*np.stack([hash3(cell + 7.1), hash3(cell + 3.3), hash3(cell + 5.7)], 1)
                    out |= (np.linalg.norm(g - c, axis=1) < 0.32) & (h < 0.6)
        return out
    if kind == 'stripes': return (np.sin(prest[:, 0]*arg + prest[:, 2]*0.6) > 0.35)   # tabby bands across the back
    raise ValueError(kind)

# ---------------- the models ----------------
# Local frame: x forward, y left, z up, feet on z = 0. Units: about one screen pixel.
def dog(pose, var, build='floppy'):
    P = []
    plain = var == 'plain'
    spitz = build == 'spitz'
    acc = (lambda: ('all', 0)) if plain else (lambda: None)
    body = Xf(); head = Xf(); hip = (-2.8, 0, 6.4)
    legs = {'fl': 0, 'fr': 0, 'bl': 0, 'br': 0}; tail = 35; tongue = False; eyes = True; bfold = False; ffold = False; fup = False; liftleg = False
    if pose in ('walk1', 'walk2'):
        s = 24 if pose == 'walk1' else -24; legs = {'fl': s, 'br': s, 'fr': -s, 'bl': -s}; tail = 45 if pose == 'walk1' else 25
    if pose == 'sit': body = about('y', 32, hip, (0, 0, -3.1)); head = about('y', -32, (4.6, 0, 8.6)); bfold = True; tail = -60
    if pose == 'happy': body = about('y', 68, hip, (1.2, 0, -3.3)); head = about('y', -66, (4.6, 0, 8.6)); bfold = True; fup = True; tongue = True; tail = -70
    if pose in ('lie', 'sleep'): body = Xf(np.eye(3), (0, 0, -3.3)); ffold = True; bfold = True; tail = -10 if pose == 'lie' else -40
    if pose == 'sleep': head = about('y', -40, (4.6, 0, 8.6), (0.2, 0, -1.6)) * about('z', 12, (4.6, 0, 8.6)); eyes = False
    if pose == 'eat': head = about('y', -58, (3.9, 0, 7.9), (0.4, 0, -0.4)); tail = 50
    if pose == 'pee': body = about('x', 0, hip); liftleg = True; tail = 55
    hb = body * head
    # body
    P += [Prim('ell', (0, 0, 6.2), r=(4.7, 2.6, 2.7), pat=('below', 4.9) if plain else ('spots', 2.2), xf=body),
          Prim('sph', (2.6, 0, 6.7), r=2.85, pat=('below', 6.0) if plain else ('spots', 2.2), xf=body),
          Prim('sph', (-2.8, 0, 6.5), r=2.5, pat=None if plain else ('spots', 2.2), xf=body),
          Prim('cap', (3.5, 0, 7.6), (4.9, 0, 9.4), r=1.85, pat=('below', 7.6) if plain else None, xf=body),
          Prim('cap', (4.15, 0, 8.25), (4.45, 0, 8.65), r=2.02, lab=RED, xf=body)]
    # head
    P += [Prim('sph', (5.6, 0, 10.1), r=2.75, pat=None if plain else ('spots', 2.0), xf=hb),
          Prim('ell', (8.0, 0, 9.3), r=(1.85, 1.35, 1.15), pat=('all', 0) if plain else None, xf=hb),
          Prim('sph', (9.65, 0, 9.75), r=0.68, lab=EYE, xf=hb),
          ]
    if spitz: P += [Prim('cap', (5.1, 1.45, 11.9), (4.9, 1.85, 14.5), r=(1.15, 0.2), xf=hb), Prim('cap', (5.1, -1.45, 11.9), (4.9, -1.85, 14.5), r=(1.15, 0.2), xf=hb)]   # pointy ears
    else: P += [Prim('ell', (4.9, 2.45, 10.0), r=(1.05, 0.7, 2.0), xf=hb * about('x', -12, (4.9, 2.45, 11.5))),
                Prim('ell', (4.9, -2.45, 10.0), r=(1.05, 0.7, 2.0), xf=hb * about('x', 12, (4.9, -2.45, 11.5)))]   # floppy ears
    if eyes: P += [Prim('sph', (7.45, 1.3, 11.0), r=0.68, lab=EYE, xf=hb), Prim('sph', (7.45, -1.3, 11.0), r=0.68, lab=EYE, xf=hb)]
    else: P += [Prim('cap', (7.6, 1.0, 10.8), (7.4, 1.7, 10.8), r=0.3, lab=EYE, xf=hb), Prim('cap', (7.6, -1.0, 10.8), (7.4, -1.7, 10.8), r=0.3, lab=EYE, xf=hb)]
    if tongue: P += [Prim('ell', (8.6, 0, 8.0), r=(0.7, 0.55, 0.9), lab=RED, xf=hb)]
    # legs: shoulder / hip pivots on the body; a swing turns them about it
    for k, (x, y) in {'fl': (2.7, 1.45), 'fr': (2.7, -1.45), 'bl': (-2.9, 1.45), 'br': (-2.9, -1.45)}.items():
        front = k[0] == 'f'; piv = (x, y, 5.6)
        if front and fup:   # begging: the front paws up in front of the chest
            a, b = (x, y, 5.6), (x + 2.6, y*0.8, 4.0)
            lx = body * about('y', -10, piv)
        elif front and ffold:   # lying: the front legs stretched out along the floor
            a, b = (x, y, 4.6), (x + 3.4, y, 3.65)
            lx = body
        elif (not front) and bfold:   # sitting / lying: the hind legs folded under
            a, b = (x, y*1.25, 5.0), (x + 2.6, y*1.35, 3.65)
            lx = body
        else:
            a, b = (x, y, 5.6), (x - (0.2 if not front else 0), y, 1.0)
            ang = legs[k]
            if liftleg and k == 'br': lx = body * about('x', -62, (x, y, 5.6)); a, b = (x, y, 5.6), (x - 0.6, y, 2.0)   # (the side the camera sees)
            else: lx = body * about('y', ang, piv)
        P += [Prim('cap', a, b, r=(1.1, 0.9), xf=lx),
              Prim('sph', np.asarray(b) + (0.35, 0, 0.1 if not (front and fup) else 0.0), r=0.98, pat=('all', 0) if plain else None, xf=lx)]
    if spitz:   # a tail curled up over the back
        P += [Prim('cap', (-4.3, 0, 7.6), (-5.4, 0, 10.4), r=(1.0, 0.9), xf=body), Prim('cap', (-5.4, 0, 10.4), (-3.4, 0, 10.9), r=(0.9, 0.6), pat=('all', 0) if plain else None, xf=body)]
    else:
        tx = body * about('y', -tail, (-4.4, 0, 7.6))
        P += [Prim('cap', (-4.4, 0, 7.6), (-7.3, 0, 7.6), r=(0.85, 0.55), xf=tx),
              Prim('sph', (-7.3, 0, 7.6), r=0.62, pat=('all', 0) if plain else None, xf=tx)]
    return P

def cat(pose, var):
    P = []
    acc_belly = ('below', 4.6) if var == 'plain' else None
    acc_body = ('stripes', 1.25) if var == 'tabby' else acc_belly
    pts = ('all', 0) if var in ('plain', 'points') else None   # muzzle and paws (plain: white; points: the dark Siamese points)
    ptsx = ('all', 0) if var == 'points' else None              # ears and tail: only the points
    body = Xf(); head = Xf(); hip = (-2.6, 0, 5.0)
    legs = {'fl': 0, 'fr': 0, 'bl': 0, 'br': 0}; eyes = True; bfold = ffold = False; tailc = 'up'; squat = False
    if pose in ('walk1', 'walk2'):
        s = 22 if pose == 'walk1' else -22; legs = {'fl': s, 'br': s, 'fr': -s, 'bl': -s}
    if pose == 'sit': body = about('y', 38, hip, (0, 0, -2.4)); head = about('y', -38, (4.6, 0, 7.4)); bfold = True; tailc = 'wrap'
    if pose == 'happy': body = about('y', -20, (2.8, 0, 5.0), (0, 0, -0.6)); head = about('y', 20, (4.6, 0, 7.4), (0.6, 0, -0.4)); ffold = True; tailc = 'high'   # the play bow
    if pose in ('lie', 'sleep'): body = Xf(np.eye(3), (0, 0, -2.6)); ffold = bfold = True; tailc = 'wrap'
    if pose == 'sleep': head = about('y', -30, (4.6, 0, 7.4), (0, 0, -1.4)) * about('z', 25, (4.6, 0, 7.4)); eyes = False
    if pose == 'eat': head = about('y', -55, (3.6, 0, 6.6), (0.3, 0, -0.3))
    if pose == 'pee': body = about('y', 14, hip, (0, 0, -1.4)); squat = True; tailc = 'high'
    hb = body * head
    P += [Prim('ell', (0, 0, 5.2), r=(4.4, 2.15, 2.2), pat=acc_body, xf=body),
          Prim('sph', (2.3, 0, 5.4), r=2.2, pat=acc_belly if var != 'tabby' else None, xf=body),
          Prim('sph', (-2.5, 0, 5.3), r=2.1, pat=('stripes', 1.25) if var == 'tabby' else None, xf=body),
          Prim('cap', (3.3, 0, 6.0), (4.5, 0, 7.4), r=1.4, xf=body)]
    P += [Prim('sph', (5.3, 0, 8.3), r=2.45, pat=('stripes', 1.6) if var == 'tabby' else None, xf=hb),
          Prim('ell', (7.2, 0, 7.75), r=(1.0, 1.25, 0.9), pat=pts, xf=hb),
          Prim('sph', (8.05, 0, 8.1), r=0.48, lab=RED, xf=hb),
          Prim('cap', (5.0, 1.35, 9.9), (4.85, 1.75, 12.0), r=(0.95, 0.15), pat=ptsx, xf=hb),
          Prim('cap', (5.0, -1.35, 9.9), (4.85, -1.75, 12.0), r=(0.95, 0.15), pat=ptsx, xf=hb)]
    if eyes: P += [Prim('sph', (7.05, 1.05, 8.95), r=0.62, lab=EYE, xf=hb), Prim('sph', (7.05, -1.05, 8.95), r=0.62, lab=EYE, xf=hb)]
    else: P += [Prim('cap', (7.2, 0.7, 8.8), (7.0, 1.4, 8.8), r=0.28, lab=EYE, xf=hb), Prim('cap', (7.2, -0.7, 8.8), (7.0, -1.4, 8.8), r=0.28, lab=EYE, xf=hb)]
    for k, (x, y) in {'fl': (2.5, 1.15), 'fr': (2.5, -1.15), 'bl': (-2.6, 1.15), 'br': (-2.6, -1.15)}.items():
        front = k[0] == 'f'
        if front and ffold: a, b, lx = (x, y, 4.0), (x + 2.6, y, 3.25 if pose != 'happy' else 0.8), body
        elif (not front) and bfold: a, b, lx = (x, y*1.2, 4.0), (x + 2.2, y*1.3, 3.15), body
        elif (not front) and squat: a, b, lx = (x, y*1.2, 4.0), (x + 1.6, y*1.4, 1.6), body
        else: a, b = (x, y, 4.4), (x, y, 0.75); lx = body * about('y', legs[k], (x, y, 4.4))
        P += [Prim('cap', a, b, r=(0.85, 0.7), pat=ptsx, xf=lx), Prim('sph', np.asarray(b) + (0.3, 0, 0.05), r=0.75, pat=pts, xf=lx)]
    if tailc == 'up':
        P += [Prim('cap', (-4.1, 0, 6.0), (-6.4, 0, 7.4), r=(0.7, 0.6), pat=ptsx or (('stripes', 1.8) if var == 'tabby' else None), xf=body),
              Prim('cap', (-6.4, 0, 7.4), (-7.0, 0, 10.6), r=(0.6, 0.55), pat=ptsx or (('stripes', 1.8) if var == 'tabby' else None), xf=body)]
    elif tailc == 'high':
        P += [Prim('cap', (-4.1, 0, 6.0), (-5.3, 0, 9.6), r=(0.7, 0.6), pat=ptsx, xf=body), Prim('cap', (-5.3, 0, 9.6), (-4.6, 0, 11.6), r=(0.6, 0.5), pat=ptsx, xf=body)]
    else:   # wrapped round the paws
        P += [Prim('cap', (-4.0, 0, 5.0), (-3.4, 2.6, 3.4), r=(0.7, 0.6), pat=ptsx, xf=body), Prim('cap', (-3.4, 2.6, 3.4), (1.6, 2.9, 3.0), r=(0.6, 0.5), pat=ptsx, xf=body)]
    return P

# ---------------- render ----------------
VIEWS = {'front': np.array([[0, -1, 0], [1, 0, 0], [0, 0, 1]], float),    # local -> world: forward +Y (screen left-down)
         'back':  np.array([[-1, 0, 0], [0, -1, 0], [0, 0, 1]], float)}   # forward -X (screen left-up)

def render(prims, view, S):
    V = VIEWS[view]
    n = 32*SS; ii, jj = np.meshgrid(np.arange(n), np.arange(n), indexing='ij')   # ii row (y), jj column (x)
    sx = ((jj + 0.5)/SS - AX)/S; sy = ((ii + 0.5)/SS - AY)/S
    p0 = sx.reshape(-1, 1)*R + sy.reshape(-1, 1)*D - 60*F
    # world -> local: local = V^T world
    o = p0 @ V; f = F @ V
    t = np.zeros(len(o)); hit = np.zeros(len(o), bool); alive = np.ones(len(o), bool)
    for it in range(140):
        idx = np.nonzero(alive)[0]
        if len(idx) == 0: break
        p = o[idx] + np.outer(t[idx], f)
        d = smin_all(scene(prims, p))
        h = d < 0.02; hit[idx[h]] = True; alive[idx[h]] = False
        t[idx] += np.maximum(d, 0.01)
        far = t[idx] > 130; alive[idx[far]] = False
    lab = np.full(len(o), -1); lam = np.zeros(len(o))
    idx = np.nonzero(hit)[0]
    if len(idx):
        p = o[idx] + np.outer(t[idx], f)
        ds = scene(prims, p); k = np.argmin(ds, axis=0)
        e = 0.05; g = np.zeros((len(idx), 3))
        for a in range(3):
            dv = np.zeros(3); dv[a] = e
            g[:, a] = smin_all(scene(prims, p + dv)) - smin_all(scene(prims, p - dv))
        nrm = g/np.maximum(np.linalg.norm(g, axis=1, keepdims=True), 1e-6)
        lam[idx] = (nrm @ V.T) @ LIGHT   # the normal back in world space
        labs = np.array([q.lab for q in prims])[k]
        # the pattern: coat parts that are accent here
        for qi, q in enumerate(prims):
            sel = (k == qi) & (labs == COAT)
            if q.pat is not None and sel.any():
                m = pattern(q, to_rest(p[sel], q.xf)); s2 = np.nonzero(sel)[0]; labs[s2[m]] = ACC
        lab[idx] = labs
    lab = lab.reshape(n, n); lam = lam.reshape(n, n)
    img = np.zeros((32, 32), np.uint8)
    W = {COAT: 1.0, ACC: 1.15, EYE: 2.6, RED: 2.2}
    for y in range(32):
        for x in range(32):
            L = lab[y*SS:(y + 1)*SS, x*SS:(x + 1)*SS].ravel(); M = lam[y*SS:(y + 1)*SS, x*SS:(x + 1)*SS].ravel()
            on = L >= 0
            if on.sum() < 6: continue
            best, bw = COAT, -1
            for c in (COAT, ACC, EYE, RED):
                w = (L == c).sum()*W[c]
                if w > bw: best, bw = c, w
            if best == EYE: img[y, x] = 2
            elif best == RED: img[y, x] = 3
            elif best == ACC: img[y, x] = 7
            else:
                v = M[L == COAT].mean()
                img[y, x] = 6 if v > 0.62 else 5 if v > 0.18 else 4
    out = img.copy()   # the outline: every clear pixel next to the pet
    for y in range(32):
        for x in range(32):
            if img[y, x]: continue
            for dy, dx in ((-1, 0), (1, 0), (0, -1), (0, 1)):
                yy, xx = y + dy, x + dx
                if 0 <= yy < 32 and 0 <= xx < 32 and img[yy, xx]: out[y, x] = 1; break
    return out

POSES = ['stand', 'walk1', 'walk2', 'sit', 'lie', 'sleep', 'eat', 'happy', 'pee']
VARS = [('dog', 'plain', 'floppy'), ('dog', 'spots', 'floppy'), ('dog', 'plain', 'spitz'), ('dog', 'spots', 'spitz'), ('cat', 'plain', ''), ('cat', 'tabby', ''), ('cat', 'points', '')]

def tiles(img):   # 32 x 32 -> 16 tiles of 8 x 8 4bpp, row-major (1D OBJ mapping), as 128 u32
    words = []
    for ty in range(4):
        for tx in range(4):
            for r in range(8):
                v = 0
                for c in range(8): v |= int(img[ty*8 + r, tx*8 + c]) << (4*c)
                words.append(v)
    return words

def coats():   # parse petCoat from pets.h so the preview shows the game's colours
    src = open(os.path.join(os.path.dirname(__file__), '..', 'source', 'pets.h')).read() if os.path.exists(os.path.join(os.path.dirname(__file__), '..', 'source', 'pets.h')) else ''
    import re
    out = []
    for m in re.finditer(r'\{"([A-Z .\-]+)",\s*(\d),\s*\{((?:RGB\(\d+,\d+,\d+\),?\s*){4})\}\}', src):
        cols = [tuple(int(v) for v in c) for c in re.findall(r'RGB\((\d+),(\d+),(\d+)\)', m.group(3))]
        out.append((m.group(1), int(m.group(2)), cols))
    return out

def model(sp, var, build, pose): return dog(pose, var, build) if sp == 'dog' else cat(pose, var)

def main():
    frames = {}
    for vi, (sp, var, build) in enumerate(VARS):
        for zi, S in enumerate(SIZES):
            for view in ('front', 'back'):
                for pose in POSES:
                    frames[(vi, zi, view, pose)] = render(model(sp, var, build, pose), view, S)
        print(sp, var, build, 'done', file=sys.stderr)
    with open(os.path.join(os.path.dirname(__file__), '..', 'source', 'petart.h'), 'w') as fo:
        fo.write('// petart.h - made by tools/make_pets.py (do not edit): the pets\' 32 x 32 sprites, 4bpp tiles in 1D order.\n')
        fo.write('// petArt[look][size][view][pose]. Looks: DOG (floppy ears) PLAIN, SPOTS; DOG (spitz: pointy ears, curled tail) PLAIN, SPOTS; CAT PLAIN, TABBY, POINTS.\n')
        fo.write('// Sizes SMALL MEDIUM LARGE. Views FRONT (facing screen left-down), BACK (left-up); the game flips them for the other two.\n')
        fo.write('// Poses STAND WALK1 WALK2 SIT LIE SLEEP EAT HAPPY PEE. Colours: 1 outline, 2 eyes / nose, 3 red, 4..7 coat dark, mid, light, accent.\n')
        fo.write('#define PET_LOOKS %d\n#define PET_SZ %d\n#define PET_FR %d\n' % (len(VARS), len(SIZES), len(POSES)))
        fo.write('static const u32 petArt[PET_LOOKS][PET_SZ][2][PET_FR][128]={\n')
        for vi in range(len(VARS)):
            fo.write(' {\n')
            for zi in range(len(SIZES)):
                fo.write('  {\n')
                for view in ('front', 'back'):
                    fo.write('   {\n')
                    for pose in POSES:
                        fo.write('    {' + ','.join('0x%X' % v for v in tiles(frames[(vi, zi, view, pose)])) + '},\n')
                    fo.write('   },\n')
                fo.write('  },\n')
            fo.write(' },\n')
        fo.write('};\n')
    cl = coats()
    pal0 = {1: (2, 2, 5), 2: (3, 2, 4), 3: (28, 8, 9)}
    rows = []
    for vi, (sp, var, build) in enumerate(VARS):
        use = [c for c in cl if c[1] == vi][:2] or [('X', vi, [(10, 8, 6), (18, 14, 10), (26, 21, 15), (30, 28, 24)])]
        for nm, _, cols in use:
            pal = dict(pal0); pal.update({4 + i: cols[i] for i in range(4)})
            row = Image.new('RGB', (32*len(POSES)*2 + 32*3, 32), (14, 22, 14))
            for k, view in enumerate(('front', 'back')):
                for j, pose in enumerate(POSES):
                    im = frames[(vi, 1, view, pose)]
                    a = np.zeros((32, 32, 3), np.uint8); a[:] = (110, 150, 110) if k == 0 else (130, 120, 100)
                    for ci, c in pal.items(): a[im == ci] = [v*255//31 for v in c]
                    row.paste(Image.fromarray(a), ((k*len(POSES) + j)*32, 0))
            for zi in range(3):
                im = frames[(vi, zi, 'front', 'stand')]
                a = np.zeros((32, 32, 3), np.uint8); a[:] = (90, 90, 120)
                for ci, c in pal.items(): a[im == ci] = [v*255//31 for v in c]
                row.paste(Image.fromarray(a), ((2*len(POSES) + zi)*32, 0))
            rows.append(row)
    sheet = Image.new('RGB', (rows[0].width, 32*len(rows)))
    for i, r in enumerate(rows): sheet.paste(r, (0, 32*i))
    sheet = sheet.resize((sheet.width*3, sheet.height*3), Image.NEAREST)
    sheet.save(os.path.join(os.path.dirname(__file__), 'pets_preview.png'))

if __name__ == '__main__':
    main()
