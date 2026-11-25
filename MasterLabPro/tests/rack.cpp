#include "RackOrder.h"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <iostream>
int main(){ml::RackOrder r;assert(r.slots().size()==10);assert(r.insert(0,101));assert(r.insert(1,102));assert(r.insert(2,103));assert(!r.insert(3,101));assert(!r.insert(10,9));assert(!r.insert(3,0));assert(!r.move(9,0));assert(r.move(0,2));assert(r.slots()[0]==102&&r.slots()[1]==103&&r.slots()[2]==101);assert(r.move(2,0));assert(r.slots()[0]==101&&r.slots()[1]==102&&r.slots()[2]==103);assert(r.move(0,8));assert(r.slots()[0]==0&&r.slots()[8]==101);assert(r.remove(8));assert(!r.remove(8));assert(!r.move(0,10));std::cout<<"PASS: 10 slots, duplicate/invalid rejection, forward/backward reorder, move to empty, removal\n";}
