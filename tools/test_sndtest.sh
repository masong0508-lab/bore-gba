#!/bin/sh
# test_sndtest.sh - the debug menu's SOUND TEST (source/dbgmenu.h) compiled on the PC against stubs: every effect and voice has a name, the names follow the lists, no more, no fewer.
# From the repo root:   sh tools/test_sndtest.sh
set -e
T=${TMPDIR:-/tmp}
python3 - "$T/sndtest.c" <<'P'
import sys,re
d=open('source/dbgmenu.h',encoding='utf8').read(); a=d.index('#include "voicenames.h"'); b=d.index('static void dbgMenu(void){')
snd=d[a:b].replace('#include "voicenames.h"','#include "source/voicenames.h"')
enum=[l for l in open('source/main.c',encoding='utf8') if l.startswith('enum { SFX_BONK')][0]
enum=enum[:enum.index('};')+2]
stub='''#include <stdint.h>
#include <stdio.h>
#include <string.h>
typedef uint8_t u8; typedef uint16_t u16; typedef uint32_t u32;
#include "source/voices.h"
'''+enum+'''
typedef struct { int x; } XmSong;
typedef struct { const char*name; const u8*adp; const XmSong*xm; } Song;
static const Song songs[]={ {"A",0,0},{"B",0,0} };
#define NSONGS ((int)(sizeof(songs)/sizeof(songs[0])))
static int sSnd=1, menuOn, creOn, hhPUid, menuInit, menuQuiet;
static void toast(const char*m){(void)m;} static void sfxPlay(int i){(void)i;} static void sfxStop(void){} static void musStop(void){}
static void musBegin(int k,const u8*a,const XmSong*x){(void)k;(void)a;(void)x;}
static void voxSayP(int u,int v,int p){(void)u;(void)v;(void)p;}
static int menu(const char*t,const char*const*it,int n){(void)t;(void)it;(void)n;return -1;}
'''
main='''
#define SDN(n) #n,
int main(void){ static const char* const slug[]={ VOICE_LIST(SDN) }; int bad=0;
    for(int i=0;i<VOICE_N;i++){ char w[40]; int k=0; for(const char*s=slug[i];*s;s++) w[k++]=(char)(*s=='_'?' ':(*s>='a'&&*s<='z'?*s-32:*s)); w[k]=0; if(strcmp(w,sdVoiceN[i])){ printf("FAIL voice %d: %s vs %s\\n",i,w,sdVoiceN[i]); bad++; } }
    sdMenu();   /* (the stub menu answers "back" at once: the code runs, nothing plays) */
    printf("== sound test: %d effects + toot named, %d voice clips named, every name matches its clip\\n",SFX_VOICE0,VOICE_N);
    printf("%s: %d failure(s)\\n",bad?"FAILED":"ALL PASSED",bad); return bad?1:0; }
'''
open(sys.argv[1],'w').write(stub+snd+main)
P
cc -Wall -Wno-unused-variable -Wno-unused-function -I. -o "$T/sndtest" "$T/sndtest.c"
"$T/sndtest"
