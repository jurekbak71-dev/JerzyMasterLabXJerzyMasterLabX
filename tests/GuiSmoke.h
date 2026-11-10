#pragma once
#include "../Source/ParallelPanel.h"
#include "../Source/ModulePanel.h"
#include "../Source/ConceptCOverviewPanel.h"

// Runs the actual JUCE message loop: timer-driven DSP analysis must reach the editor.
class GuiSmoke final : private juce::Timer {
    juce::AudioProcessor& processor;
    std::unique_ptr<juce::AudioProcessorEditor> editor;
    int iteration=0,phase=0,sample=0,brainWait=0;
    bool success=true;std::vector<float> brainBefore;
    void fail(const char* message){std::cerr<<message<<'\n';success=false;stopTimer();editor.reset();juce::MessageManager::getInstance()->stopDispatchLoop();}
    juce::RangedAudioParameter* parameter(const char* id){for(auto* p:processor.getParameters())if(auto* ranged=dynamic_cast<juce::RangedAudioParameter*>(p))if(ranged->paramID==id)return ranged;return nullptr;}
    float actual(const char* id){auto* p=parameter(id);return p->convertFrom0to1(p->getValue());}
    void open(){editor.reset(processor.createEditor());editor->addToDesktop(0);editor->setVisible(true);
        // Legacy EQ interaction regression continues from its real page; console is tested separately.
        if(auto* eq=dynamic_cast<juce::Button*>(editor->findChildWithID("view-eq")))eq->onClick();
        phase=0;}
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
        auto position=[](float hz,float db){return juce::Point<float>(48+700*std::log(hz/20)/std::log(24000.f/20),218+262*.5f-db*262/36);};
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
        // Hybrid Studio: actual Exiter component bindings through APVTS.
        auto* exciter=editor->findChildWithID("harmonic-exciter-panel");
        if(!exciter)return false;
        auto* exciterDrive=dynamic_cast<juce::Slider*>(exciter->findChildWithID("exciterDrive"));
        auto* exciterPresent=dynamic_cast<juce::Button*>(exciter->findChildWithID("exciter-present"));
        if(!exciterDrive||!exciterPresent)return false;
        exciter->setVisible(true);
        exciterDrive->setValue(9.f,juce::sendNotificationSync);
        auto* exciterImpact=dynamic_cast<juce::Slider*>(exciter->findChildWithID("exciterImpact"));if(!exciterImpact)return false;
        exciterImpact->setValue(.7,juce::sendNotificationSync);
        if(std::abs(actual("exciterImpact")-.7f)>.02f)return false;
        exciterPresent->setToggleState(true,juce::sendNotificationSync);
        if(std::abs(actual("exciterDrive")-9.f)>.02f||actual("exciterPresent")!=1.f)return false;
        exciterPresent->setToggleState(false,juce::sendNotificationSync);
        if(actual("exciterPresent")!=0.f)return false;
        exciter->setVisible(false);
        // Final limiter is a real DSP stage with APVTS automation, not decorative UI.
        auto* truePeakPanel=editor->findChildWithID("final-true-peak-panel");
        if(!truePeakPanel)return false;
        auto* finalCeiling=dynamic_cast<juce::Slider*>(truePeakPanel->findChildWithID("truePeakCeiling"));
        auto* finalRelease=dynamic_cast<juce::Slider*>(truePeakPanel->findChildWithID("truePeakRelease"));
        auto* truePeakPresent=dynamic_cast<juce::Button*>(truePeakPanel->findChildWithID("true-peak-present"));
        if(!finalCeiling||!finalRelease||!truePeakPresent)return false;
        truePeakPanel->setVisible(true);
        finalCeiling->setValue(-2.0,juce::sendNotificationSync);
        finalRelease->setValue(200.0,juce::sendNotificationSync);
        if(std::abs(actual("truePeakCeiling")+2.f)>.02f||std::abs(actual("truePeakRelease")-200.f)>.02f)return false;
        truePeakPresent->setToggleState(true,juce::sendNotificationSync);
        if(actual("truePeakPresent")!=1.f)return false;
        truePeakPresent->setToggleState(false,juce::sendNotificationSync);
        truePeakPanel->setVisible(false);
        // Transient Sculptor controls must bind to real host parameters.
        auto* transientPanel=editor->findChildWithID("transient-sculptor-panel");
        if(!transientPanel)return false;
        auto* lowAttack=dynamic_cast<juce::Slider*>(transientPanel->findChildWithID("transientAttackLow"));
        auto* highSustain=dynamic_cast<juce::Slider*>(transientPanel->findChildWithID("transientSustainHigh"));
        auto* transientPresent=dynamic_cast<juce::Button*>(transientPanel->findChildWithID("transient-present"));
        if(!lowAttack||!highSustain||!transientPresent)return false;
        transientPanel->setVisible(true);
        lowAttack->setValue(.8,juce::sendNotificationSync);
        highSustain->setValue(-.6,juce::sendNotificationSync);
        if(std::abs(actual("transientAttackLow")-.8f)>.02f||std::abs(actual("transientSustainHigh")+.6f)>.02f)return false;
        transientPresent->setToggleState(true,juce::sendNotificationSync);
        if(actual("transientPresent")!=1.f)return false;
        transientPresent->setToggleState(false,juce::sendNotificationSync);
        transientPanel->setVisible(false);
        // Bass Architect must expose working APVTS-bound controls, not painted-only widgets.
        auto* bassPanel=editor->findChildWithID("bass-architect-panel");
        if(!bassPanel)return false;
        auto* bassSub=dynamic_cast<juce::Slider*>(bassPanel->findChildWithID("bassSub"));
        auto* bassPresent=dynamic_cast<juce::Button*>(bassPanel->findChildWithID("bass-present"));
        if(!bassSub||!bassPresent||bassPanel->findChildWithID("bassMono"))return false;
        bassPanel->setVisible(true);
        bassSub->setValue(.7,juce::sendNotificationSync);
        if(std::abs(actual("bassSub")-.7f)>.02f||!parameter("bassMono"))return false;
        bassPresent->setToggleState(true,juce::sendNotificationSync);
        if(actual("bassPresent")!=1.f)return false;
        bassPresent->setToggleState(false,juce::sendNotificationSync);
        bassPanel->setVisible(false);
        // Analog Color Hybrid panel exposes genuine APVTS-bound mode and drive.
        auto* analogPanel=editor->findChildWithID("analog-color-panel");
        if(!analogPanel)return false;
        auto* colorDrive=dynamic_cast<juce::Slider*>(analogPanel->findChildWithID("analogDrive"));
        auto* colorMode=dynamic_cast<juce::ComboBox*>(analogPanel->findChildWithID("analog-model"));
        auto* colorPresent=dynamic_cast<juce::Button*>(analogPanel->findChildWithID("analog-present"));
        if(!colorDrive||!colorMode||!colorPresent)return false;
        analogPanel->setVisible(true);
        colorDrive->setValue(13.f,juce::sendNotificationSync);
        auto* analogImpact=dynamic_cast<juce::Slider*>(analogPanel->findChildWithID("analogImpact"));if(!analogImpact)return false;
        analogImpact->setValue(.65,juce::sendNotificationSync);
        if(std::abs(actual("analogImpact")-.65f)>.02f)return false;
        colorMode->setSelectedId(3,juce::sendNotificationSync);
        colorPresent->setToggleState(true,juce::sendNotificationSync);
        if(std::abs(actual("analogDrive")-13.f)>.02f||actual("analogMode")!=2.f||actual("analogPresent")!=1.f)return false;
        colorPresent->setToggleState(false,juce::sendNotificationSync);
        if(actual("analogPresent")!=0.f)return false;
        analogPanel->setVisible(false);
        auto* brain=dynamic_cast<juce::Button*>(editor->findChildWithID("view-brain"));auto* analyse=dynamic_cast<juce::Button*>(editor->findChildWithID("brain-analyse"));
        if(!brain||!analyse)return false;brain->onClick();
        auto* genre=dynamic_cast<juce::ComboBox*>(editor->findChildWithID("brain-genre"));if(!genre)return false;genre->setSelectedId(5,juce::sendNotificationSync);
        parameter("order")->setValueNotifyingHost(0);
        for(const char* id:{"dynamicEqPresent","dynamicsPresent","stereoPresent"})parameter(id)->setValueNotifyingHost(1);
        // Drop the Imager before Dynamic EQ, then verify Undo restores the route.
        auto* target=dynamic_cast<juce::DragAndDropTarget*>(editor.get());if(!target)return false;
        const float routeBefore=actual("fullRouteSlot1");
        juce::Point<float> chainStart(24+(editor->getWidth()-48.f)/5*2.5f,104),chainEnd(70,104);
        auto chainEvent=[&](juce::Point<float> point,bool dragged){return juce::MouseEvent(source,point,juce::ModifierKeys(juce::ModifierKeys::leftButtonModifier),1,0,0,0,0,editor.get(),editor.get(),now,chainStart,now,1,dragged);};
        editor->mouseDown(chainEvent(chainStart,false));editor->mouseDrag(chainEvent(chainEnd,true));editor->mouseUp(chainEvent(chainEnd,true));if(actual("fullRouteSlot1")==routeBefore)return false;undo->onClick();if(actual("fullRouteSlot1")!=routeBefore)return false;
        auto* routing=dynamic_cast<juce::ComboBox*>(editor->findChildWithID("chain-routing"));if(!routing)return false;routing->setSelectedId(2,juce::sendNotificationSync);if(actual("routingMode")!=1)return false;
        auto* parallel=editor->findChildWithID("parallel-panel");if(!parallel||!parallel->isVisible())return false;
        auto* trim=dynamic_cast<juce::Slider*>(parallel->findChildWithID("parallelTrimDynamics"));auto* mix=dynamic_cast<juce::Slider*>(parallel->findChildWithID("parallelMixDynamics"));auto* compensate=dynamic_cast<juce::Button*>(parallel->findChildWithID("parallel-compensated"));if(!trim||!mix||!compensate||!trim->isVisible()||!mix->isVisible())return false;
        trim->setValue(5,juce::sendNotificationSync);mix->setValue(.4,juce::sendNotificationSync);compensate->setToggleState(true,juce::sendNotificationSync);if(std::abs(actual("parallelTrimDynamics")-5)>.01f||std::abs(actual("parallelMixDynamics")-.4f)>.01f||actual("parallelCompensated")!=1)return false;
        auto* concrete=dynamic_cast<Processor*>(&processor);auto* parallelView=dynamic_cast<ParallelPanel*>(parallel);if(!concrete||!parallelView)return false;auto readLatest=[&](Processor::ResponseFrame& frame){int responseStart,responseCount,responseStart2,responseCount2;concrete->responseFifo.prepareToRead(concrete->responseFifo.getNumReady(),responseStart,responseCount,responseStart2,responseCount2);bool found=false;for(int i=0;i<responseCount;++i){frame=concrete->responses[responseStart+i];found=true;}for(int i=0;i<responseCount2;++i){frame=concrete->responses[responseStart2+i];found=true;}concrete->responseFifo.finishedRead(responseCount+responseCount2);return found;};Processor::ResponseFrame latest;readLatest(latest);feed(8);readLatest(latest);feed(24);const bool haveParallelFrame=readLatest(latest);if(!haveParallelFrame||!latest.parallel.active||latest.parallel.branches<2||latest.parallel.rms[3]<=0||!latest.parallel.limiterBypassed){std::cerr<<"parallel frame: have="<<haveParallelFrame<<" active="<<latest.parallel.active<<" branches="<<latest.parallel.branches<<" eqRms="<<latest.parallel.rms[3]<<" limiterWarning="<<latest.parallel.limiterBypassed<<'\n';return false;}parallelView->update(latest);
        for(auto size:{juce::Point<int>(750,550),juce::Point<int>(1000,700),juce::Point<int>(2000,1400)}){editor->setSize(size.x,size.y);if(!controlsFit())return false;auto image=editor->createComponentSnapshot(editor->getLocalBounds());if(!image.isValid())return false;if(iteration==0&&size.x==1000){auto output=juce::File::getCurrentWorkingDirectory().getChildFile("build-plugin/parallel-smoke.png");juce::FileOutputStream stream(output);stream.setPosition(0);stream.truncate();if(!juce::PNGImageFormat{}.writeImageToStream(image,stream))return false;}}
        routing->setSelectedId(1,juce::sendNotificationSync);if(actual("routingMode")!=0||parallel->isVisible())return false;

