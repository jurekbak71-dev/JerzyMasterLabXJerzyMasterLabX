#include "RackLayout.h"
#include <cstdlib>
#include <iostream>
int main(){for(auto w:{800,1600,2400})for(auto h:{500,1000,1500}){for(auto r:ml::rackLayout){auto s=ml::scale(r,w,h);if(s.x<0||s.y<0||s.w<=0||s.h<=0||s.x+s.w>w||s.y+s.h>h)return 1;}auto a=ml::scale(ml::rackLayout[3],w,h),b=ml::scale(ml::rackLayout[4],w,h);if(a.x+a.w>b.x||a.y!=b.y||a.h!=b.h)return 2;}std::cout<<"PASS layout bounds and right meter separation\n";}
