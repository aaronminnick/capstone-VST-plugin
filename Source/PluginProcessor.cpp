/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
class CombFilterBankAudioProcessor::DelayLine
{
public:
    //DelayLine() {} //do I need to use explicit constructor?

    void clear() noexcept { std::fill(rawData.begin(), rawData.end(), 0); }

    size_t size() const noexcept { return rawData.size(); }

    void resize(size_t newValue) //do I want to resize buffer? Will cause audio glitch
    {
        rawData.resize(newValue);
        leastRecentIndex = 0;
}

    float back() const noexcept { return rawData[leastRecentIndex]; }

    float get(size_t delayInSamples) const noexcept
    {
        jassert(delayInSamples >= 0 && delayInSamples < size());
        return rawData[(leastRecentIndex + 1 + delayInSamples) % size()];
    }

    void set(size_t delayInSamples, float newValue) noexcept
    {
        jassert(delayInSamples >= 0 && delayInSamples < size());
        rawData[(leastRecentIndex + 1 + delayInSamples)] = newValue;
    }

    void push(float valueToAdd) noexcept
    {
        rawData[leastRecentIndex] = valueToAdd;
        leastRecentIndex = leastRecentIndex == 0 ? size() - 1 : leastRecentIndex - 1;
    }

private:
    std::vector<float> rawData;
    size_t leastRecentIndex = 0;
};

//==============================================================================
class CombFilterBankAudioProcessor::Comb
{
public:
    Comb() //need to pass freq
    {
        active = false;
        //need to think about whether I need to initialize these values or pass them into constructor
        setFeedback(0.5f);
        setLevel(0.25f);
        //need to replace below value with frequency-determined size in samples
        delayTimeSamples = (size_t)1024;
        for (auto& dline : delayLines) dline.resize(delayTimeSamples);
    }

    void prepare(const juce::dsp::ProcessSpec& spec)
    {
        jassert(spec.numChannels <= maxNumChannels);
        double sampleRate = spec.sampleRate;
        //I'm leaving out some code about resizing buffer from example

        //1: should play around with different types of filters for decay
        //2: need to calculate cutoff frequency based on pitch of comb
        coefs = juce::dsp::IIR::Coefficients<double>::makeFirstOrderLowPass(sampleRate, 1e3);

        for (auto& f : filters)
        {
            f.prepare(spec);
            f.coefficients = coefs;
        }
    }

    void reset() noexcept
    {
        for (auto& f : filters) f.reset();
        for (auto& dline : delayLines) dline.clear();
    }

    size_t getNumChannels() const noexcept { return delayLines.size(); }

    bool isActive() noexcept { return active; }
    void toggleActive() noexcept { active = !active; }

    void setFeedback(float newValue) noexcept
    {
        jassert(newValue >= 0.0f && newValue <= 1.0f);
        feedback = newValue;
    }

    void setLevel(float newValue) noexcept
    {
        jassert(newValue >= 0.0f && newValue <= 1.0f);
        level = newValue;
    }

    float processSample(size_t ch, float currentSamp) noexcept
    {
        auto& dline = delayLines[ch];
        auto& filter = filters[ch];

        auto delayedSample = filter.processSample(dline.get(delayTimeSamples));
        //I don't know enough math to understand hyperbolic tangent, but this is supposed to balance sum
        //if my comb doesn't work correctly, I suspect this may need closer examination
        auto dlineInputSample = std::tanh(currentSamp + (feedback * delayedSample));
        dline.push(dlineInputSample);
        //below may need balancing or rethinking if filter doesn't sound right
        //return (currentSamp + delayedSample) * level;
        return delayedSample * level;
    }

private:
    bool active;
    static const size_t maxNumChannels{ 2 };
    std::array<DelayLine, maxNumChannels> delayLines;
    float feedback{ 0.0f };
    float level{ 0.0f };
    size_t delayTimeSamples;

    std::array<juce::dsp::IIR::Filter<double>, maxNumChannels> filters;
    typename juce::dsp::IIR::Coefficients<double>::Ptr coefs;

    double sampleRate{ 44.1e3 }; //need to update if I make sample rate contingent on DAW settings
};

//==============================================================================
//why is this happy to come after private classes when editor isn't??
CombFilterBankAudioProcessor::CombFilterBankAudioProcessor() 
    : AudioProcessor(BusesProperties().withInput("Input", juce::AudioChannelSet::stereo())
                                      .withOutput("Output", juce::AudioChannelSet::stereo()))
{
    bypass = true;
    LPActive = false;
    HPActive = false;
    combs.resize(4);
    for (size_t i = 0; i < 4; ++i) combs[i] = Comb();
}

CombFilterBankAudioProcessor::~CombFilterBankAudioProcessor()
{
}

//==============================================================================
const juce::String CombFilterBankAudioProcessor::getName() const { return JucePlugin_Name; }
bool CombFilterBankAudioProcessor::acceptsMidi() const { return false; }
bool CombFilterBankAudioProcessor::producesMidi() const { return false; }
bool CombFilterBankAudioProcessor::isMidiEffect() const { return false; }
double CombFilterBankAudioProcessor::getTailLengthSeconds() const { return 0.0; }
int CombFilterBankAudioProcessor::getNumPrograms() { return 1; }
int CombFilterBankAudioProcessor::getCurrentProgram() { return 0; }
void CombFilterBankAudioProcessor::setCurrentProgram (int index) {}
const juce::String CombFilterBankAudioProcessor::getProgramName(int index) { return {}; }
void CombFilterBankAudioProcessor::changeProgramName (int index, const juce::String& newName) {}

//==============================================================================
void CombFilterBankAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
}

void CombFilterBankAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool CombFilterBankAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void CombFilterBankAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    auto mainInputOutput = getBusBuffer(buffer, true, 0);

    // prevents feedback if outputs > inputs
    //for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        //buffer.clear (i, 0, buffer.getNumSamples());

     // This is the place where you'd normally do the guts of your plugin's
    // audio processing...
    // Make sure to reset the state if your inner loop is processing
    // the samples and the outer loop is handling the channels.
    // Alternatively, you can process the samples with the channels
    // interleaved by keeping the same state.
    for (size_t channel = 0; channel < mainInputOutput.getNumChannels(); ++channel)
    {       
        //attempt at combing 
        float combedSamps[4] = {0.0f, 0.0f, 0.0f, 0.0f};
        for (size_t samp = 0; samp < buffer.getNumSamples(); ++samp)
        {
            float wetSamp = 0.0f;
            size_t balanceDivisor = 0;
            for (size_t c = 0; c < 4; ++c)
            {
                combedSamps[c] = 0.0f;
                if (combs[c].isActive()) 
                {
                    balanceDivisor++;
                    //I need to figure out how I'm getting data back if not passing a context?
                    combedSamps[c] = combs[c].processSample(channel, *mainInputOutput.getReadPointer(channel, samp));
                }
                //add samples to output
                wetSamp += combedSamps[c];
            }
            wetSamp /= balanceDivisor;
            //output the sample into buffer here, balancing with input based on current wet/dry
            float wetLevel = 0.5f; //hard coded 50% wet for now until I link slider
            *mainInputOutput.getWritePointer(channel, samp) = *mainInputOutput.getReadPointer(channel, samp) * (1.0f - wetLevel) + wetSamp * wetLevel;
        }
    }
}

//==============================================================================
bool CombFilterBankAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* CombFilterBankAudioProcessor::createEditor()
{
    return new CombFilterBankAudioProcessorEditor (*this);
}

//==============================================================================
void CombFilterBankAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void CombFilterBankAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new CombFilterBankAudioProcessor();
}
