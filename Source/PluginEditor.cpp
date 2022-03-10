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

    addAndMakeVisible(preGainSlider);
    preGainSlider.setRange(0.0, 1.0); // need to refactor for dB including skew
    preGainSlider.setValue(0.0, juce::dontSendNotification);
    //preGainSlider.setTextValueSuffix("dB");
    addAndMakeVisible(preGainLabel);
    preGainLabel.attachToComponent(&preGainSlider, false);

    addAndMakeVisible(gainSlider);
    gainSlider.setRange(0.0, 1.0); // need to refactor for dB including skew
    gainSlider.setValue(0.0, juce::dontSendNotification);
    //gainSlider.setTextValueSuffix("dB");
    addAndMakeVisible(gainLabel);
    gainLabel.attachToComponent(&gainSlider, false);

    addAndMakeVisible(wetSlider);
    wetSlider.setRange(0.0, 100.0);
    wetSlider.setValue(0.0, juce::dontSendNotification);
    wetSlider.setTextValueSuffix("%");
    addAndMakeVisible(wetLabel);
    wetLabel.attachToComponent(&wetSlider, true);

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

        addAndMakeVisible(feedbackField);
        addAndMakeVisible(feedbackLabel);
        feedbackLabel.attachToComponent(&feedbackField, true);

        addAndMakeVisible(levelField);
        addAndMakeVisible(levelLabel);
        levelLabel.attachToComponent(&levelField, true);

        addAndMakeVisible(pitchBox);
        pitchBox.setText("");
        pitchBox.addItemList(pitches, 0); //need to verify offset
        addAndMakeVisible(pitchLabel);
        pitchLabel.attachToComponent(&pitchBox, true);
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

    juce::StringArray pitches
    {
        "", //need to leave index 0 untouched for no selection
        "C2", "C#2", "D2", "D#2", "E2", "F2", "F#2", "G2", "G#2", "A2", "A#2", "B2",
        "C3", "C#3", "D3", "D#3", "E3", "F3", "F#3", "G3", "G#3", "A3", "A#3", "B3",
        "C4", "C#4", "D4", "D#4", "E4", "F4", "F#4", "G4", "G#4", "A4", "A#4", "B4",
        "C5", "C#5", "D5", "D#5", "E5", "F5", "F#5", "G5", "G#5", "A5", "A#5", "B5",
    };
};

class CombFilterBankAudioProcessorEditor::LPHPComponent : public juce::Component
{
public:
    LPHPComponent(juce::String filterName)
    {
        addAndMakeVisible(activeButton);
        addAndMakeVisible(freqField);
        addAndMakeVisible(freqLabel);
        freqLabel.setText(filterName, juce::dontSendNotification);
    };

    void paint(juce::Graphics&) {};
    void resized() {};

private:
    juce::ToggleButton activeButton { "" };
    juce::Label freqLabel{ "FreqLabel", "Frequency" },
                freqField {"FreqField", "0"};
};

//need to figure out how I'm going to pass the graphics to this component, as this needs DSP info
//class CombFilterBankAudioProcessorEditor::Analyzer : juce::Component