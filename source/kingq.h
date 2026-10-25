// kingq.h - EP6 THE LAND BOREFORE TIME, Phase 5 step c: the numbers the REALM hands the MISSIONS. PURE C, no includes, no dependencies (expand.h, kingtask.h and the PC tests all use it).
// While a mission of the pack is on, KINGDOM MODE's play loop fills EXK_N numbers from the open realm (kingtask.h: ktFeed) about once a second and the mission's tasks are checked against them
// (expand.h: exKingTick). A task that is met stays met (exTk's bits are saved), so it does not matter that the realm is closed again afterwards.
// The kinds start at 100, well above story.h's and expand.h's, so nothing here depends on their enums. [ASSUMED values and names, tune freely]
//   QK_KBLD  n          n buildings BUILT (a site does not count; the crowned castle does)
//   QK_KBT   a, n       n buildings of type a built (kingsim.h's KB_ numbers)
//   QK_KPOP  n          n Nubbins in the realm
//   QK_KWORK n          n Nubbins at work in a gatherer or a processor
//   QK_KGOOD a, n       n of good a (kingsim.h's G_ numbers) that EXIST: the stockpile, the buildings and the carriers (a good used up at once is not seen)
//   QK_KCASTLE n        the castle's stages done: 1 foundation, 2 walls, 3 towers, 4 crowned
//   QK_KROAD n          n road cells laid
//   QK_KBRIDGE n        n bridge cells laid over water (step 5d)
//   QK_KDELIV n         n loads the Nubbins have delivered since this realm was opened (not saved with the realm)
#ifndef KINGQ_H
#define KINGQ_H
#define QK_KBLD    100
#define QK_KBT     101
#define QK_KPOP    102
#define QK_KWORK   103
#define QK_KGOOD   104
#define QK_KCASTLE 105
#define QK_KROAD   106
#define QK_KDELIV  107
#define QK_KBRIDGE 108
#define EXK_BUILT  0
#define EXK_POP    1
#define EXK_EMP    2
#define EXK_CASTLE 3
#define EXK_ROAD   4
#define EXK_DELIV  5
#define EXK_GOOD   6      // 8 goods: 6 to 13
#define EXK_BT     14     // 14 building types: 14 to 27 (step 5g: 12 before the well and the tavern)
#define EXK_NBT    14
#define EXK_BRIDGE 28     // bridge cells
#define EXK_N      29
#define EXK_CAP    9999   // no number is above this (a task's n is never more)
static inline int exkIs(int kind){ return kind>=QK_KBLD&&kind<=QK_KBRIDGE; }
// the number a kingdom task counts, from the realm's numbers v[EXK_N]; -1 if the kind is not one of these. a is the task's second field.
static inline int exkVal(int kind,int a,const int*v){
    switch(kind){
    case QK_KBLD:    return v[EXK_BUILT];
    case QK_KBT:     return (a>=0&&a<EXK_NBT)?v[EXK_BT+a]:0;
    case QK_KPOP:    return v[EXK_POP];
    case QK_KWORK:   return v[EXK_EMP];
    case QK_KGOOD:   return (a>=0&&a<8)?v[EXK_GOOD+a]:0;
    case QK_KCASTLE: return v[EXK_CASTLE];
    case QK_KROAD:   return v[EXK_ROAD];
    case QK_KDELIV:  return v[EXK_DELIV];
    case QK_KBRIDGE: return v[EXK_BRIDGE];
    }
    return -1;
}
#endif
