/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

auto getPhaserRateName() { return juce::String("Phaser Rate (Hz)"); }
auto getPhaserDepthName() { return juce::String("Phaser Depth (%)"); }
auto getPhaserCenterFreqName() { return juce::String("Phaser Center Frequency (Hz)"); }
auto getPhaserFeedbackName() { return juce::String("Phaser Feedback (%)"); }
auto getPhaserMixName() { return juce::String("Phaser Mix (%)"); }
auto getPhaserBypassName() { return juce::String("Phaser Bypass"); }

auto getChorusRateName() { return juce::String("Chorus Rate (Hz)"); }
auto getChorusDepthName() { return juce::String("Chorus Depth (%)"); }
auto getChorusCenterDelayName() { return juce::String("Chorus Center Delay (Ms)"); }
auto getChorusFeedbackName() { return juce::String("Chorus Feedback (%)"); }
auto getChorusMixName() { return juce::String("Chorus Mix (%)"); }
auto getChorusBypassName() { return juce::String("Chorus Bypass"); }

auto getOverdriveSaturationName() { return juce::String("Overdrive Saturation"); }
auto getOverdriveBypassName() { return juce::String("Overdrive Bypass"); }

auto getLadderFilterModeName() { return juce::String("Ladder Filter Mode"); }
auto getLadderFilterCutoffName() { return juce::String("Ladder Filter Cutoff (Hz)"); }
auto getLadderFilterResonanceName() { return juce::String("Ladder Filter Resonance"); }
auto getLadderFilterDriveName() { return juce::String("Ladder Filter Drive"); }
auto getLadderFilterBypassName() { return juce::String("Ladder Filter Bypass"); }

auto getLadderFilterChoices() 
{
    return juce::StringArray
    {
        "LPF12", // low-pass  12 dB/octave 
        "HPF12", // high-pass 12 dB/octave 
        "BPF12", // band-pass 12 dB/octave 
        "LPF24", // low-pass  24 dB/octave 
        "HPF24", // high-pass 24 dB/octave 
        "BPF24"  // band-pass 24 dB/octave 
    };
}

auto getGeneralFilterModeName() { return juce::String("General Filter Mode"); }
auto getGeneralFilterFreqName() { return juce::String("General Filter Frequency (Hz)"); }
auto getGeneralFilterQualityName() { return juce::String("General Filter Quality"); }
auto getGeneralFilterGainName() { return juce::String("General Filter Gain (dB)"); }
auto getGeneralFilterBypassName() { return juce::String("General Filter Bypass"); }

auto getGeneralFilterChoices()
{
    return juce::StringArray
    {
        "Peak",
        "Bandpass",
        "Notch",
        "Allpass"
    };
}

auto getSelectedTabName() { return juce::String("Selected Tab"); }

auto getInputGainName() { return juce::String("Input Gain (dB)"); }
auto getOutputGainName() { return juce::String("Output Gain (dB)"); }

//==============================================================================
JUCE_MultiFX_ProcessorAudioProcessor::JUCE_MultiFX_ProcessorAudioProcessor()
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

    for (size_t i = 0; i < static_cast<size_t>(DSP_Option::END_OF_LIST); ++i)
    {
		dspOrder[i] = static_cast<DSP_Option>(i);
	}

	restoreDspOrderFifo.push(dspOrder);

    auto floatParams = std::array
    {
		&phaserRateHz,
		&phaserDepthPercent,
		&phaserCenterFreqHz,
		&phaserFeedbackPercent,
        &phaserMixPercent,

        &chorusRateHz,
        &chorusDepthPercent,
        &chorusCenterDelayMs,
        &chorusFeedbackPercent,
        &chorusMixPercent,

        &overdriveSaturation,

        &ladderFilterCutoffHz,
        &ladderFilterResonance,
        &ladderFilterDrive,

        &generalFilterFreqHz,
        &generalFilterQuality,
        &generalFilterGain,

        &inputGain,
		&outputGain,
    };

    auto floatNameFuncs = std::array
    {
		&getPhaserRateName,
		&getPhaserDepthName,
		&getPhaserCenterFreqName,
        &getPhaserFeedbackName,
        &getPhaserMixName,

        &getChorusRateName,
        &getChorusDepthName,
        &getChorusCenterDelayName,
        &getChorusFeedbackName,
        &getChorusMixName,

		&getOverdriveSaturationName,

        &getLadderFilterCutoffName,
        &getLadderFilterResonanceName,
        &getLadderFilterDriveName,

		&getGeneralFilterFreqName,
        &getGeneralFilterQualityName,
		&getGeneralFilterGainName,

		&getInputGainName,
		&getOutputGainName,

    };

	initCachedParams<juce::AudioParameterFloat*>(floatParams, floatNameFuncs);

	auto choiceParams = std::array
    {
        &ladderFilterMode,
        &generalFilterMode,
    };

    auto choiceNameFuncs = std::array
    {
        &getLadderFilterModeName,
        &getGeneralFilterModeName,
    };

    initCachedParams<juce::AudioParameterChoice*>(choiceParams, choiceNameFuncs);

    auto bypassParams = std::array
    {
        &phaserBypass,
        &chorusBypass,
        &overdriveBypass,
        &ladderFilterBypass,
        &generalFilterBypass,
	};

    auto bypassNameFuncs = std::array
    {
        &getPhaserBypassName,
        &getChorusBypassName,
        &getOverdriveBypassName,
        &getLadderFilterBypassName,
        &getGeneralFilterBypassName,
    };

	initCachedParams<juce::AudioParameterBool*>(bypassParams, bypassNameFuncs);

    auto intParams = std::array
    {
        &selectedTab,
	};

    auto intFuncs = std::array
    {
        &getSelectedTabName,
	};

	initCachedParams<juce::AudioParameterInt*>(intParams, intFuncs);

}

