/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
CombFilterBankAudioProcessor::CombFilterBankAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
#endif
{
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
template <typename Type>
class DelayLine
{
public:
    //DelayLine() {} //do I need to use explicit constructor?

    void clear() noexcept { std::fill(rawData.begin(), rawData.end(), Type(0)); }

    size_t size() const noexcept { return rawData.size(); }

    void resize() (size_t newValue) //do I want to resize buffer? Will cause audio glitch
    {
        rawData.resize(newValue);
        leastRecentIndex = 0;
    }

    Type back() const noexcept { return rawData[leastRecentIndex] };

    Type get() (size_t delayInSamples) const noexcept
    {
        jassert(delayInSamples >= 0 && delayInSamples < size());
        return rawData[(leastRecentIndex + 1 + delayInSamples) % size()];
    }

    void set(size_t delayInSamples, Type newValue) noexcept
    {
        jassert(delayInSamples >= 0 && delayInSamples < size());
        rawData[(leastRecentIndex + 1 + delayInSamples)] = newValue;
    }

    void push(Type valueToAdd) noexcept
    {
        rawData[leastRecentIndex] = valueToAdd;
        leastRecentIndex = leastRecentIndex == 0 ? size() - 1 : leastRecentIndex - 1;
    }

private:
    std::vector<Type> rawData;
    size_t leastRecentIndex = 0;
};

//==============================================================================
template <typename Type, size_t maxNumChannels = 2>
class Comb
{
public:
    Comb() //need to pass freq
    {
        //need to think about whether I need to initialize these values or pass them into constructor
        setFeedback(0.5f);
        setLevel(0.25f);
        //need to replace below value with frequency-determined size in samples
        delayTimeSamples = (size_t) 1024;
        for (auto& dline : delayLines) dline.resize(delayTimeSamples);
    }

    void prepare(const juce::dsp::ProcessSpec& spec)
    {
        jassert(spec.numChannels <= maxNumChannels);
        sampleRate = (Type)spec.sampleRate;
        //I'm leaving out some code about resizing buffer from example

        //1: should play around with different types of filters for decay
        //2: need to calculate cutoff frequency based on pitch of comb
        coefs = juce::dsp::IIR::Coefficients<Type>::makeFirstOrderLowPass(sampleRate, Type(1e3));

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

    size_t getNumChannels() const noexcept { return delayLines.size() };

    void setFeedback(Type newValue) noexcept
    {
        jassert(newValue >= Type(0) && newValue <= Type(1));
        feedback = newValue;
    }

    void setLevel(Type newValue) noexcept
    {
        jassert(newValue >= Type(0) && newValue <= Type(1));
        level = newValue;
    }

    template <typename ProcessContext>
    void process(const ProcessContext& context) noexcept
    {
        auto& inputBlock = context.getInputBlock();
        auto& outputBlock = context.getOutputBlock();
        auto numSamples = outputBlock.getNumSamples();
        auto numChannels = outputBlock.getNumChannels();

        jassert(inputBlock.getNumSamples() == numSamples);
        jassert(inputBlock.getNumChannels() == numChannels);

        for (size_t ch = 0; ch < numChannels; ++ch)
        {
            auto* input = inputBlock.getChannelPointer(ch);
            auto* output = outputBlock.getChannelPointer(ch);
            auto& dline = delayLines[ch];
            auto& filter = filters[ch];

            for (size_t samp = 0; samp < numSamples; ++samp)
            {
                auto delayedSample = filter.processSample(dline.get(delayTimeSamples));
                auto inputSample = input[i];
                //I don't know enough math to understand hyperbolic tangent, but this is supposed to balance sum
                //if my comb doesn't work correctly, I suspect this may need closer examination
                auto dlineInputSample = std::tanh(inputSample + feedback * delayedSample);
                dline.push(dlineInputSample);
                //below may need balancing or rethinking if filter doesn't sound right
                auto outputSample = (inputSample + delayedSample) * level;
                output[i] = outputSample;
            }
        }
    }

private:
    std::array<DelayLine<Type>, maxNumChannels> delayLines;
    Type feedback { Type(0) };
    Type level { Type(0) };
    size_t delayTimeSamples;

    std::array<juce::dsp::IIR::Filter<Type>, maxNumChannels> filters;
    typename juce::dsp::IIR::Coefficients<Type>::Ptr coefs;

    Type sampleRate { Type(44.1e3) }; //need to update if I make sample rate contingent on DAW settings
};


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
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // prevents feedback if outputs > inputs
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    // This is the place where you'd normally do the guts of your plugin's
    // audio processing...
    // Make sure to reset the state if your inner loop is processing
    // the samples and the outer loop is handling the channels.
    // Alternatively, you can process the samples with the channels
    // interleaved by keeping the same state.
    for (int channel = 0; channel < totalNumInputChannels; ++channel)
    {
        auto* channelData = buffer.getWritePointer (channel);

        // ..do something to the data...
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
