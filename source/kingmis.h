// kingmis.h - EP6 THE LAND BOREFORE TIME, Phase 5 step c: THE TEN MISSIONS. Included by expand.h in place of the old three placeholder missions (xmKing), after XM and before the macros are undefined.
// Played in your own life, done in the realm: each mission's tasks are checked against the open realm (kingq.h, kingtask.h; expand.h exKingTick) while you play KINGDOM MODE, and the mission closes, pays and
// opens the next one when you are back in the life game. Words: up to 119 characters a line, 30 a task name. [ASSUMED numbers; the pay is in hundreds of Simoleons]
// THE FAR BANK (8) has no bridge to build yet (bridges are a later step), so it asks the realm to spread out instead.
#define TQ(k,a,n,nm) {k,a,n,nm}
static const XMs xmKing[]={
  XM("BIG FEET, SMALL FIELD","You wake up enormous in a muddy field, with a very small crowd looking up at you and clearing their throats.","Open KINGDOM MODE and found a settlement: get three buildings up, lay some paths and bring the folk to eight.",XS_NONE,20,
     TQ(QK_KBLD,0,3,"RAISE 3 BUILDINGS"),TQ(QK_KROAD,0,12,"LAY 12 ROAD CELLS"),TQ(QK_KPOP,0,8,"HAVE 8 NUBBINS")),
  XM("TIMBER!","The little folk need wood, and you are, to be fair, a walking forest management plan.","Put up a woodcutter, raise a sawmill beside it and make ten planks: houses need planks, and so does most of the rest.",XS_NONE,25,
     TQ(QK_KBT,KB_WOODCUT,1,"BUILD A WOODCUTTER"),TQ(QK_KBT,KB_SAWMILL,1,"BUILD A SAWMILL"),TQ(QK_KGOOD,G_PLANKS,10,"HAVE 10 PLANKS")),
  XM("A ROCK AND A HARD PLACE","Wood only gets you so far. The little folk are building a very small castle in their heads and it needs rocks.","Open a quarry, haul twenty stone, and raise a mason to cut some of it.",XS_NONE,30,
     TQ(QK_KBT,KB_QUARRY,1,"BUILD A QUARRY"),TQ(QK_KGOOD,G_STONE,20,"HAVE 20 STONE"),TQ(QK_KBT,KB_MASON,1,"BUILD A MASON")),
  XM("FLUFF","The folk have discovered wool and now want everything softer, including you. Sheep are not optional.","Pen some sheep on a meadow, gather ten wool and raise a weaver to turn it into cloth.",XS_NONE,35,
     TQ(QK_KBT,KB_SHEEP,1,"BUILD A SHEEP PEN"),TQ(QK_KGOOD,G_WOOL,10,"HAVE 10 WOOL"),TQ(QK_KBT,KB_WEAVER,1,"BUILD A WEAVER")),
  XM("SHINY THINGS","Something glitters in the rocks and the little folk would like it, please, polished.","Mine crystal, set up a cutter and polish four crystals: a whole chain, from rock to ready.",XS_NONE,40,
     TQ(QK_KBT,KB_CRYMINE,1,"BUILD A CRYSTAL MINE"),TQ(QK_KBT,KB_CUTTER,1,"BUILD A CUTTER"),TQ(QK_KGOOD,G_POLISH,4,"HAVE 4 POLISHED CRYSTAL")),
  XM("RULES OF THE ROAD","Your Nubbins have started carrying things the long way round, and one of them has been seen walking in circles.","Lay proper roads so goods move without you, and let the Nubbins deliver twenty loads on their own.",XS_NONE,45,
     TQ(QK_KROAD,0,24,"LAY 24 ROAD CELLS"),TQ(QK_KDELIV,0,20,"DELIVER 20 LOADS"),TQ(QK_KBLD,0,8,"RAISE 8 BUILDINGS")),
  XM("TOO MANY MOUTHS","The realm is booming, which means everyone is hungry and nobody has a bed.","Grow the folk to fifteen, put six of them to work, and raise a house when the huts run out of room.",XS_NONE,50,
     TQ(QK_KPOP,0,15,"HAVE 15 NUBBINS"),TQ(QK_KWORK,0,6,"PUT 6 TO WORK"),TQ(QK_KBT,KB_HOUSE,1,"BUILD A HOUSE")),
  XM("THE FAR BANK","The little folk keep staring at the far bank and whispering. No bridge yet, so for now the realm spreads out.","Raise twenty buildings and a second storehouse, so the realm reaches every corner of this side.",XS_NONE,55,
     TQ(QK_KBLD,0,20,"RAISE 20 BUILDINGS"),TQ(QK_KBT,KB_STORE,2,"BUILD 2 STOREHOUSES")),
  XM("FOUNDATIONS","The folk have drawn a castle in the dirt with a stick. It is a very good drawing. Time to make it real.","Make planks and cut stone, lay the castle's foundation and raise its walls.",XS_NONE,60,
     TQ(QK_KCASTLE,0,1,"LAY THE FOUNDATION"),TQ(QK_KCASTLE,0,2,"RAISE THE WALLS")),
  XM("A CROWN, PROPERLY","The walls are up and the little folk are getting emotional. One of them has made a crown out of a bottle cap.","Raise the towers, finish the throne room and crown a monarch. Then keep playing: the realm never ends.",XS_NONE,100,
     TQ(QK_KCASTLE,0,3,"RAISE THE TOWERS"),TQ(QK_KCASTLE,0,4,"CROWN A MONARCH"),TQ(QK_KPOP,0,25,"RULE 25 NUBBINS")),
};
#undef TQ
