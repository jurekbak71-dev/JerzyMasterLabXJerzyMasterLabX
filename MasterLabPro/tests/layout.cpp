#include "RackLayout.h"
#include <cstdlib>
#include <iostream>
int main(){for(auto w:{800,1600,2400})for(auto h:{500,1000,1500}){for(auto r:ml::rackLayout){auto s=ml::scale(r,w,h);if(s.x<0||s.y<0||s.w<=0||s.h<=0||s.x+s.w>w||s.y+s.h>h)return 1;}auto a=ml::scale(ml::rackLayout[3],w,h),b=ml::scale(ml::rackLayout[4],w,h);if(a.x+a.w>b.x||a.y!=b.y||a.h!=b.h)return 2;}
 auto slices=ml::nineSlice({0,0,406,178},{10,20,1248,252},36,30,36,30);
 if(slices[0].source.w!=36||slices[0].source.h!=30||slices[0].target.w!=36||slices[0].target.h!=30)return 3;
 if(slices[4].source.w!=334||slices[4].source.h!=118||slices[4].target.w!=1176||slices[4].target.h!=192)return 4;
 if(slices[8].target.x!=1222||slices[8].target.y!=242||slices[8].target.w!=36||slices[8].target.h!=30)return 5;
 std::cout<<"PASS layout bounds, right meter separation and nine-slice geometry\n";
}
