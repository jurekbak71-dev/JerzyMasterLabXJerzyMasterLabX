#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <array>
#include <memory>
#include <functional>
#include <vector>

// Concept C console home: every rotary control is a real APVTS attachment.
// No dummy transport, unsupported oversampling selector, or synthetic audio meters.
class ConceptCOverviewPanel final : public juce::Component {
    Processor& processor_;
    StudioTheme& theme_;
    struct Control {
        int card = 0, column = 0;
        juce::Label caption;
        juce::Slider slider;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    };
    std::vector<std::unique_ptr<Control>> controls_;
    std::vector<std::unique_ptr<juce::TextButton>> openButtons_;
    Processor::ResponseFrame telemetry_{};
    bool haveTelemetry_=false;
    std::array<float,256> spectrum_{};
    bool haveSpectrum_=false;
    static constexpr std::array<const char*,9> titles_{{
        "SPECTRAL EQ", "DYNAMICS", "STEREO ARCHITECT", "FINAL TRUE-PEAK",
        "ANALOG COLOR", "BASS ARCHITECT", "TRANSIENT SCULPTOR", "STABILIZER", "MASTER BRAIN"
    }};
    static constexpr std::array<const char*,9> captions_{{
        "16-BAND PARAMETRIC", "COMPRESSION / IMPACT", "WIDTH / M-S", "FINAL SAFETY STAGE",
        "TAPE / TUBE / TRANSFORMER", "LOW END", "ATTACK / SUSTAIN", "ADAPTIVE TONE", "GUIDED MASTERING"
    }};
    juce::Rectangle<float> card(int index) const {
        const float W=static_cast<float>(getWidth()),H=static_cast<float>(getHeight());
        const float margin=12.f,gap=8.f;
        const float topH=H*.305f;
        const float centreH=H*.25f,centreY=topH+25.f;
        const float bottomY=centreY+centreH+16.f;
        if(index<4){
            const float cell=(W-2.f*margin-3.f*gap)/4.f;
            return {margin+index*(cell+gap),12.f,cell,topH};
        }
        const float cell=(W-2.f*margin-4.f*gap)/5.f;
        return {margin+(index-4)*(cell+gap),bottomY,cell,juce::jmax(60.f,H-bottomY-12.f)};
    }
    void addControl(int cardNumber,int column,const char* label,const char* id) {
        if(processor_.state.getParameter(id)==nullptr)return;
        auto c=std::make_unique<Control>();
        c->card=cardNumber;c->column=column;
        c->caption.setText(label,juce::dontSendNotification);
        c->caption.setJustificationType(juce::Justification::centred);
        c->caption.setColour(juce::Label::textColourId,juce::Colour(0xff1a2a38));
        c->slider.setComponentID(juce::String("console-")+id);
        c->slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        c->slider.setTextBoxStyle(juce::Slider::TextBoxBelow,false,78,20);
        c->slider.setColour(juce::Slider::textBoxTextColourId,juce::Colour(0xff182e3c));
        c->slider.setColour(juce::Slider::textBoxBackgroundColourId,juce::Colour(0xffe8edf0));
        c->slider.setColour(juce::Slider::textBoxOutlineColourId,juce::Colour(0xffa2b0bc));
        c->slider.setColour(juce::Slider::rotarySliderFillColourId,juce::Colour(0xff087dff));
        addAndMakeVisible(c->caption);addAndMakeVisible(c->slider);
        c->attachment=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            processor_.state,id,c->slider);
        controls_.push_back(std::move(c));
    }
    void section(juce::Graphics& g,juce::Rectangle<float> r,int index) {
        g.setColour(juce::Colour(0xff9ca6ae).withAlpha(.38f));
        g.fillRoundedRectangle(r.translated(2.f,3.f),8.f);
        g.setGradientFill(juce::ColourGradient(juce::Colour(0xfff8fafb),r.getX(),r.getY(),
                                               juce::Colour(0xffcbd4db),r.getX(),r.getBottom(),false));
        g.fillRoundedRectangle(r,8.f);
        g.setColour(juce::Colour(0xff9aa8b4));g.drawRoundedRectangle(r,8.f,1.f);
        g.setColour(juce::Colour(index==3?0xfff53148:0xff087dff));
        g.fillRoundedRectangle(r.getX()+8.f,r.getY()+9.f,4.f,24.f,2.f);
        if(index<8){
            constexpr std::array<const char*,8> onIds{{
                "eq","dynamicsOn","stereoOn","truePeakOn",
                "analogOn","bassOn","transientOn","stabilizerOn"
            }};
            auto* value=processor_.state.getRawParameterValue(onIds[static_cast<size_t>(index)]);
            const bool on=value!=nullptr&&value->load()>.5f;
            theme_.paintPowerLed(g,juce::roundToInt(r.getRight()-85.f),
                                 juce::roundToInt(r.getY()+11.f),22,on);
        }
        g.setColour(juce::Colour(0xff172631));g.setFont(theme_.font(r.getWidth()<200?10.f:12.f));
        g.drawFittedText(titles_[static_cast<size_t>(index)],juce::Rectangle<int>(
            static_cast<int>(r.getX()+17),static_cast<int>(r.getY()+8),
            juce::jmax(40,static_cast<int>(r.getWidth()-82)),23),juce::Justification::left,1);
        g.setColour(juce::Colour(0xff586a79));g.setFont(theme_.font(8.f));
        g.drawFittedText(captions_[static_cast<size_t>(index)],
            static_cast<int>(r.getX()+15),static_cast<int>(r.getY()+34),
            juce::jmax(40,static_cast<int>(r.getWidth()-30)),13,juce::Justification::left,1);
    }
    static juce::String db(float gain){
        return gain>1.0e-9f?juce::String(juce::Decibels::gainToDecibels(gain),1):"--";
    }
