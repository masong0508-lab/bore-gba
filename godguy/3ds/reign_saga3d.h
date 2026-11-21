// THE UNWRITTEN REIGN — 3DS branching story, film director and endings.
// Four acts with six authored opening beats each, four consequential
// dilemmas, eight choice aftermaths, and nine six-beat alternate finales.
// The camera visits ACTUAL world[][] coordinates; no extra render surface,
// duplicated game world, asset heap or 2D/DS/DSi story modification.
#ifndef GG_REIGN_SAGA3D_H
#define GG_REIGN_SAGA3D_H
#include "film_walk3d.h"
#include "film_camera3d.h"
#include "film_stage3d.h"
#include "film_framing3d.h"
#include "film_direction3d.h"
#include "film_projection3d.h"
#include "film_partner3d.h"
#include "film_type3d.h"
#ifndef GG_SAGA_SAVE_PATH
#define GG_SAGA_SAVE_PATH "sdmc:/godguy_saga.dat"
#endif
enum { GG_SAGA_IDLE,GG_SAGA_OPEN,GG_SAGA_CHOICE,GG_SAGA_OUTRO,
       GG_SAGA_INTERLUDE,
       GG_SAGA_BEAT_TICKS=96,GG_SAGA_OPEN_BEATS=6,
       GG_SAGA_OUTRO_BEATS=3,GG_SAGA_END_BEATS=6,
       GG_SAGA_ENDINGS=9,GG_AMBIENT_N=12,GG_AMBIENT_BEATS=3 };
enum { GG_END_HEARTH,GG_END_REPUBLIC,GG_END_CROWN,
       GG_END_MARSHAL,GG_END_VEIL,GG_END_STARBORN,
       GG_END_WITNESS,GG_END_WANDERER,GG_END_PRESIDENT };
// Shared story event IDs must exist BEFORE the scene renderer uses them.
enum { GG_AMBIENT_TOWN,GG_AMBIENT_FOREST,GG_AMBIENT_COAST,
       GG_AMBIENT_RUIN,GG_AMBIENT_MARKET,GG_AMBIENT_FARM,
       GG_AMBIENT_CLINIC,GG_AMBIENT_ARCHIVE,GG_AMBIENT_SQUAD,
       GG_AMBIENT_OATH,GG_AMBIENT_DELIVERY,GG_AMBIENT_MIRACLE };
