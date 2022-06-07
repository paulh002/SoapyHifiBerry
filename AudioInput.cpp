#include "AudioInput.h"

#define dB2mag(x) pow(10.0, (x) / 20.0)

int record(void *outputBuffer, void *inputBuffer, unsigned int nBufferFrames, double streamTime, RtAudioStreamStatus status, void *userData)
{
	AudioInput					*audioinput = (AudioInput *)userData ;
	DataBuffer<IQSample>		*databuffer = audioinput->get_databuffer();
	
	if (status)
		std::cout << "Stream overflow detected!" << std::endl;

	//printf("frames %u \n", nBufferFrames);
	IQSampleVector	buf;
	for (int i = 0; i < nBufferFrames; i++)
	{
		IQSample f = ((IQSample *)inputBuffer)[i];
		buf.push_back(f);
	}
	databuffer->clear();
	databuffer->push(move(buf));
	return 0;
}


void AudioInput::listDevices(std::vector<std::string> &devices)
{
	int noDevices = this->getDeviceCount();
	struct DeviceInfo	dev;
		
	if (noDevices < 1) {
		std::cout << "\nNo audio devices found!\n";
		return ;
	}
	for (int i = 0; i < noDevices; i++)
	{
		dev = getDeviceInfo(i);
		if (dev.outputChannels > 0 || dev.inputChannels > 0)
			devices.push_back(dev.name);
	}
}

int AudioInput::getDevices(std::string device)
{
	int noDevices = this->getDeviceCount();
		
	if (noDevices < 1) {
		std::cout << "\nNo audio devices found!\n";
		return -1;
	}
	for (int i = 0; i < noDevices; i++)
	{
		info = getDeviceInfo(i);
		printf("%d device: %s input %d output %d\n", i, info.name.c_str(), info.inputChannels, info.outputChannels);

		if (info.name.find(device) != std::string::npos && info.inputChannels > 0)
		{
			if (info.outputChannels < parameters.nChannels)
				parameters.nChannels = info.outputChannels;
			return i;
		}
	}
	return 0; // return default device
}

AudioInput::AudioInput(unsigned int pcmrate, bool stereo, DataBuffer<IQSample> *AudioBuffer, RtAudio::Api api)
	: RtAudio(api), parameters{}, m_volume{0.5}, asteps{}, tune_tone{0}
{
	m_stereo = stereo;
	databuffer = AudioBuffer; 
	parameters.nChannels = 2;
	parameters.firstChannel = 0;
	sampleRate = pcmrate;
	bufferFrames = 2048;
	gaindb = 0;
}

std::vector<RtAudio::Api> AudioInput::listApis()
{
	std::vector<RtAudio::Api> apis;
	RtAudio ::getCompiledApi(apis);

	std::cout << "\nCompiled APIs:\n";
	for (size_t i = 0; i < apis.size(); i++)
		std::cout << i << ". " << RtAudio::getApiDisplayName(apis[i])
				  << " (" << RtAudio::getApiName(apis[i]) << ")" << std::endl;

	return apis;
}

bool AudioInput::open(std::string device)
{
	RtAudioErrorType err;

	if (this->getDeviceCount() < 1)
	{
		std::cout << "\nNo audio devices found!\n";
		return false;
	}
	parameters.nChannels = 2; //	info.inputChannels;
	err = this->openStream(NULL, &parameters, RTAUDIO_FLOAT64, sampleRate, &bufferFrames, &record, (void *)this);
	if (err != RTAUDIO_NO_ERROR)
	{
		printf("Cannot open audio input stream\n");
		return false;
	}
	this->startStream();
	printf("audio input device = %d %s samplerate %d\n", parameters.deviceId, device.c_str(), sampleRate);
	return true;	
}

bool AudioInput::open(unsigned int device)
{
	RtAudioErrorType err;

	parameters.deviceId = device;
	info = getDeviceInfo(device);
	parameters.nChannels = 2; //	info.inputChannels;
	err = this->openStream(NULL, &parameters, RTAUDIO_FLOAT64, sampleRate, &bufferFrames, &record, (void *)this);
	if (err != RTAUDIO_NO_ERROR)
	{
		printf("Cannot open audio input stream\n");
		return false;
	}
	this->startStream();
	printf("audio input device = %d %s samplerate %d\n", parameters.deviceId, info.name.c_str(), sampleRate);
	return true;
}

void AudioInput::set_volume(int vol)
{
	// log volume
	m_volume = exp(((double)vol * 6.908) / 100.0) / 5.0;
	printf("mic vol %f\n", (float)m_volume);
}

void AudioInput::adjust_gain(IQSampleVector& samples)
{
	for (unsigned int i = 0, n = samples.size(); i < n; i++) {
		samples[i] *= m_volume * dB2mag(gaindb);
	}
}


bool AudioInput::read(IQSampleVector& samples)
{
	if (databuffer == nullptr)
		return false;
	if (!isStreamOpen())
		return false;
	samples = databuffer->pull();
	if (samples.empty())
		return false;
	adjust_gain(samples);
	return true;
}

void AudioInput::close()
{
	if (isStreamOpen()) 
		closeStream();
}

AudioInput::~AudioInput()
{
	close();
}
