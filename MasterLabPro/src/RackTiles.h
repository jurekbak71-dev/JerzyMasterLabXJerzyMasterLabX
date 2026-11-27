#pragma once
#include "RackOrder.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>
#include <memory>
namespace ml {
// GUI only: controller must validate and commit real DSP graph separately.
class RackTiles final:public juce::Component,public juce::DragAndDropContainer {
 struct Tile final:juce::Component,juce::DragAndDropTarget {
 RackTiles& owner;int pos;bool target=false;bool hover=false;
 Tile(RackTiles& o,int p):owner(o),pos(p){}
 void paint(juce::Graphics& g) override {
  const auto id=owner.order.slots()[pos];auto r=getLocalBounds().toFloat().reduced(3,2);
  if(r.getWidth()<12||r.getHeight()<12)return;
  const bool chosen=id&&owner.selected==id;
  const auto accent=target?juce::Colour(0xff71ee78):chosen?juce::Colour(0xff35ddff):juce::Colour(0xff52616c);
  g.setColour(juce::Colour(0xff05090d));g.fillRoundedRectangle(r,5);
  auto inner=r.reduced(2);
  g.setGradientFill({juce::Colour(id?0xff273640:0xff1d242a),inner.getTopLeft(),juce::Colour(0xff080e13),inner.getBottomLeft(),false});g.fillRoundedRectangle(inner,4);
  g.setColour(accent.withAlpha(chosen||target?1.f:.7f));g.drawRoundedRectangle(r,5,chosen||target?2.f:1.f);
  if(chosen){g.setColour(accent.withAlpha(.22f));g.drawRoundedRectangle(inner,4,3);}
  auto content=getLocalBounds().reduced(8,5);auto head=content.removeFromTop(std::min(20,content.getHeight()/3));
  auto number=head.removeFromLeft(std::min(23,head.getWidth()));g.setColour(juce::Colour(0xff080d11));g.fillRoundedRectangle(number.toFloat(),3);g.setColour(juce::Colour(0xffa4b5c0));g.setFont(juce::FontOptions(11.f,juce::Font::bold));g.drawFittedText(juce::String(pos+1),number,juce::Justification::centred,1);
  const auto name=id&&owner.label?owner.label(id):juce::String(id?"Module":"EMPTY");
  auto nameArea=content.removeFromBottom(std::min(30,content.getHeight()/2));
  g.setColour(id?juce::Colour(0xffd5e6ed):juce::Colour(0xff77838c));g.setFont(juce::FontOptions(12.f,juce::Font::bold));g.drawFittedText(name,nameArea,juce::Justification::centred,2,.7f);
  if(id){g.setColour(chosen?accent:juce::Colour(0xff8ea5b4));g.setFont(juce::FontOptions(18.f,juce::Font::bold));g.drawFittedText(name.substring(0,2).toUpperCase(),content,juce::Justification::centred,1,.7f);}
  else{g.setColour(juce::Colour(0xff35434c));const auto c=content.getCentre();g.drawLine(float(c.x-6),float(c.y),float(c.x+6),float(c.y),1.4f);g.drawLine(float(c.x),float(c.y-6),float(c.x),float(c.y+6),1.4f);}
 }
 void mouseEnter(const juce::MouseEvent&) override {hover=true;repaint();}
 void mouseExit(const juce::MouseEvent&) override {hover=false;repaint();}
 void mouseDown(const juce::MouseEvent& e) override {if(!e.mods.isLeftButtonDown())return;owner.selected=owner.order.slots()[pos];if(owner.onSelect)owner.onSelect(owner.selected,pos);owner.repaintTiles();}
 void mouseDrag(const juce::MouseEvent& e) override {if(e.mods.isLeftButtonDown()&&e.getDistanceFromDragStart()>5&&owner.order.slots()[pos]&&!owner.isDragAndDropActive())owner.startDragging(juce::var(pos),this);}
 bool isInterestedInDragSource(const SourceDetails& d) override {auto* p=d.sourceComponent.get();return p&&p->getParentComponent()==&owner&&d.description.isInt()&&int(d.description)>=0&&int(d.description)<10;}
 void itemDragEnter(const SourceDetails&)override{target=true;repaint();}void itemDragExit(const SourceDetails&)override{target=false;repaint();}
 void itemDropped(const SourceDetails& d)override{target=false;if(isInterestedInDragSource(d)&&owner.onMove)owner.onMove(int(d.description),pos);owner.repaintTiles();}
 };
public:
 explicit RackTiles(const RackOrder& model):order(model){for(int i=0;i<10;++i){tiles[i]=std::make_unique<Tile>(*this,i);addAndMakeVisible(*tiles[i]);}}
 std::function<void(int,int)> onMove;std::function<void(RackOrder::Id,int)> onSelect;std::function<juce::String(RackOrder::Id)> label;
 void setSelection(RackOrder::Id id){selected=id;repaintTiles();}
 void setSkin(juce::Image skin){slotSkin=std::move(skin);repaintTiles();}
 void paintSkin(juce::Graphics& g,juce::Rectangle<float> r){
  // Blank areas from approved slot_empty.png; never paint its baked label.
  if(slotSkin.isValid()&&slotSkin.getWidth()==356&&slotSkin.getHeight()==58&&r.getWidth()>48){
   const float edge=juce::jmin(r.getWidth()/4.f,r.getHeight()*26.f/58.f);
   auto left=r.removeFromLeft(edge),right=r.removeFromRight(edge);
   g.drawImage(slotSkin.getClippedImage({0,0,26,58}),left);
   g.drawImage(slotSkin.getClippedImage({30,0,90,58}),r);
   g.drawImage(slotSkin.getClippedImage({330,0,26,58}),right);
  }else{g.setColour(juce::Colour(0xff20252b));g.fillRoundedRectangle(r,5);}
 }
 void repaintTiles(){for(auto& t:tiles)t->repaint();}
 void resized()override{for(int i=0;i<10;++i){const int x=i*getWidth()/10,next=(i+1)*getWidth()/10;tiles[i]->setBounds(x,0,next-x,getHeight());}}
private:juce::Image slotSkin;const RackOrder& order;RackOrder::Id selected=0;std::array<std::unique_ptr<Tile>,10> tiles;
};}
