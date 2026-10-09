// itemids.h - sprite size and sprite numbers, shared by the GBA game (items.h) and the PC baker (tools/bake_items.c).
#define IW 21
#define IH0 28   // the height the hand-drawn pixel art (phone, radio, DeadSet ...) was drawn at
#define IH 38    // the sprite canvas: 10 rows taller than it was, so ramps and boxes can be taller (see the SCALE note in docs/NOTES.md)
#define IPAD (IH-IH0)   // empty rows added at the top of the hand-drawn art
#define IOX 10   // sprite column of the tile centre
#define IOY 32   // sprite row of the tile centre (was 22)
#define IKEY 0x8000
// Sprite numbers. EVERY item sprite is pre-baked in ROM (itemrom.h, generated), so new items cost no RAM, only ROM.
// NEW ITEM: add its art to itembake.h (or an art header it includes), add its V_ number here BEFORE NIV, add one bakeOne(...) line
// in bakeAll() in itembake.h, run tools/bake_items.sh, then draw it with blitItem(V_xxx,..) in items.h. NIV is the sprite count.
enum { V_CRATE, V_FRIDGE, V_TOILET=V_FRIDGE+4, V_RAILU=V_TOILET+4, V_RAILV, V_DOOR, V_BOARD,
       V_KICKER, V_QPIPE=V_KICKER+4, V_LEDGEU=V_QPIPE+4, V_LEDGEV, V_BENCHU, V_BENCHV, V_BED, V_SHOWER=V_BED+4, V_SOFA=V_SHOWER+4,
       V_LAUNCH=V_SOFA+4, V_FUNBOX=V_LAUNCH+4, V_BARREL, V_TRASH, V_PLANTER, V_PICNIC, V_JERSEYU, V_JERSEYV, V_MPAD,
       V_PIPE, V_LAVA, V_BEANBAG, V_DEADSET=V_BEANBAG+4, V_PHONE=V_DEADSET+2, V_RADIO=V_PHONE+1, V_STEREO=V_RADIO+1,
       V_TV=V_STEREO+1, V_SHELF=V_TV+4, V_COFFEE=V_SHELF+4, V_AQUA=V_COFFEE+1, V_TREAD=V_AQUA+1, V_RUG=V_TREAD+4, V_TABLE=V_RUG+1, V_CHAIR=V_TABLE+1, V_DESK=V_CHAIR+4, V_LAMP=V_DESK+4, V_PLANT=V_LAMP+1, V_DRESSER=V_PLANT+1, V_FIRE=V_DRESSER+4, V_COUNTER=V_FIRE+4, V_KSEG=V_COUNTER+4, V_LSEG=V_KSEG+16, NIV=V_LSEG+12 };   // LONG RAMPS (ramps.h): V_KSEG = 4 kicker tiles (3 climbing + the deck) x 4 turns, V_LSEG = 3 launch tiles (2 climbing + the deck) x 4 turns   // living and decor pack (decorart.h): rug, table, chair x4, desk x4, lamp, plant, dresser x4, fireplace x4, counter x4   // home pack: TV, bookshelf (4 turns each), coffee maker, aquarium, treadmill (4 turns)   // chill pack: water pipe, lava lamp, beanbag (4 turns)   // sound pack: radio (R), sound system (A)
