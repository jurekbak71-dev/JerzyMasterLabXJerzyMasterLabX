#pragma once
#include "RackOrder.h"
#include <array>
#include <limits>
namespace ml {
// Control-thread configuration only. No audio graph or DSP implied.
class RackController {
public:
 using Id=RackOrder::Id;
 const RackOrder& order()const noexcept{return model;}
 Id add(std::size_t slot) noexcept {
  if(next==std::numeric_limits<Id>::max())return 0;
  if(!model.insert(slot,next))return 0;
  for(auto& s:states)if(!s.id){s={next,7};return next++;}
  model.remove(slot);return 0;
 }
 bool remove(std::size_t slot) noexcept {
  if(slot>=model.slots().size())return false;
  auto id=model.slots()[slot];if(!model.remove(slot))return false;
  if(auto* s=find(id))*s={};
  if(selected==id)selected=0;
  return true;
 }
 bool move(std::size_t from,std::size_t to) noexcept{return model.move(from,to);}
 bool select(Id id) noexcept {if(id&&!find(id))return false;selected=id;return true;}
 Id selection()const noexcept{return selected;}
 bool setBand(Id id,int band) noexcept {auto* s=find(id);if(!s||band<0||band>7)return false;s->band=band;return true;}
 int band(Id id)const noexcept{for(const auto& s:states)if(id&&s.id==id)return s.band;return -1;}
private:
 struct State{Id id=0;int band=7;};
 State* find(Id id) noexcept{if(id)for(auto& s:states)if(s.id==id)return &s;return nullptr;}
 RackOrder model;std::array<State,10> states{};Id next=1,selected=0;
};
}