JUCE_MultiFX_ProcessorAudioProcessor::~JUCE_MultiFX_ProcessorAudioProcessor()
{
}

//==============================================================================
const juce::String JUCE_MultiFX_ProcessorAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool JUCE_MultiFX_ProcessorAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool JUCE_MultiFX_ProcessorAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool JUCE_MultiFX_ProcessorAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double JUCE_MultiFX_ProcessorAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int JUCE_MultiFX_ProcessorAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int JUCE_MultiFX_ProcessorAudioProcessor::getCurrentProgram()
{
    return 0;
}

void JUCE_MultiFX_ProcessorAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String JUCE_MultiFX_ProcessorAudioProcessor::getProgramName (int index)
{
    return {};
}

void JUCE_MultiFX_ProcessorAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void JUCE_MultiFX_ProcessorAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..

	juce::dsp::ProcessSpec spec;
	spec.sampleRate = sampleRate;
    spec.maximumBlockSize = samplesPerBlock;
	spec.numChannels = 1; // Mono processing for each channel

    leftChannel.prepare(spec);
	rightChannel.prepare(spec);

    for (auto smoother : getSmoothers())
    {
        smoother->reset(sampleRate, 0.005);
	}

	updateSmoothersFromParams(1, SmootherUpdateMode::initialize);

    spec.numChannels = getTotalNumInputChannels();

    inputGainDSP.prepare(spec);
    outputGainDSP.prepare(spec);

    leftSCSF.prepare(samplesPerBlock);
	rightSCSF.prepare(samplesPerBlock);

}

void JUCE_MultiFX_ProcessorAudioProcessor::updateSmoothersFromParams(int numSamplesToSkip, SmootherUpdateMode init)
{
    auto paramsNeedingSmoothing = std::vector
    {
        phaserRateHz,
        phaserDepthPercent,
        phaserCenterFreqHz,
        phaserFeedbackPercent,
        phaserMixPercent,
        chorusRateHz,
        chorusDepthPercent,
        chorusCenterDelayMs,
        chorusFeedbackPercent,
        chorusMixPercent,
        overdriveSaturation,
        ladderFilterCutoffHz,
        ladderFilterResonance,
        ladderFilterDrive,
        generalFilterFreqHz,
        generalFilterQuality,
        generalFilterGain,
		inputGain,
		outputGain,
    };

	auto smoothers = getSmoothers();
	jassert(smoothers.size() == paramsNeedingSmoothing.size());

    for (size_t i = 0; i < smoothers.size(); i++)
    {
		auto smoother = smoothers[i];
		auto param = paramsNeedingSmoothing[i];

        if ( init == SmootherUpdateMode::initialize)
			smoother->setCurrentAndTargetValue(param->get());
        else
			smoother->setTargetValue(param->get());

		smoother->skip(numSamplesToSkip);
    }

}

