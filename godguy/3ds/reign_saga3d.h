// THE UNWRITTEN REIGN — 3DS branching story, film director and endings.
// Four acts with six authored opening beats each, four consequential
// dilemmas, eight choice aftermaths, and nine six-beat alternate finales.
// The camera visits ACTUAL world[][] coordinates; no extra render surface,
// duplicated game world, asset heap or 2D/DS/DSi story modification.
#ifndef GG_REIGN_SAGA3D_H
#define GG_REIGN_SAGA3D_H
#ifndef GG_SAGA_SAVE_PATH
#define GG_SAGA_SAVE_PATH "sdmc:/godguy_saga.dat"
#endif
enum { GG_SAGA_IDLE,GG_SAGA_OPEN,GG_SAGA_CHOICE,GG_SAGA_OUTRO,
       GG_SAGA_BEAT_TICKS=96,GG_SAGA_OPEN_BEATS=6,
       GG_SAGA_OUTRO_BEATS=3,GG_SAGA_END_BEATS=6,
       GG_SAGA_ENDINGS=9 };
enum { GG_END_HEARTH,GG_END_REPUBLIC,GG_END_CROWN,
       GG_END_MARSHAL,GG_END_VEIL,GG_END_STARBORN,
       GG_END_WITNESS,GG_END_WANDERER,GG_END_PRESIDENT };
