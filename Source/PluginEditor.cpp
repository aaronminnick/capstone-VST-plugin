/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
CombFilterBankAudioProcessorEditor::CombFilterBankAudioProcessorEditor (CombFilterBankAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p) //listener class?
{
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    setSize (800, 600);

    addAndMakeVisible(bypassButton);
    addAndMakeVisible(preGainLabel);
    addAndMakeVisible(preGainSlider);
    addAndMakeVisible(gainLabel);
    addAndMakeVisible(gainSlider);
    addAndMakeVisible(dryLabel);
    addAndMakeVisible(wetLabel);
    addAndMakeVisible(dryWetSlider);

    combs.resize(4);
    for (auto& c : combs) addAndMakeVisible(c);

    LPHPComponent LP("Lowpass");
    LPHPComponent HP("Highpass");
}

CombFilterBankAudioProcessorEditor::~CombFilterBankAudioProcessorEditor()
{
}

//==============================================================================
void CombFilterBankAudioProcessorEditor::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    g.setColour (juce::Colours::white);
    g.setFont (15.0f);
    g.drawFittedText ("Hello World!", getLocalBounds(), juce::Justification::centred, 1);
}

void CombFilterBankAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..
}

//==============================================================================
class CombFilterBankAudioProcessorEditor::CombComponent : public juce::Component
{
public:
    CombComponent()
    {
        addAndMakeVisible(activeButton);
        addAndMakeVisible(bandsButton);
        addAndMakeVisible(pitchLabel);
        addAndMakeVisible(feedbackLabel);
        addAndMakeVisible(feedbackField);
        addAndMakeVisible(levelLabel);
        addAndMakeVisible(levelField);
        addAndMakeVisible(pitchBox);
        pitchBox.setText("Pitch");
    };

    void paint(juce::Graphics&) {};
    void resized() {};

private:
    juce::ToggleButton activeButton { "" },
                       bandsButton {"Display Bands"};
    juce::Label pitchLabel {"PitchLabel", "Pitch"}, 
                feedbackLabel {"FeedbackLabel", "Feedback"}, 
                feedbackField {"FeedbackField", "0.0"}, 
                levelLabel {"LevelLabel", "Level"}, 
                levelField {"LevelField", "0.0"};
    juce::ComboBox pitchBox {"PitchBox"};
};

class CombFilterBankAudioProcessorEditor::LPHPComponent : public juce::Component
{
public:
    LPHPComponent(juce::String filterName)
    {
        addAndMakeVisible(activeButton);
        addAndMakeVisible(freqLabel);
        freqLabel.setText(filterName);
        addAndMakeVisible(freqField);
    };

    void paint(juce::Graphics&) {};
    void resized() {};

private:
    juce::ToggleButton activeButton { "" };
    juce::Label freqLabel {"FreqLabel", "Frequency"},
                freqField {"FreqField", "0"};
};

//need to figure out how I'm going to pass the graphics to this component, as this needs DSP info
//class CombFilterBankAudioProcessorEditor::Analyzer : juce::Component