void JUCE_MultiFX_ProcessorAudioProcessor::MonoChannelDSP::prepare(const juce::dsp::ProcessSpec& spec)
{
	jassert(spec.numChannels == 1); // This DSP is designed for mono channels only
    std::vector<juce::dsp::ProcessorBase*> dsp
    {
        &phaser,
        &chorus,
        &overdrive,
        &ladderFilter,
        &generalFilter
    };

    for (auto p : dsp)
    {
        p->prepare(spec);
        p->reset();
    }

	overdrive.dsp.setCutoffFrequencyHz(20000.f);
}

void JUCE_MultiFX_ProcessorAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool JUCE_MultiFX_ProcessorAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
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

juce::AudioProcessorValueTreeState::ParameterLayout JUCE_MultiFX_ProcessorAudioProcessor::createParameterlayout() {

	juce::AudioProcessorValueTreeState::ParameterLayout layout;

    const int versionHint = 1;
    
    auto name = getInputGainName();
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{name, versionHint},
        name,
        juce::NormalisableRange<float>(-18.f, 18.f, 0.1f, 1.f),
        0.f,
        "dB"
	));

	name = getOutputGainName();
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{name, versionHint},
        name,
        juce::NormalisableRange<float>(-18.f, 18.f, 0.1f, 1.f),
        0.f,
        "dB"
    ));

	// Phaser parameters
    /*
    Phaser:
    Rate: hz
    Depth: 0 to 100
    Center freq: hz
    Feedback: -1 to 1
    Mix: 0 to 1
    */

	name = getPhaserRateName();
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{name, versionHint},
        name,
        juce::NormalisableRange<float>(0.01f, 2.f, 0.01f, 1.f),
        0.2f,
        "Hz"
	));

    name = getPhaserDepthName();
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{name, versionHint},
        name,
        juce::NormalisableRange<float>(0.0f, 100.f, 0.1f, 1.f),
        5.f,
        "%"
	));

    name = getPhaserCenterFreqName();
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{name, versionHint},
        name,
        juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 1.f),
        1000.f,
        "Hz"
    ));
    name = getPhaserFeedbackName();
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{name, versionHint},
        name,
        juce::NormalisableRange<float>(-100.f, 100.f, 0.1f, 1.f),
        0.0f,
        "%"
    ));
    name = getPhaserMixName();
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{name, versionHint},
        name,
        juce::NormalisableRange<float>(0.f, 100.f, 0.1f, 1.f),
        50.f,
        "%"
	));

    name = getPhaserBypassName();
    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{ name, versionHint },
        name,
        false
    ));

    /*
    Chorus:
    Rate: hz
    Depth: 0 to 1
	Center delay: ms (1 to 100)
    Feedback: -1 to 1
    Mix: 0 to 1
    */

    name = getChorusRateName();
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ name, versionHint },
        name,
        juce::NormalisableRange<float>(0.01f, 15.f, 0.01f, 1.f),
        0.9f,
        "Hz"
    ));

	name = getChorusDepthName();
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ name, versionHint },
        name,
        juce::NormalisableRange<float>(0.f, 100.f, 0.1f, 1.f),
        5.f,
        "%"
    ));

    name = getChorusCenterDelayName();
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ name, versionHint },
        name,
        juce::NormalisableRange<float>(0.01f, 20.f, 0.01f, 1.f),
        3.f,
        "Ms"
    ));

    name = getChorusFeedbackName();
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ name, versionHint },
        name,
        juce::NormalisableRange<float>(-100.f, 100.f, 0.1f, 1.f),
        0.0f,
        "%"
    ));

    name = getChorusMixName();
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ name, versionHint },
        name,
        juce::NormalisableRange<float>(0.0f, 100.f, 0.1f, 5.f),
        50.f,
        "%"
    ));

	name = getChorusBypassName();
    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{ name, versionHint },
        name,
        false
	));

    /*
    Overdrive:
	Uses the drive parameter of the ladder filter
    Drive: 1 to 100
    */

    name = getOverdriveSaturationName();
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ name, versionHint },
        name,
        juce::NormalisableRange<float>(1.f, 100.f, 0.1f, 1.f),
        1.f,
        ""
	));

	name = getOverdriveBypassName();
    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{ name, versionHint },
        name,
		false
	));

    /*
	Ladder Filter:
    Mode: LadderFilterMode enum (int)
	Cutoff: Hz
	Resonance: 0 to 1
	Drive: 1 to 100
    */

	name = getLadderFilterModeName();
	auto choices = getLadderFilterChoices();
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{ name, versionHint },
        name,
        choices,
        0 // Default to LPF12
	));

	name = getLadderFilterCutoffName();
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ name, versionHint },
        name,
        juce::NormalisableRange<float>(20.f, 20000.f, 0.1f, 1.f),
        20000.f,
        "Hz"
    ));

	name = getLadderFilterResonanceName();
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ name, versionHint },
        name,
        juce::NormalisableRange<float>(0.f, 100.f, 0.1f, 1.f),
        0.0f,
        "%"
    ));

	name = getLadderFilterDriveName();
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ name, versionHint },
        name,
        juce::NormalisableRange<float>(1.f, 100.f, 0.1f, 1.f),
        1.f,
        ""
    ));

	name = getLadderFilterBypassName();
    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{ name, versionHint },
        name,
		false
	));
          

    /*
	General Filter:
	Mode: Peak, Bandpass, Notch, Allpass
	Frequency: Hz (20 to 20000) (1 Hz steps)
	Q: 0.1 to 10 (0.05 steps)
	Gain: -24db to +24db (0.5db steps)
    */
	name = getGeneralFilterModeName();
	choices = getGeneralFilterChoices();
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{ name, versionHint },
        name,
        choices,
        0 // Default to Peak
	));

    name = getGeneralFilterFreqName();
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ name, versionHint },
        name,
        juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 1.f),
        750.f,
        "Hz"
	));

    name = getGeneralFilterQualityName();
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ name, versionHint },
        name,
        juce::NormalisableRange<float>(0.01f, 100.f, 0.01f, 1.f),
        0.72f,
        ""
    ));

    name = getGeneralFilterGainName();
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ name, versionHint },
        name,
        juce::NormalisableRange<float>(-24.f, 24.f, 0.5f, 1.f),
        0.0f,
        "dB"
	));

    name = getGeneralFilterBypassName();
    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{ name, versionHint },
        name,
        false
	));

	name = getSelectedTabName();
    layout.add(std::make_unique<juce::AudioParameterInt>(
        juce::ParameterID{ name, versionHint },
        name,
        0, 
        static_cast<int>(DSP_Option::END_OF_LIST) - 1,
		static_cast<int>(DSP_Option::Chorus)
    ));

	return layout;
}

