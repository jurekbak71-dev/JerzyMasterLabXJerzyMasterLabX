#pragma once
// Dedicated Hybrid Studio colour panel. Included after Processor declaration.
#include <memory>
#include <vector>
class AnalogColorPanel final : public juce::Component {
    Processor& processor_;
    StudioTheme& theme_;
    juce::ToggleButton included_{"In chain"},enabled_{"Active"},autoGain_{"Level compensation"};
    juce::ComboBox mode_;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> presentAttach_,onAttach_,gainAttach_;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> modeAttach_;
    struct Control {
        juce::Label label;
        juce::Slider slider;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    };
    std::vector<std::unique_ptr<Control>> controls_;
    masterlab::AnalogColorReading reading_{};
    void addControl(const char* title,const char* parameter){
        auto c=std::make_unique<Control>();
        c->label.setText(title,juce::dontSendNotification);
        c->label.setColour(juce::Label::textColourId,juce::Colour(0xffd4c3b2));
        c->slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        c->slider.setTextBoxStyle(juce::Slider::TextBoxBelow,false,85,22);
        c->slider.setComponentID(parameter);
        c->slider.setColour(juce::Slider::rotarySliderFillColourId,juce::Colour(0xffefa268));
        addAndMakeVisible(c->label);addAndMakeVisible(c->slider);
        c->attachment=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor_.state,parameter,c->slider);
        controls_.push_back(std::move(c));
    }
public:
    AnalogColorPanel(Processor& processor,StudioTheme& theme):processor_(processor),theme_(theme){
        setComponentID("analog-color-panel");
        setLookAndFeel(&theme_);
        included_.setComponentID("analog-present");enabled_.setComponentID("analog-enabled");autoGain_.setComponentID("analog-auto-gain");
        addAndMakeVisible(included_);addAndMakeVisible(enabled_);addAndMakeVisible(autoGain_);addAndMakeVisible(mode_);
        mode_.setComponentID("analog-model");
        mode_.addItem("Tube",1);mode_.addItem("Tape",2);mode_.addItem("Transformer",3);mode_.addItem("Solid State",4);
        presentAttach_=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(processor.state,"analogPresent",included_);
        onAttach_=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(processor.state,"analogOn",enabled_);
        gainAttach_=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(processor.state,"analogAutoGain",autoGain_);
        modeAttach_=std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(processor.state,"analogMode",mode_);
        addControl("Drive","analogDrive");addControl("Bias","analogBias");addControl("Tone","analogTone");
        addControl("Compression","analogDynamics");addControl("Impact","analogImpact");addControl("Dry / Wet","analogMix");addControl("Output","analogOutput");
        included_.setTooltip("Analog Color is an optional post-chain processor after Harmonic Exciter.");
        mode_.setTooltip("Four original distinct saturation algorithms. These are not hardware circuit clones.");
    }
    ~AnalogColorPanel()override {setLookAndFeel(nullptr);}
    void update(const Processor::ResponseFrame& r){reading_=r.analog;repaint();}
    void resized()override {
        included_.setBounds(24,16,120,28);enabled_.setBounds(156,16,110,28);
        autoGain_.setBounds(278,16,182,28);mode_.setBounds(470,16,200,28);
        const int cols=std::max(3,std::min(6,getWidth()/145));
        const int width=std::max(110,(getWidth()-48)/cols);
        for(size_t i=0;i<controls_.size();++i){int x=24+static_cast<int>(i%cols)*width;int y=110+static_cast<int>(i/cols)*138;
            controls_[i]->label.setBounds(x,y,width-8,22);
            controls_[i]->slider.setBounds(x,y+24,width-8,110);
        }
    }
    void paint(juce::Graphics& g)override {
        g.fillAll(juce::Colour(0xffedf0f2));
  // Concept C powder-coated sheet-metal side rails and inset dark instrument panel.
  const float panelWidth=juce::jmax(1.f,static_cast<float>(getWidth())-20.f);
  g.setGradientFill(juce::ColourGradient(juce::Colour(0xff344553),10.f,0.f,
                                        juce::Colour(0xff111b25),10.f,static_cast<float>(getHeight()),false));
  g.fillRect(10.f,0.f,panelWidth,static_cast<float>(getHeight()));
  g.setColour(juce::Colour(0xff96a7b6));
  g.drawVerticalLine(10,0.f,static_cast<float>(getHeight()));
  g.drawVerticalLine(juce::jmax(10,getWidth()-11),0.f,static_cast<float>(getHeight()));g.setColour(juce::Colour(0xffe9bb89));
        g.setFont(juce::Font(juce::FontOptions(22.f)));
        g.drawText("ANALOG COLOR",24,56,getWidth()-48,34,juce::Justification::left);
        g.setFont(13.f);g.setColour(juce::Colour(0xffc1b6ac));
        const int y=std::max(325,getHeight()-132);
        g.drawText("ACTUAL DSP  /  Input RMS: "+juce::String(reading_.inputRms,4)+"   Output RMS: "+juce::String(reading_.outputRms,4)+"   Delta RMS: "+juce::String(reading_.deltaRms,4),24,y,getWidth()-48,28,juce::Justification::left);
        g.setColour(juce::Colour(0xff343e46));g.fillRoundedRectangle(24.f,static_cast<float>(y+35),static_cast<float>(getWidth()-48),12.f,4.f);
        g.setColour(juce::Colour(0xffefa268));g.fillRoundedRectangle(24.f,static_cast<float>(y+35),static_cast<float>(getWidth()-48)*juce::jlimit(0.f,1.f,reading_.deltaRms*4.f),12.f,4.f);
        g.setColour(juce::Colour(0xffaaa7a2));g.setFont(12.f);
        g.drawText("Experimental 1x: high-frequency aliasing possible. Processing after limiter may exceed sample-peak ceiling.",24,getHeight()-40,getWidth()-48,32,juce::Justification::left);
    }
};
