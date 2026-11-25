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
 void paint(juce::Graphics& g) override {auto id=owner.order.slots()[pos];auto r=getLocalBounds().toFloat().reduced(2);owner.paintSkin(g,r);g.setColour(target?juce::Colours::lime:owner.selected==id&&id?juce::Colours::cyan:hover?juce::Colour(0xff83b9d0):juce::Colour(0xff68717b));g.drawRoundedRectangle(r,5,2);g.setColour(juce::Colours::white);g.drawText(juce::String(pos+1)+"  "+(id&&owner.label?owner.label(id):juce::String(id?"Module":"EMPTY")),getLocalBounds().reduced(8),juce::Justification::centred);}
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
 void resized()override{for(int i=0;i<10;++i){int x=i%5,y=i/5;tiles[i]->setBounds(x*getWidth()/5,y*getHeight()/2,(x+1)*getWidth()/5-x*getWidth()/5,(y+1)*getHeight()/2-y*getHeight()/2);}}
private:juce::Image slotSkin;const RackOrder& order;RackOrder::Id selected=0;std::array<std::unique_ptr<Tile>,10> tiles;
};}