public:
    std::function<void(int)> onOpen;
    ConceptCOverviewPanel(Processor& p,StudioTheme& theme):processor_(p),theme_(theme){
        setLookAndFeel(&theme_);setComponentID("concept-c-overview");
        const std::array<std::array<const char*,3>,4> topIds{{
            {{"gain","frequency","q"}},
            {{"dynThreshold","dynRatio","dynMix"}},
            {{"stereoLowWidth","stereoMidWidth","stereoHighWidth"}},
            {{"truePeakCeiling","truePeakDrive","truePeakRelease"}}
        }};
        const std::array<std::array<const char*,3>,4> topLabels{{
            {{"GAIN B1","FREQ B1","Q B1"}},
            {{"THRESHOLD","RATIO","MIX"}},
            {{"LOW WIDTH","MID WIDTH","HIGH WIDTH"}},
            {{"CEILING","DRIVE","RELEASE"}}
        }};
        for(int card=0;card<4;++card)
            for(int c=0;c<3;++c)addControl(card,c,topLabels[static_cast<size_t>(card)][static_cast<size_t>(c)],
                                           topIds[static_cast<size_t>(card)][static_cast<size_t>(c)]);
        const std::array<const char*,4> bottomIds{{"analogDrive","bassSub","transientAttackLow","stabilizerAmount"}};
        const std::array<const char*,4> bottomLabels{{"DRIVE","SUB","LOW ATTACK","AMOUNT"}};
        for(int i=0;i<4;++i)addControl(i+4,0,bottomLabels[static_cast<size_t>(i)],bottomIds[static_cast<size_t>(i)]);
        for(int i=0;i<9;++i){
            auto b=std::make_unique<juce::TextButton>("OPEN");
            b->setComponentID("console-open-"+juce::String(i));
            b->onClick=[this,i]{if(onOpen)onOpen(i);};
            addAndMakeVisible(*b);openButtons_.push_back(std::move(b));
        }
    }
    ~ConceptCOverviewPanel()override{setLookAndFeel(nullptr);}
    void update(const Processor::ResponseFrame& frame){telemetry_=frame;haveTelemetry_=true;repaint();}
    void updateSpectrum(const std::array<float,256>& bins){spectrum_=bins;haveSpectrum_=true;repaint();}
    bool hasMeasuredSpectrum() const noexcept{return haveSpectrum_;}
    int connectedControlCount() const noexcept{return static_cast<int>(controls_.size());}
    void resized()override{
        for(size_t i=0;i<openButtons_.size();++i){
            auto r=card(static_cast<int>(i));
            openButtons_[i]->setBounds(static_cast<int>(r.getRight()-62),static_cast<int>(r.getY()+11),54,22);
        }
        for(auto& p:controls_){
            const auto r=card(p->card);
            const float cell=p->card<4?(r.getWidth()-22.f)/3.f:r.getWidth()-22.f;
            const float x=r.getX()+11.f+p->column*cell;
            const float y=r.getY()+(p->card<4?70.f:72.f);
            const float h=juce::jmax(35.f,r.getBottom()-y-9.f);
            p->caption.setBounds(static_cast<int>(x),static_cast<int>(y-15.f),
                                 static_cast<int>(cell),17);
            p->slider.setBounds(static_cast<int>(x),static_cast<int>(y),
                                static_cast<int>(cell),static_cast<int>(h));
        }
    }
    void paint(juce::Graphics& g)override{
        theme_.paintPowderCoat(g,getLocalBounds());
        for(int index=0;index<9;++index)section(g,card(index),index);
        const float W=static_cast<float>(getWidth()),H=static_cast<float>(getHeight());
        const float top=H*.305f+25.f,centreHeight=H*.25f;
        const juce::Rectangle<float> pane(12.f,top,W-24.f,centreHeight);
        g.setColour(juce::Colour(0xff202c38));g.fillRoundedRectangle(pane,9.f);
        g.setColour(juce::Colour(0xff8298aa));g.drawRoundedRectangle(pane,9.f,1.f);
        g.setFont(theme_.font(12.f));g.setColour(juce::Colour(0xff1d91ff));
        g.drawText("MASTER MONITOR / LIVE DSP",static_cast<int>(pane.getX()+18),
                   static_cast<int>(pane.getY()+10),static_cast<int>(pane.getWidth()-36),24,
                   juce::Justification::left);
        if(haveTelemetry_){
            const bool clipped=telemetry_.outputReading.peak>1.f;
            const int lampX=juce::roundToInt(pane.getRight()-123.f);
            theme_.paintStatusLed(g,lampX,juce::roundToInt(pane.getY()+10.f),23,clipped);
            g.setColour(juce::Colour(clipped?0xffff5669:0xff70e89e));
            g.setFont(theme_.font(9.f));
            g.drawText(clipped?"SAMPLE CLIP":"SIGNAL OK",lampX+24,
                       juce::roundToInt(pane.getY()+11.f),89,20,juce::Justification::left);
        }
        // Analyzer is fed with the same real FFT amplitude bins as the legacy editor.
        const float plotX=pane.getX()+18.f,plotY=pane.getY()+41.f;
        const float plotW=juce::jmax(80.f,pane.getWidth()*.59f-26.f);
        const float plotH=juce::jmax(34.f,pane.getHeight()-69.f);
        const juce::Rectangle<float> plot(plotX,plotY,plotW,plotH);
        g.setColour(juce::Colour(0xff0c1825));g.fillRoundedRectangle(plot,4.f);
        for(int tick=1;tick<4;++tick){
            const float y=plotY+tick*plotH/4.f;
            g.setColour(juce::Colour(0xff273c4e));g.drawHorizontalLine(static_cast<int>(y),plotX,plotX+plotW);
        }
        if(haveSpectrum_){
            juce::Path p;
            for(size_t i=0;i<spectrum_.size();++i){
                const float dbValue=juce::Decibels::gainToDecibels(juce::jmax(0.f,spectrum_[i]),-100.f);
                const float normalized=juce::jlimit(0.f,1.f,(dbValue+90.f)/90.f);
                const float x=plotX+plotW*static_cast<float>(i)/(spectrum_.size()-1);
                const float y=plotY+plotH*(1.f-normalized);
                if(i==0)p.startNewSubPath(x,y);else p.lineTo(x,y);
            }
            juce::Graphics::ScopedSaveState clip(g);
            g.reduceClipRegion(plot.toNearestInt());
            g.setColour(juce::Colour(0xff047cff).withAlpha(.22f));
            juce::Path area=p;area.lineTo(plot.getRight(),plot.getBottom());
            area.lineTo(plot.getX(),plot.getBottom());area.closeSubPath();g.fillPath(area);
            g.setColour(juce::Colour(0xff3bbcff));g.strokePath(p,juce::PathStrokeType(1.5f));
        }else{
            g.setColour(juce::Colour(0xff7294aa));g.setFont(theme_.font(10.f));
            g.drawText("Waiting for audio / FFT",plot.toNearestInt(),juce::Justification::centred);
        }
        const float statX=plot.getRight()+18.f;
        const float statW=juce::jmax(80.f,pane.getRight()-statX-13.f);
        const float statY=pane.getY()+41.f;
        const float cellW=statW*.5f,cellH=juce::jmax(24.f,(pane.getHeight()-61.f)*.5f);
        const std::array<juce::String,4> values{{
            haveTelemetry_?db(telemetry_.inputReading.peak)+" dBFS":"--",
            haveTelemetry_?db(telemetry_.outputReading.peak)+" dBFS":"--",
            haveTelemetry_?juce::String(telemetry_.outputLufs.integrated,1)+" LUFS":"--",
            haveTelemetry_?juce::String(telemetry_.truePeak.gainReductionDb,2)+" dB":"--"
        }};
        const std::array<const char*,4> labels{{
            "INPUT PEAK","OUTPUT PEAK","LOUDNESS I","TRUE-PEAK GR"
        }};
        for(int c=0;c<4;++c){
            const int x=juce::roundToInt(statX+(c%2)*cellW);
            const int y=juce::roundToInt(statY+(c/2)*cellH);
            const int w=juce::roundToInt(cellW-8.f);
            g.setColour(juce::Colour(0xff9ab1c3));g.setFont(theme_.font(8.f));
            g.drawFittedText(labels[static_cast<size_t>(c)],x,y,w,13,juce::Justification::left,1);
            g.setColour(juce::Colour(c==3?0xff38e87a:0xffedf7ff));
            g.setFont(theme_.font(juce::jmin(16.f,cellH*.38f)));
            g.drawFittedText(values[static_cast<size_t>(c)],x,y+14,w,
                             juce::jmax(13,juce::roundToInt(cellH*.48f)),juce::Justification::left,1);
        }
        g.setColour(juce::Colour(0xff7d9aaf));g.setFont(theme_.font(8.f));
        g.drawText("20 Hz                                  FREQUENCY (LOG)                                  20 kHz",
                   static_cast<int>(plotX),static_cast<int>(plot.getBottom()+3.f),
                   static_cast<int>(plotW),13,juce::Justification::centred);
        // Master Brain is guided analysis, not a fake AI or a passive empty card.
        auto brain=card(8);
        g.setColour(juce::Colour(0xff2c4f67));
        g.fillRoundedRectangle(brain.getX()+13.f,brain.getY()+67.f,brain.getWidth()-26.f,
                               juce::jmax(12.f,brain.getHeight()-83.f),5.f);
        g.setColour(juce::Colour(0xff91d8ff));g.setFont(theme_.font(9.f));
        g.drawFittedText("ANALYSE  /  REVIEW  /  APPLY",
                         static_cast<int>(brain.getX()+18.f),static_cast<int>(brain.getY()+75.f),
                         static_cast<int>(brain.getWidth()-36.f),
                         juce::jmax(16,static_cast<int>(brain.getHeight()-97.f)),
                         juce::Justification::centred,2);
    }
};
