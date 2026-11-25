#pragma once
#include <algorithm>
#include <array>
namespace ml {
struct Rect {int x,y,w,h;};
struct ImageSlice {Rect source,target;};
// Nominal 1600x1000; both global stereo meters stay on the right.
inline constexpr std::array<Rect,9> rackLayout{{
 {12,10,1576,52},{12,72,1248,252},{12,334,1248,240},
 {1270,72,154,502},{1434,72,154,502},{12,584,1576,146},
 {12,740,1576,210},{12,960,1576,30},{24,772,1552,40}}};
inline constexpr Rect scale(Rect r,int w,int h) noexcept {
 const auto x=r.x*w/1600,y=r.y*h/1000;
 return {x,y,(r.x+r.w)*w/1600-x,(r.y+r.h)*h/1000-y};
}
inline constexpr std::array<ImageSlice,9> nineSlice(Rect source,Rect target,
 int left,int top,int right,int bottom) noexcept {
 left=std::max(0,std::min(left,std::min(source.w,target.w)));
 right=std::max(0,std::min(right,std::min(source.w-left,target.w-left)));
 top=std::max(0,std::min(top,std::min(source.h,target.h)));
 bottom=std::max(0,std::min(bottom,std::min(source.h-top,target.h-top)));
 const int sx[]{source.x,source.x+left,source.x+source.w-right};
 const int sy[]{source.y,source.y+top,source.y+source.h-bottom};
 const int sw[]{left,source.w-left-right,right};
 const int sh[]{top,source.h-top-bottom,bottom};
 const int tx[]{target.x,target.x+left,target.x+target.w-right};
 const int ty[]{target.y,target.y+top,target.y+target.h-bottom};
 const int tw[]{left,target.w-left-right,right};
 const int th[]{top,target.h-top-bottom,bottom};
 std::array<ImageSlice,9> result{};
 for(int y=0;y<3;++y)for(int x=0;x<3;++x)
  result[static_cast<std::size_t>(y*3+x)]={{sx[x],sy[y],sw[x],sh[y]},{tx[x],ty[y],tw[x],th[y]}};
 return result;
}
}
