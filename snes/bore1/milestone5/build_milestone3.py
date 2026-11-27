#!/usr/bin/env python3
"""Milestone 3: native title audio in the cartridge, preserving M2 gameplay.
Builds on the existing 65816 emitter with a bounded IPL upload. The driver
runs independently of the game loop; optional MSU-1 mutes native only once
media is accepted, and missing media leaves the native soundtrack playing.
"""
from pathlib import Path
import struct, hashlib, json, sys, types
import build_milestone2 as m2
ROOT=Path(__file__).resolve().parent
NATIVEOK=33; ACK=34

def source_with_native(length):
    s=(ROOT/'build_milestone2.py').read_text()
    def replace(old,new):
        nonlocal s
        assert s.count(old)==1,old
        s=s.replace(old,new)
    replace("call('load_game')","call('load_game');call('native_upload')")
    # Extra indicator. Do not change any existing save fields or the gameplay
    # code. Show whether the independent audio driver started successfully.
    replace("put(1,0,'BORE 1 / '+('YOUR HOME' if scene==0 else 'TOWN CENTRE'))","put(1,0,'BORE 1 / '+('YOUR HOME' if scene==0 else 'TOWN CENTRE'));put(25,0,'AUDIO')")
    replace("    # selected pack's mission/chapter and name.","    addr(1*32+25);ld(33);R(0xf0,'audio_no');imm(tile('O'));R(0x80,'audio_ind');L('audio_no');imm(tile('-'));L('audio_ind');sta(0x2118);W(0x9c,0x2119);reg(0x2118,tile('N'));W(0x9c,0x2119)\n    # selected pack's mission/chapter and name.")
    # In M2, PENDING is cleared on both successful and failed media selection.
    # Only mute native after the MSU status indicates the track is ready.
    replace("reg(0x2007,3);L('media_error');E(0x64,PENDING)","reg(0x2007,3);ld(33);R(0xf0,'media_finish');reg(0x2141,1);R(0x80,'media_finish');L('media_error');ld(33);R(0xf0,'media_finish');reg(0x2141,0);L('media_finish');E(0x64,PENDING)")
    native=f'''    # SPC700 IPL handshake. All waits have a finite 16-bit poll budget.
    # Payload is in LoROM banks 01 and 02; transfer across the bank boundary
    # using two long-indexed bases, not a CPU pointer into the unmapped half.
    L('native_upload');W(0xa0,0xffff);L('apu_ipl');W(0xad,0x2140);cmp(0xaa);R(0xd0,'apu_poll');W(0xad,0x2141);cmp(0xbb);R(0xf0,'apu_begin');L('apu_poll');E(0x88);R(0xd0,'apu_ipl');E(0x60)
    L('apu_begin');reg(0x2142,0);reg(0x2143,2);reg(0x2141,1);imm(0xcc);st(34);sta(0x2140);call('apu_ack');R(0xb0,'apu_transfer');E(0x60)
    L('apu_transfer');W(0xa2,0);L('apu_byte');W(0xe0,0x8000);R(0xb0,'apu_bank2');a.long(0xbf,0x018000);R(0x80,'apu_send');L('apu_bank2');a.long(0xbf,0x020000)
    L('apu_send');sta(0x2141);E(0x8a);st(34);sta(0x2140);call('apu_ack');R(0xb0,'apu_next');E(0x60);L('apu_next');E(0xe8);W(0xe0,{length});R(0xd0,'apu_byte')
    # End command: positive offset from the expected next byte index.
    reg(0x2142,0);reg(0x2143,2);reg(0x2141,0);E(0x8a,0x18,0x69,2);sta(0x2140)
    W(0xa0,0xffff);L('apu_ready');W(0xad,0x2140);cmp(0x42);R(0xf0,'apu_ok');E(0x88);R(0xd0,'apu_ready');E(0x60);L('apu_ok');nz(33);E(0x60)
    L('apu_ack');W(0xa0,0xffff);L('apu_ackloop');W(0xad,0x2140);E(0xc5,34);R(0xf0,'apu_ack_ok');E(0x88);R(0xd0,'apu_ackloop');E(0x18,0x60);L('apu_ack_ok');E(0x38,0x60)
'''
    replace("    L('irq');E(0x40)",native+"    L('irq');E(0x40)")
    replace("b'BORE 1 TOWN M2'","b'BORE 1 NATIVE M3'")
    # Reserve the payload in the real cartridge and calculate checksum after it
    # is inserted. M2's 128 KiB ROM already has room for both payload banks.
    replace("rom=bytearray(0x20000);rom[:len(program)]=program","rom=bytearray(0x20000);rom[:len(program)]=program;rom[0x8000:0x8000+len(AUDIO)]=AUDIO")
    return s

def build(audio=None):
    audio=(ROOT/'native_title.bin').read_bytes() if audio is None else audio
    assert 0<len(audio)<=0xe000-0x200 and len(audio)<0x18000
    ns={'__name__':'bore_m3_generated','__file__':str(ROOT/'build_milestone2.py'),'AUDIO':audio}
    exec(compile(source_with_native(len(audio)),'build_milestone3.py','exec'),ns)
    rom,labels=ns['build']();assert rom[0x8000:0x8000+len(audio)]==audio
    return rom,labels

if __name__=='__main__':
    rom,labels=build();out=Path(sys.argv[1] if len(sys.argv)>1 else ROOT/'bore1-m3.sfc');out.write_bytes(rom)
    print(json.dumps({'bytes':len(rom),'sha256':hashlib.sha256(rom).hexdigest(),'payload_bytes':len((ROOT/'native_title.bin').read_bytes()),'reset':hex(labels['reset']),'scope':'home/town life simulation prototype with resident native SPC700 title music; full game and hardware test pending'},indent=2))