typedef struct {
    int loaded,mode,frame,choice,ending,lastKeys;
    int paused; // film playback only; never alters the existing save format
    int event,eventX,eventY; // anchored real-world nonlinear scene
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
    ggSaga.paused=0;
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
    ggSaga.frame=0;ggSaga.paused=0;
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
    ggSaga.mode=GG_SAGA_OUTRO;ggSaga.frame=0;ggSaga.paused=0;
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

static const char *const ggSagaAmbientTitles[GG_AMBIENT_N]={
    "THE FIRST TOWN","UNDER THE OLD TREES","WHERE THE WATER MEETS THE LAND",
    "STONES THAT REMEMBER","THE MARKET'S VOICES","THE HANDS THAT FEED",
    "THE LANTERN CLINIC","THE PUBLIC ARCHIVE","NO SOLDIER ALONE",
    "AN OATH WITH LIMITS","A WAGON RETURNS","THE QUIET MIRACLE"
};
// These scenes can be encountered in any order. All are shot in the
// physical place where the player actually found their trigger. Choices,
// careers, village supply and quest results are read from live gameplay.
static const GgSagaBeat ggSagaAmbientBeats[GG_AMBIENT_N][GG_AMBIENT_BEATS]={
    {
        {"MIRA","A town is not a row of houses.","It is every person who trusted someone to build them."},
        {"CITIZEN","Our bread and our watchmen must both be paid for.","Kings remember the towers. Farmers remember the stores."},
        {"YOU","My strange blood does not change their worries.","Maybe listening is the first kind of power."}
    },{
        {"NARRATOR","The oldest trees predate the border.","The wind moves the same way it did in your childhood."},
        {"ELOWEN","A forest has no throne and still has its laws.","Those who take everything leave nothing to inherit."},
        {"YOU","The trees are not asking to be ruled.","For once I am content to keep walking."}
    },{
        {"NARRATOR","The coast opens beyond your familiar fields.","Fishing boats trace the horizon of a real world."},
        {"SAILOR","We can trade with cities we cannot even see yet.","That is how a little village learns to hope."},
        {"YOU","The horizon asks a better question than a map.","What else is possible beyond the next shore?"}
    },{
        {"SERIN","These stones remember empires no one named correctly.","A broken column is still evidence."},
        {"ELOWEN","The old gods made monuments to their obedience.","People made homes, wells and songs."},
        {"YOU","I will not mistake the ruin for a destiny.","What matters is what we build beside it."}
    },{
        {"TRADER","Every stall belongs to someone who took a risk.","A town survives on promises it can keep."},
        {"MIRA","CORPORATIONS rise by trading lives for numbers.","Do not forget the faces behind the inventory."},
        {"YOU","A good bargain should leave both sides standing.","Perhaps profit can have a conscience."}
    },{
        {"FARMER","The earth does not grow faster for kings.","We measure good seasons in full cupboards."},
        {"YOU","I worked these same fields before the wind answered.","My hands remember their ordinary work."},
        {"MIRA","Some miracles look like a harvest.","Others look like a farmer getting paid fairly."}
    },{
        {"HEALER","Our patients come from every banner.","The wound does not ask which side they chose."},
        {"YOU","I can mend flesh, but not every argument.","I will sit with the injured anyway."},
        {"MIRA","The clinic's light stays lit after sunset.","This is the kind of courage a kingdom should fund."}
    },{
        {"SERIN","The oldest ledgers were written by ordinary hands.","Every name was a life, not a statistic."},
        {"YOU","My own family is among the unknown entries.","There is no godhood that can replace them."},
        {"ELOWEN","Let this archive belong to the towns.","Truth is less fragile when everybody may read."}
    },{
        {"THORGRIM","A companion swears loyalty, not silence.","Give your soldiers room to disagree."},
        {"YOU","I asked them to follow me into danger.","I owe them an explanation when we return."},
        {"SOLDIER","We do not need an immortal commander.","We need one who knows when to listen."}
    },{
        {"CITIZEN","The council granted you an office.","Can it take that office away if you fail?"}, 
        {"YOU","It must be able to.","A divine bloodline is no excuse for refusing a vote."},
        {"SERIN","Then write those limits into the public charter.","Let your promises outlive your portrait."}
    },{
        {"CARTER","The wagon got through the mountain road.","Twelve crates can keep a village alive this winter."},
        {"MIRA","A journey only matters if someone reaches home.","Look at what changed in the actual stores."},
        {"YOU","I thought adventure would be grander than this.","Perhaps helping a family eat is grand enough."}
    },{
        {"ELOWEN","The light in your palms has grown steadier.","The old gods would have named it a weapon."},
        {"YOU","I was twenty-two when I first saw it.","I still do not know everything I can become."},
        {"MIRA","Then do not let the unknown make you cruel.","Come home when you can. Keep choosing."}
    }
};
static int ggSagaBeatCount(void){
    if(ggSaga.mode==GG_SAGA_INTERLUDE)return GG_AMBIENT_BEATS;
    if(ggSaga.mode==GG_SAGA_OPEN)return GG_SAGA_OPEN_BEATS;
    if(ggSaga.mode==GG_SAGA_OUTRO)return ggSaga.ending>=0?
        GG_SAGA_END_BEATS:GG_SAGA_OUTRO_BEATS;
    return 1;
}
static int ggSagaStep(int raw){
    if(!ggSaga.mode)return 0;
    int down=raw&~ggSaga.lastKeys;
    ggSaga.lastKeys=raw;
    if(down&KEY_START){
        int optional=ggSaga.mode==GG_SAGA_INTERLUDE;
        ggSaga.mode=GG_SAGA_IDLE;ggSaga.paused=0;
        return optional?1:2;
    }
    if(ggSaga.mode==GG_SAGA_CHOICE){
        if(down&(KEY_UP|KEY_DOWN|KEY_LEFT|KEY_RIGHT|KEY_B))
            ggSaga.choice^=1;
        if(down&KEY_A)ggSagaCommitChoice(ggSaga.choice);
        return 1;
    }
    if(down&KEY_B){
        ggSaga.mode=GG_SAGA_IDLE;ggSaga.paused=0;return 1;
    }
    // Let the player stop and READ story dialogue without pausing the
    // wider simulation or creating an audio/render loop. A still
    // advances a beat; B and START remain emergency escape routes.
    if(down&KEY_X){ggSaga.paused^=1;return 1;}
    if(ggSaga.paused&&!(down&KEY_A))return 1;
    int frames=ggSagaBeatCount()*GG_SAGA_BEAT_TICKS;
    if(down&KEY_A){
        ggSaga.frame=(ggSaga.frame/GG_SAGA_BEAT_TICKS+1)*
                     GG_SAGA_BEAT_TICKS;
    }else ggSaga.frame++;
    if(ggSaga.frame>=frames){
        ggSaga.frame=frames;
        ggSaga.mode=GG_SAGA_IDLE;
        ggSaga.paused=0;
    }
    return 1;
}
static const GgSagaBeat *ggSagaCurrentBeat(int n){
    if(ggSaga.mode==GG_SAGA_INTERLUDE)
        return &ggSagaAmbientBeats[ggSaga.event][n%GG_AMBIENT_BEATS];
    if(ggSaga.mode==GG_SAGA_OPEN)
        return &ggSagaOpening[ggChron.act][n%GG_SAGA_OPEN_BEATS];
    if(ggSaga.ending>=0)
        return &ggSagaEndings[ggSaga.ending][n%GG_SAGA_END_BEATS];
    return &ggSagaAfter[ggChron.act][
        (ggSaga.decisions>>ggChron.act)&1][n%GG_SAGA_OUTRO_BEATS];
}
// Film-only shot geometry: a shot's bearing/grounded vantage is solved
// ONCE per beat and reused for every frame. The actor and camera refer to
// the SAME live settlement, coast or expedition post. No new framebuffers.
typedef struct {
    int mode,act,beat,eventX,eventY,anchorX,anchorY;
    int landmarkX,landmarkY,angle,distance,valid;
    int approachX,approachY; // prevalidated entry tile, fixed for two beats
    GgFilmCameraTrack camera; // camera corridor also solved once per shot
    int sidesResolved,plusValid,minusValid;
    int pairResolved,pairKind,pairSourceHero;
    int plusX,plusY,minusX,minusY;
} GgSagaShot;
static GgSagaShot ggSagaShot;
typedef struct {int stageX,stageY;} GgSagaCameraContext;
static int ggSagaCameraFloor(int x,int y,int elevation,void *user){
    const GgSagaCameraContext *c=(const GgSagaCameraContext*)user;
    return ggChronLand(x,y)&&ggExpGround(world[y][x])==elevation&&
           ggExpLineClear(x,y,c->stageX,c->stageY);
}

// Paired dialogue lines are one establishing shot, not two unrelated
// camera teleports. A controlled second shot occurs after the pair.
static void ggSagaLocation(int beat,int *x,int *y){
    int tx=ggExplore.px>>8,ty=ggExplore.py>>8;
    if(ggSaga.mode==GG_SAGA_INTERLUDE){
        tx=ggSaga.eventX;ty=ggSaga.eventY;
    }else if(ggSaga.mode==GG_SAGA_CHOICE){
        tx=ggChron.px[GG_CHRON_POSTS-1];
        ty=ggChron.py[GG_CHRON_POSTS-1];
    }else{
        int p;
        if(ggSaga.mode==GG_SAGA_OPEN)
            p=beat/2-1; // opening: home, first post, second post
        else p=GG_CHRON_POSTS-1-beat/2; // aftermath: from the final post
        if(p>=0&&p<GG_CHRON_POSTS){
            tx=ggChron.px[p];ty=ggChron.py[p];
        }
    }
    *x=ggExpClamp(tx,1,WW-2);
    *y=ggExpClamp(ty,1,WH-2);
}
// Never film a person INSIDE a landmark or another living citizen.
// A bounded 7x7 search happens only when constructing a new shot.
static int ggSagaStagingCell(int landmarkX,int landmarkY,
                             int *stageX,int *stageY){
    if(ggChronLand(landmarkX,landmarkY)){
        *stageX=landmarkX;*stageY=landmarkY;return 1;
    }
    for(int r=1;r<=3;r++)
        for(int dy=-r;dy<=r;dy++)
            for(int dx=-r;dx<=r;dx++){
                if(abs(dx)+abs(dy)!=r)continue;
                int x=landmarkX+dx,y=landmarkY+dy;
                if(ggChronLand(x,y)){
                    *stageX=x;*stageY=y;return 1;
                }
            }
    return 0;
}
// The cast resolver is defined later with the script/voice helpers.
static void ggSagaPairGuests(int beat,int *first,int *second);
static void ggSagaCamera(int beat,int local,int *anchorX,int *anchorY){
    int shot=beat/2,tx,ty;
    ggSagaLocation(beat,&tx,&ty);
    int changed=!ggSagaShot.valid||ggSagaShot.mode!=ggSaga.mode||
       ggSagaShot.act!=ggChron.act||ggSagaShot.beat!=shot||
       ggSagaShot.eventX!=ggSaga.eventX||
       ggSagaShot.eventY!=ggSaga.eventY||
       ggSagaShot.landmarkX!=tx||ggSagaShot.landmarkY!=ty;
    if(changed){
        int landmarkX=tx,landmarkY=ty;
        // Scene staging is determined ONCE PER SHOT, not per frame.
        // This preserves the same actor position for both dialogue
        // lines and avoids repeated settlement searches in the render
        // loop on original 3DS hardware.
        if(!ggSagaStagingCell(tx,ty,&tx,&ty)){
            int px=ggExplore.px>>8,py=ggExplore.py>>8;
            if(ggChronLand(px,py)){tx=px;ty=py;}
        }
        ggSagaShot.valid=1;ggSagaShot.mode=ggSaga.mode;
        ggSagaShot.act=ggChron.act;ggSagaShot.beat=shot;
        ggSagaShot.eventX=ggSaga.eventX;
        ggSagaShot.eventY=ggSaga.eventY;
        ggSagaShot.landmarkX=landmarkX;
        ggSagaShot.landmarkY=landmarkY;
        ggSagaShot.anchorX=tx;ggSagaShot.anchorY=ty;
        ggSagaShot.sidesResolved=0;
        ggSagaShot.pairResolved=0;
        ggSagaShot.angle=(shot*51+ggChron.act*23+32)&255;
        ggSagaShot.distance=3;
        // All 24 candidate angles/distances are tested ONCE at take
        // entry. Previously the director chose the first unobstructed
        // angle even when both guests were hidden or overlapping.
        // We now favor distinct, horizon-safe silhouettes while retaining
        // the same physical tile, collision and terrain constraints.
        int found=0,bestScore=-100000,startingAngle=ggSagaShot.angle;
        for(int d=4;d>=2;d--)
            for(int turn=0;turn<8;turn++){
                int a=(startingAngle+turn*32)&255;
                int fx=ggExpCos[a],fy=ggExpSin[a];
                int cx=tx*256+128-fx*d*256/4096;
                int cy=ty*256+128-fy*d*256/4096;
                int gx=cx>>8,gy=cy>>8;
                if(!ggChronLand(gx,gy)||
                   !ggExpLineClear(gx,gy,tx,ty))continue;
                // Reject a collision-free viewpoint if the hero's boots
                // already cross the captions or the head clips the matte.
                GgFilmCameraSubject hero={
                    tx*256+128,ty*256+128,ggExpGround(world[ty][tx])
                };
                if(!ggFilmCameraSubjectFits(cx,cy,fx,fy,
                         ggExpGround(world[gy][gx]),hero,
                         ggFilmShotScale(d)))continue;
                int sx=-fy,sy=fx,dx=0,dy=0;
                if(abs(sx)>=abs(sy))dx=sx>=0?1:-1;
                else dy=sy>=0?1:-1;
                int stageHeight=ggExpGround(world[ty][tx]);
                int pValid=ggChronLand(tx+dx,ty+dy)&&
                           ggFilmStageHeightOK(stageHeight,
                               ggExpGround(world[ty+dy][tx+dx]))&&
                           ggExpLineClear(gx,gy,tx+dx,ty+dy);
                int mValid=ggChronLand(tx-dx,ty-dy)&&
                           ggFilmStageHeightOK(stageHeight,
                               ggExpGround(world[ty-dy][tx-dx]))&&
                           ggExpLineClear(gx,gy,tx-dx,ty-dy);
                int score=ggFilmFrameScore(cx,cy,fx,fy,
                              tx*256+128,ty*256+128,
                              (tx+dx)*256+128,(ty+dy)*256+128,
                              (tx-dx)*256+128,(ty-dy)*256+128,
                              pValid,mValid);
                score-=ggFilmFrameHeightPenalty(stageHeight,
                                 ggExpGround(world[gy][gx]));
                // Terrain LOS protects the center, but a near building
                // can still slice across the left/right third of frame.
                // Scan only a 7x7 NEIGHBORHOOD of existing bld[][] tiles
                // when authoring a shot (24 candidates maximum).
                for(int oy=-3;oy<=3;oy++)for(int ox=-3;ox<=3;ox++){
                    int nx=gx+ox,ny=gy+oy;
                    if((unsigned)nx>=WW||(unsigned)ny>=WH||
                       !bld[ny][nx])continue;
                    int rx=nx*256+128-cx,ry=ny*256+128-cy;
                    int z=(rx*fx+ry*fy)/4096;
                    int side=(-rx*fy+ry*fx)/4096;
                    score-=ggFilmFrameObstructionPenalty(z,side,1);
                }
                // Keep some shot variety when compositions tie, without
                // allowing a whim of shot index to hide the speakers.
                score-=turn;
                if(!found||score>bestScore){
                    found=1;bestScore=score;
                    ggSagaShot.angle=a;ggSagaShot.distance=d;
                }
            }
        // Choose one continuous dolly path through *visible*, level,
        // unoccupied world tiles. This bounded search runs only when
        // entering a new paired take; the renderer merely interpolates.
        int cameraAngle=ggSagaShot.angle;
        int cx=tx*256+128-
               ggExpCos[cameraAngle]*ggSagaShot.distance*256/4096;
        int cy=ty*256+128-
               ggExpSin[cameraAngle]*ggSagaShot.distance*256/4096;
        ggSagaShot.camera.fromX=ggSagaShot.camera.toX=cx;
        ggSagaShot.camera.fromY=ggSagaShot.camera.toY=cy;
        if(found){
            GgSagaCameraContext context={tx,ty};
            int fx=ggExpCos[cameraAngle],fy=ggExpSin[cameraAngle];
            int cameraGround=ggExpGround(world[cy>>8][cx>>8]);
            ggFilmCameraPlan(&ggSagaShot.camera,cx,cy,
                             fx,fy,cameraGround,
                             ggSagaCameraFloor,&context);
            // Hold the hero and all staged speaking partners within
            // the safe 400x240 picture across the full camera dolly.
            // This bounded check happens only once at shot setup.
            GgFilmCameraSubject cast[3]={
                {tx*256+128,ty*256+128,ggExpGround(world[ty][tx])}
            };
            int castCount=1,first=-1,second=-1;
            ggSagaPairGuests(beat,&first,&second);
            if(first>=0){
                int sideX=-fy,sideY=fx,dx=0,dy=0;
                if(abs(sideX)>=abs(sideY))dx=sideX>=0?1:-1;
                else dy=sideY>=0?1:-1;
                int stageHeight=cast[0].ground;
                int plusOK=ggChronLand(tx+dx,ty+dy)&&
                    ggFilmStageHeightOK(stageHeight,
                        ggExpGround(world[ty+dy][tx+dx]))&&
                    ggExpLineClear(cx>>8,cy>>8,tx+dx,ty+dy);
                int minusOK=ggChronLand(tx-dx,ty-dy)&&
                    ggFilmStageHeightOK(stageHeight,
                        ggExpGround(world[ty-dy][tx-dx]))&&
                    ggExpLineClear(cx>>8,cy>>8,tx-dx,ty-dy);
                int firstSide=ggFilmStageGuestSide(plusOK,minusOK,0);
                if(firstSide){
                    int guestX=tx+firstSide*dx,guestY=ty+firstSide*dy;
                    cast[castCount++]=(GgFilmCameraSubject){
                        guestX*256+128,guestY*256+128,
                        ggExpGround(world[guestY][guestX])
                    };
                }
                if(second>=0&&plusOK&&minusOK){
                    int guestX=tx-dx,guestY=ty-dy;
                    cast[castCount++]=(GgFilmCameraSubject){
                        guestX*256+128,guestY*256+128,
                        ggExpGround(world[guestY][guestX])
                    };
                }
            }
            ggFilmCameraKeepCastInFrame(&ggSagaShot.camera,cast,
                  castCount,fx,fy,cameraGround,
                  ggFilmShotScale(ggSagaShot.distance));
        }
        // Instead of walking a few pixels in place, enter from a real
        // neighboring tile toward the camera. One lookup per TWO dialogue
        // beats; no new world or collision checks in the frame renderer.
        ggSagaShot.approachX=tx;
        ggSagaShot.approachY=ty;
        if(found&&ggChronLand(tx,ty)){
            int a=ggSagaShot.angle;
            int fx=ggExpCos[a],fy=ggExpSin[a];
            int dx=abs(fx)>=abs(fy)?(fx>=0?1:-1):0;
            int dy=dx?0:(fy>=0?1:-1);
            int sx=tx-dx,sy=ty-dy;
            int cx=tx*256+128-fx*ggSagaShot.distance*256/4096;
            int cy=ty*256+128-fy*ggSagaShot.distance*256/4096;
            // Equal-height, clear floor keeps feet planted, avoids a
            // cliff-height pop halfway through the tracking shot and
            // cannot overlap a building, citizen or water cell.
            if(ggChronLand(sx,sy)&&
               ggExpGround(world[sy][sx])==ggExpGround(world[ty][tx])&&
               ggExpLineClear(cx>>8,cy>>8,sx,sy)){
                // On very tight close-ups an actor entering from the
                // adjoining tile starts nearer than the subtitle-safe
                // minimum depth. Reject that approach at shot setup,
                // rather than silently popping the figure into frame
                // half a second later. Fallback is the short on-tile
                // entrance, still controlled by the same easing curve.
                GgFilmCameraSubject entrance={
                    sx*256+128,sy*256+128,ggExpGround(world[sy][sx])
                };
                if(ggFilmCameraSubjectFits(ggSagaShot.camera.fromX,
                       ggSagaShot.camera.fromY,
                       ggExpCos[a],ggExpSin[a],
                       ggExpGround(world[ggSagaShot.camera.fromY>>8]
                                         [ggSagaShot.camera.fromX>>8]),
                       entrance,ggFilmShotScale(ggSagaShot.distance))){
                    ggSagaShot.approachX=sx;
                    ggSagaShot.approachY=sy;
                }
            }
        }
    }
    // Never recompute a shot's stage after its first frame. That would
    // allow crowd movement to make the movie's actors jump locations.
    tx=ggSagaShot.anchorX;ty=ggSagaShot.anchorY;
    *anchorX=tx;*anchorY=ty;
    int x,y;
    ggFilmCameraTrackAt(&ggSagaShot.camera,beat,local,
                        GG_SAGA_BEAT_TICKS,&x,&y);
    ggExplore.px=ggExpClamp(x,384,(WW-2)*256);
    ggExplore.py=ggExpClamp(y,384,(WH-2)*256);
    ggExplore.angle=ggSagaShot.angle;
    ggExplore.frame=ggSaga.frame;
}
// A character's actual Q8 location is projected from the FILM camera,
// not from the player's temporarily parked first-person camera.
static int ggSagaProjectActor(int worldX,int worldY,
                              int *sx,int *feet,int *scale){
    int tx=worldX>>8,ty=worldY>>8;
    if((unsigned)tx>=WW||(unsigned)ty>=WH)return 0;
    int vx=worldX-ggExplore.px,vy=worldY-ggExplore.py;
    int a=ggExplore.angle&255;
    int forward=(vx*ggExpCos[a]+vy*ggExpSin[a])/4096;
    int side=(-vx*ggExpSin[a]+vy*ggExpCos[a])/4096;
    if(forward<=80||forward>16*256||
       abs(side)*4>forward*5||
       !ggExpLineClear(ggExplore.px>>8,ggExplore.py>>8,tx,ty))
        return 0;
    *sx=ggFilmProjectScreenX(side,forward);
    if(ggExpMeshEnabled){
        // Match the actual new polygon camera's vertical perspective;
        // the legacy inverse ray-column projection floats above sloped
        // mesh ground, especially near walls and during camera dollies.
        int quality=ggExpScenicQuality>=2?1:2;
        int vertical=quality==1?7:4;
        int horizon=quality==1?116:58;
        int viewGround=ggExpGround(world[ggExplore.py>>8][ggExplore.px>>8]);
        int personGround=ggExpGround(world[ty][tx]);
        *feet=(horizon+(viewGround+13-personGround)*
               vertical*256/forward)*quality;
    }else{
        *feet=ggFilmProjectFootY(forward,
                                ggExpGround(world[ggExplore.py>>8][ggExplore.px>>8]),
                                ggExpGround(world[ty][tx]));
    }
    // A take, not an individual sprite or per-frame depth rounding,
    // owns cast scale. All actors share their actual feet plane.
    *scale=ggFilmShotScale(ggSagaShot.distance);
    return ggFilmFootSafe(*sx,*feet,*scale);
}
static void ggSagaActorPosition(int stageX,int stageY,int beat,int local,
                                int *worldX,int *worldY){
    // The hero enters on a real adjacent terrain cell if it is safe,
    // then settles at the same world-space mark for the entire response.
    // A blocked lane gets a small same-tile movement instead of
    // clipping through cliffs, houses or other residents.
    int a=ggSagaShot.angle;
    ggFilmWalkWorldPosition(stageX,stageY,
                            ggSagaShot.approachX,ggSagaShot.approachY,
                            ggExpCos[a],ggExpSin[a],beat,local,
                            GG_SAGA_BEAT_TICKS,worldX,worldY);
}
// Small, recurring film cast drawn using the same 16x24 authored human
// cel as the playable adult. Named speakers have stable clothes/hair;
 // every actor is placed on a passable tile within the live location.
// Zero imported image assets or heap allocations.
static int ggSagaGuestStyle(const char *voice){
    if(!strcmp(voice,"YOU")||!strcmp(voice,"NARRATOR"))return -1;
    if(!strcmp(voice,"MIRA"))return 0;
    if(!strcmp(voice,"THORGRIM"))return 1;
    if(!strcmp(voice,"VARRO"))return 2;
    if(!strcmp(voice,"SERIN"))return 3;
    if(!strcmp(voice,"ELOWEN"))return 4;
    return 5; // farmers, witnesses, workers and traveling residents
}
// A single real-world staging rule is shared by guest cels and scene
// props. The preferred side prevents objects from occupying a speaker.
// Lock every two-line conversation to at most two distinct named
// characters. A narrator or hero line never swaps a guest's costume
// in place; two named speakers remain present throughout the take.
static void ggSagaPairGuests(int beat,int *first,int *second){
    int begin=beat&~1,count=ggSagaBeatCount();
    *first=*second=-1;
    for(int k=begin;k<begin+2&&k<count;k++){
        int style=ggSagaGuestStyle(ggSagaCurrentBeat(k)->voice);
        if(style<0||style==*first||style==*second)continue;
        if(*first<0)*first=style;
        else if(*second<0)*second=style;
    }
    // Adjacent narrator-only beats retain a familiar actor rather than
    // replacing them with a fabricated extra or an empty empty frame.
    if(*first<0)
        for(int k=begin-1;k>=0&&k>=begin-2;k--){
            int style=ggSagaGuestStyle(ggSagaCurrentBeat(k)->voice);
            if(style>=0){*first=style;break;}
        }
}
static int ggSagaConversationGuest(int beat){
    int first,second;
    ggSagaPairGuests(beat,&first,&second);
    return first;
}
// The staging sides are selected once when a take begins. A transient
// crowd-cell change or a held dialogue frame cannot teleport a guest
// or a physical prop on screen. Both sides are distinct; no duplicates.
// Resolve actual passable left/right staging tiles once per take.
// The camera may move later, but the set and actors do not teleport.
static void ggSagaResolveSides(int tx,int ty){
    if(ggSagaShot.sidesResolved)return;
    int a=ggSagaShot.angle;
    int sx=-ggExpSin[a],sy=ggExpCos[a],dx=0,dy=0;
    if(abs(sx)>=abs(sy))dx=sx>=0?1:-1;
    else dy=sy>=0?1:-1;
    int px=ggExplore.px>>8,py=ggExplore.py>>8;
    ggSagaShot.plusX=(tx+dx)*256+128;
    ggSagaShot.plusY=(ty+dy)*256+128;
    ggSagaShot.minusX=(tx-dx)*256+128;
    ggSagaShot.minusY=(ty-dy)*256+128;
    int stageHeight=ggExpGround(world[ty][tx]);
    ggSagaShot.plusValid=ggChronLand(tx+dx,ty+dy)&&
                         ggFilmStageHeightOK(stageHeight,
                           ggExpGround(world[ty+dy][tx+dx]))&&
                         ggExpLineClear(px,py,tx+dx,ty+dy);
    ggSagaShot.minusValid=ggChronLand(tx-dx,ty-dy)&&
                          ggFilmStageHeightOK(stageHeight,
                            ggExpGround(world[ty-dy][tx-dx]))&&
                          ggExpLineClear(px,py,tx-dx,ty-dy);
    ggSagaShot.sidesResolved=1;
}
static void ggSagaSidePosition(int side,int *worldX,int *worldY){
    *worldX=side>0?ggSagaShot.plusX:ggSagaShot.minusX;
    *worldY=side>0?ggSagaShot.plusY:ggSagaShot.minusY;
}
static int ggSagaSideTile(int tx,int ty,int preferred,int fallback,
                          int *worldX,int *worldY){
    ggSagaResolveSides(tx,ty);
    int plus=preferred>=0;
    if(plus?ggSagaShot.plusValid:ggSagaShot.minusValid){
        ggSagaSidePosition(plus?1:-1,worldX,worldY);
        return 1;
    }
    if(fallback&&
       (plus?ggSagaShot.minusValid:ggSagaShot.plusValid)){
        ggSagaSidePosition(plus?-1:1,worldX,worldY);
        return 1;
    }
    return 0;
}
static int ggSagaGuestTile(int tx,int ty,int *guestX,int *guestY){
    return ggSagaSideTile(tx,ty,1,1,guestX,guestY);
}
// Use the same foreground span test for the direct RGB shadow blend.
#define GG_FILM_SHADOW_MASKED(x,y) ggExpFilmPixelHidden((x),(y))
#include "film_acting3d.h"
#undef GG_FILM_SHADOW_MASKED
static void ggSagaGuestPortrait(u8 *out,int x,int feet,int scale,
                                int style,int talking){
    static const u8 palette[6][3][3]={
        {{126,65,46},{212,168,133},{68,132,111}}, // Mira
        {{72,64,58},{200,159,129},{68,99,135}},  // Thorgrim
        {{103,78,64},{207,164,137},{143,58,66}}, // Varro
        {{83,57,53},{201,154,120},{105,118,78}}, // Serin
        {{211,191,146},{226,188,155},{110,91,143}}, // Elowen
        {{90,74,58},{204,167,136},{112,99,78}}   // working people
    };
    const u8 *hair=palette[style][0],*skin=palette[style][1];
    const u8 *coat=palette[style][2];
    ggExpRect(out,x-7*scale,feet-2,14*scale,2,28,36,40);
    for(int py=0;py<24;py++){
        const char *row=ggOriginHumanCel[py];
        // Run-length groups cut dozens of pixel-rectangle calls per cel.
        for(int px=0;px<16;){
            int kind=row[px],end=px+1;
            while(end<16&&row[end]==kind)end++;
            if(kind!='.'){
                int r=42,g=45,b=50;
                if(kind=='h'){r=hair[0];g=hair[1];b=hair[2];}
                else if(kind=='s'){r=skin[0];g=skin[1];b=skin[2];}
                else if(kind=='e'){r=235;g=229;b=213;}
                else if(kind=='i'){r=46;g=49;b=53;}
                else if(kind=='c'){r=coat[0];g=coat[1];b=coat[2];}
                else if(kind=='d'){r=coat[0]*3/5;g=coat[1]*3/5;b=coat[2]*3/5;}
                else if(kind=='a'){r=198;g=167;b=94;}
                else if(kind=='m'){r=199;g=188;b=159;}
                else if(kind=='p'){r=55;g=64;b=74;}
                int gesture=talking&&kind=='s'&&py>=10&&py<=14&&
                            (px<4||px>=12)?(px<4?-1:1):0;
                ggExpRect(out,x+(px-8+gesture)*scale,
                          feet-(24-py)*scale,(end-px)*scale,scale,
                          r,g,b);
            }
            px=end;
        }
    }
}
// World-linked story props. A wagon by the ford, watch standards, records
// in the archive, and lamps or provisions in living towns. These are
// small screen-space cel arrangements PROJECTED from actual ground tiles,
// never screen-anchored stickers or a second 3D asset renderer.
enum { GG_FILM_PROP_CART,GG_FILM_PROP_BANNER,GG_FILM_PROP_RECORDS,
       GG_FILM_PROP_CRATES,GG_FILM_PROP_LANTERN };
static int ggSagaPropKind(void){
    if(ggSaga.mode==GG_SAGA_INTERLUDE){
        switch(ggSaga.event){
        case GG_AMBIENT_COAST:case GG_AMBIENT_DELIVERY:
        case GG_AMBIENT_MARKET:return GG_FILM_PROP_CART;
        case GG_AMBIENT_ARCHIVE:case GG_AMBIENT_RUIN:
            return GG_FILM_PROP_RECORDS;
        case GG_AMBIENT_SQUAD:case GG_AMBIENT_OATH:
            return GG_FILM_PROP_BANNER;
        case GG_AMBIENT_CLINIC:case GG_AMBIENT_MIRACLE:
            return GG_FILM_PROP_LANTERN;
        default:return GG_FILM_PROP_CRATES;
        }
    }
    switch(ggChron.act){
    case 0:return GG_FILM_PROP_CART;
    case 1:return GG_FILM_PROP_BANNER;
    case 2:return GG_FILM_PROP_RECORDS;
    default:return GG_FILM_PROP_LANTERN;
    }
}
static void ggSagaPropDraw(u8*out,int cx,int feet,int scale,int kind){
    int s=ggExpClamp(scale,1,3);
    int x=cx-9*s,y=feet;
    if(kind==GG_FILM_PROP_CART){
        // Chassis, wagon bed, cargo and wheels: objects visibly belong
        // to the road and remain grounded in the shot.
        ggExpRect(out,x+2*s,y-9*s,15*s,5*s,108,73,47);
        ggExpRect(out,x+4*s,y-13*s,11*s,4*s,171,130,75);
        ggExpRect(out,x+3*s,y-17*s,5*s,4*s,143,102,65);
        ggExpRect(out,x+9*s,y-16*s,5*s,3*s,184,147,96);
        ggExpRect(out,x+4*s,y-4*s,4*s,4*s,42,46,50);
        ggExpRect(out,x+13*s,y-4*s,4*s,4*s,42,46,50);
        ggExpRect(out,x+5*s,y-3*s,2*s,2*s,163,143,104);
        ggExpRect(out,x+14*s,y-3*s,2*s,2*s,163,143,104);
    }else if(kind==GG_FILM_PROP_BANNER){
        ggExpRect(out,cx-s,y-23*s,2*s,23*s,105,101,93);
        ggExpRect(out,cx+s,y-21*s,11*s,12*s,119,66,83);
        ggExpRect(out,cx+3*s,y-18*s,6*s,4*s,224,186,102);
        ggExpRect(out,cx-5*s,y-2*s,10*s,2*s,71,68,70);
    }else if(kind==GG_FILM_PROP_RECORDS){
        ggExpRect(out,x+1*s,y-5*s,17*s,5*s,67,57,53);
        ggExpRect(out,x+4*s,y-8*s,11*s,3*s,179,142,82);
        ggExpRect(out,x+3*s,y-11*s,12*s,3*s,224,208,171);
        ggExpRect(out,x+5*s,y-13*s,11*s,2*s,214,192,149);
        ggExpRect(out,x+5*s,y-10*s,8*s,s,102,82,76);
    }else if(kind==GG_FILM_PROP_CRATES){
        ggExpRect(out,x+2*s,y-10*s,9*s,10*s,136,95,61);
        ggExpRect(out,x+11*s,y-7*s,7*s,7*s,158,112,70);
        ggExpRect(out,x+4*s,y-9*s,2*s,8*s,184,148,94);
        ggExpRect(out,x+12*s,y-6*s,2*s,5*s,184,148,94);
    }else{
        ggExpRect(out,cx-s,y-20*s,2*s,20*s,100,94,87);
        ggExpRect(out,cx-4*s,y-22*s,8*s,9*s,117,100,65);
        ggExpRect(out,cx-3*s,y-21*s,6*s,7*s,237,195,89);
        ggExpRect(out,cx-2*s,y-20*s,4*s,5*s,255,227,153);
        ggExpRect(out,cx-5*s,y-s,10*s,s,79,77,71);
    }
}
static void ggSagaDraw(u8 *out){
    if(!ggSaga.mode)return;
    int index=ggSaga.mode==GG_SAGA_CHOICE?5:
              ggSaga.frame/GG_SAGA_BEAT_TICKS;
    int local=ggSaga.frame%GG_SAGA_BEAT_TICKS;
    int px=ggExplore.px,py=ggExplore.py;
    int angle=ggExplore.angle,frame=ggExplore.frame;
     int stageX,stageY;
    ggSagaCamera(index,local,&stageX,&stageY);
    // The camera/actor/world all receive the SAME small, continuous
    // speaker-follow yaw after staging, so the next character's line
    // feels intentionally filmed rather than a motionless wide shot.
    // Three moving performers and one optional stage prop, all in the
    // existing scene. An interaction cue is resolved once per two-line
    // take, using the actual script, not random animation selection.
    GgFilmStageItem actors[4];
    int actorCount=0,primaryGuest,secondGuest;
    ggSagaPairGuests(index,&primaryGuest,&secondGuest);
    ggSagaResolveSides(stageX,stageY);
    const GgSagaBeat *spoken=ggSaga.mode==GG_SAGA_CHOICE?0:
                            ggSagaCurrentBeat(index);
    int activeStyle=spoken?ggSagaGuestStyle(spoken->voice):-1;
    int propKind=ggSagaPropKind();
    int filmCos=ggExpCos[ggExplore.angle&255];
    int filmSin=ggExpSin[ggExplore.angle&255];
    int guestCount=primaryGuest<0?0:secondGuest<0?1:2;
    int firstSide=guestCount?
                  ggFilmStageGuestSide(ggSagaShot.plusValid,
                                       ggSagaShot.minusValid,0):0;
    int secondSide=guestCount>1?
                   ggFilmStageGuestSide(ggSagaShot.plusValid,
                                        ggSagaShot.minusValid,1):0;
    int speakerSide=activeStyle==primaryGuest?firstSide:
                    activeStyle==secondGuest?secondSide:0;
    ggExplore.angle=(ggExplore.angle+
        ggFilmFocusYaw(local,GG_SAGA_BEAT_TICKS,speakerSide))&255;
    filmCos=ggExpCos[ggExplore.angle&255];
    filmSin=ggExpSin[ggExplore.angle&255];
    // Only the live world raycaster paints cinematic architecture;
    // no second floating facade decal layer or separate perspective.
    ggExpRenderMode(out,1);
    if(!ggSagaShot.pairResolved){
        ggSagaShot.pairKind=GG_FILM_PAIR_NONE;
        ggSagaShot.pairSourceHero=0;
        if(spoken&&guestCount==1&&firstSide&&ggOrigin.complete){
            int begin=index&~1,count=ggSagaBeatCount();
            const GgSagaBeat *a=ggSagaCurrentBeat(begin);
            const GgSagaBeat *b=begin+1<count?
                               ggSagaCurrentBeat(begin+1):0;
            int cue=ggFilmPairCue(a->line,b?b->line:"");
            if(cue){
                // Rehearse peak contact against the ACTUAL film camera and
                // world projection ONCE at shot setup. A distant guest,
                // cliff-height mismatch or narrow sightline should not
                // make the actor reach through space for a prop.
                int gx,gy,hwx,hwy,gwx,gwy;
                ggSagaSidePosition(firstSide,&gx,&gy);
                ggFilmPairPosition(stageX*256+128,stageY*256+128,
                                   gx,gy,96,1,&hwx,&hwy);
                ggFilmPairPosition(stageX*256+128,stageY*256+128,
                                   gx,gy,96,0,&gwx,&gwy);
                int saveX=ggExplore.px,saveY=ggExplore.py;
                int camX,camY;
                ggFilmCameraTrackAt(&ggSagaShot.camera,1,0,
                                    GG_SAGA_BEAT_TICKS,&camX,&camY);
                ggExplore.px=camX;ggExplore.py=camY;
                int hx,hy,hs,gx2,gy2,gs;
                int visibleHero=ggSagaProjectActor(hwx,hwy,&hx,&hy,&hs);
                int visibleGuest=ggSagaProjectActor(gwx,gwy,&gx2,&gy2,&gs);
                ggExplore.px=saveX;ggExplore.py=saveY;
                if(visibleHero&&visibleGuest&&
                   ggFilmPairProjectedContact(hx,hy,
                       hs,gx2,gy2,gs)){
                    ggSagaShot.pairKind=cue;
                    ggSagaShot.pairSourceHero=!strcmp(a->voice,"YOU");
                }
            }
        }
        ggSagaShot.pairResolved=1;
    }
    int pairKind=ggSagaShot.pairKind;
    int pairInset=pairKind?ggFilmPairInset(index,local,
                                          GG_SAGA_BEAT_TICKS):0;
    int guestWX=0,guestWY=0,stageWX=stageX*256+128,
        stageWY=stageY*256+128;
    if(pairKind)ggSagaSidePosition(firstSide,&guestWX,&guestWY);
    if(ggOrigin.complete){
        int x,y,size,wx,wy;
        ggSagaActorPosition(stageX,stageY,index,local,&wx,&wy);
        if(pairKind&&pairInset)
            ggFilmPairPosition(stageWX,stageWY,guestWX,guestWY,
                               pairInset,1,&wx,&wy);
        if(ggSagaProjectActor(wx,wy,&x,&y,&size)){
            actors[actorCount++]=(GgFilmStageItem){
                GG_STAGE_HERO,x,y,size,
                ggFilmStageDepth(wx,wy,ggExplore.px,ggExplore.py,
                                 filmCos,filmSin)
            };
        }
    }
    if(spoken){
        int styles[2]={primaryGuest,secondGuest};
        for(int slot=0;slot<2;slot++){
            if(styles[slot]<0)continue;
            int side=slot?secondSide:firstSide;
            if(!side)continue;
            int wx,wy,x,y,size;
            ggSagaSidePosition(side,&wx,&wy);
            if(pairKind&&slot==0&&pairInset)
                ggFilmPairPosition(stageWX,stageWY,guestWX,guestWY,
                                   pairInset,0,&wx,&wy);
            if(!ggSagaProjectActor(wx,wy,&x,&y,&size))continue;
            actors[actorCount++]=(GgFilmStageItem){
                slot?GG_STAGE_GUEST_TWO:GG_STAGE_GUEST_ONE,
                x,y,size,
                ggFilmStageDepth(wx,wy,ggExplore.px,ggExplore.py,
                                 filmCos,filmSin)
            };
        }
    }
    // Handovers supersede a duplicate scenery book or supply pile.
    int propSide=(pairKind==GG_FILM_PAIR_BOOK||
                  pairKind==GG_FILM_PAIR_PARCEL)?0:
                 ggFilmStagePropSide(ggSagaShot.plusValid,
                                     ggSagaShot.minusValid,guestCount);
    if(propSide){
        int wx,wy,x,y,size;
        ggSagaSidePosition(propSide,&wx,&wy);
        if(ggSagaProjectActor(wx,wy,&x,&y,&size))
            actors[actorCount++]=(GgFilmStageItem){
                GG_STAGE_PROP,x,y,size,
                ggFilmStageDepth(wx,wy,ggExplore.px,ggExplore.py,
                                 filmCos,filmSin)
            };
    }
    // Apply one world-sampled climate light value to all performers in
    // this take. It shares the environment's material grade; no new
    // per-pixel normal map or frame buffer, and never changes identity.
    int cameraTerrain=world[ggExplore.py>>8][ggExplore.px>>8];
    ggFilmArtAmbient=ggFilmScenicActorLight(cameraTerrain);
    // Painter's algorithm: distant actors are drawn before near ones.
    // Prevents a character behind the hero from covering the hero's
    // shoulders, or a wagon appearing pasted across someone's legs.
    ggFilmStageSort(actors,actorCount);
    const GgFilmStageItem *heroItem=0,*partnerItem=0;
    for(int i=0;i<actorCount;i++){
        if(actors[i].kind==GG_STAGE_HERO)heroItem=&actors[i];
        if(actors[i].kind==GG_STAGE_GUEST_ONE)partnerItem=&actors[i];
    }
    int pairVisible=pairKind&&heroItem&&partnerItem;
    int heroHandX=0,heroHandY=0,guestHandX=0,guestHandY=0;
    for(int i=0;i<actorCount;i++){
        const GgFilmStageItem *item=&actors[i];
        // Hardware-style depth rejection only for the foreground actor
        // primitives, using raycast wall spans already rendered this frame.
        ggExpFilmDrawDepth=item->depth;
        if(item->kind==GG_STAGE_PROP){
            ggSagaPropDraw(out,item->x,item->feet,item->size,propKind);
        }else if(item->kind==GG_STAGE_HERO){
            int walking=ggSaga.mode!=GG_SAGA_CHOICE&&
                        !(index&1)&&local<GG_SAGA_BEAT_TICKS/2;
            GgFilmPose performance=ggFilmPoseFor(spoken?spoken->line:"",
                      index,local,1,
                      spoken&&!strcmp(spoken->voice,"YOU"),walking,propKind);
            if(spoken&&!strcmp(spoken->voice,"NARRATOR")&&!pairVisible){
                int lift=ggFilmLifeNarratorCue(index,local,1);
                if(performance.armR<lift)performance.armR=(signed char)lift;
                performance.head=(signed char)(performance.head+
                                    ggFilmLifeNarratorNod(index,local,1));
                // Keep the lips still: the invisible narrator speaks.
            }
            performance.gaze=(signed char)ggFilmStageGaze(0,
                                  speakerSide?speakerSide:firstSide);
            performance.turn=(signed char)ggFilmFigureTurn(
                local,GG_SAGA_BEAT_TICKS,
                spoken&&!strcmp(spoken->voice,"YOU")?0:performance.gaze);
            if(pairVisible){
                int toward=ggFilmPairDirection(item->x,partnerItem->x);
                ggFilmPairActPose(&performance,toward,pairInset);
                ggFilmPairHandAt(item->x,item->feet,item->size,
                                 partnerItem->x,&performance,
                                 &heroHandX,&heroHandY);
                // The object is owned by one partner, not both cels.
                if(pairKind>=GG_FILM_PAIR_BOOK)performance.holdProp=0;
            }
            ggFilmDrawActor(out,item->x,item->feet,item->size,
                            ggOriginHairRGB[ggOrigin.hair],
                            ggOriginSkinRGB[ggOrigin.skin],
                            ggOriginClothesRGB[ggOrigin.garment],
                            &performance);
        }else{
            int style=item->kind==GG_STAGE_GUEST_ONE?
                      primaryGuest:secondGuest;
            GgFilmPose performance=ggFilmPoseFor(spoken->line,index,local,
                                                 0,activeStyle==style,
                                                 0,propKind);
            if(!strcmp(spoken->voice,"NARRATOR")&&!pairVisible){
                int slot=item->kind==GG_STAGE_GUEST_ONE?0:1;
                int lift=ggFilmLifeNarratorCue(index+slot,local,0);
                if(performance.armL<lift)performance.armL=(signed char)lift;
                performance.head=(signed char)(performance.head+
                                  ggFilmLifeNarratorNod(index+slot,local,0));
            }
            int ownSide=item->kind==GG_STAGE_GUEST_ONE?
                        firstSide:secondSide;
            int lookSide=activeStyle==style?0:speakerSide;
            performance.gaze=(signed char)ggFilmStageGaze(ownSide,lookSide);
            performance.turn=(signed char)ggFilmFigureTurn(
                local,GG_SAGA_BEAT_TICKS,performance.gaze);
            if(pairVisible&&item->kind==GG_STAGE_GUEST_ONE){
                int toward=ggFilmPairDirection(item->x,heroItem->x);
                ggFilmPairActPose(&performance,toward,pairInset);
                ggFilmPairHandAt(item->x,item->feet,item->size,
                                 heroItem->x,&performance,
                                 &guestHandX,&guestHandY);
                if(pairKind>=GG_FILM_PAIR_BOOK)performance.holdProp=0;
            }
            ggFilmDrawGuestActor(out,item->x,item->feet,item->size,
                                 ggFilmGuestPalette[style][0],
                                 ggFilmGuestPalette[style][1],
                                 ggFilmGuestPalette[style][2],
                                 &performance,style);
        }
    }
    ggExpFilmDrawDepth=0;
    if(pairVisible){
        ggExpFilmDrawDepth=(heroItem->depth+partnerItem->depth)/2;
        int touching=ggFilmPairTouch(heroHandX,heroHandY,
                                    guestHandX,guestHandY);
        if(pairKind==GG_FILM_PAIR_COMFORT&&pairInset>=80&&touching){
            // An actual shared contact point, in front of BOTH sleeves.
            int cx=(heroHandX+guestHandX)/2;
            int cy=(heroHandY+guestHandY)/2;
            const u8 *a=ggOriginSkinRGB[ggOrigin.skin];
            const u8 *b=ggFilmGuestPalette[primaryGuest][1];
            ggExpRect(out,cx-3,cy-1,3,3,a[0],a[1],a[2]);
            ggExpRect(out,cx,cy-1,3,3,b[0],b[1],b[2]);
        }else if(pairKind==GG_FILM_PAIR_BOOK||
                 pairKind==GG_FILM_PAIR_PARCEL){
            // Rehearsed contact guarantees a physically attainable
            // handoff. Possession is based on the dialogue beat, not
            // whether a particular frame happened to render. Skipping,
            // pausing or replaying cannot strand or duplicate a book.
            int owner=ggFilmPairOwnerAt(pairKind,index,local,
                                        GG_SAGA_BEAT_TICKS);
            int holderHero=ggSagaShot.pairSourceHero!=owner;
            int hx=holderHero?heroHandX:guestHandX;
            int hy=holderHero?heroHandY:guestHandY;
            int size=holderHero?heroItem->size:partnerItem->size;
            const u8 *skin=holderHero?
                 ggOriginSkinRGB[ggOrigin.skin]:
                 ggFilmGuestPalette[primaryGuest][1];
            ggFilmDrawHeldObject(out,hx,hy,size,
                    pairKind==GG_FILM_PAIR_BOOK?3:4,skin);
        }
    }
    ggExpFilmDrawDepth=0;
    ggFilmArtAmbient=256;
    // World-anchored foreground foliage, poles and market clutter can
    // properly cover an actor only when PHYSICALLY nearer than the cast.
    int closestDepth=0;
    for(int i=0;i<actorCount;i++)
        if(actors[i].depth>0&&
           (!closestDepth||actors[i].depth<closestDepth))
            closestDepth=actors[i].depth;
    if(closestDepth)ggExpScenicDraw(out,1,closestDepth);
    ggExplore.px=px;ggExplore.py=py;
    ggExplore.angle=angle;ggExplore.frame=frame;
    // A single designed film language: the same restrained brass,
    // blue-slate values and inset hierarchy as the playable exploration
    // screen. Keep landscape readable and avoid a bulky retro debug HUD.
    ggExpRect(out,0,0,400,23,13,24,35);
    ggExpRect(out,0,22,400,1,174,137,85);
    ggExpRect(out,0,0,3,23,184,148,91);
    ggExpRect(out,0,188,400,52,12,22,33);
    ggExpRect(out,0,187,400,1,174,137,85);
    ggExpRect(out,0,190,3,48,174,124,80);
    ggExpRect(out,386,191,5,1,119,101,81);
    ggExpRect(out,386,193,5,1,119,101,81);
    char line[110];
    if(ggSaga.mode==GG_SAGA_CHOICE){
        snprintf(line,sizeof line,"ACT %d - YOUR CHOICE CHANGES THE REALM",
                 ggChron.act+1);
        ggFilmText(out,9,11,line,248,218,159);
        ggFilmText(out,10,54,"THE LAST STANDARD FALLS. WHAT DO YOU DO?",
                  233,221,184);
        for(int i=0;i<2;i++){
            int y=85+i*39;
            ggExpRect(out,17,y,366,31,
                      i==ggSaga.choice?56:25,
                      i==ggSaga.choice?65:38,52);
            ggFilmText(out,26,y+10,
                      ggSagaDecision[ggChron.act][i],
                      i==ggSaga.choice?255:191,
                      i==ggSaga.choice?229:208,162);
        }
        ggFilmText(out,10,198,"CHOOSE WITH THE PAD. PRESS A TO COMMIT.",
                  241,225,188);
        ggFilmText(out,10,215,"START RETURNS TO 2D WITHOUT RECORDING A CHOICE.",
                  191,210,219);
        return;
    }
    const GgSagaBeat *beat=ggSagaCurrentBeat(index);
    // Exactly ONE title: the old chapter AND ending banners repeated
    // status information, pushed the landscape down and looked like HUD.
    if(ggSaga.mode==GG_SAGA_INTERLUDE)
        snprintf(line,sizeof line,"WORLD STORY: %.44s",
                 ggSagaAmbientTitles[ggSaga.event]);
    else if(ggSaga.ending>=0)
        snprintf(line,sizeof line,"ENDING %d/9: %.45s",
                 ggSaga.ending+1,ggSagaEndNames[ggSaga.ending]);
    else snprintf(line,sizeof line,"ACT %d: %.47s",
                  ggChron.act+1,ggChronActName[ggChron.act]);
    ggFilmText(out,11,9,line,238,226,190);
    snprintf(line,sizeof line,"%s:",beat->voice);
    // One small actor plate, a primary legible line and a quiet reply.
    // Semantic structure takes precedence over repeated status banners.
    ggExpRect(out,10,191,100,14,27,40,50);
    ggExpRect(out,10,191,3,14,188,154,96);
    ggFilmText(out,19,195,line,243,217,162);
    ggFilmText(out,12,207,beat->line,235,238,233);
    ggFilmText(out,12,221,beat->sub,168,188,198);
    // Only show help during the take's opening or while paused.
    // A permanent console control strip over the scenery made even
    // dramatic shots look like static debug screenshots.
    if(ggSaga.paused||(local<21&&!(index&1))){
        ggExpRect(out,269,175,126,11,20,29,38);
        ggFilmText(out,273,177,ggSaga.paused?
                  "X RESUME A NEXT":"X PAUSE A NEXT",
                  ggSaga.paused?235:147,169,168);
    }
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
    ggFilmText(out,25,79,line,236,221,176);
    snprintf(line,sizeof line,"FINAL CHOICE: %s",
             !(ggSaga.decided&(1u<<3))?"NOT YET WRITTEN":
             (ggSaga.decisions&(1u<<3))?"PERSONAL RULE":"SHARED FREEDOM");
    ggFilmText(out,25,98,line,195,217,230);
    ggFilmText(out,25,118,"CAREERS, TOWNS, POWER AND OATHS SHAPE THE END.",
              195,210,221);
    ggFilmText(out,25,134,"REPLAY THE FINAL ACT TO SEEK A DIFFERENT FATE.",
              235,192,137);
}
#endif