        parameter("upwardAmount")->setValueNotifyingHost(4.f/18);
        auto set=[&](const char* id,float value){auto* p=parameter(id);p->setValueNotifyingHost(p->convertTo0to1(value));};set("stabilizerPresent",1);set("stabilizerAmount",85);set("stabilizerSpeed",25);set("stabilizerIntensity",2);set("dynThreshold",-36);set("dynamicsIntensity",2);set("dynPunch",70);set("deqFreq1",1000);set("deqThreshold1",-42);set("deqRange1",-4);set("deqFreq2",3333);set("deqThreshold2",-42);set("deqRange2",-6);

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
        if(iteration==0){
            if(editor->findChildWithID("view-stem-brain")||editor->findChildWithID("stem-brain-panel")){
                std::cerr<<"Removed STEMS GUI is still present\n";return false;
            }
            editor->setSize(1000,700);
            auto* console=dynamic_cast<juce::Button*>(editor->findChildWithID("view-console"));
            if(!console){std::cerr<<"Concept C console navigation missing\n";return false;}
            console->onClick();
            auto* overview=dynamic_cast<ConceptCOverviewPanel*>(editor->findChildWithID("concept-c-overview"));
            if(!overview||!overview->isVisible())return false;
            if(overview->connectedControlCount()!=16||!overview->hasMeasuredSpectrum()||!overview->hasMeasuredInputSpectrum()){
                std::cerr<<"Concept C real APVTS knobs or measured FFT missing\n";return false;
            }
            // A visible Concept C rotary must update the existing parameter, then restore it.
            auto* consoleGain=dynamic_cast<juce::Slider*>(overview->findChildWithID("console-gain"));
            if(!consoleGain)return false;
            const double initialGain=consoleGain->getValue();
            const double testGain=initialGain>0.5?initialGain*.63:initialGain+1.;
            consoleGain->setValue(testGain,juce::sendNotificationSync);
            if(std::abs(actual("gain")-static_cast<float>(consoleGain->getValue()))>.02f)
                return false;
            consoleGain->setValue(initialGain,juce::sendNotificationSync);
            editor->setSize(1000,700);
            auto image=editor->createComponentSnapshot(editor->getLocalBounds());
            if(!image.isValid())return false;
            // Assert real right-hand LCD telemetry remains dark: the old
            // white polygon glare made pale readouts completely unreadable.
            const auto lcdRight=image.getPixelAt(947,439);
            if(lcdRight.getRed()>95||lcdRight.getGreen()>105||lcdRight.getBlue()>125){
                std::cerr<<"Concept C LCD display still painted white: "
                         <<int(lcdRight.getRed())<<"/"<<int(lcdRight.getGreen())
                         <<"/"<<int(lcdRight.getBlue())<<"\n";return false;
            }
            int powderPixels=0,bluePixels=0;
            for(int y=140;y<image.getHeight()-10;y+=2)
                for(int x=15;x<image.getWidth()-15;x+=2){
                    auto px=image.getPixelAt(x,y);
                    if(px.getRed()>187&&px.getGreen()>190&&px.getBlue()>190)++powderPixels;
                    if(px.getBlue()>130&&px.getBlue()>px.getRed()+45&&px.getBlue()>px.getGreen()+20)++bluePixels;
                }
            if(powderPixels<8000||bluePixels<100){
                std::cerr<<"Concept C material or blue LED pixels missing (powder="
                         <<powderPixels<<", blue="<<bluePixels<<")\n";return false;
            }
            const auto output=juce::File::getCurrentWorkingDirectory().getChildFile("build-plugin/console-smoke.png");
            output.getParentDirectory().createDirectory();
            juce::FileOutputStream stream(output);stream.setPosition(0);stream.truncate();
            if(!juce::PNGImageFormat{}.writeImageToStream(image,stream))return false;
            // Store actual host-sized console renders for large and minimum supported viewport.
            for(const auto dim:{juce::Point<int>{750,550},juce::Point<int>{1400,1000}}){
                editor->setSize(dim.x,dim.y);
                if(!controlsFit())return false;
                auto visual=editor->createComponentSnapshot(editor->getLocalBounds());
                if(!visual.isValid())return false;
                const auto file=juce::File::getCurrentWorkingDirectory().getChildFile(
                    "build-plugin/console-"+juce::String(dim.x)+"x"+juce::String(dim.y)+".png");
                juce::FileOutputStream out(file);out.truncate();
                if(!juce::PNGImageFormat{}.writeImageToStream(visual,out))return false;
            }
            editor->setSize(1000,700);
            // Nine real OPEN actions must leave the overview and enter existing module pages.
            for(int i=0;i<9;++i){
                auto* button=dynamic_cast<juce::Button*>(overview->findChildWithID("console-open-"+juce::String(i)));
                if(!button)return false;
                button->onClick();
                if(overview->isVisible()){
                    std::cerr<<"Concept C OPEN failed for module "<<i<<"\n";return false;
                }
                console->onClick();
                if(!overview->isVisible())return false;
            }
            auto* eq=dynamic_cast<juce::Button*>(editor->findChildWithID("view-eq"));
            if(!eq)return false;eq->onClick();
        }
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
            if(juce::String(entry.second)=="dynamics"){auto* panel=dynamic_cast<ModulePanel*>(editor->findChildWithID("module-panel-1"));auto* intensity=panel?dynamic_cast<juce::ComboBox*>(panel->findChildWithID("dynamics-intensity")):nullptr;auto* punch=panel?panel->controlForParameter("dynPunch"):nullptr;auto* delta=panel?dynamic_cast<juce::Button*>(panel->findChildWithID("dynamics-delta")):nullptr;if(!intensity||!punch||!delta){std::cerr<<"Dynamics Hybrid Studio controls missing: panel="<<(panel!=nullptr)<<" intensity="<<(intensity!=nullptr)<<" punch="<<(punch!=nullptr)<<" delta="<<(delta!=nullptr)<<'\n';return false;}if(intensity->getSelectedId()!=3||std::abs(punch->getValue()-70)>.01f||actual("dynamicsIntensity")!=2||std::abs(actual("dynPunch")-70)>.01f||actual("dynDelta")!=0){std::cerr<<"Dynamics Hybrid Studio attachment mismatch: intensity="<<actual("dynamicsIntensity")<<" punch="<<actual("dynPunch")<<" delta="<<actual("dynDelta")<<'\n';return false;}feed(24);}
            if(juce::String(entry.second)=="stabilizer"){auto* panel=editor->findChildWithID("module-panel-3");auto* intensity=panel?dynamic_cast<juce::ComboBox*>(panel->findChildWithID("stabilizer-intensity")):nullptr;if(!intensity||intensity->getSelectedId()!=3||actual("stabilizerIntensity")!=2){std::cerr<<"Stabilizer Hybrid Studio intensity missing or detached\n";return false;}feed(24);}
            for(auto size:{juce::Point<int>(750,550),juce::Point<int>(1000,700),juce::Point<int>(2000,1400)}){editor->setSize(size.x,size.y);if(!controlsFit())return false;auto image=editor->createComponentSnapshot(editor->getLocalBounds());if(!image.isValid())return false;
                if(iteration==0&&size.x==1000){auto output=juce::File::getCurrentWorkingDirectory().getChildFile(juce::String("build-plugin/")+entry.second+"-smoke.png");juce::FileOutputStream stream(output);stream.setPosition(0);stream.truncate();if(!juce::PNGImageFormat{}.writeImageToStream(image,stream))return false;}}
        }
        dynamic_cast<juce::Button*>(editor->findChildWithID("view-stereo"))->onClick();editor->setSize(1000,700);
        auto* stereo=editor->findChildWithID("module-panel-2");if(!stereo)return false;
        auto* stereoize=dynamic_cast<juce::Slider*>(stereo->findChildWithID("stereoizeAmount"));if(!stereoize||!stereoize->isVisible())return false;
        const float oldStereoize=actual("stereoizeAmount");stereoize->setValue(.7,juce::sendNotificationSync);if(std::abs(actual("stereoizeAmount")-.7f)>.001f)return false;stereoize->setValue(oldStereoize,juce::sendNotificationSync);

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
        auto* a=dynamic_cast<juce::Button*>(editor->findChildWithID("brain-audition-a"));
        auto* b=dynamic_cast<juce::Button*>(editor->findChildWithID("brain-audition-b"));
        if(!a||!b||!a->isVisible()||!b->isVisible()||!a->isEnabled()||!b->isEnabled()){
            std::cerr<<"Brain GUI A/B control state failed: A "<<(a&&a->isVisible())<<"/"<<(a&&a->isEnabled())
                     <<" B "<<(b&&b->isVisible())<<"/"<<(b&&b->isEnabled())
                     <<"\n"<<report->getText()<<"\n";return false;
        }
        a->onClick();
        if(!report->getText().contains("Audition A is live")){
            std::cerr<<"Brain GUI Audition A failed: "<<report->getText()<<"\n";return false;
        }
        b->onClick();
        if(!report->getText().contains("Audition B is live")||!apply->isEnabled()){
            std::cerr<<"Brain GUI Audition B failed: "<<report->getText()<<"\n";return false;
        }
        apply->onClick();
        if(!report->getText().contains("APPLIED")||apply->isEnabled()){
            std::cerr<<"Brain GUI Apply failed: "<<report->getText()<<"\n";return false;
        }
        dynamic_cast<juce::Button*>(editor->findChildWithID("undo"))->onClick();
        for(int i=0;i<processor.getParameters().size();++i)if(processor.getParameters()[i]->getValue()!=brainBefore[static_cast<size_t>(i)]){
            std::cerr<<"Brain GUI Undo failed parameter index "<<i<<"\n";return false;
        }
        if(actual("limiterDrive")!=driveBefore||actual("gain16")!=gainBefore){
            std::cerr<<"Brain GUI Undo failed limiterDrive or gain16\n";return false;
        }
        // Screenshot-reported accessibility: entire LED+caption is a >=30px target.
        for(const auto group:std::array<std::pair<const char*,std::array<const char*,2>>,2>{{
               {"harmonic-exciter-panel",{"exciter-present","exciter-active"}},
               {"final-true-peak-panel",{"true-peak-present","true-peak-enabled"}}
            }}){
            auto* panel=editor->findChildWithID(group.first);
            if(!panel){std::cerr<<"Status panel not found: "<<group.first<<"\n";return false;}
            for(const char* id:group.second){
                // JUCE findChildWithID addresses immediate children of the chosen
                // component; it does not search arbitrary hidden grandchildren.
                auto* toggle=dynamic_cast<juce::ToggleButton*>(panel->findChildWithID(id));
                if(!toggle||toggle->getHeight()<30||toggle->getWidth()<100){
                    std::cerr<<"Tiny/unreachable toggle "<<id<<": "<<
                              (toggle?toggle->getWidth():-1)<<"x"<<
                              (toggle?toggle->getHeight():-1)<<"\n";return false;
                }
            }
        }
        // Full factory preset 0 through the actual NEW GUI button. Undo restores
        // every parameter to the exact state immediately before the reset.
        auto* factory=dynamic_cast<juce::Button*>(editor->findChildWithID("factory-default"));
        if(!factory||factory->getHeight()<30||factory->getWidth()<48){
            std::cerr<<"NEW factory control is missing or too small\n";return false;
        }
        parameter("gain16")->setValueNotifyingHost(.84f);
        std::vector<float> beforeFactory;
        for(auto* param:processor.getParameters())beforeFactory.push_back(param->getValue());
        factory->onClick();
        for(auto* param:processor.getParameters())
            if(std::abs(param->getValue()-param->getDefaultValue())>1.0e-6f){
                std::cerr<<"NEW did not restore factory parameter defaults\n";return false;
            }
        dynamic_cast<juce::Button*>(editor->findChildWithID("undo"))->onClick();
        for(int i=0;i<processor.getParameters().size();++i)
            if(std::abs(processor.getParameters()[i]->getValue()-beforeFactory[static_cast<size_t>(i)])>1.0e-6f){
                std::cerr<<"NEW Undo did not restore state at parameter "<<i<<"\n";return false;
            }
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
