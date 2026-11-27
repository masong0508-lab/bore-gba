#!/usr/bin/env python3
"""BORE 1 SNES M4: independent campaign ledgers and four shared arena maps.
Builds on M3's native title music and M2's home/town. No GBA data is modified.
The arena is a first melee-combat slice, not the finished expansion campaigns.
"""
from pathlib import Path
import hashlib,json,sys,struct
import build_milestone3 as m3
ROOT=Path(__file__).resolve().parent
LIMITS=(11,8,6,4,6,4,15,4)
CHAPTERS=(3,4,4,4,4,3,5,4)
GROUPS=(0,1,1,2,2,3,4,4)
CAMPAIGNS=('BASE GAME','BOREAGOTCHI','LAND BOREFORE TIME','BOREZIES','WORLD WAR BORE','BORTUA FIGHTER','BOREPILOGUE / LEGACY','BORE OUT MAN')
# DP state. Slots 0..39 retain the earlier build's state.
PROG,CHAP,TASK=40,48,56
HP,ENX,ENY,ENHP,DEFEATS,COOLDOWN,ATICK,OLD_SCENE=64,65,66,67,68,69,70,71

def source_with_m4(length):
    s=m3.source_with_native(length)
    def rep(old,new):
        nonlocal s
        assert s.count(old)==1,(old,s.count(old))
        s=s.replace(old,new)
    rep('PROG=40; CHAP=45; TASK=50',f'PROG={PROG}; CHAP={CHAP}; TASK={TASK}\nHP,ENX,ENY,ENHP,DEFEATS,COOLDOWN,ATICK,OLD_SCENE={HP},{ENX},{ENY},{ENHP},{DEFEATS},{COOLDOWN},{ATICK},{OLD_SCENE}\nMISSIONS={LIMITS!r}\nGROUPS={GROUPS!r}\nCAMPAIGNS={CAMPAIGNS!r}')
    rep('(PACK,0,4)','(PACK,0,7)')
    rep('for i in range(5)]+[(TASK+i,0,1) for i in range(5)]','for i in range(8)]+[(TASK+i,0,1) for i in range(8)]')
    rep('CHAPTERS=(3,4,4,3,5)',f'CHAPTERS={CHAPTERS!r}')
    rep("TASK_NAMES=('CARE AT HOME','FEED THE PET','WORK IN TOWN','VISIT THE PARK','WORK IN TOWN')","TASK_NAMES=('CARE AT HOME','FEED THE PET','VISIT THE PARK','DEFEAT THREE RIVALS','DEFEAT THREE RIVALS','DEFEAT THREE RIVALS','WORK IN TOWN','WORK IN TOWN')")
    rep("'SAVE INVALID / NEW GAME')","'SAVE INVALID / NEW GAME','ARENA / A STRIKE / B EXIT','HIT / KEEP MOVING','RIVAL DEFEATED','ARENA CLEARED','OUT OF ENERGY / REST','ARENA EXITED')")
    rep('for p in range(55):E(0x64,p)','for p in range(96):E(0x64,p)')
    # Save records are new and intentionally use a different address and magic.
    # M2/M3 SRAM remains present and is never overwritten by this version.
    rep('base=0x700010; select=0x700000','base=0x700110; select=0x700100')
    rep('imm(0xb2);a.long(0x9f,base)','imm(0xb4);a.long(0x9f,base)')
    rep('a.long(0xbf,base);cmp(0xb2)','a.long(0xbf,base);cmp(0xb4)')
    # Eight campaigns, with SELECT cycling four pack groups plus base and X
    # selecting the other campaign in a combined pack. Keep all state separate.
    rep("ld(EH);E(0x29,0x20);R(0xf0,'no_pack');E(0xe6,PACK);ld(PACK);cmp(5);R(0x90,'pack_ok');E(0x64,PACK);L('pack_ok');dirty();L('no_pack')", "ld(SCENE);cmp(2);R(0xb0,'no_pack');ld(EH);E(0x29,0x20);R(0xf0,'no_next_group');call('campaign_x');W(0xbd,'next_group');st(PACK);dirty();L('no_next_group');ld(EL);E(0x29,0x40);R(0xf0,'no_pack');call('campaign_x');W(0xbd,'other_campaign');st(PACK);dirty();L('no_pack')")
    # Retain the family cap but only change household outside the arena.
    rep("ld(EH);E(0x29,0x40);R(0xf0,'no_add')", "ld(SCENE);cmp(2);R(0xb0,'no_add');ld(EH);E(0x29,0x40);R(0xf0,'no_add')")
    rep("ld(EH);E(0x29,0x80);R(0xf0,'no_remove')", "ld(SCENE);cmp(2);R(0x90,'family_remove');ld(EH);E(0x29,0x80);R(0xf0,'no_remove');call('exit_arena');R(0x80,'no_remove');L('family_remove');ld(EH);E(0x29,0x80);R(0xf0,'no_remove')")
    # Move and render arena rival independently; only change OAM while blank.
    rep("call('move');call('actions');call('advance')","call('move');call('actions');call('advance');call('arena_tick')")
    rep("reg(0x2104,0x30);E(0x60)","reg(0x2104,0x30);ld(SCENE);cmp(2);R(0x90,'enemy_hidden');ld(ENX);E(0x0a,0x0a,0x0a);sta(0x2104);ld(ENY);E(0x0a,0x0a,0x0a);sta(0x2104);reg(0x2104,80);reg(0x2104,0x30);E(0x60);L('enemy_hidden');reg(0x2104,0);reg(0x2104,240);reg(0x2104,0);reg(0x2104,0);E(0x60)")
    rep("ld(SCENE);R(0xd0,'town_map');E(0xc2,0x20);W(0xa9,'home');R(0x80,'map_addr');L('town_map');E(0xc2,0x20);W(0xa9,'town');L('map_addr')", "ld(SCENE);E(0xc2,0x20);W(0x29,255);E(0x0a,0xaa);W(0xbd,'maps');L('map_addr')")
    rep("L('use');ld(PX)","L('use');ld(SCENE);cmp(2);longif(0xb0,'strike','not_strike');ld(PX)")
    rep("setdp(TASK+task,1);E(0x60)","setdp(TASK+task,1);E(0x60)")
    rep("nz(TASK+2);nz(TASK+4)","nz(TASK+6);nz(TASK+7)")
    rep("nz(TASK+3);status(11)","nz(TASK+2);status(11)")
    # A visit to the mission desk starts a combat trial only if the selected
    # campaign needs it. Returning after victory allows chapter completion.
    rep("L('mission');ld(PACK);E(0xc2,0x20);W(0x29,255);E(0xaa,0xe2,0x20);E(0xb5,PROG);W(0xdd,'mission_limits');R(0xb0,'campaign_done');E(0xb5,TASK);R(0xf0,'need_task');", "L('mission');call('campaign_x');E(0xb5,PROG);W(0xdd,'mission_limits');R(0xb0,'campaign_done');E(0xb5,TASK);R(0xd0,'mission_progress');ld(PACK);cmp(3);R(0x90,'need_task');cmp(6);R(0xb0,'need_task');jmp('start_arena');L('mission_progress');")
    rep("E(0x74,CHAP,0xf6,PROG);status(14);E(0x60);L('chapter_done');status(13)","E(0x74,CHAP,0xf6,PROG);call('mission_reward');status(14);E(0x60);L('chapter_done');status(13)")
    # HUD shows expansion group, campaign title, task and explicit goal counts.
    rep("put(1,23,'SELECT PACK / Y ADD B REMOVE')","put(1,23,'SELECT PACK / X CAMPAIGN')")
    rep("put(1,24,'L R MUSIC / PACK:')","put(1,24,'L R MUSIC / PACK:')")
    rep("put(1,26,'TASK:')","put(1,26,'')")
    rep("addr(24*32+18);ld(PACK);E(0x18,0x69,tile('0'))", "addr(24*32+18);call('campaign_x');W(0xbd,'group_ids');E(0x18,0x69,tile('0'))")
    rep("[(PACK,26*32+6,'tasks','taskline',24),(STATUS,27*32+1,'messages','statusline',30)]", "[(PACK,32+1,'tasks','taskline',23),(PACK,26*32+1,'campaigns','campaignline',30),(STATUS,27*32+1,'messages','statusline',30)]")
    rep("addr(25*32+26);ld(PACK);E(0xc2,0x20);W(0x29,255);E(0xaa,0xe2,0x20,0xb5,CHAP);call('number')", "addr(25*32+11);imm(tile('/'));sta(0x2118);W(0x9c,0x2119);call('campaign_x');W(0xbd,'mission_limits');call('number');addr(25*32+26);call('campaign_x');E(0xb5,CHAP);call('number');imm(tile('/'));sta(0x2118);W(0x9c,0x2119);call('campaign_x');W(0xbd,'chapter_limits');call('number')")
    # Use a dedicated line in the arena and display actual health and wins.
    rep("    E(0x60)\n    L('number');", "    ld(SCENE);cmp(2);R(0x90,'hud_done');addr(5*32+27);ld(HP);E(0x18,0x69,tile('0'));sta(0x2118);W(0x9c,0x2119);addr(24*32+27);ld(DEFEATS);E(0x18,0x69,tile('0'));sta(0x2118);W(0x9c,0x2119);L('hud_done');E(0x60)\n    L('number');")
    # Prevent a save during combat from recording arbitrary in-arena state.
    rep("ld(EH);E(0x29,0x10);R(0xf0,'no_save');call('save_game')", "ld(SCENE);cmp(2);R(0xb0,'no_save');ld(EH);E(0x29,0x10);R(0xf0,'no_save');call('save_game')")
    # Actual melee mechanics: three rival spawns, bounded hit range, one-hit
    # cooldown, enemy pursuit, contact damage, retreat and mission award.
    code=r'''    L('campaign_x');ld(PACK);E(0xc2,0x20);W(0x29,255);E(0xaa,0xe2,0x20,0x60)
    L('mission_reward');ld(CASH);E(0x18,0x69,4);cmp(100);R(0x90,'reward_store');imm(99);L('reward_store');st(CASH);E(0x60)
    L('start_arena');ld(REST);cmp(2);R(0xb0,'arena_start_ok');status(22);E(0x60);L('arena_start_ok');call('campaign_x');E(0xb5,PROG,0x18,0x75,CHAP,0x29,3,0x18,0x69,2);st(SCENE);setdp(HP,6);setdp(DEFEATS,0);setdp(ATICK,0);setdp(COOLDOWN,0);setdp(PX,15);setdp(PY,16);call('spawn_rival');status(18);call('draw_scene');E(0x60)
    L('spawn_rival');ld(DEFEATS);E(0x29,1);R(0xf0,'rival_left');setdp(ENX,23);R(0x80,'rival_y');L('rival_left');setdp(ENX,7);L('rival_y');setdp(ENY,10);setdp(ENHP,2);nz(MOVED);E(0x60)
    L('exit_arena');setdp(SCENE,1);setdp(PX,23);setdp(PY,19);status(23);call('draw_scene');E(0x60)
    # Manhattan distance, only 8-bit values in 4..27 / 9..19.
    L('distance');ld(PX);E(0x38,0xe5,ENX);R(0xb0,'dx_pos');E(0x49,255,0x1a);L('dx_pos');st(TMP);ld(PY);E(0x38,0xe5,ENY);R(0xb0,'dy_pos');E(0x49,255,0x1a);L('dy_pos');E(0x18,0x65,TMP,0x60)
    L('strike');ld(COOLDOWN);R(0xd0,'strike_done');call('distance');cmp(3);R(0xb0,'strike_done');setdp(COOLDOWN,12);E(0xc6,ENHP);R(0xd0,'strike_hit');E(0xe6,DEFEATS);ld(DEFEATS);cmp(3);R(0xb0,'arena_win');call('spawn_rival');status(20);E(0x60);L('strike_hit');status(19);L('strike_done');E(0x60)
    L('arena_win');call('campaign_x');imm(1);E(0x95,TASK);ld(REST);R(0xf0,'win_no_cost');E(0xc6,REST);L('win_no_cost');call('exit_arena');status(21);E(0x60)
    L('arena_tick');ld(SCENE);cmp(2);R(0xb0,'arena_active');E(0x60);L('arena_active');ld(COOLDOWN);R(0xf0,'no_cool');E(0xc6,COOLDOWN);L('no_cool');E(0xe6,ATICK);ld(ATICK);cmp(15);R(0x90,'arena_tick_done');E(0x64,ATICK);call('distance');cmp(2);R(0x90,'rival_attack');ld(ENX);E(0xc5,PX);R(0xf0,'rival_vertical');R(0x90,'rival_right');E(0xc6,ENX);R(0x80,'rival_moved');L('rival_right');E(0xe6,ENX);R(0x80,'rival_moved');L('rival_vertical');ld(ENY);E(0xc5,PY);R(0x90,'rival_down');E(0xc6,ENY);R(0x80,'rival_moved');L('rival_down');E(0xe6,ENY);L('rival_moved');nz(MOVED);E(0x60);L('rival_attack');E(0xc6,HP);dirty();R(0xd0,'arena_tick_done');setdp(REST,0);call('exit_arena');status(22);L('arena_tick_done');E(0x60)
'''
    rep("    L('irq');E(0x40)",code+"    L('irq');E(0x40)")
    # Build four distinct arena maps using shared tiles, no separate engine.
    rep('return bytes(data)\n\ndef screen(scene):',"data.extend(planar(picture('00000000 000E E000 00EEEE00 00E8E800 00EEEE00 000EE000 00E00E00 00000000'.replace('000E E000','000EE000'))))\n    return bytes(data)\n\ndef screen(scene):")
    rep("    put(1,0,'BORE 1 / '+('YOUR HOME' if scene==0 else 'TOWN CENTRE'));put(25,0,'AUDIO')", "    put(1,0,'BORE 1 / '+(['YOUR HOME','TOWN CENTRE','CUL DE SAC','SCRAPYARD','BATTLEFIELD','ARCADE RING'][scene]));put(25,0,'AUDIO')")
    rep("    for x,y,n in ([(7,10,67),(24,10,68),(7,19,69),(23,19,70),(15,19,76)] if not scene else [(7,10,77),(24,10,67),(7,19,78),(23,19,79),(15,19,76)]):a[y*32+x]=n", "    if scene<2:\n        for x,y,n in ([(7,10,67),(24,10,68),(7,19,69),(23,19,70),(15,19,76)] if not scene else [(7,10,77),(24,10,67),(7,19,78),(23,19,79),(15,19,76)]):a[y*32+x]=n\n    else:\n        for y in range(9,20):\n            for x in range(4,28):\n                # Decorative ground stripes/checks only: all fight cells remain passable.\n                a[y*32+x]=64 if ((x//(scene-1)+y//2+scene)%4<2) else 73")
    rep("    for x,y,s in ([(5,8,'FRIDGE'),(22,8,'BED'),(5,17,'BATH'),(22,17,'PET'),(13,20,'TOWN')] if not scene else [(5,8,'WORK'),(22,8,'SHOP'),(5,17,'PARK'),(21,17,'MISSION'),(13,20,'HOME')]):put(x,y,s)", "    if scene<2:\n        for x,y,s in ([(5,8,'FRIDGE'),(22,8,'BED'),(5,17,'BATH'),(22,17,'PET'),(13,20,'TOWN')] if not scene else [(5,8,'WORK'),(22,8,'SHOP'),(5,17,'PARK'),(21,17,'MISSION'),(13,20,'HOME')]):put(x,y,s)\n    else:\n        put(22,5,'HP:');put(22,24,'WINS:')")
    # Keep HUD slots non-overlapping: arena's HP reuses JENES area, wins
    # reuses footer end. Normal HUD still prints cash at 23, so use row 6.
    rep("put(22,5,'HP:');put(22,24,'WINS:')","put(22,6,'HP:');put(22,24,'WINS:')")
    rep("addr(5*32+27);ld(HP)","addr(6*32+27);ld(HP)")
    rep("put(1,22,'MOVE / A USE / START SAVE')","put(1,22,'MOVE / A STRIKE / B EXIT' if scene>=2 else 'MOVE / A USE / START SAVE')")
    rep("put(1,23,'SELECT PACK / X CAMPAIGN')","put(1,23,'DEFEAT 3 RIVALS' if scene>=2 else 'SELECT PACK / X CAMPAIGN')")
    rep("L('town');a.b.extend(screen(1))","L('town');a.b.extend(screen(1))\n    for i in range(4):L('arena'+str(i));a.b.extend(screen(i+2))\n    L('maps')\n    for name in ('home','town','arena0','arena1','arena2','arena3'):\n        a.fix.append((len(a.b),name,False));a.b.extend(b'\\0\\0')")
    rep("L('tasks');a.b.extend(bytes(tile(c) for s in TASK_NAMES for c in s.ljust(32)))", "L('tasks');a.b.extend(bytes(tile(c) for s in TASK_NAMES for c in s.ljust(32)));L('campaigns');a.b.extend(bytes(tile(c) for s in CAMPAIGNS for c in s.ljust(32)))\n    L('group_ids');a.b.extend(bytes(GROUPS));L('next_group');a.b.extend(bytes([1,3,3,5,5,6,0,0]));L('other_campaign');a.b.extend(bytes([0,2,1,4,3,5,7,6]))")
    rep("b'BORE 1 NATIVE M3'","b'BORE 1 CAMPAIGNS M4'")
    return s

def build(audio=None):
    audio=(ROOT/'native_title.bin').read_bytes() if audio is None else audio
    ns={'__name__':'bore_m4_generated','__file__':str(ROOT/'build_milestone2.py'),'AUDIO':audio}
    exec(compile(source_with_m4(len(audio)),'build_milestone4.py','exec'),ns)
    rom,labels=ns['build']();assert rom[0x8000:0x8000+len(audio)]==audio
    return rom,labels

if __name__=='__main__':
    rom,labels=build();out=Path(sys.argv[1] if len(sys.argv)>1 else ROOT/'bore1-m4.sfc');out.write_bytes(rom)
    print(json.dumps({'bytes':len(rom),'sha256':hashlib.sha256(rom).hexdigest(),'campaigns':dict(zip(CAMPAIGNS,LIMITS)),'arena_maps':4,'reset':hex(labels['reset']),'scope':'native SNES home/town + combat and campaign prototype; original title SPC music'},indent=2))
