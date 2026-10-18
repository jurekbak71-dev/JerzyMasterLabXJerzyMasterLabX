#pragma once
#include "Analysis.h"

// Included after Processor. GUI work and FFT never run in the audio callback.
class Editor final : public juce::AudioProcessorEditor,
                     public juce::DragAndDropContainer,
                     public juce::DragAndDropTarget,
                     private juce::Timer {
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;
    Processor& processor;
    juce::LookAndFeel_V4 theme;
    juce::TooltipWindow tooltips{this, 600};
    juce::ComboBox bandChoice, fftChoice;
    juce::Slider frequency, gain, quality, ceiling;
    juce::Label frequencyLabel, gainLabel, qualityLabel, ceilingLabel;
    juce::ToggleButton eqEnabled{"EQ active"}, limiterEnabled{"Limiter active"};
    std::unique_ptr<SliderAttachment> frequencyAttachment, gainAttachment, qualityAttachment, ceilingAttachment;
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
    const juce::Colour background{0xff10151e}, panel{0xff192330}, muted{0xff94a7b8};
    const juce::Colour accent{0xff58dac5}, curve{0xffe7bb6a};

    juce::String parameterId(const char* base, int band) const {
        return juce::String(base) + (band == 0 ? juce::String{} : juce::String(band + 1));
    }
    float value(const char* base, int band) const {
        return masterlab::clean(processor.state.getRawParameterValue(parameterId(base,band))->load());
    }
    double sampleRate() const { return processor.displayRate.load(); }
    juce::Rectangle<float> plot() const {
        return {48.f, 174.f, std::max(200.f, getWidth() - 300.f), std::max(100.f, getHeight() - 445.f)};
    }
    float xForFrequency(double hz) const {
        auto area = plot();
        return area.getX() + area.getWidth() * static_cast<float>(std::log(std::max(20.,hz)/20.) / std::log(sampleRate()*.5/20.));
    }
    float yForGain(float db) const { auto area=plot(); return area.getCentreY()-db*area.getHeight()/36.f; }
    juce::Point<float> node(int band) const {
        return {xForFrequency(std::min<double>(value("frequency",band),sampleRate()*.45)), yForGain(value("gain",band))};
    }
    void selectBand(int band) {
        selectedBand=juce::jlimit(0,15,band);
        frequencyAttachment.reset();gainAttachment.reset();qualityAttachment.reset();
        frequencyAttachment=std::make_unique<SliderAttachment>(processor.state,parameterId("frequency",selectedBand),frequency);
        gainAttachment=std::make_unique<SliderAttachment>(processor.state,parameterId("gain",selectedBand),gain);
        qualityAttachment=std::make_unique<SliderAttachment>(processor.state,parameterId("q",selectedBand),quality);
        bandChoice.setSelectedId(selectedBand+1,juce::dontSendNotification);
        repaint();
    }
    void setParameter(const char* base,float number,bool completeGesture=false) {
        auto* parameter=processor.state.getParameter(parameterId(base,selectedBand));
        if(completeGesture)parameter->beginChangeGesture();
        parameter->setValueNotifyingHost(parameter->convertTo0to1(number));
        if(completeGesture)parameter->endChangeGesture();
    }
    void endDrag() {
        if(!draggingNode)return;
        processor.state.getParameter(parameterId("frequency",selectedBand))->endChangeGesture();
        processor.state.getParameter(parameterId("gain",selectedBand))->endChangeGesture();
        draggingNode=false;
    }
    void timerCallback() override {
        int start,count,start2,count2;
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
        slider.setTextBoxStyle(juce::Slider::TextBoxBelow,false,100,24);
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
        frequency.setTooltip("Selected bell frequency. Drag its graph node horizontally to change it.");
        gain.setTooltip("Selected band gain. Drag its graph node vertically to change it.");
        quality.setTooltip("Band width. Higher Q is narrower. Use the mouse wheel over the graph node.");
        ceiling.setTooltip("Sample-peak ceiling at the limiter output. This is not a True Peak limiter. EQ after the limiter can exceed this ceiling.");
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
        selectBand(0);
        setResizable(true,true);setResizeLimits(750,550,2000,1400);setSize(1100,720);startTimerHz(30);
    }
    ~Editor() override {stopTimer();endDrag();setLookAndFeel(nullptr);}
    void resized() override {
        const int top=getHeight()-171,width=(getWidth()-64)/4;
        for(int i=0;i<4;++i){auto* label=std::array<juce::Label*,4>{&frequencyLabel,&gainLabel,&qualityLabel,&ceilingLabel}[i];auto* slider=std::array<juce::Slider*,4>{&frequency,&gain,&quality,&ceiling}[i];label->setBounds(32+i*width,top,width-12,22);slider->setBounds(32+i*width,top+22,width-16,120);}
        bandChoice.setBounds(28,getHeight()-211,150,28);fftChoice.setBounds(338,getHeight()-211,164,28);
        eqEnabled.setBounds(194,getHeight()-211,125,28);limiterEnabled.setBounds(getWidth()-180,getHeight()-211,160,28);
    }
    bool isInterestedInDragSource(const SourceDetails& details) override {return details.description=="eq"||details.description=="limiter";}
    void itemDropped(const SourceDetails& details) override {
        if(details.localPosition.y<82||details.localPosition.y>132)return;
        bool right=details.localPosition.x>getWidth()/2;
        bool first=details.description=="eq"?right:!right;
        auto* parameter=processor.state.getParameter("order");parameter->beginChangeGesture();parameter->setValueNotifyingHost(first?1.f:0.f);parameter->endChangeGesture();repaint();
    }
    void mouseDown(const juce::MouseEvent& event) override {
        if(!plot().expanded(12).contains(event.position))return;
        int nearest=selectedBand;float distance=event.position.getDistanceFrom(node(nearest));
        for(int i=0;i<16;++i){if(i!=selectedBand&&std::abs(value("gain",i))<.01f)continue;float d=event.position.getDistanceFrom(node(i));if(d<distance){distance=d;nearest=i;}}
        if(distance>14)return;
        selectBand(nearest);draggingNode=true;
        processor.state.getParameter(parameterId("frequency",selectedBand))->beginChangeGesture();
        processor.state.getParameter(parameterId("gain",selectedBand))->beginChangeGesture();
    }
    void mouseDrag(const juce::MouseEvent& event) override {
        if(draggingNode){auto area=plot();float normal=juce::jlimit(0.f,1.f,(event.position.x-area.getX())/area.getWidth());
            setParameter("frequency",static_cast<float>(20*std::pow(sampleRate()*.5/20,normal)));
            setParameter("gain",juce::jlimit(-18.f,18.f,(area.getCentreY()-event.position.y)*36/area.getHeight()));repaint();return;}
        if(event.getMouseDownY()<82||event.getMouseDownY()>132||isDragAndDropActive())return;
        bool right=event.getMouseDownX()>getWidth()/2,first=processor.state.getRawParameterValue("order")->load()>.5f;
        startDragging((right?!first:first)?"limiter":"eq",this);
    }
    void mouseUp(const juce::MouseEvent&) override {endDrag();}
    void mouseWheelMove(const juce::MouseEvent& event,const juce::MouseWheelDetails& wheel) override {
        if(plot().contains(event.position)&&event.position.getDistanceFrom(node(selectedBand))<24){setParameter("q",juce::jlimit(.1f,10.f,value("q",selectedBand)*std::exp(wheel.deltaY*2)),true);repaint();}
        else AudioProcessorEditor::mouseWheelMove(event,wheel);
    }
    void paint(juce::Graphics& g) override {
        g.fillAll(background);g.setColour(juce::Colours::whitesmoke);g.setFont(24);
        g.drawText("JERZY MASTERLAB X",24,13,getWidth()-300,30,juce::Justification::left);
        g.setColour(accent);g.setFont(12);g.drawText("SPECTRAL WORKSPACE / DEVELOPMENT",24,44,420,22,juce::Justification::left);
        g.setColour(muted);g.drawText("OUT "+juce::String(juce::Decibels::gainToDecibels(processor.peak.load()),1)+" dBFS   RMS "+juce::String(juce::Decibels::gainToDecibels(processor.rms.load()),1)+" dBFS",getWidth()-330,22,306,24,juce::Justification::right);
        bool first=processor.state.getRawParameterValue("order")->load()>.5f;
        for(int i=0;i<2;++i){int x=24+i*(getWidth()/2);g.setColour(panel);g.fillRoundedRectangle(static_cast<float>(x),84.f,getWidth()*.5f-48,48,6);g.setColour(i==0?accent:curve);g.fillRect(x,94,3,28);g.setFont(14);g.drawText((i==0?!first:first)?"SPECTRAL EQ  /  16 BELL BANDS":"LIMITER  /  SAMPLE PEAK",x+16,89,getWidth()/2-70,20,juce::Justification::left);g.setColour(muted);g.setFont(11);g.drawText("DRAG TO REORDER",x+16,109,220,16,juce::Justification::left);}
        auto area=plot();g.setColour(juce::Colour(0xff141c26));g.fillRoundedRectangle(area.expanded(24,12),8);
        g.setFont(11);
        for(float db:{-18.f,-12.f,-6.f,0.f,6.f,12.f,18.f}){float y=yForGain(db);g.setColour(db==0?juce::Colour(0xff536374):juce::Colour(0xff273240));g.drawHorizontalLine(static_cast<int>(y),area.getX(),area.getRight());g.setColour(muted);g.drawText(juce::String(db,0),8,static_cast<int>(y)-8,32,16,juce::Justification::right);}
        for(float hz:{20.f,50.f,100.f,200.f,500.f,1000.f,2000.f,5000.f,10000.f,20000.f}){if(hz>sampleRate()*.5)continue;float x=xForFrequency(hz);g.setColour(juce::Colour(0xff273240));g.drawVerticalLine(static_cast<int>(x),area.getY(),area.getBottom());g.setColour(muted);g.drawText(juce::String(hz>=1000?hz/1000:hz,0)+(hz>=1000?"k":""),static_cast<int>(x)-14,static_cast<int>(area.getBottom())+4,40,18,juce::Justification::centred);}
        {juce::Graphics::ScopedSaveState save(g);g.reduceClipRegion(area.toNearestInt());juce::Path spectral,total;
            for(int i=0;i<512;++i){double hz=20*std::pow(sampleRate()*.5/20,i/511.);float x=area.getX()+area.getWidth()*i/511.f;float binPosition=juce::jlimit(0.f,static_cast<float>((1<<fftOrder)/2-1),static_cast<float>(hz*(1<<fftOrder)/sampleRate()));int bin=static_cast<int>(binPosition);float level=spectrum[bin]+(binPosition-bin)*(spectrum[bin+1]-spectrum[bin]);float db=juce::Decibels::gainToDecibels(level,-100.f);float sy=area.getBottom()-area.getHeight()*juce::jlimit(0.f,1.f,(db+100)/100.f);
                if(i==0)spectral.startNewSubPath(x,sy);else spectral.lineTo(x,sy);
                if(haveResponse){std::complex<double> transfer{1,0};for(const auto& band:response.bands)transfer*=band.response(response.rate,hz);transfer=(1.-response.eqMix)+static_cast<double>(response.eqMix)*transfer;float y=yForGain(static_cast<float>(20*std::log10(std::max(1e-12,std::abs(transfer)))));if(i==0)total.startNewSubPath(x,y);else total.lineTo(x,y);}}
            g.setColour(accent.withAlpha(.48f));g.strokePath(spectral,juce::PathStrokeType(1.3f));
            if(haveResponse){juce::Path individual;for(int i=0;i<512;++i){double hz=20*std::pow(sampleRate()*.5/20,i/511.);float x=area.getX()+area.getWidth()*i/511.f;float y=yForGain(static_cast<float>(20*std::log10(std::max(1e-12,response.bands[selectedBand].magnitude(response.rate,hz)))));if(i==0)individual.startNewSubPath(x,y);else individual.lineTo(x,y);}g.setColour(curve.withAlpha(.3f));g.strokePath(individual,juce::PathStrokeType(1));g.setColour(curve);g.strokePath(total,juce::PathStrokeType(2));}
            for(int i=0;i<16;++i){if(i!=selectedBand&&std::abs(value("gain",i))<.01f)continue;auto position=node(i);g.setColour(i==selectedBand?curve:muted);g.fillEllipse(position.x-5,position.y-5,10,10);if(i==selectedBand){g.drawEllipse(position.x-9,position.y-9,18,18,1.5f);g.setFont(11);g.drawText(juce::String(i+1),static_cast<int>(position.x)+13,static_cast<int>(position.y)-12,26,22,juce::Justification::left);}}
        }
        int inspectorX=getWidth()-218;g.setColour(panel);g.fillRoundedRectangle(static_cast<float>(inspectorX),162.f,194.f,static_cast<float>(getHeight()-390),8);g.setColour(muted);g.setFont(12);g.drawText("STEREO INSPECTOR",inspectorX+12,174,170,20,juce::Justification::left);
        const float radius=std::min(56.f,(getHeight()-488.f)*.5f),cx=inspectorX+97.f,cy=204.f+radius;g.setColour(juce::Colour(0xff3c4c5c));g.drawEllipse(cx-radius,cy-radius,2*radius,2*radius,1);g.drawLine(cx-radius,cy,cx+radius,cy);g.drawLine(cx,cy-radius,cx,cy+radius);
        {juce::Graphics::ScopedSaveState save(g);g.reduceClipRegion(juce::Rectangle<int>(inspectorX+24,204,146,static_cast<int>(radius*2)));g.setColour(accent.withAlpha(.45f));for(auto point:scope)g.fillEllipse(cx+point.x*radius-1,cy-point.y*radius-1,2,2);}
        g.setColour(juce::Colours::whitesmoke);g.drawText("CORR "+juce::String(processor.correlation.load(),3),inspectorX+12,static_cast<int>(cy+radius+10),170,20,juce::Justification::left);
        g.setColour(muted);g.setFont(11);g.drawText("5 ms lookahead / sample peak",inspectorX+12,static_cast<int>(cy+radius+34),170,20,juce::Justification::left);
        g.setColour(accent);g.drawText("L/R ENERGY FFT",48,143,130,20,juce::Justification::left);g.setColour(curve);g.drawText(haveResponse?"EQ RESPONSE / AUDIO COEFFICIENTS":"EQ RESPONSE / WAITING FOR AUDIO",190,143,getWidth()-450,20,juce::Justification::left);
        g.setColour(muted);g.setFont(11);g.drawText("Spectrum: -100 to 0 dBFS   |   EQ: +/-18 dB   |   Drag a node / wheel adjusts Q",48,static_cast<int>(area.getBottom())+25,getWidth()-300,20,juce::Justification::left);
        if(first){g.setColour(curve);g.drawText("EQ follows limiter: final output may exceed limiter ceiling",24,getHeight()-25,getWidth()-48,18,juce::Justification::left);}
    }
};
