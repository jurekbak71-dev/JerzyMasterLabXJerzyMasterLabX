#pragma once
#include <array>
#include <cstdint>
#include <algorithm>
namespace ml {
// Zero denotes an empty slot. IDs belong to instances, not positions.
class RackOrder {
public:
 using Id=std::uint64_t;
 const auto& slots() const noexcept { return order; }
 bool insert(std::size_t p,Id id) noexcept {
  if(p>=order.size() || !id || order[p] || std::find(order.begin(),order.end(),id)!=order.end()) return false;
  order[p]=id;return true;
 }
 bool remove(std::size_t p) noexcept {if(p>=order.size()||!order[p])return false;order[p]=0;return true;}
 bool move(std::size_t from,std::size_t to) noexcept {
  if(from>=order.size()||to>=order.size()||!order[from])return false;
  if(from==to)return true;
  if(!order[to]){std::swap(order[from],order[to]);return true;}
  if(from<to)std::rotate(order.begin()+from,order.begin()+from+1,order.begin()+to+1);
  else std::rotate(order.begin()+to,order.begin()+from,order.begin()+from+1);
  return true;
 }
private: std::array<Id,10> order{};
};
}
