#pragma once
#include <array>
namespace ml {
struct Rect {int x,y,w,h;};
// Nominal 1600x1000; both global stereo meters stay on the right.
inline constexpr std::array<Rect,9> rackLayout{{
 {12,10,1576,52},{12,72,1248,252},{12,334,1248,240},
 {1270,72,154,502},{1434,72,154,502},{12,584,1576,146},
 {12,740,1576,210},{12,960,1576,30},{24,772,1552,40}}};
inline constexpr Rect scale(Rect r,int w,int h) noexcept {
 const auto x=r.x*w/1600,y=r.y*h/1000;
 return {x,y,(r.x+r.w)*w/1600-x,(r.y+r.h)*h/1000-y};
}
}
