#pragma once
#include "Analysis.h"
#include "StudioTheme.h"
#include "ModulePanel.h"
#include "ParallelPanel.h"
#include "ExciterPanel.h"
#include "AnalogColorPanel.h"
#include "AnalogizerPanel.h"
#include "RestorationPanel.h"
#include "ProcessingModePanel.h"
#include "BassArchitectPanel.h"
#include "TransientSculptorPanel.h"
#include "FinalTruePeakPanel.h"
#include "ConceptCOverviewPanel.h"

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
    std::array<std::unique_ptr<ModulePanel>,4> modulePanels;
    ExciterPanel exciterPanel;
    AnalogColorPanel analogPanel;
    AnalogizerPanel analogizerPanel;
    RestorationPanel restorationPanel;
    ProcessingModePanel modePanel;
    BassArchitectPanel bassPanel;
    TransientSculptorPanel transientPanel;
    FinalTruePeakPanel truePeakPanel;
    ConceptCOverviewPanel overviewPanel;
    ParallelPanel parallelPanel;
    juce::TextButton consoleView{"Console"};
    bool showingConsole=false;
    juce::TextButton dynamicView{"Dynamic EQ"},dynamicsView{"Dynamics"},stereoView{"Stereo"},stabilizerView{"Stabilizer"},modulesButton{"+ Modules"};
    int moduleView=-1,chainDragKind=-1,chainHover=-1;bool chainDragging=false,showingParallel=false,showingExciter=false,showingAnalog=false,showingAnalogizer=false,showingRestoration=false,showingMode=false,showingBass=false,showingTransient=false,showingTruePeak=false;
    juce::ComboBox routingChoice;std::unique_ptr<ComboAttachment> routingAttachment;
    // Legacy kind IDs remain stable for host automation. Only these ten DSP
    // stages are active in serial routing; kind 10 is locked at the tail.
    inline static constexpr std::array<const char*,12> routePresentIds{
      "dynamicEqPresent","dynamicsPresent","stereoPresent","eqPresent","limPresent","stabilizerPresent",
      "exciterPresent","analogPresent","bassPresent","transientPresent","truePeakPresent","restorationPresent"};
    inline static constexpr std::array<const char*,12> routeOnIds{
      "dynamicEqOn","dynamicsOn","stereoOn","eq","lim","stabilizerOn",
      "exciterOn","analogOn","bassOn","transientOn","truePeakOn","restorationOn"};
    inline static constexpr std::array<const char*,12> routeNames{
      "Dynamic EQ","Smart Dynamics","Stereo Architect","Spectral EQ","Legacy Limiter","Stabilizer",
      "Analog Exciter","Legacy Color","Bass Architect","Transient","Final Limiter","Restoration"};
    bool usingFullRouter()const {
      return processor.state.getRawParameterValue("routingMode")->load()<.5f;
    }
    masterlab::FullModuleOrder fullRouteOrder()const {
      masterlab::FullModuleOrder result{};
      for(size_t i=0;i<result.size();++i)
       result[i]=static_cast<int>(processor.state.getRawParameterValue(
         "fullRouteSlot"+juce::String(static_cast<int>(i+1)))->load());
      return masterlab::sanitizeFullOrder(result);
    }
    void writeFullRoute(masterlab::FullModuleOrder newOrder) {
      newOrder=masterlab::sanitizeFullOrder(newOrder);
      processor.beginUiTransaction("Reorder ten-stage audio chain");
      for(size_t i=0;i<newOrder.size();++i){
       auto* p=processor.state.getParameter("fullRouteSlot"+juce::String(static_cast<int>(i+1)));
       p->beginChangeGesture();
       p->setValueNotifyingHost(p->convertTo0to1(static_cast<float>(newOrder[i])));
       p->endChangeGesture();
      }
      processor.finishUiTransaction();
      repaint();
    }
    std::vector<int> chainModules() const {
      std::vector<int> result;
      if(usingFullRouter()){
        for(int kind:fullRouteOrder())result.push_back(kind); // OFF cards remain visible
      }else{
        const auto order=masterlab::decodeOrder(static_cast<int>(processor.state.getRawParameterValue("chainOrder")->load()),processor.state.getRawParameterValue("order")->load()>.5f);
        for(int kind:order)result.push_back(kind);
      }
      return result;
    }
    bool chainActive(int kind)const {
      auto on=[this](const char* present,const char* active){
       return processor.state.getRawParameterValue(present)->load()>.5f &&
              processor.state.getRawParameterValue(active)->load()>.5f;
      };
      if(kind==6)return on("exciterPresent","exciterOn")||
                         on("analogizerPresent","analogizerOn")||
                         on("analogPresent","analogOn");
      if(kind==10)return on("truePeakPresent","truePeakOn");
      return kind>=0&&kind<static_cast<int>(routeOnIds.size()) &&
             on(routePresentIds[static_cast<size_t>(kind)],routeOnIds[static_cast<size_t>(kind)]);
    }
    void toggleChainStage(int kind) {
      if(kind<0||kind>=static_cast<int>(routeOnIds.size()))return;
      const bool target=!chainActive(kind);
      processor.beginUiTransaction(target?"Enable DSP stage":"Disable DSP stage");
      auto assign=[this,target](const char* id){
       auto* p=processor.state.getParameter(id);
       p->beginChangeGesture();
       p->setValueNotifyingHost(p->convertTo0to1(target?1.f:0.f));
       p->endChangeGesture();
      };
      if(kind==6){
       for(const char* id:{"exciterPresent","exciterOn","analogPresent","analogOn",
                           "analogizerPresent","analogizerOn"})assign(id);
      }else if(kind==10){
       for(const char* id:{"truePeakPresent","truePeakOn","limPresent","lim"})assign(id);
      }else {
       assign(routePresentIds[static_cast<size_t>(kind)]);
       assign(routeOnIds[static_cast<size_t>(kind)]);
      }
      processor.finishUiTransaction();
      repaint();
    }
    int chainAt(float x,float y)const {
      auto list=chainModules();
      if(list.empty()||x<24.f||x>getWidth()-24.f)return -1;
      if(usingFullRouter()){
       if(y<80.f||y>=168.f)return -1;
       const int col=std::clamp(static_cast<int>((x-24.f)/((getWidth()-48.f)/5.f)),0,4);
       const int row=y<124.f?0:1;
       const size_t index=static_cast<size_t>(row*5+col);
       return index<list.size()?list[index]:-1;
      }
      if(y<80.f||y>132.f)return -1;
      size_t i=static_cast<size_t>((x-24.f)/((getWidth()-48.f)/list.size()));
      return i<list.size()?list[i]:-1;
    }
    void drawChainIcon(juce::Graphics& g,int kind,float x,float y)const {
      const auto tint=juce::Colour(0xffd7e4ed);
      g.setColour(tint);
      auto line=[&](float x1,float y1,float x2,float y2){g.drawLine(x+x1,y+y1,x+x2,y+y2,1.6f);};
      switch(kind){
       case 3: // Spectral EQ — sliders
        for(int a=0;a<3;++a){line(1.f,3.f+a*6.f,19.f,3.f+a*6.f);
          g.fillEllipse(x+(a==1?11.f:6.f),y+1.f+a*6.f,4.f,4.f);}break;
       case 0:case 1:case 9:{ // Dynamics and transients — actual wave motif
        juce::Path p;p.startNewSubPath(x+1.f,y+10.f);
        p.lineTo(x+5.f,y+10.f);p.lineTo(x+8.f,y+3.f);
        p.lineTo(x+12.f,y+17.f);p.lineTo(x+15.f,y+8.f);
        p.lineTo(x+19.f,y+10.f);g.strokePath(p,juce::PathStrokeType(1.6f));break;}
       case 2: // Stereo
        g.drawEllipse(x+1.f,y+4.f,10.f,12.f,1.5f);
        g.drawEllipse(x+9.f,y+4.f,10.f,12.f,1.5f);break;
       case 5:case 11: // Stabilizer / Restoration
        for(int a=0;a<3;++a)line(1.f,y-y+4.f+a*6.f,19.f,4.f+a*6.f);break;
       case 6:{ // Analog Exciter — lightning
        juce::Path p;p.startNewSubPath(x+11.f,y+1.f);p.lineTo(x+4.f,y+11.f);
        p.lineTo(x+10.f,y+11.f);p.lineTo(x+8.f,y+19.f);
        p.lineTo(x+18.f,y+7.f);p.lineTo(x+12.f,y+7.f);
        p.closeSubPath();g.strokePath(p,juce::PathStrokeType(1.5f));break;}
       case 8: // Bass spectrum
        g.fillRect(x+2.f,y+12.f,3.f,7.f);g.fillRect(x+8.f,y+4.f,3.f,15.f);
        g.fillRect(x+14.f,y+9.f,3.f,10.f);break;
       case 10:{ // Final limiter shield
        juce::Path p;p.startNewSubPath(x+10.f,y+1.f);p.lineTo(x+18.f,y+5.f);
        p.lineTo(x+17.f,y+13.f);p.lineTo(x+10.f,y+19.f);
        p.lineTo(x+3.f,y+13.f);p.lineTo(x+2.f,y+5.f);
        p.closeSubPath();g.strokePath(p,juce::PathStrokeType(1.6f));break;}
       default:line(2.f,10.f,18.f,10.f);break;
      }
    }
    void hideModules(){showingConsole=false;overviewPanel.setVisible(false);showingTruePeak=false;truePeakPanel.setVisible(false);showingTransient=false;transientPanel.setVisible(false);showingBass=false;bassPanel.setVisible(false);showingAnalog=false;analogPanel.setVisible(false);showingAnalogizer=false;analogizerPanel.setVisible(false);showingRestoration=false;restorationPanel.setVisible(false);showingMode=false;modePanel.setVisible(false);showingExciter=false;exciterPanel.setVisible(false);moduleView=-1;for(auto& panel:modulePanels)if(panel)panel->setVisible(false);dynamicView.setToggleState(false,juce::dontSendNotification);dynamicsView.setToggleState(false,juce::dontSendNotification);stereoView.setToggleState(false,juce::dontSendNotification);stabilizerView.setToggleState(false,juce::dontSendNotification);}
    void showConsole(){showInspector();showingInspector=false;inspectorView.setToggleState(false,juce::dontSendNotification);resetMetersButton.setVisible(false);showingConsole=true;overviewPanel.setVisible(true);repaint();}
    void showModule(int kind){showInspector();resetMetersButton.setVisible(false);showingInspector=false;inspectorView.setToggleState(false,juce::dontSendNotification);moduleView=kind;modulePanels[static_cast<size_t>(kind)]->setVisible(true);std::array<juce::TextButton*,4>{&dynamicView,&dynamicsView,&stereoView,&stabilizerView}[static_cast<size_t>(kind)]->setToggleState(true,juce::dontSendNotification);repaint();}
    void toggleParameter(const char* id){auto* parameter=processor.state.getParameter(id);processor.beginUiTransaction("Module chain");parameter->beginChangeGesture();parameter->setValueNotifyingHost(parameter->getValue()>.5f?0.f:1.f);parameter->endChangeGesture();processor.finishUiTransaction();repaint();}

    juce::TooltipWindow tooltips{this, 600};
    juce::ComboBox bandChoice, fftChoice, typeChoice;
    juce::ToggleButton bandEnabled{"Band"};
    std::unique_ptr<ComboAttachment> typeAttachment;
    std::unique_ptr<ButtonAttachment> bandEnabledAttachment;
    juce::TextButton resetMetersButton{"RESET METERS"};
    juce::TextButton undoButton{"Undo"},redoButton{"Redo"},presetButton{"Presets"},newButton{"NEW"};
    std::unique_ptr<juce::FileChooser> presetChooser;
    juce::TextButton eqView{"Spectral EQ"}, limiterView{"Final Limiter"}, inspectorView{"Inspector"}, brainView{"Master Brain"};
    bool showingLimiter=false,showingInspector=false,showingBrain=false;
    juce::ComboBox brainGenre,brainStyle,brainDuration;
    juce::Slider brainTarget,brainCeiling,brainIntensity;
    std::array<juce::Label,6> brainLabels;
    juce::ToggleButton brainEq{"EQ"},brainLimiter{"Limiter"};
    juce::TextButton brainModulesButton{"Processors..."};bool brainUseStabilizer=true,brainAutomatic=true,brainUseStereo=true,brainUseDynamics=true,brainUseDynamicEq=true;
    bool brainUseExciter=false,brainUseAnalog=false,brainUseBass=false,brainUseTransient=false,brainUseTruePeak=false;
    juce::TextButton brainAnalyse{"Analyse audio"},brainApply{"Apply changes"},brainAuditionA{"Audition A"},brainAuditionB{"Audition B"};
    juce::TextEditor brainReport;double brainProgress=0;juce::String brainProgressText;
    juce::String previousBrainReport;masterlab::BrainSnapshot brainSnapshot;
    bool brainControlsUpdating=false;
    std::array<masterlab::LimiterFrame,1000> limiterHistory{};
    size_t historyHead=0,historyCount=0;
    double lastLimiterUpdate=0;
    void showLimiter(bool enabled){
        hideModules();endDrag();setBrainVisible(false);parallelPanel.setVisible(false);showingParallel=false;resetMetersButton.setVisible(false);showingInspector=false;showingLimiter=enabled;inspectorView.setToggleState(false,juce::dontSendNotification);
        for(auto* component:std::array<juce::Component*,11>{&bandChoice,&fftChoice,&frequency,&gain,&quality,&frequencyLabel,&gainLabel,&qualityLabel,&eqEnabled,&ceiling,&ceilingLabel})component->setVisible(!enabled);
        typeChoice.setVisible(!enabled);bandEnabled.setVisible(!enabled);
        ceiling.setVisible(true);ceilingLabel.setVisible(true);limiterEnabled.setVisible(true);
        drive.setVisible(enabled);driveLabel.setVisible(enabled);release.setVisible(enabled);releaseLabel.setVisible(enabled);
        eqView.setToggleState(!enabled,juce::dontSendNotification);limiterView.setToggleState(enabled,juce::dontSendNotification);repaint();
    }
    void showInspector(){
        hideModules();endDrag();setBrainVisible(false);parallelPanel.setVisible(false);showingParallel=false;resetMetersButton.setVisible(true);showingInspector=true;showingLimiter=false;
        for(auto* component:std::initializer_list<juce::Component*>{&frequency,&gain,&quality,&ceiling,&drive,&release,&frequencyLabel,&gainLabel,&qualityLabel,&ceilingLabel,&driveLabel,&releaseLabel,&bandChoice,&fftChoice,&typeChoice,&bandEnabled,&eqEnabled,&limiterEnabled})component->setVisible(false);
        eqView.setToggleState(false,juce::dontSendNotification);limiterView.setToggleState(false,juce::dontSendNotification);inspectorView.setToggleState(true,juce::dontSendNotification);repaint();
    }
    void showFinalTruePeak(){showInspector();resetMetersButton.setVisible(false);showingInspector=false;inspectorView.setToggleState(false,juce::dontSendNotification);showingTruePeak=true;truePeakPanel.setVisible(true);repaint();}
    void showTransient(){showInspector();resetMetersButton.setVisible(false);showingInspector=false;inspectorView.setToggleState(false,juce::dontSendNotification);showingTransient=true;transientPanel.setVisible(true);repaint();}
    void showBass(){showInspector();resetMetersButton.setVisible(false);showingInspector=false;inspectorView.setToggleState(false,juce::dontSendNotification);showingBass=true;bassPanel.setVisible(true);repaint();}
    void showWorkMode(){showInspector();resetMetersButton.setVisible(false);showingInspector=false;inspectorView.setToggleState(false,juce::dontSendNotification);showingMode=true;modePanel.setVisible(true);repaint();}
    void showRestoration(){showInspector();resetMetersButton.setVisible(false);showingInspector=false;inspectorView.setToggleState(false,juce::dontSendNotification);showingRestoration=true;restorationPanel.setVisible(true);repaint();}
    void showAnalogizer(){showInspector();resetMetersButton.setVisible(false);showingInspector=false;inspectorView.setToggleState(false,juce::dontSendNotification);showingAnalogizer=true;analogizerPanel.setVisible(true);repaint();}
    void showAnalog(){showInspector();resetMetersButton.setVisible(false);showingInspector=false;inspectorView.setToggleState(false,juce::dontSendNotification);showingAnalog=true;analogPanel.setVisible(true);repaint();}
    void showExciter(){showInspector();resetMetersButton.setVisible(false);showingInspector=false;inspectorView.setToggleState(false,juce::dontSendNotification);showingExciter=true;exciterPanel.setVisible(true);repaint();}
    void showParallel(){showInspector();resetMetersButton.setVisible(false);showingInspector=false;inspectorView.setToggleState(false,juce::dontSendNotification);showingParallel=true;parallelPanel.setVisible(true);repaint();}
    void setBrainVisible(bool visible){
        showingBrain=visible;brainView.setToggleState(visible,juce::dontSendNotification);
        for(auto* component:std::initializer_list<juce::Component*>{&brainGenre,&brainStyle,&brainDuration,&brainTarget,&brainCeiling,&brainIntensity,&brainModulesButton,&brainAnalyse,&brainApply,&brainAuditionA,&brainAuditionB,&brainReport})component->setVisible(visible);
        for(auto& label:brainLabels)label.setVisible(visible);
    }
    void reflectFactoryBrainControls(){
        const masterlab::BrainConfig defaults{};
        brainGenre.setSelectedId(defaults.genre+1,juce::dontSendNotification);
        brainStyle.setSelectedId(defaults.style+1,juce::dontSendNotification);
        brainDuration.setSelectedId(defaults.seconds,juce::dontSendNotification);
        brainTarget.setValue(defaults.targetLufs,juce::dontSendNotification);
        brainCeiling.setValue(defaults.ceiling,juce::dontSendNotification);
        brainIntensity.setValue(defaults.intensity,juce::dontSendNotification);
        brainUseStabilizer=defaults.useStabilizer;
        brainAutomatic=defaults.automaticModules;
        brainUseStereo=defaults.useStereo;
        brainUseDynamics=defaults.useDynamics;
        brainUseDynamicEq=defaults.useDynamicEq;
        brainEq.setToggleState(defaults.useEq,juce::dontSendNotification);
        brainLimiter.setToggleState(defaults.useLimiter,juce::dontSendNotification);
    }
    void showBrain(){showInspector();resetMetersButton.setVisible(false);showingInspector=false;inspectorView.setToggleState(false,juce::dontSendNotification);setBrainVisible(true);refreshBrain();repaint();}
    masterlab::BrainConfig brainConfig() const {
        masterlab::BrainConfig config;config.genre=brainGenre.getSelectedId()-1;config.style=brainStyle.getSelectedId()-1;config.seconds=brainDuration.getSelectedId();config.targetLufs=static_cast<float>(brainTarget.getValue());config.ceiling=static_cast<float>(brainCeiling.getValue());config.intensity=static_cast<float>(brainIntensity.getValue());config.useEq=brainEq.getToggleState();config.useLimiter=brainLimiter.getToggleState();config.useStabilizer=brainUseStabilizer;config.automaticModules=brainAutomatic;config.useStereo=brainUseStereo;config.useDynamics=brainUseDynamics;config.useDynamicEq=brainUseDynamicEq;
        config.useExciter=brainUseExciter;config.useAnalog=brainUseAnalog;config.useBass=brainUseBass;
        config.useTransient=brainUseTransient;config.useTruePeak=brainUseTruePeak;return config;
    }
    void refreshBrain(){
        brainSnapshot=processor.brain.readUI();const auto& snapshot=brainSnapshot;const bool active=snapshot.phase==masterlab::BrainPhase::Pending||snapshot.phase==masterlab::BrainPhase::Analysing||snapshot.phase==masterlab::BrainPhase::Validating;
        brainProgress=active?snapshot.progress:((snapshot.phase==masterlab::BrainPhase::Ready||snapshot.phase==masterlab::BrainPhase::Applied)?1.:0.);
        brainProgressText=snapshot.phase==masterlab::BrainPhase::Validating?"VERIFYING ACTUAL DSP":active?juce::String(snapshot.progress*100,0)+"% AUDIO CAPTURED":(brainProgress==1.?"ANALYSIS COMPLETE":"READY TO ANALYSE");
        brainModulesButton.setButtonText(juce::String(static_cast<int>(snapshot.config.useStabilizer)+static_cast<int>(snapshot.config.useEq)+static_cast<int>(snapshot.config.useLimiter)+static_cast<int>(snapshot.config.useStereo)+static_cast<int>(snapshot.config.useDynamics)+static_cast<int>(snapshot.config.useDynamicEq)+static_cast<int>(snapshot.config.useExciter)+static_cast<int>(snapshot.config.useAnalog)+static_cast<int>(snapshot.config.useBass)+static_cast<int>(snapshot.config.useTransient)+static_cast<int>(snapshot.config.useTruePeak))+" allowed");
        brainAnalyse.setButtonText(active?"Cancel analysis":"Analyse audio");
        for(auto* component:std::initializer_list<juce::Component*>{&brainGenre,&brainStyle,&brainDuration,&brainTarget,&brainCeiling,&brainIntensity,&brainModulesButton})component->setEnabled(!active);
        const bool stale=processor.brainStaleUI();
        const bool canAudition=snapshot.phase==masterlab::BrainPhase::Ready&&
                               snapshot.proposal.valid&&snapshot.proposal.previewVerified&&!stale;
        brainAuditionA.setEnabled(canAudition);
        brainAuditionB.setEnabled(canAudition&&snapshot.alternateReady);
        brainAuditionA.setToggleState(snapshot.selectedVariant==0&&canAudition,juce::dontSendNotification);
        brainAuditionB.setToggleState(snapshot.selectedVariant==1&&canAudition,juce::dontSendNotification);
        brainApply.setEnabled(snapshot.phase==masterlab::BrainPhase::Ready&&snapshot.proposal.valid&&!stale);
        juce::String report=juce::String(snapshot.message)+"\n"+
                            "ACTION: "+processor.brainActionMessage+"\n";
        if(snapshot.alternateReady&&snapshot.phase==masterlab::BrainPhase::Ready){
            const auto& alt=snapshot.alternateProposal;
            report+="A/B: selected "+juce::String(snapshot.selectedVariant==0?"A":"B")+
                    " | A style "+juce::String(masterlab::brainStyles[snapshot.config.style])+
                    " / B style "+juce::String(masterlab::brainStyles[snapshot.config.style==2?0:2])+
                    " | current "+juce::String(snapshot.proposal.previewLufs,1)+
                    " LUFS, GR "+juce::String(snapshot.proposal.previewGr95,1)+
                    " dB | alternate "+juce::String(alt.previewLufs,1)+
                    " LUFS, GR "+juce::String(alt.previewGr95,1)+
                    " dB. Click Audition A/B to hear changes through the real DSP.\n";
        }
        if(active)report+="Audio captured: "+juce::String(snapshot.progress*snapshot.config.seconds,1)+" / "+juce::String(snapshot.config.seconds)+" s. If playback stops, analysis waits.\n";
        if(snapshot.phase==masterlab::BrainPhase::Ready||snapshot.phase==masterlab::BrainPhase::Applied){
            const auto& f=snapshot.features;const auto& p=snapshot.proposal;
            report+="\nINPUT: "+lufsText(f.integrated)+" LUFS-I   |   peak "+dbText(f.peak)+" dBFS   |   correlation "+juce::String(f.correlation,3)+"\n";
            report+="Captured "+juce::String(f.seconds,1)+" s. Target "+juce::String(snapshot.config.targetLufs,1)+" LUFS.\n";
            if(!f.detail.sectionLevelsDb.empty()){
                report+="\nWHOLE-TRACK SECTIONS (chronological 3-second RMS, dBFS):\n";
                for(size_t section=0;section<f.detail.sectionLevelsDb.size();++section){
                    const int from=static_cast<int>(section)*3;
                    report+=juce::String(from)+"-"+juce::String(juce::jmin(from+3,static_cast<int>(f.seconds)))+
                            "s: "+juce::String(f.detail.sectionLevelsDb[section],1)+" dBFS";
                    report+=(section%3==2?"\n":"  |  ");
                }
                report+="\nLarge differences between sections can be intentional arrangements, not automatic errors.\n";
            }
            if(p.previewVerified)report+="\nDSP PREVIEW: "+juce::String(p.previewLufs,1)+" LUFS | Peak "+juce::String(20*std::log10(std::max(1e-9f,p.previewPeak)),1)+" dBFS | section spread "+juce::String(p.previewSectionSpread,1)+" dB | limiter GR p95 "+juce::String(p.previewGr95,1)+" dB\n";
            if(p.valid){report+="\nPROPOSED EQ (low to high): ";for(float gain:p.gains)report+=juce::String(gain,1)+" dB   ";report+="\nLimiter drive: "+juce::String(p.drive,1)+" dB. Estimated EQ level change: "+juce::String(p.eqEstimate,1)+" dB.\n";}
            if(p.targetShares[0]>0){
                report+="\nTONAL ENERGY %: input / genre target / EQ estimate\n";
                constexpr std::array<const char*,5> regions{"20-80 Hz","80-250 Hz","250 Hz-2 kHz","2-6 kHz","6-20 kHz"};
                for(size_t i=0;i<5;++i)report+=juce::String(regions[i])+": "+juce::String(100*p.shares[i],1)+" / "+juce::String(100*p.targetShares[i],1)+" / "+juce::String(100*p.estimatedShares[i],1)+"\n";
                report+="Static EQ estimate integrates 60 spectral bands; DSP PREVIEW measures the full chain separately. Genre target is a heuristic, not a quality standard.\n";
            }
            auto proposed=[&](const std::string& id){for(const auto& change:p.changes)if(change.first==id)return change.second;return 0.f;};
            if(p.valid){report+="\nPROPOSED CHAIN: ";int number=0;for(auto entry:std::array<std::pair<const char*,const char*>,6>{{{"stabilizerPresent","Stabilizer"},{"dynamicEqPresent","Dynamic EQ"},{"dynamicsPresent","Dynamics"},{"stereoPresent","Imager"},{"eqPresent","EQ"},{"limPresent","Limiter"}}})if(proposed(entry.first)>.5f){if(number++)report+=" > ";report+=entry.second;}if(!number)report+="No additional processing needed";report+="\n";}
            if(p.valid&&snapshot.config.useStereo){report+="\nSTEREO PROPOSAL: widths (low/mid/high/air) ";for(const char* id:{"stereoLowWidth","stereoMidWidth","stereoHighWidth","stereoAirWidth"})report+=juce::String(proposed(id),2)+" x  ";report+="\nMono bass: "+juce::String(proposed("stereoMonoBass")>.5f?"on":"off")+". Low split: "+juce::String(proposed("stereoLowSplit"),0)+" Hz.\nMeasured band correlation: ";for(float correlation:f.bandCorrelation)report+=juce::String(correlation,2)+"  ";report+="\n";}
            if(p.valid&&snapshot.config.useDynamics)report+="\nCOMPRESSOR PROPOSAL: "+juce::String(proposed("dynamicsMode")>.5f?"Analog":"Digital")+"; threshold "+juce::String(proposed("dynThreshold"),1)+" dBFS; ratio "+juce::String(proposed("dynRatio"),2)+":1; attack "+juce::String(proposed("dynAttack"),0)+" ms; release "+juce::String(proposed("dynRelease"),0)+" ms.\nUpward max "+juce::String(proposed("upwardAmount"),1)+" dB below "+juce::String(proposed("upwardThreshold"),1)+" dBFS; knee 6 dB; makeup "+juce::String(proposed("dynMakeup"),1)+" dB; wet 100%.\n";
            if(p.valid&&snapshot.config.useDynamicEq){report+="\nDYNAMIC EQ PROPOSAL: adaptive frequency, Q and detector threshold.\n";for(int band=1;band<=8;++band){auto suffix=std::to_string(band);report+=juce::String(proposed("deqFreq"+suffix),0)+" Hz: range "+juce::String(proposed("deqRange"+suffix),1)+" dB; Q "+juce::String(proposed("deqQ"+suffix),2)+"; threshold "+juce::String(proposed("deqThreshold"+suffix),1)+" dBFS.\n";}}
            for(const auto& line:p.reasons)report+="\n"+juce::String(line)+"\n";
            for(const auto& line:p.warnings)report+="\nNOTE: "+juce::String(line)+"\n";
            if(stale&&snapshot.phase==masterlab::BrainPhase::Ready)report="SETTINGS CHANGED: "+processor.brainStaleReasonUI()+
                                   "\nReanalyse before Apply.\n\n"+report;
        }
        if(report!=previousBrainReport){brainReport.setText(report,false);previousBrainReport=report;}
    }
    void paintBrain(juce::Graphics& g){
        g.setColour(background);g.fillRect(0,134,getWidth(),getHeight()-134);
        g.setColour(panel);g.fillRoundedRectangle(24.f,154.f,getWidth()-48.f,178.f,18);
        g.setColour(juce::Colour(0xff284760));g.setFont(theme.font(14));g.drawText("Master Brain  /  Guided mastering",40,134,getWidth()-80,20,juce::Justification::left);
        g.setColour(juce::Colour(0xff0a1119));g.fillRoundedRectangle(40.f,294.f,getWidth()-80.f,18.f,4.f);
        if(brainProgress>0){g.setColour(accent.withAlpha(.35f));g.fillRoundedRectangle(40.f,294.f,static_cast<float>((getWidth()-80)*brainProgress),18.f,4.f);}
        g.setColour(juce::Colours::whitesmoke);g.setFont(theme.font(11));g.drawText(brainProgressText,40,294,getWidth()-80,18,juce::Justification::centred);
        const int cardWidth=std::clamp(getWidth()/2-48,300,600),cardHeight=getHeight()-426;
        g.setColour(panel);g.fillRoundedRectangle(24.f,342.f,static_cast<float>(cardWidth),static_cast<float>(cardHeight),18.f);
        g.setColour(panel);g.fillRoundedRectangle(36.f+cardWidth,342.f,getWidth()-60.f-cardWidth,static_cast<float>(cardHeight),18.f);
        g.setColour(muted);g.setFont(theme.font(11));g.drawText("Tonal balance  /  dB from target",36,350,cardWidth-24,18,juce::Justification::left);
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
        g.setColour(juce::Colour(0xff3c5265));g.setFont(theme.font(13));g.drawText("Review all selected processors. Apply can replace settings; one Undo restores them.",40,getHeight()-66,getWidth()-280,36,juce::Justification::left);
        g.setColour(juce::Colour(0xff435769));g.setFont(theme.font(12));g.drawText("Explicit DSP rules / no trained AI model / sample-peak limiter",24,getHeight()-25,getWidth()-48,18,juce::Justification::left);
    }
    juce::String dbText(float level) const {return level>1e-10f?juce::String(juce::Decibels::gainToDecibels(level),1):juce::String("--");}
    juce::String lufsText(float value) const {return value>-119?juce::String(value,1):juce::String("--");}
    void paintInspector(juce::Graphics& g){
        auto area=plot();const bool compact=area.getHeight()<220;
        g.setColour(juce::Colour(0xff324d64));g.setFont(theme.font(14));g.drawText("Master Inspector  /  Input and output",48,144,getWidth()-300,20,juce::Justification::left);
        for(int i=0;i<2;++i){auto reading=i==0?response.inputReading:response.outputReading;auto loudness=i==0?response.inputLufs:response.outputLufs;float x=24.f+i*(area.getWidth()+48)/2,w=(area.getWidth()+48)/2-10;
            g.setColour(panel);g.fillRoundedRectangle(x,162,w,area.getHeight()+24,8);
            g.setColour(i==0?muted:accent);g.setFont(theme.font(13));g.drawText(i==0?"INPUT":"OUTPUT",static_cast<int>(x)+16,176,static_cast<int>(w)-32,20,juce::Justification::left);
            g.setColour(reading.peak>1?juce::Colour(0xffee927f):juce::Colours::whitesmoke);g.setFont(theme.font(compact?26.f:38.f));g.drawText(dbText(reading.peak),static_cast<int>(x)+16,compact?200:204,static_cast<int>(w)-32,44,juce::Justification::left);
            g.setColour(muted);g.setFont(theme.font(11));g.drawText("SAMPLE PEAK / dBFS",static_cast<int>(x)+16,244,static_cast<int>(w)-32,18,juce::Justification::left);
            const float barY=compact?270.f:280.f;float normal=juce::jlimit(0.f,1.f,(juce::Decibels::gainToDecibels(reading.peak,-60.f)+60)/60);
            g.setColour(juce::Colour(0xff0a1119));g.fillRoundedRectangle(x+16,barY,w-32,8,4);g.setColour(i==0?curve:accent);g.fillRoundedRectangle(x+16,barY,(w-32)*normal,8,4);
            g.setColour(muted);g.setFont(theme.font(12));g.drawText("RMS (400 ms)  "+dbText(reading.rms)+" dBFS",static_cast<int>(x)+16,compact?287:312,static_cast<int>(w)-32,22,juce::Justification::left);
            if(!compact){g.drawText("MOMENTARY  "+lufsText(loudness.momentary)+" LUFS",static_cast<int>(x)+16,340,static_cast<int>(w)-32,20,juce::Justification::left);
                g.drawText("SHORT-TERM  "+lufsText(loudness.shortTerm)+" LUFS",static_cast<int>(x)+16,366,static_cast<int>(w)-32,20,juce::Justification::left);
                if(area.getHeight()>290){g.setColour(reading.clippedSamples?juce::Colour(0xffee927f):muted);g.drawText("CLIP SAMPLES  "+juce::String(reading.clippedSamples),static_cast<int>(x)+16,404,static_cast<int>(w)-32,20,juce::Justification::left);
                    g.setColour(muted);}
            }
        }
        int x=getWidth()-218;g.setColour(panel);g.fillRoundedRectangle(static_cast<float>(x),162,194,area.getHeight()+36,8);g.setColour(muted);g.setFont(theme.font(12));g.drawText("STEREO / OUTPUT",x+16,176,162,20,juce::Justification::left);
        float radius=compact?42.f:56.f,cx=x+97.f,cy=compact?242.f:260.f;
        g.setColour(juce::Colour(0xff3c4c5c));g.drawEllipse(cx-radius,cy-radius,radius*2,radius*2,1);g.drawLine(cx-radius,cy,cx+radius,cy);g.drawLine(cx,cy-radius,cx,cy+radius);
        {juce::Graphics::ScopedSaveState save(g);g.reduceClipRegion(juce::Rectangle<float>(cx-radius,cy-radius,radius*2,radius*2).toNearestInt());g.setColour(accent.withAlpha(.5f));for(auto point:scope)g.fillEllipse(cx+point.x*radius-1,cy-point.y*radius-1,2,2);}
        g.setColour(juce::Colours::whitesmoke);g.setFont(theme.font(14));g.drawText("CORR  "+juce::String(response.outputReading.correlation,3),x+16,static_cast<int>(cy+radius)+12,162,24,juce::Justification::left);
        if(!compact){g.setColour(muted);g.setFont(theme.font(12));g.drawText("MID  "+dbText(response.outputReading.midRms)+" dBFS",x+16,360,162,20,juce::Justification::left);g.drawText("SIDE  "+dbText(response.outputReading.sideRms)+" dBFS",x+16,386,162,20,juce::Justification::left);
            if(area.getHeight()>290)g.drawText("HEADROOM  "+(response.outputReading.peak>0?juce::String(-juce::Decibels::gainToDecibels(response.outputReading.peak),1):juce::String("--"))+" dB",x+16,430,162,20,juce::Justification::left);
        }
        const int dock=getHeight()-174,cell=(getWidth()-64)/4;
        for(int i=0;i<4;++i){int left=40+i*cell;g.setColour(muted);g.setFont(theme.font(12));g.drawText(std::array<const char*,4>{"INPUT LUFS-I","OUTPUT LUFS-I","CORRELATION","ENERGY BALANCE"}[i],left,dock+14,cell-32,20,juce::Justification::left);
            auto reading=response.outputReading;juce::String text=i==0?lufsText(response.inputLufs.integrated):(i==1?lufsText(response.outputLufs.integrated):(i==2?juce::String(reading.correlation,3):(std::abs(reading.balance)<.001f?juce::String("CENTER"):juce::String(reading.balance<0?"L ":"R ")+juce::String(std::abs(reading.balance)*100,1)+"%")));
            g.setColour(i>=2?accent:juce::Colours::whitesmoke);g.setFont(theme.font(26));g.drawText(text,left,dock+43,cell-32,38,juce::Justification::left);
            g.setColour(muted);g.setFont(theme.font(11));g.drawText(i<2?"LUFS / gated integrated":(i==2?"-1 ... +1":"(R energy - L) / total"),left,dock+101,cell-32,24,juce::Justification::left);
        }
        g.setColour(muted);g.setFont(theme.font(11));g.drawText("LUFS: K-weighted / M 400 ms / S 3 s / I since reset   |   True Peak unavailable",24,getHeight()-25,getWidth()-48,18,juce::Justification::left);
    }
    void paintLimiter(juce::Graphics& g){
        const auto area=plot();
        const float split=area.getY()+area.getHeight()*.65f;
        const float grHeight=split-area.getY()-22;
        g.setColour(panel);g.fillRoundedRectangle(area.expanded(24,12),18);
        g.setColour(juce::Colour(0xff0c131c));g.fillRect(area);
        g.setFont(theme.font(12));
        for(int seconds=0;seconds<=10;seconds+=2){float x=area.getX()+area.getWidth()*seconds/10.f;g.setColour(juce::Colour(0xff24313e));g.drawVerticalLine(static_cast<int>(x),area.getY(),area.getBottom());g.setColour(muted);g.drawText(seconds==10?"NOW":juce::String(seconds-10)+" s",static_cast<int>(x)-18,static_cast<int>(area.getBottom())+5,40,18,juce::Justification::centred);}
        for(int db:{0,6,12,18,24}){float y=area.getY()+grHeight*db/24.f;g.setColour(juce::Colour(0xff293645));g.drawHorizontalLine(static_cast<int>(y),area.getX(),area.getRight());g.setColour(muted);g.drawText(juce::String(db),8,static_cast<int>(y)-8,32,16,juce::Justification::right);}
        g.setColour(curve);g.setFont(theme.font(12));g.drawText("GAIN REDUCTION",48,144,170,20,juce::Justification::left);
        g.setColour(muted);g.drawText("10 s  /  AUDIO CLOCK",getWidth()-476,144,224,20,juce::Justification::right);
        g.setColour(accent);g.setFont(theme.font(11));g.drawText("OUTPUT PEAK   -60 ... 0 dBFS",static_cast<int>(area.getX())+10,static_cast<int>(split)-18,240,18,juce::Justification::left);
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
        g.setColour(muted);g.setFont(theme.font(12));g.drawText("MASTER OUTPUT",x+16,176,162,20,juce::Justification::left);
        g.setColour(juce::Colours::whitesmoke);g.setFont(juce::Font(juce::FontOptions(34.f,juce::Font::bold)));g.drawText(juce::String(juce::Decibels::gainToDecibels(processor.peak.load()),1),x+16,202,150,42,juce::Justification::left);
        g.setColour(muted);g.setFont(theme.font(12));g.drawText("SAMPLE PEAK / dBFS",x+16,244,162,18,juce::Justification::left);
        g.setColour(curve);g.setFont(theme.font(24));g.drawText(juce::String(maximumReduction,1)+" dB",x+16,274,162,30,juce::Justification::left);
        g.setColour(muted);g.setFont(theme.font(11));g.drawText("MAX GR / VISIBLE HISTORY",x+16,307,166,18,juce::Justification::left);
        if(area.getHeight()>235){
            const float top=345,bottom=area.getBottom()-10,height=bottom-top;
            for(int k=0;k<2;++k){float level=k==0?processor.peak.load():processor.rms.load();float db=juce::Decibels::gainToDecibels(level,-60.f);float f=juce::jlimit(0.f,1.f,(db+60)/60);
                float bx=x+20.f+k*76;g.setColour(juce::Colour(0xff0a1119));g.fillRoundedRectangle(bx,top,52,height,4);
                g.setGradientFill(juce::ColourGradient(accent,bx,bottom-height,juce::Colour(0xff267d86),bx,bottom,false));g.fillRect(bx,bottom-height*f,52.f,height*f);
                g.setColour(muted);g.setFont(theme.font(11));g.drawText(k==0?"PEAK":"RMS",static_cast<int>(bx),static_cast<int>(bottom)+4,52,16,juce::Justification::centred);
            }
        }
        const int dock=getHeight()-174,cell=(getWidth()-64)/4;
        g.setColour(muted);g.setFont(theme.font(12));g.drawText("LOOKAHEAD",48+cell*2,dock+15,cell-32,20,juce::Justification::left);
        g.setColour(juce::Colours::whitesmoke);g.setFont(theme.font(26));g.drawText("5.0 ms",48+cell*2,dock+41,cell-32,32,juce::Justification::left);
        g.setColour(muted);g.setFont(theme.font(12));g.drawText("PDC  "+juce::String(processor.getLatencySamples())+" samples",48+cell*2,dock+88,cell-32,20,juce::Justification::left);
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
    std::array<float,16385> spectrum{},beforeSpectrum{};
    std::array<std::array<float,32768>,2> samples{},beforeSamples{};
    double lastAnalysis=0;
    std::array<juce::Point<float>,1024> scope{};
    int head = 0;
    const juce::Colour background{0xffebeff2}, panel{0xff202e3b}, muted{0xffa9b8c4};
    const juce::Colour accent{0xff82dfcd}, curve{0xffe7bb6a};

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
    juce::Colour bandColour(int band) const {
        constexpr std::array<juce::uint32,16> colours{0xff82dfcd,0xffefb96f,0xffb6a0ff,0xfff48ea1,0xff73c9f4,0xffc6df77,0xffdf9ded,0xffffa77d,0xff83aef4,0xff74dc9b,0xffe7d278,0xffa7b9d1,0xffd884b8,0xff9fd0b7,0xffbcaa7f,0xffb5cbe7};return juce::Colour(colours[static_cast<size_t>(band)]);
    }
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
        for(auto* label:{&frequencyLabel,&gainLabel,&qualityLabel})label->setColour(juce::Label::textColourId,bandColour(selectedBand));bandChoice.setColour(juce::ComboBox::textColourId,bandColour(selectedBand));
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
        eqEnabled.setToggleState(processor.state.getRawParameterValue("eq")->load()>.5f&&processor.state.getRawParameterValue("eqPresent")->load()>.5f,juce::dontSendNotification);limiterEnabled.setToggleState(processor.state.getRawParameterValue("lim")->load()>.5f&&processor.state.getRawParameterValue("limPresent")->load()>.5f,juce::dontSendNotification);
        if(showingBrain)refreshBrain();
        undoButton.setEnabled(processor.undoManager.canUndo());redoButton.setEnabled(processor.undoManager.canRedo());
        gain.setEnabled(gainApplies(selectedBand));gainLabel.setText(gainApplies(selectedBand)?"Gain":"GAIN / NOT USED",juce::dontSendNotification);
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
        if(count+count2){
            for(auto& panel:modulePanels)if(panel)panel->update(response);
            parallelPanel.update(response);exciterPanel.update(response);analogPanel.update(response);
            bassPanel.update(response);transientPanel.update(response);
            truePeakPanel.update(response);overviewPanel.update(response);analogizerPanel.update(response);restorationPanel.update(response);
            // True, measured 16-band EQ transfer. UI thread only; no synthetic response curve.
            std::array<float,256> curveDb{};
            const double nyquist=juce::jmax(40.,response.rate*.5);
            for(size_t i=0;i<curveDb.size();++i){
                const double hz=20.*std::pow(nyquist/20.,static_cast<double>(i)/255.);
                std::complex<double> transfer{1.,0.};
                for(size_t band=0;band<16;++band)
                    transfer*=(1.-response.blends[band])+
                              static_cast<double>(response.blends[band])*
                              response.bands[band].response(response.rate,hz);
                transfer=(1.-response.eqMix)+static_cast<double>(response.eqMix)*transfer;
                curveDb[i]=static_cast<float>(20.*std::log10(juce::jmax(1.0e-12,std::abs(transfer))));
            }
            overviewPanel.updateEqCurve(curveDb);
        }
        processor.fifo.prepareToRead(processor.fifo.getNumReady(),start,count,start2,count2);
        auto consume=[&](int first,int size){for(int i=0;i<size;++i){float l=processor.traceL[first+i],r=processor.traceR[first+i];
            samples[0][head]=l;samples[1][head]=r;
            beforeSamples[0][head]=processor.traceBeforeL[first+i];
            beforeSamples[1][head]=processor.traceBeforeR[first+i];
            scope[head%1024]={r-l,r+l};head=(head+1)%32768;}};
        consume(start,count);consume(start2,count2);processor.fifo.finishedRead(count+count2);
        if(count+count2&&modulePanels[2])modulePanels[2]->updateScope(scope);
        if(count+count2>0){
            const int size=1<<fftOrder;
            for(int i=0;i<size;++i){float window=.5f-.5f*std::cos(juce::MathConstants<float>::twoPi*i/(size-1));int index=(head+32768-size+i)%32768;fftLeft[i]=samples[0][index]*window;fftRight[i]=samples[1][index]*window;}
            std::fill(fftLeft.begin()+size,fftLeft.begin()+size*2,0);std::fill(fftRight.begin()+size,fftRight.begin()+size*2,0);
            fft->performFrequencyOnlyForwardTransform(fftLeft.data());fft->performFrequencyOnlyForwardTransform(fftRight.data());
            double now=juce::Time::getMillisecondCounterHiRes();float release=lastAnalysis>0?static_cast<float>(std::exp(-(now-lastAnalysis)/150.)):0;lastAnalysis=now;
            for(int i=0;i<=size/2;++i){float level=masterlab::stereoSpectrumAmplitude(fftLeft[i],fftRight[i],size);spectrum[i]=level>spectrum[i]?level:release*spectrum[i]+(1-release)*level;}
            // Same window, FFT and sampling clock for BEFORE (PDC-aligned
            // input); only the UI thread ever touches these FFT buffers.
            for(int i=0;i<size;++i){
                const float window=.5f-.5f*std::cos(juce::MathConstants<float>::twoPi*i/(size-1));
                const int index=(head+32768-size+i)%32768;
                fftLeft[i]=beforeSamples[0][index]*window;
                fftRight[i]=beforeSamples[1][index]*window;
            }
            std::fill(fftLeft.begin()+size,fftLeft.begin()+size*2,0);
            std::fill(fftRight.begin()+size,fftRight.begin()+size*2,0);
            fft->performFrequencyOnlyForwardTransform(fftLeft.data());
            fft->performFrequencyOnlyForwardTransform(fftRight.data());
            for(int i=0;i<=size/2;++i){
                const float level=masterlab::stereoSpectrumAmplitude(fftLeft[i],fftRight[i],size);
                beforeSpectrum[i]=level>beforeSpectrum[i]?level:release*beforeSpectrum[i]+(1-release)*level;
            }
            if(modulePanels[2]){std::array<float,512> bins{};for(size_t i=0;i<bins.size();++i){float hz=20*std::pow(static_cast<float>(sampleRate()*.5)/20,i/511.f),position=std::clamp(hz*size/static_cast<float>(sampleRate()),0.f,static_cast<float>(size/2-1));int bin=static_cast<int>(position);bins[i]=spectrum[bin]+(position-bin)*(spectrum[bin+1]-spectrum[bin]);}modulePanels[2]->updateSpectrum(bins);
                std::array<float,256> consoleBins{};
                for(size_t n=0;n<consoleBins.size();++n){
                    const float pos=static_cast<float>(n)*511.f/255.f;
                    const int ix=juce::jlimit(0,510,static_cast<int>(pos));
                    consoleBins[n]=bins[static_cast<size_t>(ix)]*(1.f-(pos-ix))+
                                   bins[static_cast<size_t>(ix+1)]*(pos-ix);
                }
                std::array<float,256> inputBins{};
                for(size_t n=0;n<inputBins.size();++n){
                    const float hz=20.f*std::pow(static_cast<float>(sampleRate()*.5)/20.f,static_cast<float>(n)/255.f);
                    const float position=std::clamp(hz*size/static_cast<float>(sampleRate()),0.f,static_cast<float>(size/2-1));
                    const int bin=static_cast<int>(position);
                    inputBins[n]=beforeSpectrum[bin]+(position-bin)*(beforeSpectrum[bin+1]-beforeSpectrum[bin]);
                }
                overviewPanel.updateSpectrum(consoleBins);
                overviewPanel.updateInputSpectrum(inputBins);}

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
    explicit Editor(Processor& p) : AudioProcessorEditor(p),processor(p),exciterPanel(processor,theme),analogPanel(processor,theme),analogizerPanel(processor,theme),restorationPanel(processor,theme),modePanel(processor,theme),bassPanel(processor,theme),transientPanel(processor,theme),truePeakPanel(processor,theme),overviewPanel(processor,theme),parallelPanel(processor,theme) {
        theme.setColour(juce::ResizableWindow::backgroundColourId,background);
        theme.setColour(juce::Slider::textBoxOutlineColourId,juce::Colours::transparentBlack);
        theme.setColour(juce::Slider::textBoxBackgroundColourId,panel);
        setLookAndFeel(&theme);
        for(int i=0;i<4;++i){modulePanels[i]=std::make_unique<ModulePanel>(processor,theme,i);modulePanels[i]->setComponentID("module-panel-"+juce::String(i));addChildComponent(*modulePanels[i]);}
        addChildComponent(parallelPanel);
        addChildComponent(exciterPanel);
        addChildComponent(analogPanel);
        addChildComponent(analogizerPanel);
        addChildComponent(restorationPanel);
        addChildComponent(modePanel);
        addChildComponent(bassPanel);
        addChildComponent(transientPanel);
        addChildComponent(truePeakPanel);
        addChildComponent(overviewPanel);
        addAndMakeVisible(consoleView);consoleView.setComponentID("view-console");
        consoleView.onClick=[this]{showConsole();};
        overviewPanel.onOpen=[this](int index){switch(index){
            case 0:showLimiter(false);break;case 1:showModule(1);break;
            case 2:showModule(2);break;case 3:showFinalTruePeak();break;
            case 4:showAnalogizer();break;case 5:showBass();break;
            case 6:showTransient();break;case 7:showModule(3);break;
            case 8:showBrain();break;default:break;}};
        for(auto* button:{&dynamicView,&dynamicsView,&stereoView,&stabilizerView,&modulesButton})addAndMakeVisible(button);
        dynamicView.setComponentID("view-dynamic-eq");dynamicsView.setComponentID("view-dynamics");stereoView.setComponentID("view-stereo");stabilizerView.setComponentID("view-stabilizer");modulesButton.setComponentID("module-menu");
        dynamicView.onClick=[this]{showModule(0);};dynamicsView.onClick=[this]{showModule(1);};stereoView.onClick=[this]{showModule(2);};stabilizerView.onClick=[this]{showModule(3);};
        addAndMakeVisible(routingChoice);routingChoice.setComponentID("chain-routing");routingChoice.addItem("Serial",1);routingChoice.addItem("Parallel",2);routingAttachment=std::make_unique<ComboAttachment>(processor.state,"routingMode",routingChoice);routingChoice.setTooltip("Serial: modules process in card order. Parallel: each present module receives aligned input and uses explicit trim/wet controls. A limiter branch does not limit the whole sum.");routingChoice.onChange=[this]{if(routingChoice.getSelectedId()==2)showParallel();else showLimiter(false);};
        modulesButton.onClick=[this]{
            juce::PopupMenu menu;
            menu.addSectionHeader("AUDIO CHAIN - ALL DSP STAGES");
            for(int kind:masterlab::allowedFullModules){
                juce::PopupMenu item;
                const bool active=chainActive(kind);
                item.addItem(10+kind,active?"Disable processor":"Enable processor");
                item.addItem(60+kind,"Open processor");
                if(kind==10)item.addSectionHeader("Final position locked");
                menu.addSubMenu(routeNames[static_cast<size_t>(kind)],item);
            }
            menu.addSeparator();
            menu.addItem(204,"MASTER / INSERT processing mode");
            menu.addItem(205,"Legacy parallel mixer");
            auto safe=juce::Component::SafePointer<Editor>(this);
            menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(modulesButton),[safe](int id){
                if(!safe)return;
                auto& self=*safe;
                if(id==204){self.showWorkMode();return;}
                if(id==205){self.showParallel();return;}
                if(id>=10&&id<22){self.toggleChainStage(id-10);return;}
                if(id>=60&&id<72){
                    const int kind=id-60;
                    if(kind<3||kind==5)self.showModule(kind==5?3:kind);
                    else if(kind==3)self.showLimiter(false);
                    else if(kind==6)self.showAnalogizer();
                    else if(kind==8)self.showBass();
                    else if(kind==9)self.showTransient();
                    else if(kind==10)self.showFinalTruePeak();
                    else if(kind==11)self.showRestoration();
                    self.repaint();
                }
            });
        };

        configureSlider(frequency,frequencyLabel,"Frequency","eq-frequency");frequency.setTextValueSuffix(" Hz");
        configureSlider(gain,gainLabel,"Gain","eq-gain");gain.setTextValueSuffix(" dB");
        configureSlider(quality,qualityLabel,"Bandwidth","eq-q");
        auto activateEditedBand=[this]{auto* p=processor.state.getParameter("bandEnabled"+juce::String(selectedBand+1));if(p->getValue()<.5f)p->setValueNotifyingHost(1);};
        frequency.onDragStart=activateEditedBand;gain.onDragStart=activateEditedBand;quality.onDragStart=activateEditedBand;

        configureSlider(ceiling,ceilingLabel,"Output ceiling","limiter-ceiling");ceiling.setTextValueSuffix(" dBFS");
        configureSlider(drive,driveLabel,"Drive","limiter-drive");drive.setTextValueSuffix(" dB");
        configureSlider(release,releaseLabel,"Release","limiter-release");release.setTextValueSuffix(" ms");
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
        fftChoice.onChange=[this]{fftOrder=juce::jlimit(12,15,fftChoice.getSelectedId());fftChoice.setSelectedId(fftOrder,juce::dontSendNotification);fft=std::make_unique<juce::dsp::FFT>(fftOrder);spectrum.fill(0);beforeSpectrum.fill(0);lastAnalysis=0;};
        for(int i=1;i<=16;++i)bandChoice.addItem("Band "+juce::String(i),i);
        bandChoice.onChange=[this]{endDrag();selectBand(bandChoice.getSelectedId()-1);};
        addAndMakeVisible(eqEnabled);addAndMakeVisible(limiterEnabled);
        eqEnabled.setComponentID("eq-active");limiterEnabled.setComponentID("limiter-active");
        eqAttachment=std::make_unique<ButtonAttachment>(processor.state,"eq",eqEnabled);
        eqEnabled.onClick=[this]{if(eqEnabled.getToggleState())processor.setModulePresentUI(3,true);};limiterEnabled.onClick=[this]{if(limiterEnabled.getToggleState())processor.setModulePresentUI(4,true);};
        limiterAttachment=std::make_unique<ButtonAttachment>(processor.state,"lim",limiterEnabled);
        ceilingAttachment=std::make_unique<SliderAttachment>(processor.state,"ceiling",ceiling);
        addAndMakeVisible(eqView);addAndMakeVisible(limiterView);addAndMakeVisible(inspectorView);inspectorView.setComponentID("view-inspector");inspectorView.onClick=[this]{showInspector();};
        eqView.setComponentID("view-eq");limiterView.setComponentID("view-limiter");
        eqView.onClick=[this]{showLimiter(false);};limiterView.onClick=[this]{showFinalTruePeak();};
        eqView.setClickingTogglesState(false);limiterView.setClickingTogglesState(false);
        addAndMakeVisible(resetMetersButton);resetMetersButton.setComponentID("reset-meters");resetMetersButton.onClick=[this]{processor.resetMeterRequest.store(true);};resetMetersButton.setTooltip("Reset RMS, sample peaks and LUFS at the next audio callback. Audio settings and processing are preserved.");resetMetersButton.setVisible(false);
        addAndMakeVisible(undoButton);addAndMakeVisible(redoButton);addAndMakeVisible(presetButton);addAndMakeVisible(newButton);
        undoButton.setComponentID("undo");redoButton.setComponentID("redo");presetButton.setComponentID("preset");
        newButton.setComponentID("factory-default");
        newButton.setTooltip("NEW / preset 0: restore every factory parameter, clear Master Brain, and keep saved presets. Undo restores previous settings.");
        newButton.onClick=[this]{endDrag();processor.loadFactoryDefaultUI();reflectFactoryBrainControls();refreshBrain();repaint();};
        undoButton.onClick=[this]{endDrag();processor.undo();};redoButton.onClick=[this]{endDrag();processor.redo();};
        undoButton.setTooltip("Undo the last parameter transaction. Host automation may also be recorded while it runs.");
        redoButton.setTooltip("Redo an undone transaction.");
        presetButton.onClick=[this]{
            juce::PopupMenu menu;menu.addItem(3,"0  Default / NEW");menu.addSeparator();menu.addItem(1,"Load preset...");menu.addItem(2,"Save preset...");
            auto safe=juce::Component::SafePointer<Editor>(this);
            menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(presetButton),[safe](int selection){
                if(!safe)return;
                if(selection==3){safe->endDrag();safe->processor.loadFactoryDefaultUI();safe->reflectFactoryBrainControls();safe->refreshBrain();safe->repaint();}
                else if(selection==1||selection==2)safe->choosePreset(selection==2);
            });
        };
        addAndMakeVisible(brainView);brainView.setComponentID("view-brain");brainView.onClick=[this]{showBrain();};
        for(size_t i=0;i<masterlab::brainGenres.size();++i)brainGenre.addItem(masterlab::brainGenres[i],static_cast<int>(i)+1);
        for(size_t i=0;i<masterlab::brainStyles.size();++i)brainStyle.addItem(masterlab::brainStyles[i],static_cast<int>(i)+1);
        for(int seconds:{8,16,30,60,120,180,300,600})brainDuration.addItem(juce::String(seconds)+" seconds",seconds);
        auto config=processor.brain.readUI().config;
        brainGenre.setSelectedId(config.genre+1,juce::dontSendNotification);brainStyle.setSelectedId(config.style+1,juce::dontSendNotification);brainDuration.setSelectedId(config.seconds,juce::dontSendNotification);
        brainTarget.setRange(-24,-8,.1);brainTarget.setValue(config.targetLufs,juce::dontSendNotification);brainTarget.setTextValueSuffix(" LUFS");
        brainCeiling.setRange(-6,-.1,.1);brainCeiling.setValue(config.ceiling,juce::dontSendNotification);brainCeiling.setTextValueSuffix(" dBFS");
        brainIntensity.setRange(.25,1.5,.05);brainIntensity.setValue(config.intensity,juce::dontSendNotification);brainIntensity.setTextValueSuffix(" x");
        brainUseStabilizer=config.useStabilizer;brainAutomatic=config.automaticModules;brainUseStereo=config.useStereo;brainUseDynamics=config.useDynamics;brainUseDynamicEq=config.useDynamicEq;
        brainEq.setToggleState(config.useEq,juce::dontSendNotification);brainLimiter.setToggleState(config.useLimiter,juce::dontSendNotification);
        for(auto* component:std::initializer_list<juce::Component*>{&brainGenre,&brainStyle,&brainDuration,&brainTarget,&brainCeiling,&brainIntensity,&brainModulesButton,&brainAnalyse,&brainApply,&brainAuditionA,&brainAuditionB,&brainReport})addAndMakeVisible(component);
        for(auto* slider:{&brainTarget,&brainCeiling,&brainIntensity}){slider->setSliderStyle(juce::Slider::LinearHorizontal);slider->setTextBoxStyle(juce::Slider::TextBoxRight,false,72,24);}
        for(size_t i=0;i<brainLabels.size();++i){auto& label=brainLabels[i];addAndMakeVisible(label);label.setText(std::array<const char*,6>{"Genre profile","Style","Target loudness","Output ceiling","Analysis length","Tonal intensity"}[i],juce::dontSendNotification);label.setColour(juce::Label::textColourId,muted);}
        brainReport.setMultiLine(true,true);brainReport.setReadOnly(true);brainReport.setScrollbarsShown(true);brainReport.setFont(theme.font(14.f));brainReport.setColour(juce::TextEditor::backgroundColourId,juce::Colours::transparentBlack);brainReport.setColour(juce::TextEditor::textColourId,juce::Colour(0xffe2ecf4));brainReport.setColour(juce::TextEditor::outlineColourId,juce::Colours::transparentBlack);
        brainModulesButton.setComponentID("brain-processors");brainModulesButton.onClick=[this]{
            juce::PopupMenu menu;menu.addItem(7,"Stabilizer",true,brainUseStabilizer);menu.addItem(6,"Automatically select needed modules",true,brainAutomatic);menu.addSeparator();menu.addItem(1,"Spectral EQ",true,brainEq.getToggleState());menu.addItem(2,"Limiter",true,brainLimiter.getToggleState());menu.addItem(3,"Stereo Architect",true,brainUseStereo);menu.addItem(4,"Smart Dynamics",true,brainUseDynamics);menu.addItem(5,"Dynamic EQ",true,brainUseDynamicEq);menu.addSeparator();menu.addItem(8,"Harmonic Exciter (post-chain)",true,brainUseExciter);menu.addItem(9,"Analog Color (post-chain)",true,brainUseAnalog);menu.addItem(10,"Bass Architect (post-chain)",true,brainUseBass);menu.addItem(11,"Transient Sculptor (post-chain)",true,brainUseTransient);menu.addItem(12,"Final True-Peak (post-chain)",true,brainUseTruePeak);
            auto safe=juce::Component::SafePointer<Editor>(this);menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(brainModulesButton),[safe](int id){if(!safe||!id)return;if(id==7)safe->brainUseStabilizer=!safe->brainUseStabilizer;if(id==6)safe->brainAutomatic=!safe->brainAutomatic;if(id==1)safe->brainEq.setToggleState(!safe->brainEq.getToggleState(),juce::dontSendNotification);if(id==2)safe->brainLimiter.setToggleState(!safe->brainLimiter.getToggleState(),juce::dontSendNotification);if(id==3)safe->brainUseStereo=!safe->brainUseStereo;if(id==4)safe->brainUseDynamics=!safe->brainUseDynamics;if(id==5)safe->brainUseDynamicEq=!safe->brainUseDynamicEq;
            if(id==8)safe->brainUseExciter=!safe->brainUseExciter;
            if(id==9)safe->brainUseAnalog=!safe->brainUseAnalog;
            if(id==10)safe->brainUseBass=!safe->brainUseBass;
            if(id==11)safe->brainUseTransient=!safe->brainUseTransient;
            if(id==12)safe->brainUseTruePeak=!safe->brainUseTruePeak;
            safe->processor.brain.reviseUI(safe->brainConfig());safe->refreshBrain();});
        };
        auto revise=[this]{if(!brainControlsUpdating){processor.brain.reviseUI(brainConfig());refreshBrain();}};
        brainGenre.onChange=revise;brainStyle.onChange=revise;brainDuration.onChange=revise;brainTarget.onValueChange=revise;brainCeiling.onValueChange=revise;brainIntensity.onValueChange=revise;brainEq.onClick=revise;brainLimiter.onClick=revise;
        brainAnalyse.setComponentID("brain-analyse");brainApply.setComponentID("brain-apply");brainReport.setComponentID("brain-report");brainGenre.setComponentID("brain-genre");brainStyle.setComponentID("brain-style");brainTarget.setComponentID("brain-target");
        brainAnalyse.onClick=[this]{auto current=processor.brain.readUI();if(brainAnalyse.getButtonText()=="Cancel analysis"||current.phase==masterlab::BrainPhase::Pending||current.phase==masterlab::BrainPhase::Analysing||current.phase==masterlab::BrainPhase::Validating)processor.brain.cancelUI();else processor.startBrainUI(brainConfig());refreshBrain();};
        brainApply.onClick=[this]{processor.applyBrainUI();refreshBrain();repaint();};
        brainAuditionA.setComponentID("brain-audition-a");brainAuditionB.setComponentID("brain-audition-b");
        brainAuditionA.onClick=[this]{processor.auditionBrainUI(0);refreshBrain();repaint();};
        brainAuditionB.onClick=[this]{processor.auditionBrainUI(1);refreshBrain();repaint();};
        brainApply.setTooltip("Apply only the reviewed proposal, then re-measure the full song. Disabled if audio settings changed since capture. Undo restores the previous state.");
        brainEq.setTooltip("Include EQ: replaces all 16 bands on Apply. Untick to preserve the current EQ.");brainLimiter.setTooltip("Include sample-peak limiter and move it after EQ. No true-peak guarantee.");
        selectBand(0);showLimiter(false);if(routingChoice.getSelectedId()==2)showParallel();showConsole();
        setResizable(true,true);setResizeLimits(750,550,2000,1400);setSize(1200,800);startTimerHz(30);
    }
    void choosePreset(bool save){
        auto initial=juce::File::getSpecialLocation(juce::File::userDocumentsDirectory).getChildFile("MasterLab.jmlx");
        presetChooser=std::make_unique<juce::FileChooser>(save?"Save MasterLab preset":"Load MasterLab preset",initial,"*.jmlx");
        auto safe=juce::Component::SafePointer<Editor>(this);
        int flags=save?(juce::FileBrowserComponent::saveMode|juce::FileBrowserComponent::canSelectFiles|juce::FileBrowserComponent::warnAboutOverwriting):(juce::FileBrowserComponent::openMode|juce::FileBrowserComponent::canSelectFiles);
        presetChooser->launchAsync(flags,[safe,save](const juce::FileChooser& chooser){if(!safe)return;auto file=chooser.getResult();if(file==juce::File{})return;
            bool ok=save?safe->processor.savePreset(file):safe->processor.loadPreset(file);
            safe->presetButton.setButtonText(ok?"Presets":"PRESET ERROR");safe->presetButton.setTooltip(ok?file.getFileName():"Preset could not be read or written. Invalid data leaves parameters unchanged.");
        });
    }
    ~Editor() override {stopTimer();endDrag();setLookAndFeel(nullptr);}
    void resized() override {
        resetMetersButton.setBounds(getWidth()-210,136,186,26);
        newButton.setBounds(211,13,52,30);presetButton.setBounds(267,13,89,30);
        undoButton.setBounds(360,13,40,30);redoButton.setBounds(404,13,40,30);
        eqView.setBounds(getWidth()-432,48,98,26);limiterView.setBounds(getWidth()-326,48,98,26);inspectorView.setBounds(getWidth()-220,48,98,26);brainView.setBounds(getWidth()-114,48,98,26);
        const int brainCell=(getWidth()-80)/4;
        for(int i=0;i<4;++i)brainLabels[static_cast<size_t>(i)].setBounds(40+i*brainCell,166,brainCell-12,20);
        brainGenre.setBounds(40,190,brainCell-12,28);brainStyle.setBounds(40+brainCell,190,brainCell-12,28);brainTarget.setBounds(40+2*brainCell,190,brainCell-12,28);brainCeiling.setBounds(40+3*brainCell,190,brainCell-12,28);
        brainLabels[4].setBounds(40,226,brainCell-12,20);brainLabels[5].setBounds(40+brainCell,226,brainCell-12,20);brainDuration.setBounds(40,248,brainCell-12,28);brainIntensity.setBounds(40+brainCell,248,brainCell-12,28);
        brainModulesButton.setBounds(40+2*brainCell,248,brainCell-12,28);brainAnalyse.setBounds(40+3*brainCell,242,brainCell-12,34);const int resultWidth=std::clamp(getWidth()/2-48,300,600);brainReport.setBounds(36+resultWidth,342,getWidth()-60-resultWidth,getHeight()-426);brainApply.setBounds(getWidth()-210,getHeight()-66,186,34);
        brainAuditionA.setBounds(40,315,138,24);brainAuditionB.setBounds(185,315,138,24);
        const int top=getHeight()-163,width=(getWidth()-64)/4;
        for(int i=0;i<4;++i){auto* label=std::array<juce::Label*,4>{&frequencyLabel,&gainLabel,&qualityLabel,&ceilingLabel}[i];auto* slider=std::array<juce::Slider*,4>{&frequency,&gain,&quality,&ceiling}[i];label->setBounds(32+i*width,top,width-12,22);slider->setBounds(32+i*width,top+22,width-16,113);}
        driveLabel.setBounds(32,top,width-12,22);drive.setBounds(32,top+22,width-16,113);
        releaseLabel.setBounds(32+width,top,width-12,22);release.setBounds(32+width,top+22,width-16,113);
        bandChoice.setBounds(28,140,110,26);typeChoice.setBounds(148,140,155,26);fftChoice.setBounds(313,140,130,26);bandEnabled.setBounds(453,140,74,26);
        eqEnabled.setBounds(537,140,72,26);limiterEnabled.setBounds(getWidth()-114,136,90,26);
        const int navWidth=(getWidth()-48)/9;int nav=0;for(auto* button:{&consoleView,&eqView,&dynamicView,&dynamicsView,&stereoView,&stabilizerView,&limiterView,&inspectorView,&brainView}){button->setBounds(24+nav*navWidth,48,navWidth-5,26);++nav;}
        modulesButton.setBounds(602,14,120,26);routingChoice.setBounds(448,14,134,26);
        for(auto& panel:modulePanels)panel->setBounds(0,134,getWidth(),getHeight()-154);
        parallelPanel.setBounds(0,134,getWidth(),getHeight()-154);
        exciterPanel.setBounds(0,134,getWidth(),getHeight()-154);
        analogPanel.setBounds(0,134,getWidth(),getHeight()-154);
        analogizerPanel.setBounds(0,134,getWidth(),getHeight()-154);
        restorationPanel.setBounds(0,134,getWidth(),getHeight()-154);
        modePanel.setBounds(0,134,getWidth(),getHeight()-154);
        bassPanel.setBounds(0,134,getWidth(),getHeight()-154);
        transientPanel.setBounds(0,134,getWidth(),getHeight()-154);
        truePeakPanel.setBounds(0,134,getWidth(),getHeight()-154);
        overviewPanel.setBounds(0,134,getWidth(),getHeight()-154);

    }
    bool isInterestedInDragSource(const SourceDetails& details) override {
        return details.description.toString().startsWith("module:");
    }
    void itemDropped(const SourceDetails& details) override {
        const auto position=details.localPosition.toFloat();
        const int destination=chainAt(position.x,position.y);
        const int source=details.description.toString().fromFirstOccurrenceOf(":",false,false).getIntValue();
        if(destination<0||source<0||source==destination||source==10||destination==10)return;
        if(usingFullRouter()){
            auto order=fullRouteOrder();
            auto from=std::find(order.begin(),order.end(),source);
            auto to=std::find(order.begin(),order.end(),destination);
            if(from==order.end()||to==order.end())return;
            if(from<to)std::rotate(from,from+1,to+1);
            else std::rotate(to,from,from+1);
            writeFullRoute(order);
            return;
        }
        // Retained compatibility routing for the independent parallel mixer.
        if(source>=6||destination>=6)return;
        const bool legacy=processor.state.getRawParameterValue("order")->load()>.5f;
        auto order=masterlab::decodeOrder(static_cast<int>(processor.state.getRawParameterValue("chainOrder")->load()),legacy);
        auto from=std::find(order.begin(),order.end(),source),to=std::find(order.begin(),order.end(),destination);
        if(from<to)std::rotate(from,from+1,to+1);else std::rotate(to,from,from+1);
        auto* parameter=processor.state.getParameter("chainOrder");
        processor.beginUiTransaction("Move module");
        parameter->beginChangeGesture();
        parameter->setValueNotifyingHost(parameter->convertTo0to1(static_cast<float>(masterlab::encodeOrder(order,legacy))));
        parameter->endChangeGesture();
        processor.finishUiTransaction();
        repaint();
    }
    void mouseDown(const juce::MouseEvent& event) override {
        chainDragKind=-1;chainDragging=false;chainHover=-1;
        if(event.position.y>=80&&event.position.y<168){
            chainDragKind=chainAt(event.position.x,event.position.y);
            return;
        }
        if(moduleView>=0||showingParallel||showingExciter||showingAnalog||showingBrain||showingInspector||showingLimiter||!plot().expanded(12).contains(event.position))return;
        int nearest=selectedBand;float distance=event.position.getDistanceFrom(node(nearest));
        for(int i=0;i<16;++i){if(i!=selectedBand&&bandType(i)==0&&std::abs(value("gain",i))<.01f)continue;float d=event.position.getDistanceFrom(node(i));if(d<distance){distance=d;nearest=i;}}
        if(distance>14)return;
        selectBand(nearest);auto* active=processor.state.getParameter("bandEnabled"+juce::String(selectedBand+1));active->setValueNotifyingHost(1);draggingNode=true;
        processor.beginUiTransaction("EQ node");
        processor.state.getParameter(parameterId("frequency",selectedBand))->beginChangeGesture();
        processor.state.getParameter(parameterId("gain",selectedBand))->beginChangeGesture();
    }
    void mouseDrag(const juce::MouseEvent& event) override {
        if(draggingNode){auto area=plot();float normal=juce::jlimit(0.f,1.f,(event.position.x-area.getX())/area.getWidth());
            setParameter("frequency",static_cast<float>(20*std::pow(sampleRate()*.5/20,normal)));
            if(gainApplies(selectedBand))setParameter("gain",juce::jlimit(-18.f,18.f,(area.getCentreY()-event.position.y)*36/area.getHeight()));repaint();return;}
        if(chainDragKind<0||chainDragKind==10)return;
        chainDragging=event.getDistanceFromDragStart()>4;
        chainHover=chainAt(event.position.x,event.position.y);
        repaint();
    }
    void mouseUp(const juce::MouseEvent& event) override {
        const bool moved=event.mouseWasDraggedSinceMouseDown();
        endDrag();
        if(chainDragging&&chainDragKind>=0){
            itemDropped(SourceDetails("module:"+juce::String(chainDragKind),this,event.position.toInt()));
            chainDragKind=-1;chainDragging=false;chainHover=-1;repaint();return;
        }
        chainDragKind=-1;chainHover=-1;
        if(event.position.y<80||event.position.y>=168)return;
        const int kind=chainAt(event.position.x,event.position.y);
        if(kind<0)return;
        const auto list=chainModules();
        const auto position=std::find(list.begin(),list.end(),kind);
        const int index=static_cast<int>(position-list.begin());
        if(event.mods.isPopupMenu()){
            juce::PopupMenu menu;
            menu.addItem(1,"Move earlier",kind!=10&&index>0);
            menu.addItem(2,"Move later",kind!=10&&index+1<static_cast<int>(list.size())-1);
            menu.addItem(3,chainActive(kind)?"Disable processor":"Enable processor");
            auto safe=juce::Component::SafePointer<Editor>(this);
            menu.showMenuAsync({},[safe,kind,index,list](int action){
                if(!safe)return;
                if(action==3){safe->toggleChainStage(kind);return;}
                if(action==1||action==2){
                    const int target=index+(action==1?-1:1);
                    if(target<0||target>=static_cast<int>(list.size())||list[static_cast<size_t>(target)]==10)return;
                    const float cell=(safe->getWidth()-48.f)/5.f;
                    const float x=24.f+cell*(target%5+.5f);
                    const float y=80.f+44.f*(target/5)+22.f;
                    safe->itemDropped(SourceDetails("module:"+juce::String(kind),safe.getComponent(),{juce::roundToInt(x),juce::roundToInt(y)}));
                }
            });
            return;
        }
        if(!moved){
            const float cell=(getWidth()-48.f)/5.f;
            const float local=std::fmod(event.position.x-24.f,cell);
            if(usingFullRouter()&&local>cell-38.f){toggleChainStage(kind);return;}
            if(kind<3||kind==5)showModule(kind==5?3:kind);
            else if(kind==3)showLimiter(false);
            else if(kind==4)showLimiter(true);
            else if(kind==6)showAnalogizer();
            else if(kind==8)showBass();
            else if(kind==9)showTransient();
            else if(kind==10)showFinalTruePeak();
            else if(kind==11)showRestoration();
        }
    }
    void mouseWheelMove(const juce::MouseEvent& event,const juce::MouseWheelDetails& wheel) override {
        if(moduleView<0&&!showingParallel&&!showingBrain&&!showingInspector&&!showingLimiter&&plot().contains(event.position)&&event.position.getDistanceFrom(node(selectedBand))<24){auto* active=processor.state.getParameter("bandEnabled"+juce::String(selectedBand+1));if(active->getValue()<.5f)active->setValueNotifyingHost(1);setParameter("q",juce::jlimit(.1f,10.f,value("q",selectedBand)*std::exp(wheel.deltaY*2)),true);repaint();}
        else AudioProcessorEditor::mouseWheelMove(event,wheel);
    }
    void paint(juce::Graphics& g) override {
        theme.paintPowderCoat(g,getLocalBounds());
        // Hardware-inspired chassis: a distinct faceplate, not a legacy colour swap.
        g.setGradientFill(juce::ColourGradient(juce::Colour(0xfff8fafb),0.f,0.f,
                                              juce::Colour(0xffc8d0d8),0.f,80.f,false));
        g.fillRect(0,0,getWidth(),78);
        g.setColour(juce::Colour(0xff8398aa));g.drawHorizontalLine(77,0.f,static_cast<float>(getWidth()));
        g.setColour(juce::Colour(0xffe5eaf0));g.fillRoundedRectangle(15.f,5.f,190.f,39.f,7.f);
        g.setColour(juce::Colour(0xff8b99a6));g.drawRoundedRectangle(15.f,5.f,190.f,39.f,7.f,1.f);
        for(float screwX : {22.f,static_cast<float>(getWidth()-21)}) {
            g.setColour(juce::Colour(0xff090d12));g.fillEllipse(screwX-5.f,19.f,10.f,10.f);
            g.setColour(juce::Colour(0xff718290));g.drawEllipse(screwX-4.f,20.f,8.f,8.f,1.f);
            g.drawLine(screwX-2.f,26.f,screwX+2.f,22.f,.9f);
        }
        g.setColour(juce::Colour(0xff162635));g.setFont(theme.font(20.f));
        g.drawText("MASTERLAB X",32,6,167,24,juce::Justification::left);
        g.setColour(juce::Colour(0xff076fff));g.setFont(theme.font(9.f));
        g.drawText("POWDER COAT  /  CONCEPT C",32,29,167,12,juce::Justification::left);
        if(getWidth()>=1000){
            g.setColour(juce::Colour(0xff152029));
            g.fillRoundedRectangle(static_cast<float>(getWidth()-224),7.f,188.f,34.f,6.f);
            g.setColour(juce::Colour(0xff3df36e));g.setFont(theme.font(12.f));
            g.drawText(juce::String(sampleRate()/1000,1)+" kHz  /  STEREO",getWidth()-215,11,169,24,juce::Justification::centred);
        }
        const int dock=getHeight()-174,cell=(getWidth()-64)/4;
        g.setColour(panel);g.fillRoundedRectangle(24.f,static_cast<float>(dock),getWidth()-48.f,146.f,20.f);
        const auto modules=chainModules();
        bool first=std::find(modules.begin(),modules.end(),4)<std::find(modules.begin(),modules.end(),3);
        if(modules.empty()){g.setColour(juce::Colour(0xff344b60));g.setFont(theme.font(14));g.drawText("Empty chain / use + Modules to add processors",24,84,getWidth()-48,48,juce::Justification::centred);}
        for(size_t i=0;i<modules.size();++i){int kind=modules[i];const float width=(getWidth()-48.f)/modules.size(),x=24+i*width;const bool active=(kind<3||kind==5)?moduleView==(kind==5?3:kind):(!showingParallel&&!showingBrain&&!showingInspector&&moduleView<0&&(kind==3?!showingLimiter:showingLimiter));const bool on=processor.state.getRawParameterValue(routeOnIds[static_cast<size_t>(kind)])->load()>.5f;
            const auto colour=kind==4?curve:accent;
            const juce::Rectangle<float> slot(x+1.f,84.f,width-10.f,48.f);
            g.setGradientFill(juce::ColourGradient(active?juce::Colour(0xfff9fbfc):juce::Colour(0xffedf0f3),
                                                     x,84.f,juce::Colour(0xffc7d2dc),x,132.f,false));
            g.fillRoundedRectangle(slot,5.f);
            g.setColour(active?juce::Colour(0xff087cff):juce::Colour(0xff96a8b5));
            g.drawRoundedRectangle(slot,5.f,active?1.8f:.8f);
            if(chainDragging&&chainHover==kind){g.setColour(juce::Colour(0xff086bff));g.drawRoundedRectangle(slot,5.f,2.f);}
            theme.paintPowerLed(g,juce::roundToInt(x+10.f),98,22,on);
            g.setColour(juce::Colour(0xff172936));g.setFont(theme.font(modules.size()>7?9.f:modules.size()>3?12.f:16.f));g.drawText(routeNames[static_cast<size_t>(kind)],static_cast<int>(x)+34,90,static_cast<int>(width)-48,22,juce::Justification::left);
            g.setColour(juce::Colour(0xff566b7b));g.setFont(theme.font(modules.size()>7?8.f:10.f));
            g.drawText(on?std::array<const char*,11>{"3 detectors","Down / upward","4-band M/S","16 bands","5 ms lookahead","Adaptive tone","Harmonics","Color","Sub/bass","Envelope","True Peak"}[static_cast<size_t>(kind)]:"Bypassed",static_cast<int>(x)+34,111,static_cast<int>(width)-48,16,juce::Justification::left);
        }
        if(showingConsole){g.setColour(background);g.fillRect(0,134,getWidth(),getHeight()-134);return;}
        if(moduleView>=0){g.setColour(background);g.fillRect(0,134,getWidth(),getHeight()-134);return;}
        if(showingParallel||showingExciter||showingAnalog||showingAnalogizer||showingRestoration||showingMode){g.setColour(background);g.fillRect(0,134,getWidth(),getHeight()-134);return;}
        if(showingBrain){paintBrain(g);return;}
        if(showingInspector){paintInspector(g);return;}
        if(showingLimiter){paintLimiter(g);g.setColour(juce::Colour(0xff334e63));g.setFont(theme.font(12));g.drawText(processor.state.getRawParameterValue("routingMode")->load()>.5f?"PARALLEL: limiter ceiling applies to its branch, not the full sum":(first?"Processors after limiter may exceed its ceiling":"SAMPLE PEAK  /  True Peak unavailable"),24,getHeight()-25,getWidth()-48,18,juce::Justification::left);return;}
        auto area=plot();g.setColour(juce::Colour(0xff151920));g.fillRoundedRectangle(area.expanded(24,12),18);
        g.setFont(theme.font(11));
        for(float db:{-18.f,-12.f,-6.f,0.f,6.f,12.f,18.f}){float y=yForGain(db);g.setColour(db==0?juce::Colour(0xff536374):juce::Colour(0xff242a33));g.drawHorizontalLine(static_cast<int>(y),area.getX(),area.getRight());g.setColour(muted);g.drawText(juce::String(db,0),8,static_cast<int>(y)-8,32,16,juce::Justification::right);}
        for(float hz:{20.f,50.f,100.f,200.f,500.f,1000.f,2000.f,5000.f,10000.f,20000.f}){if(hz>sampleRate()*.5)continue;float x=xForFrequency(hz);g.setColour(juce::Colour(0xff242a33));g.drawVerticalLine(static_cast<int>(x),area.getY(),area.getBottom());g.setColour(muted);g.drawText(juce::String(hz>=1000?hz/1000:hz,0)+(hz>=1000?"k":""),static_cast<int>(x)-14,static_cast<int>(area.getBottom())+4,40,18,juce::Justification::centred);}
        {juce::Graphics::ScopedSaveState save(g);g.reduceClipRegion(area.toNearestInt());juce::Path spectral,total;
            for(int i=0;i<512;++i){double hz=20*std::pow(sampleRate()*.5/20,i/511.);float x=area.getX()+area.getWidth()*i/511.f;float binPosition=juce::jlimit(0.f,static_cast<float>((1<<fftOrder)/2-1),static_cast<float>(hz*(1<<fftOrder)/sampleRate()));int bin=static_cast<int>(binPosition);float level=spectrum[bin]+(binPosition-bin)*(spectrum[bin+1]-spectrum[bin]);float db=juce::Decibels::gainToDecibels(level,-100.f);float sy=area.getBottom()-area.getHeight()*juce::jlimit(0.f,1.f,(db+100)/100.f);
                if(i==0)spectral.startNewSubPath(x,sy);else spectral.lineTo(x,sy);
                if(haveResponse){std::complex<double> transfer{1,0};for(size_t k=0;k<16;++k)transfer*=(1.-response.blends[k])+static_cast<double>(response.blends[k])*response.bands[k].response(response.rate,hz);transfer=(1.-response.eqMix)+static_cast<double>(response.eqMix)*transfer;float y=yForGain(static_cast<float>(20*std::log10(std::max(1e-12,std::abs(transfer)))));if(i==0)total.startNewSubPath(x,y);else total.lineTo(x,y);}}
            auto spectralFill=spectral;spectralFill.lineTo(area.getRight(),area.getBottom());spectralFill.lineTo(area.getX(),area.getBottom());spectralFill.closeSubPath();g.setGradientFill(juce::ColourGradient(accent.withAlpha(.22f),0,area.getY(),accent.withAlpha(.01f),0,area.getBottom(),false));g.fillPath(spectralFill);g.setColour(accent.withAlpha(.8f));g.strokePath(spectral,juce::PathStrokeType(1.5f));
            if(haveResponse){
                for(int band=0;band<16;++band){
                    if(response.blends[band]<.001f||(band!=selectedBand&&bandType(band)==0&&std::abs(value("gain",band))<.01f))continue;
                    juce::Path individual;for(int i=0;i<512;++i){double hz=20*std::pow(sampleRate()*.5/20,i/511.);float x=area.getX()+area.getWidth()*i/511.f;float y=yForGain(static_cast<float>(20*std::log10(std::max(1e-12,std::abs((1.-response.blends[band])+static_cast<double>(response.blends[band])*response.bands[band].response(response.rate,hz))))));if(i==0)individual.startNewSubPath(x,y);else individual.lineTo(x,y);}
                    g.setColour(bandColour(band).withAlpha(band==selectedBand?.95f:.65f));g.strokePath(individual,juce::PathStrokeType(band==selectedBand?2.f:1.2f));
                }
                g.setColour(juce::Colours::whitesmoke.withAlpha(.65f));g.strokePath(total,juce::PathStrokeType(1.5f));
            }
            for(int i=0;i<16;++i){if(i!=selectedBand&&bandType(i)==0&&std::abs(value("gain",i))<.01f)continue;auto position=node(i);g.setColour(processor.state.getRawParameterValue("bandEnabled"+juce::String(i+1))->load()>.5f?bandColour(i):juce::Colour(0xff596575));g.fillEllipse(position.x-5,position.y-5,10,10);if(i==selectedBand){g.drawEllipse(position.x-9,position.y-9,18,18,1.5f);g.setFont(theme.font(11));g.drawText(juce::String(i+1),static_cast<int>(position.x)+13,static_cast<int>(position.y)-12,26,22,juce::Justification::left);}}
        }
        int inspectorX=getWidth()-218;g.setColour(panel);g.fillRoundedRectangle(static_cast<float>(inspectorX),162.f,194.f,static_cast<float>(getHeight()-370),18);g.setColour(muted);g.setFont(theme.font(12));g.drawText("Stereo image",inspectorX+12,174,170,20,juce::Justification::left);
        const float radius=std::min(56.f,(getHeight()-488.f)*.5f),cx=inspectorX+97.f,cy=204.f+radius;g.setColour(juce::Colour(0xff3c4c5c));g.drawEllipse(cx-radius,cy-radius,2*radius,2*radius,1);g.drawLine(cx-radius,cy,cx+radius,cy);g.drawLine(cx,cy-radius,cx,cy+radius);
        {juce::Graphics::ScopedSaveState save(g);g.reduceClipRegion(juce::Rectangle<int>(inspectorX+24,204,146,static_cast<int>(radius*2)));g.setColour(accent.withAlpha(.45f));for(auto point:scope)g.fillEllipse(cx+point.x*radius-1,cy-point.y*radius-1,2,2);}
        g.setColour(juce::Colours::whitesmoke);g.drawText("CORR "+juce::String(processor.correlation.load(),3),inspectorX+12,static_cast<int>(cy+radius+10),170,20,juce::Justification::left);
        g.setColour(muted);g.setFont(theme.font(11));g.drawText("400 ms sliding / output",inspectorX+12,static_cast<int>(cy+radius+34),170,20,juce::Justification::left);
        const float correlation=juce::jlimit(-1.f,1.f,processor.correlation.load());
        const float barY=cy+radius+64;
        g.setColour(juce::Colour(0xff0a1119));g.fillRoundedRectangle(inspectorX+16.f,barY,162,6,3);
        g.setColour(correlation<0?juce::Colour(0xffe68a79):accent);float correlationX=inspectorX+97.f+correlation*81.f;g.fillRect(std::min(inspectorX+97.f,correlationX),barY,std::max(1.f,std::abs(correlation)*81.f),6.f);
        if(getHeight()>650){g.setColour(muted);g.setFont(theme.font(11));g.drawText("OUTPUT PEAK / dBFS",inspectorX+16,static_cast<int>(barY)+22,162,18,juce::Justification::left);
            g.setColour(juce::Colours::whitesmoke);g.setFont(theme.font(24));g.drawText(juce::String(juce::Decibels::gainToDecibels(processor.peak.load()),1),inspectorX+16,static_cast<int>(barY)+42,162,30,juce::Justification::left);
            g.setColour(muted);g.setFont(theme.font(12));g.drawText("RMS  "+juce::String(juce::Decibels::gainToDecibels(processor.rms.load()),1)+" dBFS",inspectorX+16,static_cast<int>(barY)+80,162,18,juce::Justification::left);
        }
        g.setColour(muted);g.setFont(theme.font(11));g.drawText("FFT: L/R energy / colours: bands / white: total EQ",48,static_cast<int>(area.getBottom())+25,getWidth()-300,18,juce::Justification::left);
        g.setColour(muted);g.setFont(theme.font(11));if(!first)g.drawText("Drag nodes to shape tone  /  Mouse wheel adjusts Q",24,getHeight()-25,getWidth()-48,18,juce::Justification::left);
        if(first){g.setColour(curve);g.drawText("Processors after limiter may exceed its ceiling",24,getHeight()-25,getWidth()-48,18,juce::Justification::left);}
    }
};
