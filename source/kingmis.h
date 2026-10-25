// kingmis.h - EP6 THE LAND BOREFORE TIME, Phase 5 step c: THE TEN MISSIONS, and after the crown THE ROYAL YEARS: six more (11 to 16) that lead a realm from a castle to a crowd of three hundred. Included by expand.h in place of the old three placeholder missions (xmKing), after XM and before the macros are undefined.
// Played in your own life, done in the realm: each mission's tasks are checked against the open realm (kingq.h, kingtask.h; expand.h exKingTick) while you play KINGDOM MODE, and the mission closes, pays and
// opens the next one when you are back in the life game. Words: up to 119 characters a line, 30 a task name. [ASSUMED numbers; the pay is in hundreds of Simoleons]
// THE FAR BANK (8): a bridge is a road laid over water, one cut stone a cell (kingsim.h, step 5d). Expanding the map itself is still to come.
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
  XM("THE FAR BANK","The little folk keep staring at the far bank and whispering. So build a bridge. It costs cut stone.","Cut some stone and lay a bridge four cells across the water, then raise a second storehouse and a few more buildings.",XS_NONE,55,
     TQ(QK_KBRIDGE,0,4,"LAY 4 BRIDGE CELLS"),TQ(QK_KBT,KB_STORE,2,"BUILD 2 STOREHOUSES"),TQ(QK_KBLD,0,16,"RAISE 16 BUILDINGS")),
  XM("FOUNDATIONS","The folk have drawn a castle in the dirt with a stick. It is a very good drawing. Time to make it real.","Make planks and cut stone, lay the castle's foundation and raise its walls.",XS_NONE,60,
     TQ(QK_KCASTLE,0,1,"LAY THE FOUNDATION"),TQ(QK_KCASTLE,0,2,"RAISE THE WALLS")),
  XM("A CROWN, PROPERLY","The walls are up and the little folk are getting emotional. One of them has made a crown out of a bottle cap.","Raise the towers, finish the throne room and crown a monarch. Then keep playing: the realm never ends.",XS_NONE,100,
     TQ(QK_KCASTLE,0,3,"RAISE THE TOWERS"),TQ(QK_KCASTLE,0,4,"CROWN A MONARCH"),TQ(QK_KPOP,0,25,"RULE 25 NUBBINS")),
  // ---- THE ROYAL YEARS: after the crown. The realm never ends, so these ask for more of everything: a well and a tavern, a long bridge, a town, a full larder, a square with no room left, a crowd of three hundred. ----
  XM("ROYAL TASTES","The monarch has opinions, mostly about drinks, and none of them are polite.","Dig a well, open a tavern and bring the folk to forty: a crowned head wants an audience.",XS_NONE,60,
     TQ(QK_KBT,KB_WELL,1,"BUILD A WELL"),TQ(QK_KBT,KB_TAVERN,1,"BUILD A TAVERN"),TQ(QK_KPOP,0,40,"HAVE 40 NUBBINS")),
  XM("THE LONG WAY ROUND","Every Nubbin has a cousin on the far bank, and every cousin has been told to visit.","Lay sixty road cells, a long bridge twelve cells across, and let the Nubbins deliver a hundred loads.",XS_NONE,65,
     TQ(QK_KROAD,0,60,"LAY 60 ROAD CELLS"),TQ(QK_KBRIDGE,0,12,"LAY 12 BRIDGE CELLS"),TQ(QK_KDELIV,0,100,"DELIVER 100 LOADS")),
  XM("A PROPER TOWN","Nobody calls it a hamlet any more. Somebody has put up a sign, and the sign has a typo.","Raise thirty buildings, put ten Nubbins to work and bring the crowd to eighty.",XS_NONE,70,
     TQ(QK_KBLD,0,30,"RAISE 30 BUILDINGS"),TQ(QK_KWORK,0,10,"PUT 10 TO WORK"),TQ(QK_KPOP,0,80,"HAVE 80 NUBBINS")),
  XM("THE FULL LARDER","The monarch has inspected the stockpile. The monarch is making a face.","Stack sixty planks, forty cut stone and twenty cloth: enough to build whatever comes next.",XS_NONE,75,
     TQ(QK_KGOOD,G_PLANKS,60,"HAVE 60 PLANKS"),TQ(QK_KGOOD,G_CUT,40,"HAVE 40 CUT STONE"),TQ(QK_KGOOD,G_CLOTH,20,"HAVE 20 CLOTH")),
  XM("STANDING ROOM ONLY","The tavern is full. So is the square. So, somehow, is the castle, and nobody remembers inviting them.","Open a second tavern, raise twelve houses and let a hundred and fifty Nubbins fill the square.",XS_NONE,80,
     TQ(QK_KBT,KB_TAVERN,2,"BUILD 2 TAVERNS"),TQ(QK_KBT,KB_HOUSE,12,"BUILD 12 HOUSES"),TQ(QK_KPOP,0,150,"HAVE 150 NUBBINS")),
  XM("A REALM THAT NEVER ENDS","You came for a field. You now have a kingdom, a crowd and a bridge that goes nowhere in particular.","Fifty buildings and three hundred Nubbins, a dozen of them at work. After this the realm is yours to keep.",XS_NONE,100,
     TQ(QK_KBLD,0,50,"RAISE 50 BUILDINGS"),TQ(QK_KPOP,0,300,"RULE 300 NUBBINS"),TQ(QK_KWORK,0,12,"PUT 12 TO WORK")),
};
#undef TQ