void JUCE_MultiFX_ProcessorAudioProcessor::MonoChannelDSP::updateDSPFromParams()
{
    phaser.dsp.setRate(p.phaserRateHzSmoother.getCurrentValue());
    phaser.dsp.setDepth(p.phaserDepthPercentSmoother.getCurrentValue() * 0.01f );
    phaser.dsp.setCentreFrequency(p.phaserCenterFreqHzSmoother.getCurrentValue());
    phaser.dsp.setFeedback(p.phaserFeedbackPercentSmoother.getCurrentValue() * 0.01f);
    phaser.dsp.setMix(p.phaserMixPercentSmoother.getCurrentValue() * 0.01f);

    chorus.dsp.setRate(p.chorusRateHzSmoother.getCurrentValue());
    chorus.dsp.setDepth(p.chorusDepthPercentSmoother.getCurrentValue() * 0.01f);
    chorus.dsp.setCentreDelay(p.chorusCenterDelayMsSmoother.getCurrentValue());
    chorus.dsp.setFeedback(p.chorusFeedbackPercentSmoother.getCurrentValue() * 0.01f);
    chorus.dsp.setMix(p.chorusMixPercentSmoother.getCurrentValue() * 0.01f);

    overdrive.dsp.setDrive(p.overdriveSaturationSmoother.getCurrentValue());

    ladderFilter.dsp.setMode(static_cast<juce::dsp::LadderFilter<float>::Mode>(p.ladderFilterMode->getIndex()));
    ladderFilter.dsp.setCutoffFrequencyHz(p.ladderFilterCutoffHzSmoother.getCurrentValue());
    ladderFilter.dsp.setResonance(p.ladderFilterResonanceSmoother.getCurrentValue() * 0.01f);
    ladderFilter.dsp.setDrive(p.ladderFilterDriveSmoother.getCurrentValue());

    auto sampleRate = p.getSampleRate();

	// Update the general filter coefficients based on the current parameters
	auto genMode = p.generalFilterMode->getIndex();
	auto genHz = p.generalFilterFreqHzSmoother.getCurrentValue();
	auto genQ = p.generalFilterQualitySmoother.getCurrentValue();
    auto genGain = p.generalFilterGainSmoother.getCurrentValue();

	bool filterChanged = false;

    filterChanged |= (filterFreq != genHz);
    filterChanged |= (filterQ != genQ);
    filterChanged |= (filterGain != genGain);

	auto updatedMode = static_cast<GeneralFilterMode>(genMode);
    filterChanged |= (filterMode != updatedMode);

    if (filterChanged)
    {
        filterMode = updatedMode;
        filterFreq = genHz;
        filterQ = genQ;
        filterGain = genGain;

        juce::dsp::IIR::Coefficients<float>::Ptr coefficients;

        switch (filterMode)
        {
        case GeneralFilterMode::Peak:
            coefficients = juce::dsp::IIR::Coefficients<float>::makePeakFilter(sampleRate, filterFreq, filterQ, juce::Decibels::decibelsToGain(filterGain));
            break;
        case GeneralFilterMode::Bandpass:
            coefficients = juce::dsp::IIR::Coefficients<float>::makeBandPass(sampleRate, filterFreq, filterQ);
            break;
        case GeneralFilterMode::Notch:
            coefficients = juce::dsp::IIR::Coefficients<float>::makeNotch(sampleRate, filterFreq, filterQ);
            break;
        case GeneralFilterMode::Allpass:
            coefficients = juce::dsp::IIR::Coefficients<float>::makeAllPass(sampleRate, filterFreq, filterQ);
            break;
        case GeneralFilterMode::END_OF_LIST:
            jassertfalse; // This should never happen
            break;
        }

        if (coefficients != nullptr)
        {
            //if (generalFilter.dsp.coefficients->coefficients.size() != coefficients->coefficients.size())
            //{
            //    jassertfalse; // Coefficients size mismatch
            //}
            *generalFilter.dsp.coefficients = *coefficients;
            generalFilter.reset();
        }
    }
}

