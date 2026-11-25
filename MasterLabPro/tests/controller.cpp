#include "RackController.h"
#include <iostream>
int main(){
 ml::RackController c;
 auto a=c.add(0),b=c.add(1);
 if(!a||!b||a==b||c.add(0)||c.add(10))return 1;
 if(!c.select(a)||!c.setBand(a,2)||!c.setBand(b,6))return 2;
 if(!c.move(0,1)||c.order().slots()[1]!=a||c.selection()!=a||c.band(a)!=2||c.band(b)!=6)return 3;
 if(!c.move(1,9)||c.band(a)!=2||c.setBand(a,8))return 4;
 if(!c.remove(9)||c.selection()||c.band(a)!=-1||c.select(a))return 5;
 auto d=c.add(9);if(!d||d==a||c.band(d)!=7)return 6;
 for(std::size_t i=1;i<9;++i)if(!c.add(i))return 7;
 if(c.add(0)||c.add(9)||c.add(10))return 8;
 std::cout<<"PASS controller state, stable IDs, band recall, capacity\n";
}
