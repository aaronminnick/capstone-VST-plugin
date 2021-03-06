/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
/**
*/
class CombFilterBankAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    CombFilterBankAudioProcessorEditor (CombFilterBankAudioProcessor&);
    ~CombFilterBankAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    CombFilterBankAudioProcessor& audioProcessor;

    //add child components of main component here

    juce::ToggleButton bypassButton { "Bypass" };
    juce::Slider preGainSlider, gainSlider, wetSlider;
    juce::Label preGainLabel {"PreGainLabel", "Pre-gain"}, 
                gainLabel {"GainLabel", "Gain"},
                wetLabel {"WetLabel", "Wet Ratio"};

    class CombComponent;
    std::vector<CombComponent> combs;

    class LPHPComponent;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CombFilterBankAudioProcessorEditor)
};