std::vector< juce::RangedAudioParameter*> JUCE_MultiFX_ProcessorAudioProcessor::getParamsForOption(DSP_Option option)
{
    switch (option)
    {
    case DSP_Option::Phase:
        return
        {
            phaserRateHz,
            phaserDepthPercent, 
            phaserCenterFreqHz, 
            phaserFeedbackPercent, 
            phaserMixPercent, 
            phaserBypass
        };
    case DSP_Option::Chorus:
        return
        {
            chorusRateHz, 
            chorusDepthPercent, 
            chorusCenterDelayMs, 
            chorusFeedbackPercent, 
            chorusMixPercent, 
            chorusBypass
        };
    case DSP_Option::Overdrive:
        return
        {
            overdriveSaturation, 
            overdriveBypass
        };
    case DSP_Option::LadderFilter:
        return
        {
            ladderFilterMode, 
            ladderFilterCutoffHz, 
            ladderFilterResonance, 
            ladderFilterDrive, 
            ladderFilterBypass
        };
    case DSP_Option::GeneralFilter:
        return
        {
            generalFilterMode, 
            generalFilterFreqHz, 
            generalFilterQuality, 
            generalFilterGain, 
            generalFilterBypass
        };
	case DSP_Option::END_OF_LIST:
		break; // This should never happen
	}
    jassertfalse;
    return {};
    
}


