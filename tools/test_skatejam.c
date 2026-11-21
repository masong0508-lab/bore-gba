#include <assert.h>
#include <stdio.h>
#include "../source/skatejamrules.h"
int main(void){
    assert(sjDayTag(0)==1&&sjDayTag(126)==127&&sjDayTag(127)==1);
    assert(!sjQualified(0,5,100,10000));
    unsigned m=sjRecord(0,3,0);
    assert(sjAlreadyToday(m,3)&&!sjAlreadyToday(m,4));
    assert(!sjQualified(m,5,100,10000));
    unsigned w=sjRecord(m,4,1);
    assert((w&128u)&&sjAlreadyToday(w,4));
    assert(sjQualified(w,1,5,250));
    assert(!sjQualified(w,1,4,250));
    assert(!sjQualified(w,1,5,249));
    assert((sjRecord(w,5,0)&128u)!=0); // winning remains unlocked
    assert(sjPrize(0)==0&&sjPrize(1)==120&&sjPrize(2)==300&&sjPrize(3)==600);
    assert(sjMedal(0)==0&&sjMedal(999)==0);
    assert(sjMedal(1000)==1&&sjMedal(2999)==1);
    assert(sjMedal(3000)==2&&sjMedal(5999)==2);
    assert(sjMedal(6000)==3&&sjMedal(1000000)==3);
    puts("BORE skate jam rules: PASS");
    return 0;
}