typedef struct {
    int loaded,mode,frame,choice,ending,lastKeys;
    u32 decisions,decided,seen;
} GgReignSaga;
static GgReignSaga ggSaga;
typedef struct {u32 magic,version,decisions,decided,seen,checksum;} GgSagaDisk;
static u32 ggSagaChecksum(const GgSagaDisk *d){
    const u8 *p=(const u8*)d;
    u32 h=2166136261u;
    for(unsigned i=0;i<sizeof(*d)-sizeof(d->checksum);i++)
        h=(h^p[i])*16777619u;
    return h;
}
static void ggSagaSave(void){
    const GgSagaDisk d={
        0x47535341u,1,ggSaga.decisions,ggSaga.decided,ggSaga.seen,
        0 // checksum constructed below, not from padding
    };
    GgSagaDisk disk=d;
    disk.checksum=ggSagaChecksum(&disk);
    FILE *f=fopen(GG_SAGA_SAVE_PATH,"wb");
    if(f){(void)fwrite(&disk,sizeof disk,1,f);fclose(f);}
}
static void ggSagaLoad(void){
    if(ggSaga.loaded)return;
    ggSaga.loaded=1;ggSaga.ending=-1;
    FILE *f=fopen(GG_SAGA_SAVE_PATH,"rb");
    if(f){
        GgSagaDisk d;
        int ok=fread(&d,sizeof d,1,f)==1;
        fclose(f);
        if(ok&&d.magic==0x47535341u&&d.version==1&&
           d.checksum==ggSagaChecksum(&d)&&!(d.decided&~15u)&&
           !(d.decisions&~15u)&&!(d.seen&~((1u<<GG_SAGA_ENDINGS)-1))){
            ggSaga.decisions=d.decisions;
            ggSaga.decided=d.decided;
            ggSaga.seen=d.seen;
            return;
        }
    }
    // Owners of earlier 3DS campaign saves do not lose cleared acts.
    for(int a=0;a<ggChron.cleared&&a<GG_CHRON_ACTS;a++)
        ggSaga.decided|=1u<<a;
}
static void ggSagaBegin(void){
    ggSagaLoad();
    ggSaga.mode=GG_SAGA_OPEN;ggSaga.frame=0;
    ggSaga.choice=0;ggSaga.ending=-1;
    // Past moral decisions affect the next mission's available supplies,
    // camp confidence and living people's trust; no artificial new world.
    if(ggChron.act>0){
        int previous=ggChron.act-1;
        if(ggSaga.decided&(1u<<previous)){
            if(ggSaga.decisions&(1u<<previous)){
                ggChron.rations=ggExpClamp(ggChron.rations+1,0,9);
                ggChron.morale=ggExpClamp(ggChron.morale-9,0,100);
            }else{
                ggChron.morale=ggExpClamp(ggChron.morale+12,0,100);
                ggChron.hp=ggExpClamp(ggChron.hp+5,0,100);
            }
        }
    }
}
static int ggSagaChooseEnding(void){
    int last=(ggSaga.decisions>>3)&1;
    if(last){
        if(ggReign.role==GG_ROLE_SHADE&&ggReign.infamy>=18)
            return GG_END_VEIL;
        if(ggReign.role==GG_ROLE_MYSTIC&&ggReign.divine>=55&&
           (ggSaga.decisions&(1u<<2)))
            return GG_END_STARBORN;
        if((ggReign.role==GG_ROLE_SOLDIER||ggReign.role==GG_ROLE_WATCH)&&
           ggChron.score>=260)
            return GG_END_MARSHAL;
        if(ggReign.office>=4&&ggReign.legitimacy>=70)
            return GG_END_PRESIDENT;
        return GG_END_CROWN;
    }
    if(ggReign.office>=2&&ggReign.endorsements>=2)
        return GG_END_REPUBLIC;
    if(ggReign.role==GG_ROLE_SCHOLAR&&!(ggSaga.decisions&(1u<<2)))
        return GG_END_WITNESS;
    if((ggReign.role==GG_ROLE_HEALER||ggReign.role==GG_ROLE_FARMER)&&
       ggQuest.completed>0)
        return GG_END_HEARTH;
    return GG_END_WANDERER;
}
// Peaceful option available only at story-appropriate positions and if
// the player has genuinely built civic trust (or offers scarce rations).
// X first attempts diplomacy; otherwise it retains the normal fight and
// secure-post action. No extra pathfinder, actor list, or per-frame work.
static int ggSagaParley(int post){
    if(ggChron.act==GG_CHRON_ACTS-1&&post==GG_CHRON_POSTS-1)
        return 0; // the final captain must be confronted
    int pacify=0;
    if(ggChron.act==0&&post==0&&ggChron.rations>=2){
        ggChron.rations--;pacify=1;
    }else if((ggChron.act==1||ggChron.act==2||ggChron.act==3)&&
             post<2&&ggReign.influence>=18&&ggReign.legitimacy>=20&&
             ggReign.infamy<ggReign.legitimacy+20){
        ggReign.influence-=8;pacify=1;
    }
    if(!pacify)return 0;
    for(int j=0;j<ggChron.foeN;j++)if(ggChron.foe[j].post==post&&
                                      ggChron.foe[j].hp>0)
        ggChron.foe[j].hp=0; // guards surrender instead of being killed
    ggChron.morale=ggExpClamp(ggChron.morale+9,0,100);
    ggReign.renown=ggExpClamp(ggReign.renown+4,0,9999);
    ggChron.score+=30;
    ggChron.dirty=1;
    ggReignSave();
    return 1;
}
static void ggSagaStartChoice(void){
    ggSagaLoad();
    ggSaga.mode=GG_SAGA_CHOICE;
    ggSaga.frame=0;
    ggSaga.choice=(ggSaga.decisions>>ggChron.act)&1;
    ggSaga.lastKeys=0;
}
static void ggSagaCommitChoice(int choice){
    int act=ggChron.act;
    if(act<0||act>=GG_CHRON_ACTS)return;
    u32 bit=1u<<act;
    ggSaga.decided|=bit;
    if(choice)ggSaga.decisions|=bit;
    else ggSaga.decisions&=~bit;
    // The player alters tangible simulation values, not just a subtitle.
    // Decisions also leave a persistent moral record for the ending logic.
    int town=ggReignVillage(ggExplore.px>>8,ggExplore.py>>8,12);
    if(!choice){
        ggChron.morale=ggExpClamp(ggChron.morale+15,0,100);
        ggReign.legitimacy=ggExpClamp(ggReign.legitimacy+8,0,9999);
        ggReign.influence=ggExpClamp(ggReign.influence+4,0,9999);
        if(town>=0)ggReignVillageLoyalty(town,6);
    }else{
        ggReign.credits=ggExpClamp(ggReign.credits+18,0,9999);
        ggReign.infamy=ggExpClamp(ggReign.infamy+6,0,9999);
        ggChron.score+=35;
        if(town>=0)ggReignVillageLoyalty(town,-6);
    }
    ggChron.won=1;ggChron.briefTime=0;ggChron.dirty=1;
    if(ggChron.cleared<act+1)ggChron.cleared=act+1;
    ggChronSave();ggReignSave();
    ggSaga.ending=act==GG_CHRON_ACTS-1?ggSagaChooseEnding():-1;
    if(ggSaga.ending>=0)ggSaga.seen|=1u<<ggSaga.ending;
    ggSagaSave();
    ggSaga.mode=GG_SAGA_OUTRO;ggSaga.frame=0;
}
static const char *const ggSagaDecision[GG_CHRON_ACTS][2]={
    {"SAVE THE STRANDED FAMILIES","TAKE THE ARMY'S SUPPLIES"},
    {"NEGOTIATE WITH THE BROKEN WATCH","BREAK THEIR OATH BY FORCE"},
    {"PRESERVE THE NAMES AND RECORDS","CLAIM THE FORBIDDEN RECORDS"},
    {"RETURN THE WORLD TO ITS PEOPLE","TAKE THE REINS OF POWER"}
};
typedef struct {const char *voice,*line,*sub;} GgSagaBeat;
static const GgSagaBeat ggSagaOpening[GG_CHRON_ACTS][GG_SAGA_OPEN_BEATS]={
    {
        {"MIRA","The ford looked ordinary when you were a child.","That is why you knew what losing it would mean."},
        {"NARRATOR","Age twenty-two. The wind answered your outstretched hand.","But the people of the valley still needed bread."},
        {"MIRA","A wagon overturned beyond the watch tower.","The wounded cannot cross while soldiers block the road."},
        {"VARRO","I have orders to seize supplies for the frontier.","Tell your villagers that the crown will pay them later."},
        {"YOU","I know the roads. I know these families.","What I do not know is why the wind obeyed me."},
        {"MIRA","Find the three standards. Open a passage for us.","Then choose whom the saved provisions belong to."}
    },{
        {"THORGRIM","Our guards broke an oath, but not all betrayed us.","You must learn who gave the order."},
        {"VARRO","A hungry soldier is easily bought by a false promise.","A beaten soldier remembers only humiliation."},
        {"MIRA","The same road runs through three guard positions.","Their captains each know a different part of the truth."},
        {"NARRATOR","Your reputation follows you out of the ford.","Mercy and threats travel faster than your footsteps."},
        {"THORGRIM","I can hold a position. You must take the lead.","Give the squad an order and let its actions matter."},
        {"YOU","Their oath was made to people, not to stone.","Whether I restore it or break it is my burden."}
    },{
        {"SERIN","There is a library buried beyond the border.","It lists every family erased from the kingdom rolls."},
        {"NARRATOR","Your parents are among those named in its records.","They never knew what blood they passed to you."},
        {"ELOWEN","Old gods once asked mortals to surrender their names.","That is how they made obedience seem like destiny."},
        {"SERIN","Some pages heal a wound. Others teach domination.","No book can decide which reader you become."},
        {"MIRA","There are witnesses scattered among the villages.","Do not forget the living while chasing old secrets."},
        {"YOU","I was born neither king nor savior.","The truth should not be another kind of cage."}
    },{
        {"NARRATOR","The Hollow March opens before your own towns.","Their houses and fields are the stakes of this war."},
        {"VARRO","The rival banners have gathered for a last defense.","They will kneel to strength. Perhaps not to justice."},
        {"THORGRIM","The men will fight beside you if you ask them.","But even a loyal army cannot govern a peaceful day."},
        {"SERIN","Your name is written in both human and divine ink.","Neither page tells you what to do tomorrow."},
        {"ELOWEN","There is a throne. There is also a road home.","Only you can decide whether a crown is a duty."},
        {"YOU","No prophecy will speak my answer.","I will earn it in the world I actually lived in."}
    }
};
static const GgSagaBeat ggSagaAfter[GG_CHRON_ACTS][2][GG_SAGA_OUTRO_BEATS]={
    {
        {{"MIRA","The children have crossed the ford.","The wagons will feed more families tonight."},
         {"NARRATOR","Word spreads through the actual nearby towns.","A godborn stranger chose the living over tribute."},
         {"YOU","A miracle meant nothing until I made a choice.","Tomorrow, there will be another road to open."}},
        {{"VARRO","You took the provisions under your own banner.","The army will remember who held the stores."},
         {"MIRA","The travelers passed, but not all will trust you.","The money in your hand came from their table."},
         {"YOU","I thought strength could solve the crisis.","Now I must live with what it cost."}}
    },{
        {{"THORGRIM","The watch has begun to repair its broken oath.","No one says forgiveness happens in an afternoon."},
         {"VARRO","A negotiated peace is still a peace.","You will have to defend it when the banners change."},
         {"YOU","The soldiers' names remain in the village ledgers.","That should make betraying them much harder."}},
        {{"THORGRIM","The watch surrendered beneath your assault.","The streets are quiet, but not reconciled."},
         {"VARRO","Fear buys obedience. It never buys loyalty.","Remember that when you need an ally."},
         {"YOU","The standards are mine, and the cost is real.","The question is whether my next order is wiser."}}
    },{
        {{"SERIN","The names remain where ordinary people can read.","The archive has become a public testimony."},
         {"ELOWEN","You did not hide your parents' names from truth.","Their humanity was never a weakness."},
         {"YOU","No god can erase what these people were.","Their children will know the story."}},
        {{"SERIN","The secret pages belong to you alone now.","They contain power, and the means to abuse it."},
         {"ELOWEN","The old gods made this choice once before.","Their kingdoms were magnificent and terribly silent."},
         {"YOU","Knowing how to command is not the same as wisdom.","I hope I can tell the difference."}}
    },{
        {{"MIRA","The banners lower. The villages keep their own keys.","Your powers did not have to become a throne."},
         {"SERIN","Others will argue about your legacy.","Let their children have the right to disagree."},
         {"YOU","I will remain answerable to the lives I touched.","There is a world beyond this war."}},
        {{"VARRO","The banners lower before your personal standard.","What follows is yours to maintain or ruin."},
         {"THORGRIM","No army can fight forever. Men need homes.","Even your triumph must eventually become peace."},
         {"YOU","I have taken the reins in my own hands.","Now I must prove I deserve to hold them."}}
    }
};
static const char *const ggSagaEndNames[GG_SAGA_ENDINGS]={
    "THE HEARTH WARDEN","THE FREE REPUBLIC","THE HIGH CROWN",
    "THE WAR MARSHAL","THE NIGHT VEIL","THE STAR ASCENDANT",
    "THE WITNESS OF AGES","THE OPEN ROAD","THE ELECTED PRESIDENT"
};
static const GgSagaBeat ggSagaEndings[GG_SAGA_ENDINGS][GG_SAGA_END_BEATS]={
    {
        {"MIRA","The first winter after the war came early.","You stayed to mend roofs rather than claim a palace."},
        {"FARMER","The godborn one helped bring in the harvest.","No one here needs to bow to say thank you."},
        {"NARRATOR","Your deliveries and tending repaired real stores.","The world measured your rule in warm homes."},
        {"ELOWEN","The divine light in your hands grew gentler.","The old gods never understood this kind of power."},
        {"YOU","I was a child here before I knew my blood.","I can still belong here as an adult."},
        {"MIRA","The valley keeps a place for you at the fire.","The story ends where another life begins."}
    },{
        {"SERIN","The councils met without a monarch above them.","Towns bargained, argued and elected their delegates."},
        {"MIRA","Your endorsements had become responsibilities.","You let the people keep the authority they gave."},
        {"NARRATOR","The farms, docks and watch towers did not vanish.","Their elected keepers answered to living neighbors."},
        {"THORGRIM","The army took its orders from a public charter.","No bloodline could demand permanent obedience."},
        {"YOU","I could command the wind. Not the people's future.","Let them choose who deserves to lead."},
        {"ELOWEN","The republic was imperfect and alive.","That was better than a perfect silence."}
    },{
        {"VARRO","A new crown rose from the field of banners.","Its ruler held both an army and a god's strength."},
        {"SERIN","Your decrees rebuilt roads and sealed old borders.","Each victory also widened the throne's shadow."},
        {"NARRATOR","Some towns prospered; others feared disagreement.","A kingdom is never only its victories."},
        {"THORGRIM","You can order soldiers to march.","You cannot order every heart to believe."},
        {"YOU","I chose to govern by my own authority.","Every day I must justify that choice anew."},
        {"ELOWEN","The crown outlived the battle.","History still had to decide what it meant."}
    },{
        {"THORGRIM","The surviving companies rallied behind you.","Your orders had brought them through the march."},
        {"VARRO","Every border captain now studies your victories.","Some will admire you. Others will prepare."},
        {"NARRATOR","Your army made travel safe along contested roads.","It also became a power no village could ignore."},
        {"SOLDIER","We followed you into danger. Where next?","A leader owes an answer beyond the battlefield."},
        {"YOU","Winning was never the same as finishing.","My soldiers deserve peace that lasts."},
        {"MIRA","The march ended, though history kept moving.","Your finest command might yet be to stand down."}
    },{
        {"NARRATOR","The dark routes became an empire of whispers.","No throne room could contain the influence you built."},
        {"VARRO","The watch could guard a gate, not every secret.","The underworld learned to fear your name."},
        {"SERIN","The ledgers showed fortunes changing hands.","They could not show who held the strings."},
        {"MIRA","People still needed protection from their rulers.","I wondered whether they needed it from you."},
        {"YOU","I survived without permission or inheritance.","I must decide what my hidden power is for."},
        {"ELOWEN","The veiled reign grew without a coronation.","Even shadows leave footprints on the earth."}
    },{
        {"ELOWEN","The forbidden pages opened beneath your touch.","Stars answered where the old gods once commanded."},
        {"SERIN","No archive could hold what you had become.","Only the names of people kept you grounded."},
        {"NARRATOR","The Hollow March glowed with strange weather.","Farmers looked up and returned to their work."},
        {"VARRO","No army could force the heavens to bargain.","Yet a godborn soul could still be lonely."},
        {"YOU","This power was never proof that I was better.","It was only a question asked on a larger scale."},
        {"MIRA","You crossed the sky and came back to the ford.","That return became the miracle I remembered."}
    },{
        {"SERIN","You placed the rescued records in public trust.","Every name was copied for those who survived."},
        {"NARRATOR","Children could trace their families across the march.","No ruler could erase their past so easily."},
        {"ELOWEN","The old gods ruled by writing destinies.","You taught people to write their own."},
        {"SCHOLAR","The library welcomes questions, not vows.","Knowledge has become a civic defense."},
        {"YOU","My parents were ordinary people with a secret.","Their life matters more than my legend."},
        {"MIRA","The books remained open after you left.","That was the ending worth preserving."}
    },{
        {"NARRATOR","The road did not end at the final standard.","There were coastlines, villages and names beyond it."},
        {"MIRA","You had a place in the valley, if you wanted.","I knew you could not promise to stay."},
        {"THORGRIM","The surviving watch kept guard without you.","That was how we knew we had succeeded."},
        {"SERIN","The world's pages were never finished.","Nor was the person who carried its strange blood."},
        {"YOU","I was not born knowing I was a god.","I refuse to finish life thinking that is all I am."},
        {"ELOWEN","The godborn traveler took the open road.","No prophecy followed."}
    },{
        {"MIRA","Four towns had entrusted you with an election.","You answered with service rather than a coronation."},
        {"SERIN","The new president signed the civic charter.","Rights belonged to households, not divine ancestry."},
        {"THORGRIM","The army swore to protect the charter.","Its commander could be replaced by the next vote."},
        {"NARRATOR","Factories, docks and farms rebuilt on real ground.","The authority of office had a human limit."},
        {"YOU","I was elected for a term, not an eternity.","Even a demigod must leave when the term ends."},
        {"ELOWEN","The republic became an argument worth having.","That was the kind of peace the gods had missed."}
    }
};
static int ggSagaBeatCount(void){
    if(ggSaga.mode==GG_SAGA_OPEN)return GG_SAGA_OPEN_BEATS;
    if(ggSaga.mode==GG_SAGA_OUTRO)return ggSaga.ending>=0?
        GG_SAGA_END_BEATS:GG_SAGA_OUTRO_BEATS;
    return 1;
}
static int ggSagaStep(int raw){
    if(!ggSaga.mode)return 0;
    int down=raw&~ggSaga.lastKeys;
    ggSaga.lastKeys=raw;
    if(down&KEY_START){ggSaga.mode=GG_SAGA_IDLE;return 2;}
    if(ggSaga.mode==GG_SAGA_CHOICE){
        if(down&(KEY_UP|KEY_DOWN|KEY_LEFT|KEY_RIGHT|KEY_B))
            ggSaga.choice^=1;
        if(down&KEY_A)ggSagaCommitChoice(ggSaga.choice);
        return 1;
    }
    if(down&KEY_B){
        ggSaga.mode=GG_SAGA_IDLE;return 1;
    }
    int frames=ggSagaBeatCount()*GG_SAGA_BEAT_TICKS;
    if(down&KEY_A){
        ggSaga.frame=(ggSaga.frame/GG_SAGA_BEAT_TICKS+1)*
                     GG_SAGA_BEAT_TICKS;
    }else ggSaga.frame++;
    if(ggSaga.frame>=frames)ggSaga.mode=GG_SAGA_IDLE;
    return 1;
}
static const GgSagaBeat *ggSagaCurrentBeat(int n){
    if(ggSaga.mode==GG_SAGA_OPEN)
        return &ggSagaOpening[ggChron.act][n%GG_SAGA_OPEN_BEATS];
    if(ggSaga.ending>=0)
        return &ggSagaEndings[ggSaga.ending][n%GG_SAGA_END_BEATS];
    return &ggSagaAfter[ggChron.act][
        (ggSaga.decisions>>ggChron.act)&1][n%GG_SAGA_OUTRO_BEATS];
}
static void ggSagaCamera(int beat,int local,int *anchorX,int *anchorY){
    // Moving camera cut from one real expedition landmark to another.
    // Smooth 3/4-tile dolly is bounded; all camera math is fixed point.
    int anchor=beat%4,tx=ggExplore.px>>8,ty=ggExplore.py>>8;
    if(anchor) {
        tx=ggChron.px[anchor-1];
        ty=ggChron.py[anchor-1];
    }
    *anchorX=tx;*anchorY=ty;
    int a=(beat*51+ggChron.act*23+32)&255;
    int move=local*192/GG_SAGA_BEAT_TICKS;
    int distance=(4+(beat&1))*256-move;
    int side=(local*112/GG_SAGA_BEAT_TICKS)-56;
    int x=tx*256+128-ggExpCos[a]*distance/4096+
          ggExpSin[a]*side/4096;
    int y=ty*256+128-ggExpSin[a]*distance/4096-
          ggExpCos[a]*side/4096;
    ggExplore.px=ggExpClamp(x,384,(WW-2)*256);
    ggExplore.py=ggExpClamp(y,384,(WH-2)*256);
    ggExplore.angle=a;
    ggExplore.frame=ggSaga.frame;
}
static void ggSagaDraw(u8 *out){
    if(!ggSaga.mode)return;
    int index=ggSaga.mode==GG_SAGA_CHOICE?5:
              ggSaga.frame/GG_SAGA_BEAT_TICKS;
    int local=ggSaga.frame%GG_SAGA_BEAT_TICKS;
    int px=ggExplore.px,py=ggExplore.py;
    int angle=ggExplore.angle,frame=ggExplore.frame;
    int chronFrame=ggChron.frame;
    int stageX,stageY;
    ggSagaCamera(index,local,&stageX,&stageY);
    ggExpRender(out);
    ggChron.frame+=ggSaga.frame; // cinematic farm crops and flags move
    ggReignSceneDraw(out);
    ggChron.frame=chronFrame;
    ggExplore.px=px;ggExplore.py=py;
    ggExplore.angle=angle;ggExplore.frame=frame;
    // The protagonist is projected from the SAME world-space landmark
    // used by the camera. Occluding real buildings hide the cel. This
    // eliminates the previous floating 2D overlay in outdoor shots.
    if(ggOrigin.complete&&index%3!=2){
        int x,feet,size;
        if(ggChronProject(stageX,stageY,&x,&feet,&size))
            ggOriginPortrait(out,x,feet,ggExpClamp(size/12,1,3),
                             (ggSaga.frame/17)&1,0);
    }
    ggExpRect(out,0,0,400,43,13,24,37);
    ggExpRect(out,0,41,400,2,204,156,88);
    ggExpRect(out,0,185,400,55,13,23,36);
    ggExpRect(out,0,185,400,2,204,156,88);
    char line[110];
    if(ggSaga.mode==GG_SAGA_CHOICE){
        snprintf(line,sizeof line,"ACT %d - YOUR CHOICE CHANGES THE REALM",
                 ggChron.act+1);
        ggExpText(out,9,11,line,248,218,159);
        ggExpText(out,10,54,"THE LAST STANDARD FALLS. WHAT DO YOU DO?",
                  233,221,184);
        for(int i=0;i<2;i++){
            int y=85+i*39;
            ggExpRect(out,17,y,366,31,
                      i==ggSaga.choice?56:25,
                      i==ggSaga.choice?65:38,52);
            ggExpText(out,26,y+10,
                      ggSagaDecision[ggChron.act][i],
                      i==ggSaga.choice?255:191,
                      i==ggSaga.choice?229:208,162);
        }
        ggExpText(out,10,198,"CHOOSE WITH THE PAD. PRESS A TO COMMIT.",
                  241,225,188);
        ggExpText(out,10,215,"START RETURNS TO 2D WITHOUT RECORDING A CHOICE.",
                  191,210,219);
        return;
    }
    const GgSagaBeat *beat=ggSagaCurrentBeat(index);
    snprintf(line,sizeof line,"THE UNWRITTEN REIGN / ACT %d: %.32s",
             ggChron.act+1,ggChronActName[ggChron.act]);
    ggExpText(out,10,7,line,246,224,175);
    if(ggSaga.ending>=0){
        snprintf(line,sizeof line,"FINAL ENDING %d/9: %.34s",
                 ggSaga.ending+1,ggSagaEndNames[ggSaga.ending]);
        ggExpText(out,10,27,line,223,190,130);
    }else{
        snprintf(line,sizeof line,"WORLD CHAPTER / %s",
                 ggSaga.mode==GG_SAGA_OPEN?"BEFORE THE ROAD":"AFTER THE STANDARD");
        ggExpText(out,10,27,line,185,207,215);
    }
    snprintf(line,sizeof line,"%s:",beat->voice);
    ggExpText(out,11,194,line,255,216,145);
    ggExpText(out,11,208,beat->line,231,232,223);
    ggExpText(out,11,222,beat->sub,182,202,214);
    int full=ggSagaBeatCount()*GG_SAGA_BEAT_TICKS;
    ggExpRect(out,0,238,400*ggSaga.frame/full,2,207,169,103);
}
static void ggSagaFateDraw(u8 *out){
    char line[100];
    int chosen=0;
    for(int a=0;a<GG_CHRON_ACTS;a++)
        chosen+=!!(ggSaga.decided&(1u<<a));
    int seen=0;
    for(int e=0;e<GG_SAGA_ENDINGS;e++)
        seen+=!!(ggSaga.seen&(1u<<e));
    snprintf(line,sizeof line,"ACT CHOICES %d/4   DISCOVERED ENDINGS %d/%d",
             chosen,seen,GG_SAGA_ENDINGS);
    ggExpText(out,25,79,line,236,221,176);
    snprintf(line,sizeof line,"FINAL CHOICE: %s",
             !(ggSaga.decided&(1u<<3))?"NOT YET WRITTEN":
             (ggSaga.decisions&(1u<<3))?"PERSONAL RULE":"SHARED FREEDOM");
    ggExpText(out,25,98,line,195,217,230);
    ggExpText(out,25,118,"CAREERS, TOWNS, POWER AND OATHS SHAPE THE END.",
              195,210,221);
    ggExpText(out,25,134,"REPLAY THE FINAL ACT TO SEEK A DIFFERENT FATE.",
              235,192,137);
}
#endif