void JUCE_MultiFX_ProcessorAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // In case we have more outputs than inputs, this code clears any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    // This is here to avoid people getting screaming feedback
    // when they first compile a plugin, but obviously you don't need to keep
    // this code if your algorithm always overwrites all the output channels.
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

	// TODO: wet/dry mix control [STRETCH]
	// TODO: mono and stereo versions [STRETCH]
	// TODO: modulators (eg. LFOs, envelopes, etc.) [STRETCH]
	// TODO: thread-safe filter updates [STRETCH]
	// TODO: pre/post filtering [STRETCH]
	// TODO: delay module [STRETCH]

	leftChannel.updateDSPFromParams();
	rightChannel.updateDSPFromParams();

    auto newDSPOrder = DSP_Order();

	// Try to pull the DSP order from the FIFO
    while (dspOrderFifo.pull(newDSPOrder))
    {
#if VERIFY_BYPASS_FUNCTIONALITY
        jassertfalse;
#endif
    }

	// If the DSP order has changed, we need to reconfigure the DSP chain
    if ( newDSPOrder != DSP_Order() )
		dspOrder = newDSPOrder;

    if (guiNeedsLatestDspOrder.compareAndSetBool(false, true))
    {
		restoreDspOrderFifo.push(dspOrder);
    }

	/*auto block = juce::dsp::AudioBlock<float>(buffer);

	leftChannel.process(block.getSingleChannelBlock(0), dspOrder);
	rightChannel.process(block.getSingleChannelBlock(1), dspOrder);*/

    auto block = juce::dsp::AudioBlock<float>(buffer);

	auto preCtx = juce::dsp::ProcessContextReplacing<float>(block);

	inputGainSmoother.setTargetValue(inputGain->get());
	outputGainSmoother.setTargetValue(outputGain->get());
    inputGainDSP.setGainDecibels(inputGainSmoother.getNextValue());
	inputGainDSP.process(preCtx);

    const auto numSamples = buffer.getNumSamples();
	auto samplesRemaining = numSamples;
	auto maxSamplesToProcess = juce::jmin(samplesRemaining, 64);

	leftPreRMS.set(buffer.getRMSLevel(0, 0, numSamples));
	rightPreRMS.set(buffer.getRMSLevel(1, 0, numSamples));

    
    size_t startSample = 0;
    while (samplesRemaining > 0)
    {
		auto samplesToProcess = juce::jmin(samplesRemaining, maxSamplesToProcess);
		updateSmoothersFromParams(samplesToProcess, SmootherUpdateMode::liveInRealtime);

		leftChannel.updateDSPFromParams();
        rightChannel.updateDSPFromParams();

		auto subBlock = block.getSubBlock(startSample, samplesToProcess);

        leftChannel.process(subBlock.getSingleChannelBlock(0), dspOrder);
        rightChannel.process(subBlock.getSingleChannelBlock(1), dspOrder);
         
		startSample += samplesToProcess;
		samplesRemaining -= samplesToProcess;
    }

	auto postCtx = juce::dsp::ProcessContextReplacing<float>(block);
	outputGainDSP.setGainDecibels(outputGainSmoother.getNextValue());
	outputGainDSP.process(postCtx);

	leftPostRMS.set(buffer.getRMSLevel(0, 0, numSamples));
	rightPostRMS.set(buffer.getRMSLevel(1, 0, numSamples));

    leftSCSF.update(buffer);
	rightSCSF.update(buffer);

}

std::vector<juce::SmoothedValue<float>*> JUCE_MultiFX_ProcessorAudioProcessor::getSmoothers()
{
    auto smoothers = std::vector
    {
		&phaserRateHzSmoother,
		&phaserDepthPercentSmoother,
		&phaserCenterFreqHzSmoother,
		&phaserFeedbackPercentSmoother,
		&phaserMixPercentSmoother,
		&chorusRateHzSmoother,
		&chorusDepthPercentSmoother,
		&chorusCenterDelayMsSmoother,
		&chorusFeedbackPercentSmoother,
		&chorusMixPercentSmoother,
		&overdriveSaturationSmoother,
		&ladderFilterCutoffHzSmoother,
		&ladderFilterResonanceSmoother,
		&ladderFilterDriveSmoother,
		&generalFilterFreqHzSmoother,
		&generalFilterQualitySmoother,
        &generalFilterGainSmoother,
		&inputGainSmoother,
		&outputGainSmoother,
    };

	return smoothers;
}

void JUCE_MultiFX_ProcessorAudioProcessor::MonoChannelDSP::process(juce::dsp::AudioBlock<float> block, const DSP_Order &dspOrder)
{
    // Convert DSP_Order to DSP_Pointers
    DSP_Pointers dspPointers;
    //dspPointers.fill(nullptr); // Initialize all pointers to nullptr
    dspPointers.fill({});

    for (size_t i = 0; i < dspPointers.size(); ++i)
    {
        switch (dspOrder[i])
        {
        case DSP_Option::Phase:
            dspPointers[i].processor = &phaser;
            dspPointers[i].bypassed = p.phaserBypass->get();
            break;
        case DSP_Option::Chorus:
            dspPointers[i].processor = &chorus;
            dspPointers[i].bypassed = p.chorusBypass->get();
            break;
        case DSP_Option::Overdrive:
            dspPointers[i].processor = &overdrive;
            dspPointers[i].bypassed = p.overdriveBypass->get();
            break;
        case DSP_Option::LadderFilter:
            dspPointers[i].processor = &ladderFilter;
            dspPointers[i].bypassed = p.ladderFilterBypass->get();
            break;
        case DSP_Option::GeneralFilter:
            dspPointers[i].processor = &generalFilter;
            dspPointers[i].bypassed = p.generalFilterBypass->get();
            break;
        case DSP_Option::END_OF_LIST:
            jassertfalse; // This should never happen
            break;
        }
    }

    // Process the audio through the DSP chain
    auto context = juce::dsp::ProcessContextReplacing<float>(block);

    for (size_t i = 0; i < dspPointers.size(); ++i)
    {
        if (dspPointers[i].processor != nullptr)
        {
            juce::ScopedValueSetter<bool> svs(context.isBypassed, dspPointers[i].bypassed);

#if VERIFY_BYPASS_FUNCTIONALITY
            if (context.isBypassed)
            {
                jassertfalse;
            }

            if (dspPointers[i].processor == &generalFilter)
            {
                continue;
            }

#endif


            dspPointers[i].processor->process(context);
        }
    }
}

