#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <cmath>
namespace ml {
// Message-thread component. Parameter must outlive this component.
class RotaryParameterControl final : public juce::Component {
public:
 RotaryParameterControl(juce::RangedAudioParameter& p,float neutralValue,juce::Image metal,juce::Image indicator,juce::Colour band)
 :param(p),neutral(p.convertFrom0to1(p.convertTo0to1(neutralValue))),cap(std::move(metal)),pointer(std::move(indicator)),colour(band),attachment(p,[this](float v){shown=param.convertTo0to1(v);repaint();}) {
 setName(p.getName(128)); attachment.sendInitialUpdate();
 }
 ~RotaryParameterControl() override {finish();}
 void mouseDown(const juce::MouseEvent& e) override {
 if(!isEnabled())return;
 if(e.mods.isRightButtonDown()){finish();attachment.setValueAsCompleteGesture(neutral);return;}
 if(e.mods.isLeftButtonDown()&&!dragging){dragging=true;lastY=e.position.y;dragValue=param.getValue();attachment.beginGesture();}
 }
 void mouseDrag(const juce::MouseEvent& e) override {
 if(!dragging)return;
 auto dy=lastY-e.position.y;lastY=e.position.y;
 dragValue=juce::jlimit(0.0f,1.0f,dragValue+dy*(e.mods.isShiftDown()?0.0005f:0.005f));
 attachment.setValueAsPartOfGesture(param.convertFrom0to1(dragValue));
 }
 void mouseUp(const juce::MouseEvent&) override {finish();}
 void enablementChanged() override {if(!isEnabled())finish();repaint();}
 void paint(juce::Graphics& g) override {
 auto b=getLocalBounds().toFloat();auto label=b.removeFromTop(18);auto value=b.removeFromBottom(20);
 const float side=juce::jmin(b.getWidth(),b.getHeight())*0.82f;
 if(side<=0)return;
 auto r=juce::Rectangle<float>(side,side).withCentre(b.getCentre());
 constexpr float start=-2.35619449f,end=2.35619449f;
 auto arc=[&](float a,float z,juce::Colour c,float width){if(z<=a)return;juce::Path path;auto rad=side*.56f;path.addCentredArc(r.getCentreX(),r.getCentreY(),rad,rad,0,a,z,true);g.setColour(c);g.strokePath(path,juce::PathStrokeType(width));};
 // JUCE angles: clockwise from top.
 const float a=start,z=end;
 arc(a,z,juce::Colour(0xff4a5058),2.0f);if(isEnabled()){arc(a,a+(z-a)*shown,colour.withAlpha(0.10f),10.0f);arc(a,a+(z-a)*shown,colour.withAlpha(0.28f),6.0f);}arc(a,a+(z-a)*shown,isEnabled()?colour:juce::Colours::grey,3.0f);
 g.setOpacity(isEnabled()?1.0f:0.45f);if(cap.isValid())g.drawImage(cap,r);else{g.setColour(juce::Colour(0xff626970));g.fillEllipse(r);}
 if(pointer.isValid()) {auto t=juce::AffineTransform::scale(side/pointer.getWidth(),side/pointer.getHeight()).translated(r.getX(),r.getY()).rotated(-2.35619449f+4.71238898f*shown,r.getCentreX(),r.getCentreY());g.drawImageTransformed(pointer,t);}
 g.setOpacity(1.0f);g.setColour(juce::Colours::whitesmoke);g.setFont(12.0f);g.drawText(param.getName(128),label,juce::Justification::centred,true);g.drawText(param.getText(shown,64),value,juce::Justification::centred,true);
 }
private:
 void finish(){if(dragging){dragging=false;attachment.endGesture();}}
 juce::RangedAudioParameter& param;float neutral;juce::Image cap,pointer;juce::Colour colour;float shown=0,lastY=0,dragValue=0;bool dragging=false;juce::ParameterAttachment attachment;
};
}
