/* Length-delimited UTF-16 keys must remain distinct across hashing and cloning. */
#include "nsHashtable.h"
#include <stdio.h>

static unsigned checks, failures;
static void check(PRBool value, const char* label) {
    ++checks;
    if (!value) { ++failures; fprintf(stderr,"FAIL: %s\n",label); }
}
int main() {
    const PRUnichar first[] = {'a',0,'b',0};
    const PRUnichar second[] = {'a',0,'c',0};
    const PRUnichar prefix[] = {'a',0};
    nsStringKey a(first,3,nsStringKey::NEVER_OWN);
    nsStringKey b(second,3,nsStringKey::NEVER_OWN);
    nsStringKey c(prefix,1,nsStringKey::NEVER_OWN);
    check(!a.Equals(&b),"different suffix before hashing");
    check(!a.Equals(&c),"different length before hashing");
    PRUint32 ah=a.HashCode(),bh=b.HashCode(),ch=c.HashCode();
    check(a.GetStringLength()==3,"first length after hashing");
    check(b.GetStringLength()==3,"second length after hashing");
    check(c.GetStringLength()==1,"prefix length after hashing");
    check(!a.Equals(&b),"different suffix after hashing");
    check(!a.Equals(&c),"different length after hashing");
    check(a.HashCode()==ah && b.HashCode()==bh && c.HashCode()==ch,"stable repeated hashes");
    nsHashKey* clone=a.Clone();
    check(clone && clone->Equals(&a),"cloned key equality");
    check(clone && clone->HashCode()==ah,"cloned key hash");
    check(clone && !clone->Equals(&b),"clone retains suffix");
    nsHashtable table;
    int av=1,bv=2,cv=3;
    table.Put(&a,&av);table.Put(&b,&bv);table.Put(&c,&cv);
    check(table.Count()==3,"distinct stored keys");
    check(table.Get(&a)==&av,"first lookup");
    check(table.Get(&b)==&bv,"second lookup");
    check(table.Get(&c)==&cv,"prefix lookup");
    check(clone && table.Get(clone)==&av,"cloned key lookup");
    check(table.Remove(&b)==&bv,"remove exact key");
    check(table.Count()==2,"remove preserves other keys");
    check(table.Get(&a)==&av && table.Get(&c)==&cv,"surviving keys remain distinct");
    delete clone;
    printf("STRING-KEY-LENGTH checks=%u failures=%u\n",checks,failures);
    return failures ? 1 : 0;
}