//==============================================================================
bool JUCE_MultiFX_ProcessorAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* JUCE_MultiFX_ProcessorAudioProcessor::createEditor()
{
    return new JUCE_MultiFX_ProcessorAudioProcessorEditor (*this);
	//return new juce::GenericAudioProcessorEditor(*this);
}

//==============================================================================

template<>
struct juce::VariantConverter<JUCE_MultiFX_ProcessorAudioProcessor::DSP_Order>
{
    static JUCE_MultiFX_ProcessorAudioProcessor::DSP_Order fromVar(const juce::var& v)
    {
		using T = JUCE_MultiFX_ProcessorAudioProcessor::DSP_Order;
		T dspOrder;

		jassert(v.isBinaryData()); // Ensure the var is a binary data type
        if (v.isBinaryData() == false)
        {
			dspOrder.fill(JUCE_MultiFX_ProcessorAudioProcessor::DSP_Option::END_OF_LIST);
        }
        else
        {
			auto mb = *v.getBinaryData();
			juce::MemoryInputStream mis(mb, false);
			std::vector<int> arr;
            while (!mis.isExhausted())
            {
				arr.push_back(mis.readInt());
            }

			jassert(arr.size() == dspOrder.size()); // Ensure we don't overflow the dspOrder array

            for (size_t i = 0; i < dspOrder.size(); i++)
            {
                dspOrder[i] = static_cast<JUCE_MultiFX_ProcessorAudioProcessor::DSP_Option>(arr[i]);
            }
        }
		return dspOrder;
	}
     
    static juce::var toVar(const JUCE_MultiFX_ProcessorAudioProcessor::DSP_Order& t)
    {
        juce::MemoryBlock mb;

        //JUCE memory output stream uses scoping to complete writing to the MemoryBlock
        {
            juce::MemoryOutputStream mos(mb, false);

            for (const auto& v : t)
            {
				mos.writeInt(static_cast<int>(v));
            }
		}
        return mb;
    }
};

void JUCE_MultiFX_ProcessorAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.

	apvts.state.setProperty("dspOrder", juce::VariantConverter<JUCE_MultiFX_ProcessorAudioProcessor::DSP_Order>::toVar(dspOrder), nullptr);

	juce::MemoryOutputStream mos(destData, false);
    apvts.state.writeToStream(mos);
}

void JUCE_MultiFX_ProcessorAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.

	auto tree = juce::ValueTree::readFromData(data, sizeInBytes);
    if (tree.isValid())
    {
        apvts.replaceState(tree);

        if (apvts.state.hasProperty("dspOrder"))
        {
            auto order = juce::VariantConverter<JUCE_MultiFX_ProcessorAudioProcessor::DSP_Order>::fromVar(
                apvts.state.getProperty("dspOrder"));
            dspOrderFifo.push(order);
			restoreDspOrderFifo.push(order);
        }
        DBG(apvts.state.toXmlString());

#if VERIFY_BYPASS_FUNCTIONALITY 
        juce::Timer::callAfterDelay(1000, [this]()
            {
                DSP_Order dspOrder;
                dspOrder.fill(DSP_Option::LadderFilter);
                dspOrder[0] = DSP_Option::Overdrive;

                overdriveBypass->setValueNotifyingHost(1.f);
                dspOrderFifo.push(dspOrder);
            });
#endif

    }
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new JUCE_MultiFX_ProcessorAudioProcessor();
}
