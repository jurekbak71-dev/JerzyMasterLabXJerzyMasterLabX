#pragma once
// Hybrid Studio — phase 3A post-chain Harmonic Exciter controls.
// Include after Processor has been declared.
#include <array>
#include <memory>
#include <vector>
#include <utility>
class ExciterPanel final : public juce::Component {
    Processor& processor;
    StudioTheme& theme;
    juce::ToggleButton inChain{"In chain"},active{"Active"};
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> presenceAttachment,activeAttachment;
    struct Knob {
        juce::Label label;
        juce::Slider slider;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    };
    std::vector<std::unique_ptr<Knob>> knobs;
    masterlab::HarmonicExciterReading reading{};
    void addKnob(const juce::String& label,const char* id) {
        auto knob=std::make_unique<Knob>();
        knob->label.setText(label,juce::dontSendNotification);
        knob->slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        knob->slider.setTextBoxStyle(juce::Slider::TextBoxBelow,false,76,22);
        knob->slider.setComponentID(id);
        knob->slider.setColour(juce::Slider::rotarySliderFillColourId,juce::Colour(0xffdcb16e));
        addAndMakeVisible(knob->label);addAndMakeVisible(knob->slider);
        knob->attachment=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.state,id,knob->slider);
        knobs.push_back(std::move(knob));
    }
public:
    ExciterPanel(Processor& p,StudioTheme& t):processor(p),theme(t) {
        setComponentID("harmonic-exciter-panel");
        setLookAndFeel(&theme);
        addAndMakeVisible(inChain);addAndMakeVisible(active);
        inChain.setComponentID("exciter-present");active.setComponentID("exciter-active");
        presenceAttachment=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(p.state,"exciterPresent",inChain);
        activeAttachment=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(p.state,"exciterOn",active);
        inChain.setTooltip("Adds a dedicated Harmonic Exciter after the legacy 6-module chain. It is not an adjustable parallel branch.");
        active.setTooltip("Bypass the post-chain Harmonic Exciter without discarding settings.");
        for(const auto& entry:std::array<std::pair<const char*,const char*>,10>{{
            {"Low","exciterAmountLow"},{"Mid","exciterAmountMid"},
            {"High","exciterAmountHigh"},{"Air","exciterAmountAir"},
            {"Even H2","exciterEven"},{"Odd H3","exciterOdd"},
            {"Drive dB","exciterDrive"},{"Impact","exciterImpact"},{"Dry / Wet","exciterMix"},{"Output dB","exciterOutput"}
        }})addKnob(entry.first,entry.second);
    }
    ~ExciterPanel()override{setLookAndFeel(nullptr);}
    void update(const Processor::ResponseFrame& frame){reading=frame.exciter;repaint();}
    juce::Slider* controlForParameter(const juce::String& id) {
        for(auto& knob:knobs)if(knob->slider.getComponentID()==id)return &knob->slider;
        return nullptr;
    }
    void resized()override {
        inChain.setBounds(26,16,114,28);active.setBounds(146,16,110,28);
        const int columns=std::max(3,std::min(5,getWidth()/140));
        const int cell=std::max(90,(getWidth()-48)/columns);
        for(size_t i=0;i<knobs.size();++i) {
            const int col=static_cast<int>(i)%columns,row=static_cast<int>(i)/columns;
            const int x=24+col*cell,y=88+row*128;
            knobs[i]->label.setBounds(x,y,cell-8,22);
            knobs[i]->slider.setBounds(x,y+24,cell-8,100);
        }
    }
    void paint(juce::Graphics& g)override {
        g.fillAll(juce::Colour(0xff111822));
        g.setColour(juce::Colour(0xffe5c185));g.setFont(21.f);
        g.drawText("HARMONIC EXCITER",26,50,getWidth()-50,32,juce::Justification::left);
        g.setFont(12.f);g.setColour(juce::Colour(0xffaab9c9));
        g.drawText("POST CHAIN  |  phase 3A experimental 1x processor",getWidth()/2,20,getWidth()/2-24,26,juce::Justification::right);
        const int y=std::max(330,getHeight()-110);
        g.drawText("DSP GENERATED RMS PER BAND",24,y,getWidth()-48,25,juce::Justification::left);
        const std::array<juce::Colour,4> colours{
            juce::Colour(0xff5fcce8),juce::Colour(0xffd67bea),
            juce::Colour(0xffef907d),juce::Colour(0xffa7dfcb)};
        for(int i=0;i<4;++i) {
            const int w=(getWidth()-48)/4,x=24+i*w;
            const float level=reading.generatedRms[static_cast<size_t>(i)];
            const float scaled=juce::jlimit(0.f,1.f,level*3.f);
            g.setColour(juce::Colour(0xff222c38));g.fillRoundedRectangle(static_cast<float>(x),static_cast<float>(y+30),static_cast<float>(w-8),10.f,3.f);
            g.setColour(colours[static_cast<size_t>(i)]);g.fillRoundedRectangle(static_cast<float>(x),static_cast<float>(y+30),static_cast<float>(w-8)*scaled,10.f,3.f);
        }
        g.setColour(juce::Colour(0xffaab9c9));
        g.drawText("Output follows all previous processors. Not true-peak limited after this stage.",24,getHeight()-35,getWidth()-48,20,juce::Justification::left);
    }
};
