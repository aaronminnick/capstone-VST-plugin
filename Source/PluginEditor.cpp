/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
CombFilterBankAudioProcessorEditor::CombFilterBankAudioProcessorEditor (CombFilterBankAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    setSize (800, 600);

    combs.resize(4);

    LPHPComponent LP;
    LPHPComponent HP;
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
class CombFilterBankAudioProcessorEditor::CombComponent : juce::Component
{
public:
    CombComponent() {};

    void paint(juce::Graphics&) {};
    void resized() {};

private:
    juce::ToggleButton active { "" },
                       bands { "Display Bands" };
    juce::ComboBox pitch;
    juce::TextEditor feedback { "0.0" },
                     level { "0.0" };
};

class CombFilterBankAudioProcessorEditor::LPHPComponent : juce::Component
{
public:
    LPHPComponent() {};

    void paint(juce::Graphics&) {};
    void resized() {};

private:
    juce::ToggleButton active { "" };
    juce::TextEditor frequency { "0" };
};

//need to figure out how I'm going to pass the graphics to this component, as this needs DSP info
//class CombFilterBankAudioProcessorEditor::Analyzer : juce::Component