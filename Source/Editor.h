#pragma once
#include "Analysis.h"
#include "StudioTheme.h"

// Included after Processor. GUI work and FFT never run in the audio callback.
class Editor final : public juce::AudioProcessorEditor,
                     public juce::DragAndDropContainer,
                     public juce::DragAndDropTarget,
                     private juce::Timer {
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;
    Processor& processor;
    StudioTheme theme;
    juce::TooltipWindow tooltips{this, 600};
    juce::ComboBox bandChoice, fftChoice, typeChoice;
    juce::ToggleButton bandEnabled{"Band"};
    std::unique_ptr<ComboAttachment> typeAttachment;
    std::unique_ptr<ButtonAttachment> bandEnabledAttachment;
    juce::TextButton resetMetersButton{"RESET METERS"};
    juce::TextButton undoButton{"UNDO"},redoButton{"REDO"},presetButton{"PRESET"};
    std::unique_ptr<juce::FileChooser> presetChooser;
    juce::TextButton eqView{"SPECTRAL EQ"}, limiterView{"LIMITER"}, inspectorView{"INSPECTOR"}, brainView{"MASTER BRAIN"};
    bool showingLimiter=false,showingInspector=false,showingBrain=false;
    juce::ComboBox brainGenre,brainStyle,brainDuration;
    juce::Slider brainTarget,brainCeiling,brainIntensity;
    std::array<juce::Label,6> brainLabels;
    juce::ToggleButton brainEq{"EQ"},brainLimiter{"Limiter"};
    juce::TextButton brainAnalyse{"ANALYSE AUDIO"},brainApply{"APPLY PROPOSAL"};
    juce::TextEditor brainReport;double brainProgress=0;juce::String brainProgressText;
    juce::String previousBrainReport;masterlab::BrainSnapshot brainSnapshot;
    bool brainControlsUpdating=false;
    std::array<masterlab::LimiterFrame,1000> limiterHistory{};
    size_t historyHead=0,historyCount=0;
    double lastLimiterUpdate=0;
    void showLimiter(bool enabled){
        endDrag();setBrainVisible(false);resetMetersButton.setVisible(false);showingInspector=false;showingLimiter=enabled;inspectorView.setToggleState(false,juce::dontSendNotification);
        for(auto* component:std::array<juce::Component*,11>{&bandChoice,&fftChoice,&frequency,&gain,&quality,&frequencyLabel,&gainLabel,&qualityLabel,&eqEnabled,&ceiling,&ceilingLabel})component->setVisible(!enabled);
        typeChoice.setVisible(!enabled);bandEnabled.setVisible(!enabled);
        ceiling.setVisible(true);ceilingLabel.setVisible(true);
        drive.setVisible(enabled);driveLabel.setVisible(enabled);release.setVisible(enabled);releaseLabel.setVisible(enabled);
        eqView.setToggleState(!enabled,juce::dontSendNotification);limiterView.setToggleState(enabled,juce::dontSendNotification);repaint();
    }
    void showInspector(){
        endDrag();setBrainVisible(false);resetMetersButton.setVisible(true);showingInspector=true;showingLimiter=false;
        for(auto* component:std::initializer_list<juce::Component*>{&frequency,&gain,&quality,&ceiling,&drive,&release,&frequencyLabel,&gainLabel,&qualityLabel,&ceilingLabel,&driveLabel,&releaseLabel,&bandChoice,&fftChoice,&typeChoice,&bandEnabled,&eqEnabled,&limiterEnabled})component->setVisible(false);
        eqView.setToggleState(false,juce::dontSendNotification);limiterView.setToggleState(false,juce::dontSendNotification);inspectorView.setToggleState(true,juce::dontSendNotification);repaint();
    }
    void setBrainVisible(bool visible){
        showingBrain=visible;brainView.setToggleState(visible,juce::dontSendNotification);
        for(auto* component:std::initializer_list<juce::Component*>{&brainGenre,&brainStyle,&brainDuration,&brainTarget,&brainCeiling,&brainIntensity,&brainEq,&brainLimiter,&brainAnalyse,&brainApply,&brainReport})component->setVisible(visible);
        for(auto& label:brainLabels)label.setVisible(visible);
    }
    void showBrain(){showInspector();resetMetersButton.setVisible(false);showingInspector=false;inspectorView.setToggleState(false,juce::dontSendNotification);setBrainVisible(true);refreshBrain();repaint();}
    masterlab::BrainConfig brainConfig() const {
        masterlab::BrainConfig config;config.genre=brainGenre.getSelectedId()-1;config.style=brainStyle.getSelectedId()-1;config.seconds=brainDuration.getSelectedId();config.targetLufs=static_cast<float>(brainTarget.getValue());config.ceiling=static_cast<float>(brainCeiling.getValue());config.intensity=static_cast<float>(brainIntensity.getValue());config.useEq=brainEq.getToggleState();config.useLimiter=brainLimiter.getToggleState();return config;
    }
    void refreshBrain(){
        brainSnapshot=processor.brain.readUI();const auto& snapshot=brainSnapshot;const bool active=snapshot.phase==masterlab::BrainPhase::Pending||snapshot.phase==masterlab::BrainPhase::Analysing;
        brainProgress=active?snapshot.progress:((snapshot.phase==masterlab::BrainPhase::Ready||snapshot.phase==masterlab::BrainPhase::Applied)?1.:0.);
        brainProgressText=active?juce::String(snapshot.progress*100,0)+"% AUDIO CAPTURED":(brainProgress==1.?"ANALYSIS COMPLETE":"READY TO ANALYSE");
        brainAnalyse.setButtonText(active?"CANCEL ANALYSIS":"ANALYSE AUDIO");
        for(auto* component:std::initializer_list<juce::Component*>{&brainGenre,&brainStyle,&brainDuration,&brainTarget,&brainCeiling,&brainIntensity,&brainEq,&brainLimiter})component->setEnabled(!active);
        const bool stale=processor.brainStaleUI();brainApply.setEnabled(snapshot.phase==masterlab::BrainPhase::Ready&&snapshot.proposal.valid&&!stale);
        juce::String report=juce::String(snapshot.message)+"\n";
        if(active)report+="Audio captured: "+juce::String(snapshot.progress*snapshot.config.seconds,1)+" / "+juce::String(snapshot.config.seconds)+" s. If playback stops, analysis waits.\n";
        if(snapshot.phase==masterlab::BrainPhase::Ready||snapshot.phase==masterlab::BrainPhase::Applied){
            const auto& f=snapshot.features;const auto& p=snapshot.proposal;
            report+="\nINPUT: "+lufsText(f.integrated)+" LUFS-I   |   peak "+dbText(f.peak)+" dBFS   |   correlation "+juce::String(f.correlation,3)+"\n";
            report+="Captured "+juce::String(f.seconds,1)+" s. Target "+juce::String(snapshot.config.targetLufs,1)+" LUFS.\n";
            if(p.valid){report+="\nPROPOSED EQ (low to high): ";for(float gain:p.gains)report+=juce::String(gain,1)+" dB   ";report+="\nLimiter drive: "+juce::String(p.drive,1)+" dB. Estimated EQ level change: "+juce::String(p.eqEstimate,1)+" dB.\n";}
            if(p.targetShares[0]>0){
                report+="\nTONAL ENERGY %: input / genre target / EQ estimate\n";
                constexpr std::array<const char*,5> regions{"20-80 Hz","80-250 Hz","250 Hz-2 kHz","2-6 kHz","6-20 kHz"};
                for(size_t i=0;i<5;++i)report+=juce::String(regions[i])+": "+juce::String(100*p.shares[i],1)+" / "+juce::String(100*p.targetShares[i],1)+" / "+juce::String(100*p.estimatedShares[i],1)+"\n";
                report+="EQ estimate uses five representative frequencies; excludes limiter and is not a measured output. Chart clips at +/-18 dB. Target is a heuristic, not a quality standard.\n";
            }
            for(const auto& line:p.reasons)report+="\n"+juce::String(line)+"\n";
            for(const auto& line:p.warnings)report+="\nNOTE: "+juce::String(line)+"\n";
            if(stale&&snapshot.phase==masterlab::BrainPhase::Ready)report="SETTINGS CHANGED: reanalyse before Apply.\n\n"+report;
        }
        if(report!=previousBrainReport){brainReport.setText(report,false);previousBrainReport=report;}
    }
    void paintBrain(juce::Graphics& g){
        g.setColour(background);g.fillRect(0,134,getWidth(),getHeight()-134);
        g.setColour(panel);g.fillRoundedRectangle(24.f,154.f,getWidth()-48.f,178.f,8);
        g.setColour(accent);g.setFont(12);g.drawText("MASTER BRAIN  /  EXPLAINABLE MASTERING ASSISTANT",40,134,getWidth()-80,20,juce::Justification::left);
        g.setColour(juce::Colour(0xff0a1119));g.fillRoundedRectangle(40.f,294.f,getWidth()-80.f,18.f,4.f);
        if(brainProgress>0){g.setColour(accent.withAlpha(.35f));g.fillRoundedRectangle(40.f,294.f,static_cast<float>((getWidth()-80)*brainProgress),18.f,4.f);}
        g.setColour(juce::Colours::whitesmoke);g.setFont(11);g.drawText(brainProgressText,40,294,getWidth()-80,18,juce::Justification::centred);
        const int cardWidth=std::clamp(getWidth()/2-48,300,600),cardHeight=getHeight()-426;
        g.setColour(panel);g.fillRoundedRectangle(24.f,342.f,static_cast<float>(cardWidth),static_cast<float>(cardHeight),8.f);
        g.setColour(muted);g.setFont(11);g.drawText("TONAL BALANCE / RELATIVE TO TARGET",36,350,cardWidth-24,18,juce::Justification::left);
        const bool valid=(brainSnapshot.phase==masterlab::BrainPhase::Ready||brainSnapshot.phase==masterlab::BrainPhase::Applied)&&brainSnapshot.proposal.targetShares[0]>0;
        if(valid){
            g.setColour(juce::Colours::whitesmoke);g.drawText("INPUT",36,370,54,16,juce::Justification::left);
            g.setColour(accent);g.drawText("EQ ESTIMATE",92,370,94,16,juce::Justification::left);
            const float left=60.f,right=cardWidth+8.f,top=398.f,bottom=342.f+cardHeight-32.f;
            auto yFor=[&](double db){return juce::jmap(static_cast<float>(std::clamp(db,-18.,18.)), -18.f,18.f,bottom,top);};
            for(int db:{-18,-9,0,9,18}){const float y=yFor(db);g.setColour(db==0?accent.withAlpha(.45f):muted.withAlpha(.15f));g.drawLine(left,y,right,y);g.setColour(muted);g.drawText(juce::String(db),30,static_cast<int>(y)-7,26,14,juce::Justification::right);}
            juce::Path measured,estimated;
            constexpr std::array<const char*,5> names{"SUB","BASS","MID","PRES","AIR"};
            for(size_t i=0;i<5;++i){const float x=left+(right-left)*static_cast<float>(i)/4.f;const auto& proposal=brainSnapshot.proposal;
                const double target=std::max(1e-20f,proposal.targetShares[i]);
                const float measuredY=yFor(10*std::log10(std::max(1e-20f,proposal.shares[i])/target)),estimatedY=yFor(10*std::log10(std::max(1e-20f,proposal.estimatedShares[i])/target));
                if(i==0){measured.startNewSubPath(x,measuredY);estimated.startNewSubPath(x,estimatedY);}else{measured.lineTo(x,measuredY);estimated.lineTo(x,estimatedY);}
                g.setColour(juce::Colours::whitesmoke);g.fillEllipse(x-2.5f,measuredY-2.5f,5.f,5.f);g.setColour(accent);g.fillEllipse(x-2.5f,estimatedY-2.5f,5.f,5.f);
                g.setColour(muted);g.drawText(names[i],static_cast<int>(x)-21,static_cast<int>(bottom)+5,42,14,juce::Justification::centred);
            }
            g.setColour(juce::Colours::whitesmoke);g.strokePath(measured,juce::PathStrokeType(1.5f));g.setColour(accent);g.strokePath(estimated,juce::PathStrokeType(1.8f));
        }else{g.setColour(muted);g.drawFittedText("Play audio and analyse to measure five frequency regions.",36,382,cardWidth-24,cardHeight-48,juce::Justification::centred,3);}
        g.setColour(muted);g.setFont(11);g.drawText("Review the proposal. EQ settings may be replaced; Apply is one Undo transaction.",40,getHeight()-66,getWidth()-280,36,juce::Justification::left);
        g.drawText("Explicit DSP rules / no trained AI model / sample-peak limiter",24,getHeight()-25,getWidth()-48,18,juce::Justification::left);
    }
    juce::String dbText(float level) const {return level>1e-10f?juce::String(juce::Decibels::gainToDecibels(level),1):juce::String("--");}
    juce::String lufsText(float value) const {return value>-119?juce::String(value,1):juce::String("--");}
    void paintInspector(juce::Graphics& g){
        auto area=plot();const bool compact=area.getHeight()<220;
        g.setColour(muted);g.setFont(12);g.drawText("MASTER INSPECTOR  /  INPUT - OUTPUT",48,144,getWidth()-300,20,juce::Justification::left);
        for(int i=0;i<2;++i){auto reading=i==0?response.inputReading:response.outputReading;auto loudness=i==0?response.inputLufs:response.outputLufs;float x=24.f+i*(area.getWidth()+48)/2,w=(area.getWidth()+48)/2-10;
            g.setColour(panel);g.fillRoundedRectangle(x,162,w,area.getHeight()+24,8);
            g.setColour(i==0?muted:accent);g.setFont(13);g.drawText(i==0?"INPUT":"OUTPUT",static_cast<int>(x)+16,176,static_cast<int>(w)-32,20,juce::Justification::left);
            g.setColour(reading.peak>1?juce::Colour(0xffee927f):juce::Colours::whitesmoke);g.setFont(compact?26.f:38.f);g.drawText(dbText(reading.peak),static_cast<int>(x)+16,compact?200:204,static_cast<int>(w)-32,44,juce::Justification::left);
            g.setColour(muted);g.setFont(11);g.drawText("SAMPLE PEAK / dBFS",static_cast<int>(x)+16,244,static_cast<int>(w)-32,18,juce::Justification::left);
            const float barY=compact?270.f:280.f;float normal=juce::jlimit(0.f,1.f,(juce::Decibels::gainToDecibels(reading.peak,-60.f)+60)/60);
            g.setColour(juce::Colour(0xff0a1119));g.fillRoundedRectangle(x+16,barY,w-32,8,4);g.setColour(i==0?curve:accent);g.fillRoundedRectangle(x+16,barY,(w-32)*normal,8,4);
            g.setColour(muted);g.setFont(12);g.drawText("RMS (400 ms)  "+dbText(reading.rms)+" dBFS",static_cast<int>(x)+16,compact?287:312,static_cast<int>(w)-32,22,juce::Justification::left);
            if(!compact){g.drawText("MOMENTARY  "+lufsText(loudness.momentary)+" LUFS",static_cast<int>(x)+16,340,static_cast<int>(w)-32,20,juce::Justification::left);
                g.drawText("SHORT-TERM  "+lufsText(loudness.shortTerm)+" LUFS",static_cast<int>(x)+16,366,static_cast<int>(w)-32,20,juce::Justification::left);
                if(area.getHeight()>290){g.setColour(reading.clippedSamples?juce::Colour(0xffee927f):muted);g.drawText("CLIP SAMPLES  "+juce::String(reading.clippedSamples),static_cast<int>(x)+16,404,static_cast<int>(w)-32,20,juce::Justification::left);
                    g.setColour(muted);}
            }
        }
        int x=getWidth()-218;g.setColour(panel);g.fillRoundedRectangle(static_cast<float>(x),162,194,area.getHeight()+36,8);g.setColour(muted);g.setFont(12);g.drawText("STEREO / OUTPUT",x+16,176,162,20,juce::Justification::left);
        float radius=compact?42.f:56.f,cx=x+97.f,cy=compact?242.f:260.f;
        g.setColour(juce::Colour(0xff3c4c5c));g.drawEllipse(cx-radius,cy-radius,radius*2,radius*2,1);g.drawLine(cx-radius,cy,cx+radius,cy);g.drawLine(cx,cy-radius,cx,cy+radius);
        {juce::Graphics::ScopedSaveState save(g);g.reduceClipRegion(juce::Rectangle<float>(cx-radius,cy-radius,radius*2,radius*2).toNearestInt());g.setColour(accent.withAlpha(.5f));for(auto point:scope)g.fillEllipse(cx+point.x*radius-1,cy-point.y*radius-1,2,2);}
        g.setColour(juce::Colours::whitesmoke);g.setFont(14);g.drawText("CORR  "+juce::String(response.outputReading.correlation,3),x+16,static_cast<int>(cy+radius)+12,162,24,juce::Justification::left);
        if(!compact){g.setColour(muted);g.setFont(12);g.drawText("MID  "+dbText(response.outputReading.midRms)+" dBFS",x+16,360,162,20,juce::Justification::left);g.drawText("SIDE  "+dbText(response.outputReading.sideRms)+" dBFS",x+16,386,162,20,juce::Justification::left);
            if(area.getHeight()>290)g.drawText("HEADROOM  "+(response.outputReading.peak>0?juce::String(-juce::Decibels::gainToDecibels(response.outputReading.peak),1):juce::String("--"))+" dB",x+16,430,162,20,juce::Justification::left);
        }
        const int dock=getHeight()-174,cell=(getWidth()-64)/4;
        for(int i=0;i<4;++i){int left=40+i*cell;g.setColour(muted);g.setFont(12);g.drawText(std::array<const char*,4>{"INPUT LUFS-I","OUTPUT LUFS-I","CORRELATION","ENERGY BALANCE"}[i],left,dock+14,cell-32,20,juce::Justification::left);
            auto reading=response.outputReading;juce::String text=i==0?lufsText(response.inputLufs.integrated):(i==1?lufsText(response.outputLufs.integrated):(i==2?juce::String(reading.correlation,3):(std::abs(reading.balance)<.001f?juce::String("CENTER"):juce::String(reading.balance<0?"L ":"R ")+juce::String(std::abs(reading.balance)*100,1)+"%")));
            g.setColour(i>=2?accent:juce::Colours::whitesmoke);g.setFont(26);g.drawText(text,left,dock+43,cell-32,38,juce::Justification::left);
            g.setColour(muted);g.setFont(11);g.drawText(i<2?"LUFS / gated integrated":(i==2?"-1 ... +1":"(R energy - L) / total"),left,dock+101,cell-32,24,juce::Justification::left);
        }
        g.setColour(muted);g.setFont(11);g.drawText("LUFS: K-weighted / M 400 ms / S 3 s / I since reset   |   True Peak unavailable",24,getHeight()-25,getWidth()-48,18,juce::Justification::left);
    }
    void paintLimiter(juce::Graphics& g){
        const auto area=plot();
        const float split=area.getY()+area.getHeight()*.65f;
        const float grHeight=split-area.getY()-22;
        g.setColour(panel);g.fillRoundedRectangle(area.expanded(24,12),8);
        g.setColour(juce::Colour(0xff0c131c));g.fillRect(area);
        g.setFont(12);
        for(int seconds=0;seconds<=10;seconds+=2){float x=area.getX()+area.getWidth()*seconds/10.f;g.setColour(juce::Colour(0xff24313e));g.drawVerticalLine(static_cast<int>(x),area.getY(),area.getBottom());g.setColour(muted);g.drawText(seconds==10?"NOW":juce::String(seconds-10)+" s",static_cast<int>(x)-18,static_cast<int>(area.getBottom())+5,40,18,juce::Justification::centred);}
        for(int db:{0,6,12,18,24}){float y=area.getY()+grHeight*db/24.f;g.setColour(juce::Colour(0xff293645));g.drawHorizontalLine(static_cast<int>(y),area.getX(),area.getRight());g.setColour(muted);g.drawText(juce::String(db),8,static_cast<int>(y)-8,32,16,juce::Justification::right);}
        g.setColour(curve);g.setFont(12);g.drawText("GAIN REDUCTION",48,144,170,20,juce::Justification::left);
        g.setColour(muted);g.drawText("10 s  /  AUDIO CLOCK",getWidth()-476,144,224,20,juce::Justification::right);
        g.setColour(accent);g.setFont(11);g.drawText("OUTPUT PEAK   -60 ... 0 dBFS",static_cast<int>(area.getX())+10,static_cast<int>(split)-18,240,18,juce::Justification::left);
        g.setColour(juce::Colour(0xff354657));g.drawHorizontalLine(static_cast<int>(split),area.getX(),area.getRight());
        float maximumReduction=0,maximumPeak=0;
        if(historyCount){
            const auto latest=limiterHistory[(historyHead+999)%1000].endSample;
            const double duration=sampleRate()*10.;
            juce::Path reduction,output,fill;bool started=false;std::uint64_t previous=0;float lastX=0;
            for(size_t i=0;i<historyCount;++i){const auto& frame=limiterHistory[(historyHead+1000-historyCount+i)%1000];
                double age=static_cast<double>(latest-frame.endSample);if(age>duration)continue;
                float x=area.getRight()-static_cast<float>(age/duration)*area.getWidth();
                float gr=-juce::Decibels::gainToDecibels(frame.minimumGain,-120.f);
                float y=area.getY()+grHeight*juce::jlimit(0.f,1.f,gr/24.f);
                float peakDb=juce::Decibels::gainToDecibels(frame.outputPeak,-100.f);
                float py=area.getBottom()-(area.getBottom()-split-8)*juce::jlimit(0.f,1.f,(peakDb+60)/60.f);
                const bool gap=started&&frame.endSample-previous>std::ceil(sampleRate()*.01)+1;
                if(!started||gap){if(started){fill.lineTo(lastX,area.getY());fill.closeSubPath();}reduction.startNewSubPath(x,y);output.startNewSubPath(x,py);fill.startNewSubPath(x,area.getY());fill.lineTo(x,y);}else{reduction.lineTo(x,y);output.lineTo(x,py);fill.lineTo(x,y);}
                started=true;previous=frame.endSample;lastX=x;maximumReduction=std::max(maximumReduction,gr);maximumPeak=std::max(maximumPeak,frame.outputPeak);
            }
            if(started){fill.lineTo(lastX,area.getY());fill.closeSubPath();}
            juce::Graphics::ScopedSaveState save(g);g.reduceClipRegion(area.toNearestInt());
            g.setGradientFill(juce::ColourGradient(curve.withAlpha(.23f),0,area.getY(),curve.withAlpha(.04f),0,split,false));g.fillPath(fill);
            g.setColour(accent.withAlpha(.10f));g.strokePath(output,juce::PathStrokeType(5));g.setColour(accent);g.strokePath(output,juce::PathStrokeType(1.6f));
            g.setColour(curve.withAlpha(.10f));g.strokePath(reduction,juce::PathStrokeType(7));g.setColour(curve);g.strokePath(reduction,juce::PathStrokeType(2));
        }
        const int x=getWidth()-218;
        g.setColour(panel);g.fillRoundedRectangle(static_cast<float>(x),162.f,194.f,area.getHeight()+36,8);
        g.setColour(muted);g.setFont(12);g.drawText("MASTER OUTPUT",x+16,176,162,20,juce::Justification::left);
        g.setColour(juce::Colours::whitesmoke);g.setFont(juce::Font(juce::FontOptions(34.f,juce::Font::bold)));g.drawText(juce::String(juce::Decibels::gainToDecibels(processor.peak.load()),1),x+16,202,150,42,juce::Justification::left);
        g.setColour(muted);g.setFont(12);g.drawText("SAMPLE PEAK / dBFS",x+16,244,162,18,juce::Justification::left);
        g.setColour(curve);g.setFont(24);g.drawText(juce::String(maximumReduction,1)+" dB",x+16,274,162,30,juce::Justification::left);
        g.setColour(muted);g.setFont(11);g.drawText("MAX GR / VISIBLE HISTORY",x+16,307,166,18,juce::Justification::left);
        if(area.getHeight()>235){
            const float top=345,bottom=area.getBottom()-10,height=bottom-top;
            for(int k=0;k<2;++k){float level=k==0?processor.peak.load():processor.rms.load();float db=juce::Decibels::gainToDecibels(level,-60.f);float f=juce::jlimit(0.f,1.f,(db+60)/60);
                float bx=x+20.f+k*76;g.setColour(juce::Colour(0xff0a1119));g.fillRoundedRectangle(bx,top,52,height,4);
                g.setGradientFill(juce::ColourGradient(accent,bx,bottom-height,juce::Colour(0xff267d86),bx,bottom,false));g.fillRect(bx,bottom-height*f,52.f,height*f);
                g.setColour(muted);g.setFont(11);g.drawText(k==0?"PEAK":"RMS",static_cast<int>(bx),static_cast<int>(bottom)+4,52,16,juce::Justification::centred);
            }
        }
        const int dock=getHeight()-174,cell=(getWidth()-64)/4;
        g.setColour(muted);g.setFont(12);g.drawText("LOOKAHEAD",48+cell*2,dock+15,cell-32,20,juce::Justification::left);
        g.setColour(juce::Colours::whitesmoke);g.setFont(26);g.drawText("5.0 ms",48+cell*2,dock+41,cell-32,32,juce::Justification::left);
        g.setColour(muted);g.setFont(12);g.drawText("PDC  "+juce::String(processor.getLatencySamples())+" samples",48+cell*2,dock+88,cell-32,20,juce::Justification::left);
        g.drawText(juce::String(sampleRate()/1000,1)+" kHz / stereo",48+cell*2,dock+109,cell-32,20,juce::Justification::left);

        if(!historyCount||juce::Time::getMillisecondCounterHiRes()-lastLimiterUpdate>500){g.setColour(muted);g.drawText("WAITING FOR AUDIO",area.toNearestInt().reduced(10),juce::Justification::centred);}
    }
    juce::Slider frequency, gain, quality, ceiling, drive, release;
    juce::Label frequencyLabel, gainLabel, qualityLabel, ceilingLabel, driveLabel, releaseLabel;
    juce::ToggleButton eqEnabled{"EQ"}, limiterEnabled{"Limiter"};
    std::unique_ptr<SliderAttachment> frequencyAttachment, gainAttachment, qualityAttachment, ceilingAttachment, driveAttachment, releaseAttachment;
    std::unique_ptr<ButtonAttachment> eqAttachment, limiterAttachment;
    Processor::ResponseFrame response;
    bool haveResponse = false, draggingNode = false;
    int selectedBand = 0;
    int fftOrder=13;
    std::unique_ptr<juce::dsp::FFT> fft=std::make_unique<juce::dsp::FFT>(fftOrder);
    std::array<float,65536> fftLeft{},fftRight{};
    std::array<float,16385> spectrum{};
    std::array<std::array<float,32768>,2> samples{};
    double lastAnalysis=0;
    std::array<juce::Point<float>,1024> scope{};
    int head = 0;
    const juce::Colour background{0xff0b1017}, panel{0xff151e29}, muted{0xffa7b8c8};
    const juce::Colour accent{0xff58dac5}, curve{0xffe7bb6a};

    juce::String parameterId(const char* base, int band) const {
        return juce::String(base) + (band == 0 ? juce::String{} : juce::String(band + 1));
    }
    float value(const char* base, int band) const {
        return masterlab::clean(processor.state.getRawParameterValue(parameterId(base,band))->load());
    }
    double sampleRate() const { return processor.displayRate.load(); }
    juce::Rectangle<float> plot() const {
        return {48.f, 174.f, std::max(200.f, getWidth() - 300.f), std::max(100.f, getHeight() - 394.f)};
    }
    float xForFrequency(double hz) const {
        auto area = plot();
        return area.getX() + area.getWidth() * static_cast<float>(std::log(std::max(20.,hz)/20.) / std::log(sampleRate()*.5/20.));
    }
    int bandType(int band) const {return static_cast<int>(processor.state.getRawParameterValue("bandType"+juce::String(band+1))->load());}
    bool gainApplies(int band) const {return bandType(band)<=2;}
    float yForGain(float db) const { auto area=plot(); return area.getCentreY()-db*area.getHeight()/36.f; }
    juce::Point<float> node(int band) const {
        return {xForFrequency(std::min<double>(value("frequency",band),sampleRate()*.45)), yForGain(gainApplies(band)?value("gain",band):0.f)};
    }
    void selectBand(int band) {
        selectedBand=juce::jlimit(0,15,band);
        frequencyAttachment.reset();gainAttachment.reset();qualityAttachment.reset();typeAttachment.reset();bandEnabledAttachment.reset();
        frequencyAttachment=std::make_unique<SliderAttachment>(processor.state,parameterId("frequency",selectedBand),frequency);
        gainAttachment=std::make_unique<SliderAttachment>(processor.state,parameterId("gain",selectedBand),gain);
        qualityAttachment=std::make_unique<SliderAttachment>(processor.state,parameterId("q",selectedBand),quality);
        typeAttachment=std::make_unique<ComboAttachment>(processor.state,"bandType"+juce::String(selectedBand+1),typeChoice);
        bandEnabledAttachment=std::make_unique<ButtonAttachment>(processor.state,"bandEnabled"+juce::String(selectedBand+1),bandEnabled);
        bandChoice.setSelectedId(selectedBand+1,juce::dontSendNotification);
        repaint();
    }
    void setParameter(const char* base,float number,bool completeGesture=false) {
        auto* parameter=processor.state.getParameter(parameterId(base,selectedBand));
        if(completeGesture){processor.beginUiTransaction("EQ Q");parameter->beginChangeGesture();}
        parameter->setValueNotifyingHost(parameter->convertTo0to1(number));
        if(completeGesture){parameter->endChangeGesture();processor.finishUiTransaction();}
    }
    void endDrag() {
        if(!draggingNode)return;
        processor.state.getParameter(parameterId("frequency",selectedBand))->endChangeGesture();
        processor.state.getParameter(parameterId("gain",selectedBand))->endChangeGesture();
        draggingNode=false;processor.finishUiTransaction();
    }
    void timerCallback() override {
        if(showingBrain)refreshBrain();
        undoButton.setEnabled(processor.undoManager.canUndo());redoButton.setEnabled(processor.undoManager.canRedo());
        gain.setEnabled(gainApplies(selectedBand));gainLabel.setText(gainApplies(selectedBand)?"GAIN":"GAIN / NOT USED",juce::dontSendNotification);
        int start,count,start2,count2;
        processor.limiterFifo.prepareToRead(processor.limiterFifo.getNumReady(),start,count,start2,count2);
        auto takeFrames=[&](int first,int size){for(int i=0;i<size;++i){auto frame=processor.limiterFrames[first+i];
            if(historyCount&&(frame.generation!=limiterHistory[(historyHead+999)%1000].generation||frame.endSample<=limiterHistory[(historyHead+999)%1000].endSample)){historyCount=0;historyHead=0;}
            limiterHistory[historyHead]=frame;historyHead=(historyHead+1)%1000;historyCount=std::min<size_t>(1000,historyCount+1);
        }};
        takeFrames(start,count);takeFrames(start2,count2);processor.limiterFifo.finishedRead(count+count2);
        if(count+count2)lastLimiterUpdate=juce::Time::getMillisecondCounterHiRes();
        processor.responseFifo.prepareToRead(processor.responseFifo.getNumReady(),start,count,start2,count2);
        for(int i=0;i<count;++i){response=processor.responses[start+i];haveResponse=true;}
        for(int i=0;i<count2;++i){response=processor.responses[start2+i];haveResponse=true;}
        processor.responseFifo.finishedRead(count+count2);
        processor.fifo.prepareToRead(processor.fifo.getNumReady(),start,count,start2,count2);
        auto consume=[&](int first,int size){for(int i=0;i<size;++i){float l=processor.traceL[first+i],r=processor.traceR[first+i];samples[0][head]=l;samples[1][head]=r;scope[head%1024]={r-l,r+l};head=(head+1)%32768;}};
        consume(start,count);consume(start2,count2);processor.fifo.finishedRead(count+count2);
        if(count+count2>0){
            const int size=1<<fftOrder;
            for(int i=0;i<size;++i){float window=.5f-.5f*std::cos(juce::MathConstants<float>::twoPi*i/(size-1));int index=(head+32768-size+i)%32768;fftLeft[i]=samples[0][index]*window;fftRight[i]=samples[1][index]*window;}
            std::fill(fftLeft.begin()+size,fftLeft.begin()+size*2,0);std::fill(fftRight.begin()+size,fftRight.begin()+size*2,0);
            fft->performFrequencyOnlyForwardTransform(fftLeft.data());fft->performFrequencyOnlyForwardTransform(fftRight.data());
            double now=juce::Time::getMillisecondCounterHiRes();float release=lastAnalysis>0?static_cast<float>(std::exp(-(now-lastAnalysis)/150.)):0;lastAnalysis=now;
            for(int i=0;i<=size/2;++i){float level=masterlab::stereoSpectrumAmplitude(fftLeft[i],fftRight[i],size);spectrum[i]=level>spectrum[i]?level:release*spectrum[i]+(1-release)*level;}
        }
        repaint();
    }
    void configureSlider(juce::Slider& slider,juce::Label& label,const char* name,const char* id) {
        addAndMakeVisible(slider);addAndMakeVisible(label);
        label.setText(name,juce::dontSendNotification);label.setColour(juce::Label::textColourId,muted);
        slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle(juce::Slider::TextBoxBelow,false,132,28);
        slider.setComponentID(id);slider.setColour(juce::Slider::rotarySliderFillColourId,accent);
    }
public:
    explicit Editor(Processor& p) : AudioProcessorEditor(p),processor(p) {
        theme.setColour(juce::ResizableWindow::backgroundColourId,background);
        theme.setColour(juce::Slider::textBoxOutlineColourId,juce::Colours::transparentBlack);
        theme.setColour(juce::Slider::textBoxBackgroundColourId,panel);
        setLookAndFeel(&theme);
        configureSlider(frequency,frequencyLabel,"FREQUENCY","eq-frequency");frequency.setTextValueSuffix(" Hz");
        configureSlider(gain,gainLabel,"GAIN","eq-gain");gain.setTextValueSuffix(" dB");
        configureSlider(quality,qualityLabel,"Q / BANDWIDTH","eq-q");
        configureSlider(ceiling,ceilingLabel,"SAMPLE CEILING","limiter-ceiling");ceiling.setTextValueSuffix(" dBFS");
        configureSlider(drive,driveLabel,"LIMITER DRIVE","limiter-drive");drive.setTextValueSuffix(" dB");
        configureSlider(release,releaseLabel,"RELEASE","limiter-release");release.setTextValueSuffix(" ms");
        drive.setTooltip("Gain into limiting, 0 to 24 dB. Included only in the wet limiter signal. Smoothed over 20 ms.");
        release.setTooltip("Limiter envelope recovery, 10 to 1000 ms. Short settings can distort low frequencies. The coefficient is smoothed over 20 ms.");
        driveAttachment=std::make_unique<SliderAttachment>(processor.state,"limiterDrive",drive);
        releaseAttachment=std::make_unique<SliderAttachment>(processor.state,"limiterRelease",release);
        frequency.setTooltip("Selected bell frequency. Drag its graph node horizontally to change it.");
        gain.setTooltip("Selected band gain. Drag its graph node vertically to change it.");
        quality.setTooltip("Band width. Higher Q is narrower. Use the mouse wheel over the graph node.");
        ceiling.setTooltip("Sample-peak ceiling at the limiter output. This is not a True Peak limiter. EQ after the limiter can exceed this ceiling.");
        addAndMakeVisible(typeChoice);typeChoice.setComponentID("eq-type");
        typeChoice.addItemList(juce::StringArray{"Bell","Low Shelf","High Shelf","High Pass (12)","Low Pass (12)","Notch"},1);
        typeChoice.setTooltip("Minimum-phase filters. High/low pass: fixed 12 dB/oct. Shelf Q controls resonance. Gain is unused for high/low pass and notch.");
        addAndMakeVisible(bandEnabled);bandEnabled.setComponentID("eq-band-active");bandEnabled.setTooltip("Enable this band with a smoothed transition; the filter remains warm.");
        addAndMakeVisible(bandChoice);bandChoice.setComponentID("eq-band");
        addAndMakeVisible(fftChoice);fftChoice.setComponentID("fft-size");
        for(int order=12;order<=15;++order)fftChoice.addItem("FFT "+juce::String(1<<order),order);
        fftChoice.setSelectedId(fftOrder,juce::dontSendNotification);
        fftChoice.setTooltip("Spectrum resolution only. FFT runs outside the audio callback and does not alter audio latency. L/R energy preserves opposite-phase signals.");
        fftChoice.onChange=[this]{fftOrder=juce::jlimit(12,15,fftChoice.getSelectedId());fftChoice.setSelectedId(fftOrder,juce::dontSendNotification);fft=std::make_unique<juce::dsp::FFT>(fftOrder);spectrum.fill(0);lastAnalysis=0;};
        for(int i=1;i<=16;++i)bandChoice.addItem("Band "+juce::String(i),i);
        bandChoice.onChange=[this]{endDrag();selectBand(bandChoice.getSelectedId()-1);};
        addAndMakeVisible(eqEnabled);addAndMakeVisible(limiterEnabled);
        eqEnabled.setComponentID("eq-active");limiterEnabled.setComponentID("limiter-active");
        eqAttachment=std::make_unique<ButtonAttachment>(processor.state,"eq",eqEnabled);
        limiterAttachment=std::make_unique<ButtonAttachment>(processor.state,"lim",limiterEnabled);
        ceilingAttachment=std::make_unique<SliderAttachment>(processor.state,"ceiling",ceiling);
        addAndMakeVisible(eqView);addAndMakeVisible(limiterView);addAndMakeVisible(inspectorView);inspectorView.setComponentID("view-inspector");inspectorView.onClick=[this]{showInspector();};
        eqView.setComponentID("view-eq");limiterView.setComponentID("view-limiter");
        eqView.onClick=[this]{showLimiter(false);};limiterView.onClick=[this]{showLimiter(true);};
        eqView.setClickingTogglesState(false);limiterView.setClickingTogglesState(false);
        addAndMakeVisible(resetMetersButton);resetMetersButton.setComponentID("reset-meters");resetMetersButton.onClick=[this]{processor.resetMeterRequest.store(true);};resetMetersButton.setTooltip("Reset RMS, sample peaks and LUFS at the next audio callback. Audio settings and processing are preserved.");resetMetersButton.setVisible(false);
        addAndMakeVisible(undoButton);addAndMakeVisible(redoButton);addAndMakeVisible(presetButton);
        undoButton.setComponentID("undo");redoButton.setComponentID("redo");presetButton.setComponentID("preset");
        undoButton.onClick=[this]{endDrag();processor.undo();};redoButton.onClick=[this]{endDrag();processor.redo();};
        undoButton.setTooltip("Undo the last parameter transaction. Host automation may also be recorded while it runs.");
        redoButton.setTooltip("Redo an undone transaction.");
        presetButton.onClick=[this]{
            juce::PopupMenu menu;menu.addItem(1,"Load preset...");menu.addItem(2,"Save preset...");
            auto safe=juce::Component::SafePointer<Editor>(this);
            menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(presetButton),[safe](int selection){if(safe&&(selection==1||selection==2))safe->choosePreset(selection==2);});
        };
        addAndMakeVisible(brainView);brainView.setComponentID("view-brain");brainView.onClick=[this]{showBrain();};
        for(size_t i=0;i<masterlab::brainGenres.size();++i)brainGenre.addItem(masterlab::brainGenres[i],static_cast<int>(i)+1);
        for(size_t i=0;i<masterlab::brainStyles.size();++i)brainStyle.addItem(masterlab::brainStyles[i],static_cast<int>(i)+1);
        for(int seconds:{8,16,30,60})brainDuration.addItem(juce::String(seconds)+" seconds",seconds);
        auto config=processor.brain.readUI().config;
        brainGenre.setSelectedId(config.genre+1,juce::dontSendNotification);brainStyle.setSelectedId(config.style+1,juce::dontSendNotification);brainDuration.setSelectedId(config.seconds,juce::dontSendNotification);
        brainTarget.setRange(-24,-8,.1);brainTarget.setValue(config.targetLufs,juce::dontSendNotification);brainTarget.setTextValueSuffix(" LUFS");
        brainCeiling.setRange(-6,-.1,.1);brainCeiling.setValue(config.ceiling,juce::dontSendNotification);brainCeiling.setTextValueSuffix(" dBFS");
        brainIntensity.setRange(.25,1.5,.05);brainIntensity.setValue(config.intensity,juce::dontSendNotification);brainIntensity.setTextValueSuffix(" x");
        brainEq.setToggleState(config.useEq,juce::dontSendNotification);brainLimiter.setToggleState(config.useLimiter,juce::dontSendNotification);
        for(auto* component:std::initializer_list<juce::Component*>{&brainGenre,&brainStyle,&brainDuration,&brainTarget,&brainCeiling,&brainIntensity,&brainEq,&brainLimiter,&brainAnalyse,&brainApply,&brainReport})addAndMakeVisible(component);
        for(auto* slider:{&brainTarget,&brainCeiling,&brainIntensity}){slider->setSliderStyle(juce::Slider::LinearHorizontal);slider->setTextBoxStyle(juce::Slider::TextBoxRight,false,72,24);}
        for(size_t i=0;i<brainLabels.size();++i){auto& label=brainLabels[i];addAndMakeVisible(label);label.setText(std::array<const char*,6>{"GENRE PROFILE","STYLE","TARGET LOUDNESS","SAMPLE CEILING","ANALYSIS LENGTH","EQ INTENSITY"}[i],juce::dontSendNotification);label.setColour(juce::Label::textColourId,muted);}
        brainReport.setMultiLine(true,true);brainReport.setReadOnly(true);brainReport.setScrollbarsShown(true);brainReport.setFont(juce::Font(juce::FontOptions(13.f)));brainReport.setColour(juce::TextEditor::backgroundColourId,juce::Colour(0xff151e29));brainReport.setColour(juce::TextEditor::textColourId,juce::Colour(0xffe2ecf4));brainReport.setColour(juce::TextEditor::outlineColourId,juce::Colour(0xff344452));
        auto revise=[this]{if(!brainControlsUpdating){processor.brain.reviseUI(brainConfig());refreshBrain();}};
        brainGenre.onChange=revise;brainStyle.onChange=revise;brainDuration.onChange=revise;brainTarget.onValueChange=revise;brainCeiling.onValueChange=revise;brainIntensity.onValueChange=revise;brainEq.onClick=revise;brainLimiter.onClick=revise;
        brainAnalyse.setComponentID("brain-analyse");brainApply.setComponentID("brain-apply");brainReport.setComponentID("brain-report");brainGenre.setComponentID("brain-genre");brainStyle.setComponentID("brain-style");brainTarget.setComponentID("brain-target");
        brainAnalyse.onClick=[this]{auto current=processor.brain.readUI();if(brainAnalyse.getButtonText()=="CANCEL ANALYSIS"||current.phase==masterlab::BrainPhase::Pending||current.phase==masterlab::BrainPhase::Analysing)processor.brain.cancelUI();else processor.startBrainUI(brainConfig());refreshBrain();};
        brainApply.onClick=[this]{processor.applyBrainUI();refreshBrain();};
        brainApply.setTooltip("Apply only the reviewed proposal, then re-measure the full song. Disabled if audio settings changed since capture. Undo restores the previous state.");
        brainEq.setTooltip("Include EQ: replaces all 16 bands on Apply. Untick to preserve the current EQ.");brainLimiter.setTooltip("Include sample-peak limiter and move it after EQ. No true-peak guarantee.");
        selectBand(0);showLimiter(false);
        setResizable(true,true);setResizeLimits(750,550,2000,1400);setSize(1200,800);startTimerHz(30);
    }
    void choosePreset(bool save){
        auto initial=juce::File::getSpecialLocation(juce::File::userDocumentsDirectory).getChildFile("MasterLab.jmlx");
        presetChooser=std::make_unique<juce::FileChooser>(save?"Save MasterLab preset":"Load MasterLab preset",initial,"*.jmlx");
        auto safe=juce::Component::SafePointer<Editor>(this);
        int flags=save?(juce::FileBrowserComponent::saveMode|juce::FileBrowserComponent::canSelectFiles|juce::FileBrowserComponent::warnAboutOverwriting):(juce::FileBrowserComponent::openMode|juce::FileBrowserComponent::canSelectFiles);
        presetChooser->launchAsync(flags,[safe,save](const juce::FileChooser& chooser){if(!safe)return;auto file=chooser.getResult();if(file==juce::File{})return;
            bool ok=save?safe->processor.savePreset(file):safe->processor.loadPreset(file);
            safe->presetButton.setButtonText(ok?"PRESET":"PRESET ERROR");safe->presetButton.setTooltip(ok?file.getFileName():"Preset could not be read or written. Invalid data leaves parameters unchanged.");
        });
    }
    ~Editor() override {stopTimer();endDrag();setLookAndFeel(nullptr);}
    void resized() override {
        resetMetersButton.setBounds(getWidth()-210,136,186,26);
        presetButton.setBounds(320,14,106,26);undoButton.setBounds(436,14,62,26);redoButton.setBounds(508,14,62,26);
        eqView.setBounds(getWidth()-432,48,98,26);limiterView.setBounds(getWidth()-326,48,98,26);inspectorView.setBounds(getWidth()-220,48,98,26);brainView.setBounds(getWidth()-114,48,98,26);
        const int brainCell=(getWidth()-80)/4;
        for(int i=0;i<4;++i)brainLabels[static_cast<size_t>(i)].setBounds(40+i*brainCell,166,brainCell-12,20);
        brainGenre.setBounds(40,190,brainCell-12,28);brainStyle.setBounds(40+brainCell,190,brainCell-12,28);brainTarget.setBounds(40+2*brainCell,190,brainCell-12,28);brainCeiling.setBounds(40+3*brainCell,190,brainCell-12,28);
        brainLabels[4].setBounds(40,226,brainCell-12,20);brainLabels[5].setBounds(40+brainCell,226,brainCell-12,20);brainDuration.setBounds(40,248,brainCell-12,28);brainIntensity.setBounds(40+brainCell,248,brainCell-12,28);
        brainEq.setBounds(40+2*brainCell,248,64,28);brainLimiter.setBounds(108+2*brainCell,248,brainCell-80,28);brainAnalyse.setBounds(40+3*brainCell,242,brainCell-12,34);const int resultWidth=std::clamp(getWidth()/2-48,300,600);brainReport.setBounds(36+resultWidth,342,getWidth()-60-resultWidth,getHeight()-426);brainApply.setBounds(getWidth()-210,getHeight()-66,186,34);
        const int top=getHeight()-163,width=(getWidth()-64)/4;
        for(int i=0;i<4;++i){auto* label=std::array<juce::Label*,4>{&frequencyLabel,&gainLabel,&qualityLabel,&ceilingLabel}[i];auto* slider=std::array<juce::Slider*,4>{&frequency,&gain,&quality,&ceiling}[i];label->setBounds(32+i*width,top,width-12,22);slider->setBounds(32+i*width,top+22,width-16,113);}
        driveLabel.setBounds(32,top,width-12,22);drive.setBounds(32,top+22,width-16,113);
        releaseLabel.setBounds(32+width,top,width-12,22);release.setBounds(32+width,top+22,width-16,113);
        bandChoice.setBounds(28,140,110,26);typeChoice.setBounds(148,140,155,26);fftChoice.setBounds(313,140,130,26);bandEnabled.setBounds(453,140,74,26);
        eqEnabled.setBounds(537,140,72,26);limiterEnabled.setBounds(getWidth()-114,136,90,26);
    }
    bool isInterestedInDragSource(const SourceDetails& details) override {return details.description=="eq"||details.description=="limiter";}
    void itemDropped(const SourceDetails& details) override {
        if(details.localPosition.y<82||details.localPosition.y>132)return;
        bool right=details.localPosition.x>getWidth()/2;
        bool first=details.description=="eq"?right:!right;
        auto* parameter=processor.state.getParameter("order");processor.beginUiTransaction("Module order");parameter->beginChangeGesture();parameter->setValueNotifyingHost(first?1.f:0.f);parameter->endChangeGesture();processor.finishUiTransaction();repaint();
    }
    void mouseDown(const juce::MouseEvent& event) override {
        if(showingBrain||showingInspector||showingLimiter||!plot().expanded(12).contains(event.position))return;
        int nearest=selectedBand;float distance=event.position.getDistanceFrom(node(nearest));
        for(int i=0;i<16;++i){if(i!=selectedBand&&bandType(i)==0&&std::abs(value("gain",i))<.01f)continue;float d=event.position.getDistanceFrom(node(i));if(d<distance){distance=d;nearest=i;}}
        if(distance>14)return;
        selectBand(nearest);draggingNode=true;
        processor.beginUiTransaction("EQ node");
        processor.state.getParameter(parameterId("frequency",selectedBand))->beginChangeGesture();
        processor.state.getParameter(parameterId("gain",selectedBand))->beginChangeGesture();
    }
    void mouseDrag(const juce::MouseEvent& event) override {
        if(draggingNode){auto area=plot();float normal=juce::jlimit(0.f,1.f,(event.position.x-area.getX())/area.getWidth());
            setParameter("frequency",static_cast<float>(20*std::pow(sampleRate()*.5/20,normal)));
            if(gainApplies(selectedBand))setParameter("gain",juce::jlimit(-18.f,18.f,(area.getCentreY()-event.position.y)*36/area.getHeight()));repaint();return;}
        if(event.getMouseDownY()<82||event.getMouseDownY()>132||isDragAndDropActive())return;
        bool right=event.getMouseDownX()>getWidth()/2,first=processor.state.getRawParameterValue("order")->load()>.5f;
        startDragging((right?!first:first)?"limiter":"eq",this);
    }
    void mouseUp(const juce::MouseEvent& event) override {
        const bool moved=event.mouseWasDraggedSinceMouseDown();endDrag();
        if(!moved&&event.position.y>=84&&event.position.y<=132){bool first=processor.state.getRawParameterValue("order")->load()>.5f;showLimiter(event.position.x>getWidth()/2?!first:first);}
    }
    void mouseWheelMove(const juce::MouseEvent& event,const juce::MouseWheelDetails& wheel) override {
        if(!showingBrain&&!showingInspector&&!showingLimiter&&plot().contains(event.position)&&event.position.getDistanceFrom(node(selectedBand))<24){setParameter("q",juce::jlimit(.1f,10.f,value("q",selectedBand)*std::exp(wheel.deltaY*2)),true);repaint();}
        else AudioProcessorEditor::mouseWheelMove(event,wheel);
    }
    void paint(juce::Graphics& g) override {
        g.fillAll(background);
        g.setGradientFill(juce::ColourGradient(juce::Colour(0xff1c2d3b),0,0,background,static_cast<float>(getWidth()),100,false));g.fillRect(0,0,getWidth(),76);
        g.setColour(accent);g.fillRoundedRectangle(24.f,19.f,5.f,32.f,2);
        g.setColour(juce::Colours::whitesmoke);g.setFont(juce::Font(juce::FontOptions(23.f,juce::Font::bold)));g.drawText("MASTERLAB",42,12,190,30,juce::Justification::left);
        g.setColour(accent);g.drawText("X",190,12,40,30,juce::Justification::left);
        g.setColour(muted);g.setFont(11);g.drawText("JERZY  /  MODULAR MASTERING",42,43,300,18,juce::Justification::left);
        if(getWidth()>=1000)g.drawText(juce::String(sampleRate()/1000,1)+" kHz   /   STEREO   /   DEVELOPMENT",getWidth()-335,14,310,22,juce::Justification::right);
        const int dock=getHeight()-174,cell=(getWidth()-64)/4;
        for(int i=0;i<4;++i){g.setColour(panel);g.fillRoundedRectangle(static_cast<float>(24+i*cell),static_cast<float>(dock),static_cast<float>(cell-8),146.f,7);g.setColour(juce::Colour(0xff2b3a49));g.drawRoundedRectangle(static_cast<float>(24+i*cell)+.5f,static_cast<float>(dock)+.5f,static_cast<float>(cell-9),145.f,7,1);}
        bool first=processor.state.getRawParameterValue("order")->load()>.5f;
        for(int i=0;i<2;++i){int x=24+i*(getWidth()/2);bool isEq=i==0?!first:first,active=!showingBrain&&!showingInspector&&(isEq?!showingLimiter:showingLimiter);auto colour=isEq?accent:curve;
            g.setColour(active?juce::Colour(0xff20323c):panel);g.fillRoundedRectangle(static_cast<float>(x),84.f,getWidth()*.5f-48,48,6);
            g.setColour(active?colour:juce::Colour(0xff354553));g.drawRoundedRectangle(static_cast<float>(x)+.5f,84.5f,getWidth()*.5f-49,47,6,1);
            g.setColour(colour);g.setFont(12);g.drawText("0"+juce::String(i+1),x+12,92,30,24,juce::Justification::left);
            g.setColour(juce::Colours::whitesmoke);g.setFont(juce::Font(juce::FontOptions(15.f,juce::Font::bold)));g.drawText(isEq?"SPECTRAL EQ":"LIMITER",x+50,90,getWidth()/2-116,22,juce::Justification::left);
            g.setColour(muted);g.setFont(11);g.drawText(isEq?"16-band EQ  /  drag to reorder":"Lookahead dynamics  /  drag to reorder",x+50,111,getWidth()/2-116,16,juce::Justification::left);
        }
        if(showingBrain){paintBrain(g);return;}
        if(showingInspector){paintInspector(g);return;}
        if(showingLimiter){paintLimiter(g);g.setColour(first?curve:muted);g.setFont(11);g.drawText(first?"EQ follows limiter: final output may exceed limiter ceiling":"SAMPLE PEAK  /  True Peak unavailable",24,getHeight()-25,getWidth()-48,18,juce::Justification::left);return;}
        auto area=plot();g.setColour(juce::Colour(0xff141c26));g.fillRoundedRectangle(area.expanded(24,12),8);
        g.setFont(11);
        for(float db:{-18.f,-12.f,-6.f,0.f,6.f,12.f,18.f}){float y=yForGain(db);g.setColour(db==0?juce::Colour(0xff536374):juce::Colour(0xff273240));g.drawHorizontalLine(static_cast<int>(y),area.getX(),area.getRight());g.setColour(muted);g.drawText(juce::String(db,0),8,static_cast<int>(y)-8,32,16,juce::Justification::right);}
        for(float hz:{20.f,50.f,100.f,200.f,500.f,1000.f,2000.f,5000.f,10000.f,20000.f}){if(hz>sampleRate()*.5)continue;float x=xForFrequency(hz);g.setColour(juce::Colour(0xff273240));g.drawVerticalLine(static_cast<int>(x),area.getY(),area.getBottom());g.setColour(muted);g.drawText(juce::String(hz>=1000?hz/1000:hz,0)+(hz>=1000?"k":""),static_cast<int>(x)-14,static_cast<int>(area.getBottom())+4,40,18,juce::Justification::centred);}
        {juce::Graphics::ScopedSaveState save(g);g.reduceClipRegion(area.toNearestInt());juce::Path spectral,total;
            for(int i=0;i<512;++i){double hz=20*std::pow(sampleRate()*.5/20,i/511.);float x=area.getX()+area.getWidth()*i/511.f;float binPosition=juce::jlimit(0.f,static_cast<float>((1<<fftOrder)/2-1),static_cast<float>(hz*(1<<fftOrder)/sampleRate()));int bin=static_cast<int>(binPosition);float level=spectrum[bin]+(binPosition-bin)*(spectrum[bin+1]-spectrum[bin]);float db=juce::Decibels::gainToDecibels(level,-100.f);float sy=area.getBottom()-area.getHeight()*juce::jlimit(0.f,1.f,(db+100)/100.f);
                if(i==0)spectral.startNewSubPath(x,sy);else spectral.lineTo(x,sy);
                if(haveResponse){std::complex<double> transfer{1,0};for(size_t k=0;k<16;++k)transfer*=(1.-response.blends[k])+static_cast<double>(response.blends[k])*response.bands[k].response(response.rate,hz);transfer=(1.-response.eqMix)+static_cast<double>(response.eqMix)*transfer;float y=yForGain(static_cast<float>(20*std::log10(std::max(1e-12,std::abs(transfer)))));if(i==0)total.startNewSubPath(x,y);else total.lineTo(x,y);}}
            auto spectralFill=spectral;spectralFill.lineTo(area.getRight(),area.getBottom());spectralFill.lineTo(area.getX(),area.getBottom());spectralFill.closeSubPath();g.setGradientFill(juce::ColourGradient(accent.withAlpha(.22f),0,area.getY(),accent.withAlpha(.01f),0,area.getBottom(),false));g.fillPath(spectralFill);g.setColour(accent.withAlpha(.8f));g.strokePath(spectral,juce::PathStrokeType(1.5f));
            if(haveResponse){juce::Path individual;for(int i=0;i<512;++i){double hz=20*std::pow(sampleRate()*.5/20,i/511.);float x=area.getX()+area.getWidth()*i/511.f;float y=yForGain(static_cast<float>(20*std::log10(std::max(1e-12,std::abs((1.-response.blends[selectedBand])+static_cast<double>(response.blends[selectedBand])*response.bands[selectedBand].response(response.rate,hz))))));if(i==0)individual.startNewSubPath(x,y);else individual.lineTo(x,y);}g.setColour(curve.withAlpha(.3f));g.strokePath(individual,juce::PathStrokeType(1));g.setColour(curve);g.strokePath(total,juce::PathStrokeType(2));}
            for(int i=0;i<16;++i){if(i!=selectedBand&&bandType(i)==0&&std::abs(value("gain",i))<.01f)continue;auto position=node(i);g.setColour(processor.state.getRawParameterValue("bandEnabled"+juce::String(i+1))->load()>.5f?(i==selectedBand?curve:muted):juce::Colour(0xff596575));g.fillEllipse(position.x-5,position.y-5,10,10);if(i==selectedBand){g.drawEllipse(position.x-9,position.y-9,18,18,1.5f);g.setFont(11);g.drawText(juce::String(i+1),static_cast<int>(position.x)+13,static_cast<int>(position.y)-12,26,22,juce::Justification::left);}}
        }
        int inspectorX=getWidth()-218;g.setColour(panel);g.fillRoundedRectangle(static_cast<float>(inspectorX),162.f,194.f,static_cast<float>(getHeight()-370),8);g.setColour(muted);g.setFont(12);g.drawText("STEREO INSPECTOR",inspectorX+12,174,170,20,juce::Justification::left);
        const float radius=std::min(56.f,(getHeight()-488.f)*.5f),cx=inspectorX+97.f,cy=204.f+radius;g.setColour(juce::Colour(0xff3c4c5c));g.drawEllipse(cx-radius,cy-radius,2*radius,2*radius,1);g.drawLine(cx-radius,cy,cx+radius,cy);g.drawLine(cx,cy-radius,cx,cy+radius);
        {juce::Graphics::ScopedSaveState save(g);g.reduceClipRegion(juce::Rectangle<int>(inspectorX+24,204,146,static_cast<int>(radius*2)));g.setColour(accent.withAlpha(.45f));for(auto point:scope)g.fillEllipse(cx+point.x*radius-1,cy-point.y*radius-1,2,2);}
        g.setColour(juce::Colours::whitesmoke);g.drawText("CORR "+juce::String(processor.correlation.load(),3),inspectorX+12,static_cast<int>(cy+radius+10),170,20,juce::Justification::left);
        g.setColour(muted);g.setFont(11);g.drawText("400 ms sliding / output",inspectorX+12,static_cast<int>(cy+radius+34),170,20,juce::Justification::left);
        const float correlation=juce::jlimit(-1.f,1.f,processor.correlation.load());
        const float barY=cy+radius+64;
        g.setColour(juce::Colour(0xff0a1119));g.fillRoundedRectangle(inspectorX+16.f,barY,162,6,3);
        g.setColour(correlation<0?juce::Colour(0xffe68a79):accent);float correlationX=inspectorX+97.f+correlation*81.f;g.fillRect(std::min(inspectorX+97.f,correlationX),barY,std::max(1.f,std::abs(correlation)*81.f),6.f);
        if(getHeight()>650){g.setColour(muted);g.setFont(11);g.drawText("OUTPUT PEAK / dBFS",inspectorX+16,static_cast<int>(barY)+22,162,18,juce::Justification::left);
            g.setColour(juce::Colours::whitesmoke);g.setFont(24);g.drawText(juce::String(juce::Decibels::gainToDecibels(processor.peak.load()),1),inspectorX+16,static_cast<int>(barY)+42,162,30,juce::Justification::left);
            g.setColour(muted);g.setFont(12);g.drawText("RMS  "+juce::String(juce::Decibels::gainToDecibels(processor.rms.load()),1)+" dBFS",inspectorX+16,static_cast<int>(barY)+80,162,18,juce::Justification::left);
        }
        g.setColour(muted);g.setFont(11);g.drawText("FFT: L/R energy  /  gold: EQ response",48,static_cast<int>(area.getBottom())+25,getWidth()-300,18,juce::Justification::left);
        g.setColour(muted);g.setFont(11);if(!first)g.drawText("Drag nodes to shape tone  /  Mouse wheel adjusts Q",24,getHeight()-25,getWidth()-48,18,juce::Justification::left);
        if(first){g.setColour(curve);g.drawText("EQ follows limiter: final output may exceed limiter ceiling",24,getHeight()-25,getWidth()-48,18,juce::Justification::left);}
    }
};
