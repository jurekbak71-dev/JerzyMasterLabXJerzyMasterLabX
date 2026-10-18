#pragma once

// Runs the actual JUCE message loop: timer-driven DSP analysis must reach the editor.
class GuiSmoke final : private juce::Timer {
    juce::AudioProcessor& processor;
    std::unique_ptr<juce::AudioProcessorEditor> editor;
    int iteration=0,phase=0,sample=0;
    bool success=true;
    void fail(const char* message){std::cerr<<message<<'\n';success=false;stopTimer();editor.reset();juce::MessageManager::getInstance()->stopDispatchLoop();}
    juce::RangedAudioParameter* parameter(const char* id){for(auto* p:processor.getParameters())if(auto* ranged=dynamic_cast<juce::RangedAudioParameter*>(p))if(ranged->paramID==id)return ranged;return nullptr;}
    float actual(const char* id){auto* p=parameter(id);return p->convertFrom0to1(p->getValue());}
    void open(){editor.reset(processor.createEditor());editor->addToDesktop(0);editor->setVisible(true);phase=0;}
    void feed(int blocks){juce::AudioBuffer<float> buffer(2,256);juce::MidiBuffer midi;for(int block=0;block<blocks;++block){for(int i=0;i<256;++i,++sample){buffer.setSample(0,i,.08f*std::sin(juce::MathConstants<float>::twoPi*1000*sample/48000.f)+.04f*std::sin(juce::MathConstants<float>::twoPi*3333*sample/48000.f));buffer.setSample(1,i,.08f*std::sin(juce::MathConstants<float>::twoPi*1000*sample/48000.f));}processor.processBlock(buffer,midi);}}
    bool gestures(){
        editor->setSize(1000,700);
        auto* choice=dynamic_cast<juce::ComboBox*>(editor->findChildWithID("eq-band"));
        auto* frequency=dynamic_cast<juce::Slider*>(editor->findChildWithID("eq-frequency"));
        auto* gain=dynamic_cast<juce::Slider*>(editor->findChildWithID("eq-gain"));
        auto* q=dynamic_cast<juce::Slider*>(editor->findChildWithID("eq-q"));
        if(!choice||!frequency||!gain||!q)return false;
        auto* fftChoice=dynamic_cast<juce::ComboBox*>(editor->findChildWithID("fft-size"));if(!fftChoice)return false;
        int latency=processor.getLatencySamples();for(int order:{12,13,14,15}){fftChoice->setSelectedId(order,juce::sendNotificationSync);if(processor.getLatencySamples()!=latency)return false;}fftChoice->setSelectedId(iteration==1?15:(iteration==2?12:13),juce::sendNotificationSync);
        float firstGain=actual("gain");choice->setSelectedId(16,juce::sendNotificationSync);
        frequency->setValue(3333,juce::sendNotificationSync);gain->setValue(-4.5,juce::sendNotificationSync);q->setValue(1.7,juce::sendNotificationSync);
        if(std::abs(actual("frequency16")-3333)>1||std::abs(actual("gain16")+4.5)>.02f||std::abs(actual("q16")-1.7f)>.02f||actual("gain")!=firstGain)return false;
        auto position=[](float hz,float db){return juce::Point<float>(48+700*std::log(hz/20)/std::log(24000.f/20),174+255*.5f-db*255/36);};
        auto start=position(3333,-4.5),end=position(2000,6);auto now=juce::Time::getCurrentTime();auto source=juce::Desktop::getInstance().getMainMouseSource();
        auto event=[&](juce::Point<float> point,bool dragged){return juce::MouseEvent(source,point,juce::ModifierKeys(juce::ModifierKeys::leftButtonModifier),1,0,0,0,0,editor.get(),editor.get(),now,start,now,1,dragged);};
        struct Listener:juce::AudioProcessorParameter::Listener{int begins=0,ends=0,changes=0;void parameterValueChanged(int,float)override{++changes;}void parameterGestureChanged(int,bool starting)override{if(starting)++begins;else++ends;}} listener;
        parameter("frequency16")->addListener(&listener);parameter("gain16")->addListener(&listener);
        editor->mouseDown(event(start,false));editor->mouseDrag(event(end,true));editor->mouseUp(event(end,true));
        parameter("frequency16")->removeListener(&listener);parameter("gain16")->removeListener(&listener);
        if(listener.begins!=2||listener.ends!=2||listener.changes<2||std::abs(actual("frequency16")-2000)>1||std::abs(actual("gain16")-6)>.02f)return false;
        float oldQ=actual("q16");juce::MouseWheelDetails wheel{};wheel.deltaY=.2f;editor->mouseWheelMove(event(end,false),wheel);
        return actual("q16")>oldQ&&actual("gain")==firstGain;
    }
    bool capture(){
        for(auto size:{juce::Point<int>(750,550),juce::Point<int>(1000,700),juce::Point<int>(2000,1400)}){
            editor->setSize(size.x,size.y);auto image=editor->createComponentSnapshot(editor->getLocalBounds());
            if(!image.isValid()||image.getWidth()!=size.x||image.getHeight()!=size.y)return false;
            int curvePixels=0;for(int y=174;y<size.y-271;++y)for(int x=48;x<size.x-252;++x){auto colour=image.getPixelAt(x,y);if(colour.getRed()>180&&colour.getGreen()>130&&colour.getBlue()<150)++curvePixels;}
            if(curvePixels<400){std::cerr<<"EQ response missing from real GUI frame: "<<curvePixels<<'\n';return false;}
            if(iteration==0&&size.x==1000){auto output=juce::File::getCurrentWorkingDirectory().getChildFile("build-plugin/gui-smoke.png");output.getParentDirectory().createDirectory();juce::FileOutputStream stream(output);stream.setPosition(0);stream.truncate();if(!juce::PNGImageFormat{}.writeImageToStream(image,stream))return false;}
            for(int i=0;i<editor->getNumChildComponents();++i){auto* child=editor->getChildComponent(i);if(child->isVisible()&&!editor->getLocalBounds().contains(child->getBounds()))return false;}
        }
        return true;
    }
    void timerCallback()override{
        if(phase==0){if(!gestures()){fail("EQ controls, drag, wheel or host gestures failed");return;}feed(64);phase=1;return;}
        if(phase==1){feed(1);phase=2;return;}
        if(!capture()){fail("GUI resize or measured EQ response rendering failed");return;}
        editor->setVisible(false);editor->removeFromDesktop();editor.reset();
        if(++iteration<3){open();return;}
        stopTimer();juce::MessageManager::getInstance()->stopDispatchLoop();
    }
public:
    explicit GuiSmoke(juce::AudioProcessor& p):processor(p){open();startTimer(70);}
    ~GuiSmoke()override{stopTimer();}
    bool passed()const{return success&&iteration==3;}
};
