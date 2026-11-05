#pragma once
#include "SerialGraph.h"
namespace masterlab {
template<class Getter> void configureModules(SerialGraph& engine,Getter v) noexcept {
  masterlab::StereoSettings s{v(88)>.5f&&v(89)>.5f,v(95)>.5f,v(96)>.5f,v(90),v(91),v(92),v(93),v(94),v(97),v(136),v(137)};for(size_t i=0;i<4;++i){s.bandEnabled[i]=v(139+i*2)>.5f;s.solo[i]=v(140+i*2)>.5f;}s.amount=v(147);s.delta=v(148)>.5f;s.stereoize=v(162);engine.stereoSettings(s);
  engine.dynamicsSettings({v(98)>.5f&&v(99)>.5f,v(100)>.5f,v(101),v(102),v(103),v(104),v(105),v(106),v(107),v(108),v(109),static_cast<int>(v(176)),v(177),v(178)>.5f});
  masterlab::DynamicEqSettings c;c.enabled=v(110)>.5f&&v(111)>.5f;for(size_t i=0;i<3;++i){size_t k=112+i*8;c.bands[i]={v(k),v(k+1),v(k+2),v(k+3),v(k+4),v(k+5),v(k+6),v(k+7)>.5f};}engine.dynamicEqSettings(c);
  engine.stabilizerSettings({v(149)>.5f&&v(150)>.5f,v(151)>.5f,v(159)>.5f,v(160)>.5f,static_cast<int>(v(152)),v(153),v(154),v(155),v(156),v(157),v(158),static_cast<int>(v(179))});
  engine.topology(v(161)>.5f,{v(110)>.5f,v(98)>.5f,v(88)>.5f,v(86)>.5f,v(87)>.5f,v(149)>.5f},{v(111)>.5f,v(99)>.5f,v(89)>.5f,v(4)>.5f,v(5)>.5f,v(150)>.5f});
  std::array<float,6> trims{},mixes{};for(size_t i=0;i<6;++i){trims[i]=v(164+i*2);mixes[i]=v(165+i*2);}engine.parallelSettings(v(163)>.5f,trims,mixes);
  HarmonicExciterSettings h;h.enabled=v(180)>.5f&&v(181)>.5f;for(size_t k=0;k<4;++k)h.amount[k]=v(182+k);h.even=v(186);h.odd=v(187);h.driveDb=v(188);h.mix=v(189);h.outputDb=v(190);h.impact=v(201);h.oversample2x=false;engine.harmonicExciterSettings(h);
  AnalogColorSettings a;a.enabled=v(191)>.5f&&v(192)>.5f;a.mode=static_cast<AnalogColorMode>(std::clamp(static_cast<int>(v(193)),0,3));a.driveDb=v(194);a.bias=v(195);a.tone=v(196);a.dynamics=v(197);a.mix=v(198);a.outputDb=v(199);a.autoGain=v(200)>.5f;a.impact=v(202);engine.analogColorSettings(a);
  BassArchitectSettings b;b.enabled=v(203)>.5f&&v(204)>.5f;b.sub=v(205);b.body=v(206);b.harmonics=v(207);b.monoBass=v(208);b.mix=v(209);b.outputDb=v(210);engine.bassArchitectSettings(b);
  TransientSculptorSettings t;t.enabled=v(211)>.5f&&v(212)>.5f;for(size_t k=0;k<3;++k){t.attack[k]=v(213+k);t.sustain[k]=v(216+k);}t.mix=v(219);t.outputDb=v(220);engine.transientSculptorSettings(t);
  engine.routing(static_cast<int>(v(138)),v(6)>.5f);
}
template<class Getter> void configureGraph(SerialGraph& engine,Getter v) noexcept {
 configureModules(engine,v);engine.limiterParameters(v(52),v(53));
 engine.parameters(v(0),v(1),v(2),v(3),v(4)>.5f&&v(86)>.5f,v(5)>.5f&&v(87)>.5f);
 for(size_t k=1;k<16;++k){size_t i=7+(k-1)*3;engine.setBand(k,v(i),v(i+1),v(i+2));}
 for(size_t k=0;k<16;++k)engine.setBandMode(k,static_cast<FilterType>(static_cast<int>(v(54+k*2))),v(55+k*2)>.5f);
}
}
