#pragma once

// Runs the actual JUCE message loop: timer-driven DSP analysis must reach the editor.
class GuiSmoke final : private juce::Timer {
    juce::AudioProcessor& processor;
    std::unique_ptr<juce::AudioProcessorEditor> editor;
    int iteration=0,phase=0,sample=0,brainWait=0;
    bool success=true;std::vector<float> brainBefore;
    void fail(const char* message){std::cerr<<message<<'\n';success=false;stopTimer();editor.reset();juce::MessageManager::getInstance()->stopDispatchLoop();}
    juce::RangedAudioParameter* parameter(const char* id){for(auto* p:processor.getParameters())if(auto* ranged=dynamic_cast<juce::RangedAudioParameter*>(p))if(ranged->paramID==id)return ranged;return nullptr;}
    float actual(const char* id){auto* p=parameter(id);return p->convertFrom0to1(p->getValue());}
    void open(){editor.reset(processor.createEditor());editor->addToDesktop(0);editor->setVisible(true);phase=0;}
    void feed(int blocks){juce::AudioBuffer<float> buffer(2,256);juce::MidiBuffer midi;for(int block=0;block<blocks;++block){for(int i=0;i<256;++i,++sample){float envelope=.35f+1.65f*std::pow(.5f+.5f*std::sin(sample/48000.f*2.1f),3.f);buffer.setSample(0,i,envelope*.08f*std::sin(juce::MathConstants<float>::twoPi*1000*sample/48000.f)+envelope*.04f*std::sin(juce::MathConstants<float>::twoPi*3333*sample/48000.f));buffer.setSample(1,i,envelope*.08f*std::sin(juce::MathConstants<float>::twoPi*1000*sample/48000.f));}processor.processBlock(buffer,midi);}}
    bool gestures(){
        editor->setSize(1000,700);
        auto* choice=dynamic_cast<juce::ComboBox*>(editor->findChildWithID("eq-band"));
        auto* frequency=dynamic_cast<juce::Slider*>(editor->findChildWithID("eq-frequency"));
        auto* gain=dynamic_cast<juce::Slider*>(editor->findChildWithID("eq-gain"));
        auto* q=dynamic_cast<juce::Slider*>(editor->findChildWithID("eq-q"));
        if(!choice||!frequency||!gain||!q)return false;
        auto* fftChoice=dynamic_cast<juce::ComboBox*>(editor->findChildWithID("fft-size"));if(!fftChoice)return false;
        int latency=processor.getLatencySamples();for(int order:{12,13,14,15}){fftChoice->setSelectedId(order,juce::sendNotificationSync);if(processor.getLatencySamples()!=latency)return false;}fftChoice->setSelectedId(iteration==1?15:(iteration==2?12:13),juce::sendNotificationSync);
        auto* ceiling=dynamic_cast<juce::Slider*>(editor->findChildWithID("limiter-ceiling"));if(!ceiling)return false;ceiling->setValue(-6,juce::sendNotificationSync);
        auto* initialDrive=dynamic_cast<juce::Slider*>(editor->findChildWithID("limiter-drive"));
        auto* initialRelease=dynamic_cast<juce::Slider*>(editor->findChildWithID("limiter-release"));
        if(!initialDrive||!initialRelease)return false;initialDrive->setValue(24,juce::sendNotificationSync);initialRelease->setValue(150,juce::sendNotificationSync);
        float firstGain=actual("gain");choice->setSelectedId(16,juce::sendNotificationSync);
        auto* type=dynamic_cast<juce::ComboBox*>(editor->findChildWithID("eq-type"));
        auto* bandPower=dynamic_cast<juce::Button*>(editor->findChildWithID("eq-band-active"));if(!type||!bandPower)return false;
        for(int kind=0;kind<6;++kind){type->setSelectedId(kind+1,juce::sendNotificationSync);if(actual("bandType16")!=kind||actual("bandType1")!=0)return false;}
        type->setSelectedId(1,juce::sendNotificationSync);
        bandPower->setToggleState(false,juce::sendNotificationSync);if(actual("bandEnabled16")!=0||actual("bandEnabled1")!=1)return false;
        bandPower->setToggleState(true,juce::sendNotificationSync);if(actual("bandEnabled16")!=1)return false;

        frequency->setValue(3333,juce::sendNotificationSync);gain->setValue(-4.5,juce::sendNotificationSync);q->setValue(1.7,juce::sendNotificationSync);
        if(std::abs(actual("frequency16")-3333)>1||std::abs(actual("gain16")+4.5)>.02f||std::abs(actual("q16")-1.7f)>.02f||actual("gain")!=firstGain)return false;
        auto position=[](float hz,float db){return juce::Point<float>(48+700*std::log(hz/20)/std::log(24000.f/20),174+306*.5f-db*306/36);};
        auto start=position(3333,-4.5),end=position(2000,6);auto now=juce::Time::getCurrentTime();auto source=juce::Desktop::getInstance().getMainMouseSource();
        auto event=[&](juce::Point<float> point,bool dragged){return juce::MouseEvent(source,point,juce::ModifierKeys(juce::ModifierKeys::leftButtonModifier),1,0,0,0,0,editor.get(),editor.get(),now,start,now,1,dragged);};
        struct Listener:juce::AudioProcessorParameter::Listener{int begins=0,ends=0,changes=0;void parameterValueChanged(int,float)override{++changes;}void parameterGestureChanged(int,bool starting)override{if(starting)++begins;else++ends;}} listener;
        parameter("frequency16")->addListener(&listener);parameter("gain16")->addListener(&listener);
        editor->mouseDown(event(start,false));editor->mouseDrag(event(end,true));editor->mouseUp(event(end,true));
        parameter("frequency16")->removeListener(&listener);parameter("gain16")->removeListener(&listener);
        if(listener.begins!=2||listener.ends!=2||listener.changes<2||std::abs(actual("frequency16")-2000)>1||std::abs(actual("gain16")-6)>.02f)return false;
        auto* undo=dynamic_cast<juce::Button*>(editor->findChildWithID("undo"));auto* redo=dynamic_cast<juce::Button*>(editor->findChildWithID("redo"));
        if(!undo||!redo)return false;undo->onClick();
        if(std::abs(actual("frequency16")-3333)>1||std::abs(actual("gain16")+4.5)>.02f)return false;
        redo->onClick();if(std::abs(actual("frequency16")-2000)>1||std::abs(actual("gain16")-6)>.02f)return false;
        float oldQ=actual("q16");juce::MouseWheelDetails wheel{};wheel.deltaY=.2f;editor->mouseWheelMove(event(end,false),wheel);
        if(!(actual("q16")>oldQ&&actual("gain")==firstGain))return false;
        auto* brain=dynamic_cast<juce::Button*>(editor->findChildWithID("view-brain"));auto* analyse=dynamic_cast<juce::Button*>(editor->findChildWithID("brain-analyse"));
        if(!brain||!analyse)return false;brain->onClick();
        auto* genre=dynamic_cast<juce::ComboBox*>(editor->findChildWithID("brain-genre"));if(!genre)return false;genre->setSelectedId(5,juce::sendNotificationSync);
        parameter("order")->setValueNotifyingHost(0);
        for(const char* id:{"dynamicEqPresent","dynamicsPresent","stereoPresent"})parameter(id)->setValueNotifyingHost(1);
        // Drop the Imager before Dynamic EQ, then verify Undo restores the route.
        auto* target=dynamic_cast<juce::DragAndDropTarget*>(editor.get());if(!target)return false;
        const float routeBefore=actual("chainOrder");
        juce::Point<float> chainStart(24+(editor->getWidth()-48.f)/5*2.5f,104),chainEnd(70,104);
        auto chainEvent=[&](juce::Point<float> point,bool dragged){return juce::MouseEvent(source,point,juce::ModifierKeys(juce::ModifierKeys::leftButtonModifier),1,0,0,0,0,editor.get(),editor.get(),now,chainStart,now,1,dragged);};
        editor->mouseDown(chainEvent(chainStart,false));editor->mouseDrag(chainEvent(chainEnd,true));editor->mouseUp(chainEvent(chainEnd,true));if(actual("chainOrder")==routeBefore)return false;undo->onClick();if(actual("chainOrder")!=routeBefore)return false;
        auto* routing=dynamic_cast<juce::ComboBox*>(editor->findChildWithID("chain-routing"));if(!routing)return false;routing->setSelectedId(2,juce::sendNotificationSync);if(actual("routingMode")!=1)return false;routing->setSelectedId(1,juce::sendNotificationSync);if(actual("routingMode")!=0)return false;

        parameter("upwardAmount")->setValueNotifyingHost(4.f/18);
        auto set=[&](const char* id,float value){auto* p=parameter(id);p->setValueNotifyingHost(p->convertTo0to1(value));};set("stabilizerPresent",1);set("stabilizerAmount",55);set("stabilizerSpeed",25);set("dynThreshold",-36);set("deqFreq1",1000);set("deqThreshold1",-42);set("deqRange1",-4);set("deqFreq2",3333);set("deqThreshold2",-42);set("deqRange2",-6);

        brainBefore.clear();for(auto* parameter:processor.getParameters())brainBefore.push_back(parameter->getValue());analyse->onClick();
        for(int i=0;i<processor.getParameters().size();++i)if(processor.getParameters()[i]->getValue()!=brainBefore[static_cast<size_t>(i)])return false;
        dynamic_cast<juce::Button*>(editor->findChildWithID("view-eq"))->onClick();return true;
    }
    bool controlsFit(){
        for(int i=0;i<editor->getNumChildComponents();++i){auto* a=editor->getChildComponent(i);if(!a->isVisible())continue;
            if(!editor->getLocalBounds().contains(a->getBounds()))return false;
            for(int j=i+1;j<editor->getNumChildComponents();++j){auto* b=editor->getChildComponent(j);if(b->isVisible()&&a->getBounds().intersects(b->getBounds())){std::cerr<<"Overlapping controls: "<<a->getComponentID()<<" / "<<b->getComponentID()<<" bounds "<<a->getBounds().toString()<<" / "<<b->getBounds().toString()<<'\n';return false;}}
        }
        return true;
    }
    bool capture(){
        for(auto size:{juce::Point<int>(750,550),juce::Point<int>(1000,700),juce::Point<int>(2000,1400)}){
            editor->setSize(size.x,size.y);auto image=editor->createComponentSnapshot(editor->getLocalBounds());
            if(!image.isValid()||image.getWidth()!=size.x||image.getHeight()!=size.y)return false;
            int curvePixels=0;for(int y=174;y<size.y-271;++y)for(int x=48;x<size.x-252;++x){auto colour=image.getPixelAt(x,y);if(std::abs(static_cast<int>(colour.getRed())-181)<14&&std::abs(static_cast<int>(colour.getGreen())-203)<14&&std::abs(static_cast<int>(colour.getBlue())-231)<14)++curvePixels;}
            if(curvePixels<400){std::cerr<<"EQ response missing from real GUI frame: "<<curvePixels<<'\n';return false;}
            if(iteration==0&&size.x==1000){auto output=juce::File::getCurrentWorkingDirectory().getChildFile("build-plugin/gui-smoke.png");output.getParentDirectory().createDirectory();juce::FileOutputStream stream(output);stream.setPosition(0);stream.truncate();if(!juce::PNGImageFormat{}.writeImageToStream(image,stream))return false;}
            if(!controlsFit())return false;
        }
        auto* limiter=dynamic_cast<juce::Button*>(editor->findChildWithID("view-limiter"));
        auto* eq=dynamic_cast<juce::Button*>(editor->findChildWithID("view-eq"));
        if(!limiter||!eq)return false;
        // A click on a chain card selects its workspace without changing order.
        const bool limiterFirst=actual("order")>.5f;
        const float chainCount=actual("stabilizerPresent")>.5f?6.f:5.f;
        auto point=juce::Point<float>(24.f+(editor->getWidth()-48.f)/chainCount*(limiterFirst?chainCount-1.5f:chainCount-.5f),104.f);
        auto now=juce::Time::getCurrentTime();auto source=juce::Desktop::getInstance().getMainMouseSource();
        juce::MouseEvent click(source,point,juce::ModifierKeys(),1,0,0,0,0,editor.get(),editor.get(),now,point,now,1,false);
        editor->mouseUp(click);
        if(editor->findChildWithID("eq-frequency")->isVisible())return false;
        limiter->onClick();
        auto* drive=dynamic_cast<juce::Slider*>(editor->findChildWithID("limiter-drive"));
        auto* release=dynamic_cast<juce::Slider*>(editor->findChildWithID("limiter-release"));
        if(!drive||!release||!drive->isVisible()||!release->isVisible())return false;
        drive->setValue(24,juce::sendNotificationSync);release->setValue(150,juce::sendNotificationSync);
        if(std::abs(actual("limiterDrive")-24)>.01f||std::abs(actual("limiterRelease")-150)>.11f)return false;
        auto* frequency=editor->findChildWithID("eq-frequency");
        if(!frequency||frequency->isVisible()||!editor->findChildWithID("limiter-ceiling")->isVisible())return false;
        for(auto size:{juce::Point<int>(750,550),juce::Point<int>(1000,700),juce::Point<int>(2000,1400)}){
            editor->setSize(size.x,size.y);auto image=editor->createComponentSnapshot(editor->getLocalBounds());
            int historyPixels=0,reductionPixels=0;for(int y=174;y<size.y-271;++y)for(int x=48;x<size.x-252;++x){auto colour=image.getPixelAt(x,y);if(colour.getRed()>180&&colour.getGreen()>130&&colour.getBlue()<150){++historyPixels;if(y>178)++reductionPixels;}}
            if(!controlsFit())return false;
            if(historyPixels<10||reductionPixels<5){std::cerr<<"Limiter history missing from real GUI frame: "<<historyPixels<<'\n';return false;}
            if(iteration==0&&size.x==1000){auto output=juce::File::getCurrentWorkingDirectory().getChildFile("build-plugin/limiter-smoke.png");juce::FileOutputStream stream(output);stream.setPosition(0);stream.truncate();if(!juce::PNGImageFormat{}.writeImageToStream(image,stream))return false;}
        }
        for(const auto& entry:std::array<std::pair<const char*,const char*>,4>{{{"view-dynamic-eq","dynamic-eq"},{"view-dynamics","dynamics"},{"view-stereo","stereo"},{"view-stabilizer","stabilizer"}}}){
            auto* tab=dynamic_cast<juce::Button*>(editor->findChildWithID(entry.first));if(!tab)return false;tab->onClick();
            for(auto size:{juce::Point<int>(750,550),juce::Point<int>(1000,700),juce::Point<int>(2000,1400)}){editor->setSize(size.x,size.y);if(!controlsFit())return false;auto image=editor->createComponentSnapshot(editor->getLocalBounds());if(!image.isValid())return false;
                if(iteration==0&&size.x==1000){auto output=juce::File::getCurrentWorkingDirectory().getChildFile(juce::String("build-plugin/")+entry.second+"-smoke.png");juce::FileOutputStream stream(output);stream.setPosition(0);stream.truncate();if(!juce::PNGImageFormat{}.writeImageToStream(image,stream))return false;}}
        }
        dynamic_cast<juce::Button*>(editor->findChildWithID("view-stereo"))->onClick();editor->setSize(1000,700);
        auto* stereo=editor->findChildWithID("module-panel-2");if(!stereo)return false;
        const float originalSplit=actual("stereoLowSplit");auto coordinate=[](float hz){return 24+952*std::log(hz/20)/std::log(24000.f/20);};auto begin=juce::Point<float>(coordinate(originalSplit),100),finish=juce::Point<float>(coordinate(200),100);auto when=juce::Time::getCurrentTime();
        auto crossoverEvent=[&](juce::Point<float> point,bool dragged){return juce::MouseEvent(source,point,juce::ModifierKeys(juce::ModifierKeys::leftButtonModifier),1,0,0,0,0,stereo,stereo,when,begin,when,1,dragged);};stereo->mouseDown(crossoverEvent(begin,false));stereo->mouseDrag(crossoverEvent(finish,true));stereo->mouseUp(crossoverEvent(finish,true));if(std::abs(actual("stereoLowSplit")-200)>1)return false;dynamic_cast<juce::Button*>(editor->findChildWithID("undo"))->onClick();if(actual("stereoLowSplit")!=originalSplit)return false;
        auto* inspector=dynamic_cast<juce::Button*>(editor->findChildWithID("view-inspector"));if(!inspector)return false;
        float driveBefore=actual("limiterDrive"),gainBefore=actual("gain16");inspector->onClick();
        if(editor->findChildWithID("limiter-ceiling")->isVisible()||frequency->isVisible())return false;
        for(auto size:{juce::Point<int>(750,550),juce::Point<int>(1000,700),juce::Point<int>(2000,1400)}){
            editor->setSize(size.x,size.y);if(!controlsFit())return false;auto image=editor->createComponentSnapshot(editor->getLocalBounds());if(!image.isValid())return false;
            if(iteration==0&&size.x==1000){auto output=juce::File::getCurrentWorkingDirectory().getChildFile("build-plugin/inspector-smoke.png");juce::FileOutputStream stream(output);stream.setPosition(0);stream.truncate();if(!juce::PNGImageFormat{}.writeImageToStream(image,stream))return false;}
        }
        auto* brain=dynamic_cast<juce::Button*>(editor->findChildWithID("view-brain"));auto* apply=dynamic_cast<juce::Button*>(editor->findChildWithID("brain-apply"));auto* report=dynamic_cast<juce::TextEditor*>(editor->findChildWithID("brain-report"));
        if(!brain||!apply||!report)return false;brain->onClick();
        if(!apply->isEnabled()||!report->getText().contains("PROPOSED EQ")||!report->getText().contains("TONAL ENERGY %")){std::cerr<<report->getText()<<'\n';return false;}
        for(int i=0;i<processor.getParameters().size();++i)if(processor.getParameters()[i]->getValue()!=brainBefore[static_cast<size_t>(i)])return false;
        for(auto size:{juce::Point<int>(750,550),juce::Point<int>(1000,700),juce::Point<int>(2000,1400)}){
            editor->setSize(size.x,size.y);if(!controlsFit())return false;auto image=editor->createComponentSnapshot(editor->getLocalBounds());if(!image.isValid())return false;
            if(iteration==0&&size.x==1000){auto output=juce::File::getCurrentWorkingDirectory().getChildFile("build-plugin/brain-smoke.png");juce::FileOutputStream stream(output);stream.setPosition(0);stream.truncate();if(!juce::PNGImageFormat{}.writeImageToStream(image,stream))return false;}
        }
        apply->onClick();if(apply->isEnabled())return false;dynamic_cast<juce::Button*>(editor->findChildWithID("undo"))->onClick();
        for(int i=0;i<processor.getParameters().size();++i)if(processor.getParameters()[i]->getValue()!=brainBefore[static_cast<size_t>(i)])return false;
        if(actual("limiterDrive")!=driveBefore||actual("gain16")!=gainBefore)return false;
        eq->onClick();if(!frequency->isVisible()||drive->isVisible()||release->isVisible())return false;
        return true;
    }
    void timerCallback()override{
        if(phase==0){if(!gestures()){fail("EQ controls, drag, wheel or host gestures failed");return;}feed(16);brainWait=0;phase=1;return;}
        if(phase==1){feed(16);if(auto* view=dynamic_cast<juce::Button*>(editor->findChildWithID("view-brain")))view->onClick();auto* apply=dynamic_cast<juce::Button*>(editor->findChildWithID("brain-apply"));if(apply&&apply->isEnabled()){if(auto* eq=dynamic_cast<juce::Button*>(editor->findChildWithID("view-eq")))eq->onClick();phase=2;return;}if(++brainWait>500){if(auto* report=dynamic_cast<juce::TextEditor*>(editor->findChildWithID("brain-report")))std::cerr<<report->getText();fail("Brain verification timed out");}return;}
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
