// draw_model.c - EP6 Phase 0, drawing cost MODEL (PC). Not a measurement: it adds up GBA memory cycles from the known bus speeds for a giant that runs
// across a 128 x 128 realm, for Plan A (the game's own software drawing in mode 3, fb in EWRAM) and Plan B (mode 0, hardware tile backgrounds
// and sprites). Every constant is printed so it can be argued with, and replaced by a number from a cart. Run: sh tools/phase0/run_phase0.sh
#include <stdio.h>
#include <math.h>
#define FRAME 280896.0            // 228 lines x 1232 cycles
#define VBLANK 83776.0            // 68 lines x 1232
// bus costs in CPU cycles (GBATEK): EWRAM 16-bit 3 (32-bit 6), IWRAM 1, VRAM/OAM/palette 16-bit bus: 16-bit 1, 32-bit 2, ROM (game pak, 3/1 waits) 16-bit sequential 2 / non-sequential 4
#define EW32 6.0
#define VR32 2.0
#define IW32 1.0
#define SW 240
#define SH 160
static double frames(double c){ return ceil(c/FRAME); }   // the game waits for vblank: a frame that runs over costs a whole extra frame
static void row(const char*n,double c){ printf("  %-58s %9.0f cycles  %6.1f%% of a frame  -> %2.0f fps\n",n,c,c*100/FRAME,60.0/frames(c)); }
int main(void){
    double words=SW*SH/2.0;                                   // 19,200 words of 2 pixels
    double present=words*(EW32+VR32);                         // DMA fb (EWRAM) -> VRAM
    double shiftEW=words*(EW32+EW32);                         // DMA or LDM/STM fb -> fb, moving the picture by a few pixels
    double shiftVR=words*(VR32+VR32);                         // VRAM -> VRAM (tears: mode 3 has one buffer; only 83,776 cycles of vblank)
    printf("GBA frame %.0f cycles, vblank %.0f.  Whole-screen costs from the bus speeds:\n",FRAME,VBLANK);
    printf("  present fb -> VRAM (DMA, 19,200 words x (EWRAM read %.0f + VRAM write %.0f)):  %.0f cycles = %.1f%% of a frame\n",EW32,VR32,present,present*100/FRAME);
    printf("  shift fb inside EWRAM (read %.0f + write %.0f a word):                       %.0f cycles = %.1f%% of a frame\n",EW32,EW32,shiftEW,shiftEW*100/FRAME);
    printf("  shift inside VRAM (read %.0f + write %.0f a word):                           %.0f cycles = %.1f%% of a frame, and more than the vblank gap leaves for the strip\n\n",VR32,VR32,shiftVR,shiftVR*100/FRAME);
    double pxLo=8,pxHi=14;     // cycles to make one drawn pixel in software: ROM tile read (2) + transparency test + EWRAM 16-bit write (3) + loop (3..9)
    int spd[3]={1,2,3};        // the giant moves 1 px a frame walking, 2 running, 3 sprinting [ASSUMED]; the camera follows, so the whole view scrolls
    int nWork=40, wPix=8*12, giantPix=24*36;
    for(int cs=0;cs<2;cs++){ double px=cs?pxHi:pxLo; printf("=== software pixel cost %.0f cycles ===\n",px);
        printf("Plan A0, redraw the whole view every frame:\n");
        row("A0 whole view: 38,400 px drawn + present",SW*SH*px+present);
        printf("Plan A1, shift the picture, draw only the new strip, present everything (mode 3 shows the whole fb):\n");
        for(int i=0;i<3;i++){ double strip=(double)spd[i]*(SW+SH)*px; double ents=(nWork*wPix+giantPix)*px;   // entities redrawn every frame (sorted over the ground): 40 Nubbins + the giant
            char n[96]; snprintf(n,sizeof n,"A1 at %d px/frame: shift + strip %.0f px + %d workers and the giant + present",spd[i],strip/px,nWork); row(n,shiftEW+strip+ents+present); }
        printf("Plan A2, tile steps: the picture moves 8 px at once every 8 frames; the other 7 frames only redraw the entities and present:\n");
        { double strip=8.0*(SW+SH)*px, ents=(nWork*wPix+giantPix)*px; double big=shiftEW+strip+ents+present, small=ents+present; char n[96];
          snprintf(n,sizeof n,"A2 the frame that scrolls (a hitch)"); row(n,big); snprintf(n,sizeof n,"A2 the 7 frames in between"); row(n,small); }
        printf("\n"); }
    // Plan B
    printf("=== Plan B: mode 0, hardware backgrounds + sprites ===\n");
    double regs=40;                                              // scroll registers, DISPCNT
    double colTiles=22, rowTiles=32;                             // a new tile-map column (20 rows + slack) or row (30 columns + slack) when the view crosses an 8 px line
    double perTile=25;                                           // cycles to turn one cell into a tile-map entry and write it to VRAM (a lookup, an or, a 16-bit store)
    double stream=(colTiles+rowTiles)*perTile;                   // worst case: both a column and a row cross in one frame (running diagonally)
    int sprites=nWork+1+8;                                       // workers, the giant (one 32x64 sprite), a few building overlays
    double oam=sprites*35;                                       // compute the screen position, priority and attributes, three 16-bit OAM writes
    double frameB=regs+stream+oam;
    printf("  scroll registers %.0f + map streaming (worst frame, %.0f + %.0f entries x %.0f) %.0f + %d sprites x 35 = %.0f  =>  %.0f cycles = %.2f%% of a frame -> %.0f fps\n",regs,colTiles,rowTiles,perTile,stream,sprites,oam,frameB,frameB*100/FRAME,60.0/frames(frameB));
    double reload=32768.0/4*8.0;
    printf("  coming back from a mode 3 menu: tiles and map go back into VRAM (32 KB of DMA from ROM, about %.0f cycles = %.0f%% of one frame, once)\n",reload,reload*100/FRAME);
    printf("  sprite limits: 128 sprites on screen; 40 Nubbins at 8 px wide use about %d of the %d cycles a scanline gives sprites, even if all stand on one line\n",40*8,1210);
    printf("\nPlan B leaves about %.0f%% of every frame for the simulation, hauling (see haul_proto: up to 7-17%% in its worst frame) and the life game's own work.\n",100-frameB*100/FRAME);
    return 0; }
