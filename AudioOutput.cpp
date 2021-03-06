#include "AudioOutput.h"
#include "alsa/asoundlib.h"

/*
 * Audioout fills the audio output buffer.
 * If there are no samples available (underrun) mutted sound is send
 * Sound data is pulled from databuffer and copied to rtaudio buffer
 * A underrun counter is increased for adjusting samplerate of the radio
 **/

const int audio_buffer_size{2024};

int Audioout( void *outputBuffer,void *inputBuffer,unsigned int nBufferFrames,double streamTime,RtAudioStreamStatus status,	void *userData)
{
	IQSample *buffer = (IQSample *) outputBuffer;
	
	if (status)
		std::cout << "Stream underflow detected!\n" << std::endl;
	// Write interleaved audio data.
	
	if(((DataBuffer<IQSample> *)userData)->queued_samples() == 0)
	{
		for (int i = 0; i < nBufferFrames; i++)
		{
			buffer[i] = IQSample(0,0);
		}
		return 0;
	}
	IQSampleVector samples = ((DataBuffer<IQSample> *)userData)->pull();
	int i = 0;
	for (auto& col : samples)
	{
		IQSample v = col;
		((IQSample *)buffer)[i++] = v;
	}
	return 0;
}

/*
* List alsa audio devices but skip the Monitor ones 
*
**/

void AudioOutput::listDevices(std::vector<std::string> &devices)
{
	for (auto const &dev : device_map)
	{
		if (dev.second.size() > 0 && dev.second.find("Monitor") == std::string::npos)
			devices.push_back(dev.second);
	}
}

/*
* Search for device number based on device name
* Issue -> very slow when user is not allowed to open device
* 
*/

int AudioOutput::getDevices(std::string device)
{
	RtAudio::DeviceInfo		dev_info;
	int noDevices = this->getDeviceCount();
	int retval = 0;

	if (noDevices < 1) {
		std::cout << "\nNo audio devices found!\n";
		return -1;
	}
	for (int i = 0; i < noDevices; i++)
	{
		dev_info = getDeviceInfo(i);
		printf("%d device: %s input %d output %d\n", i, info.name.c_str(), info.inputChannels, info.outputChannels);
		if (dev_info.name.find(device) != std::string::npos && dev_info.outputChannels > 0)
		{
			printf("audio device = %s Samplerate %d\n", info.name.c_str(), info.preferredSampleRate);
			info = dev_info;
			retval = i;
		}
	}
	return retval; // return default device
}


AudioOutput::AudioOutput(int pcmrate, DataBuffer<IQSample> *AudioBuffer, RtAudio::Api api)
	: RtAudio(api),
	  parameters{}, bufferFrames{}, m_volume{}, underrun{0}, info{0}
{
	m_sampleRate = pcmrate;
	databuffer = AudioBuffer;
	bufferFrames = audio_buffer_size;
	parameters.nChannels = 2;
	parameters.firstChannel = 0;
	parameters.deviceId = 0;
}

/*
 * Open sound device based on name
 * if name is default open default device
 * GetDevics() fills the map with device names and ID's
 * Use samplerate which is optimized for device 
 **/

bool AudioOutput::open(std::string device)
{
	int retry{0};
	RtAudioErrorType err;
	StreamOptions option{{0}, {0}, {0}, {0}};
	option.flags = RTAUDIO_MINIMIZE_LATENCY;

	getDevices();
	parameters.deviceId = 0;
	parameters.firstChannel = 0;
	parameters.nChannels = 2;
	if (device == "default")
		parameters.deviceId = this->getDefaultInputDevice();
	else
	{
		for (auto const &it : device_map)
		{
			if (it.second.find(device) != std::string::npos)
			{
				parameters.deviceId = it.first;
			}
		}
	}
	info = this->getDeviceInfo(parameters.deviceId);
	alsa_device = parameters.deviceId - 1;
	parameters.nChannels = info.outputChannels;
	printf("audio device = %d %s samplerate %d channels %d\n", parameters.deviceId, device.c_str(), m_sampleRate, parameters.nChannels);
	err = this->openStream(&parameters, NULL, RTAUDIO_FLOAT32, m_sampleRate, &bufferFrames, &Audioout, (void *)databuffer, NULL);
	if (err != RTAUDIO_NO_ERROR)
	{
		printf("Cannot open audio output stream\n");
		return false;
	}
	this->startStream();
	return true;	
}

/*
 * Set volume of output use log scale
 **/
void AudioOutput::set_volume(int vol) 
{
	// log volume
	m_volume = exp(((double)vol * 6.908) / 100.0) / 1000;
	//fprintf(stderr,"vol %f\n", (float)m_volume);
} 

void AudioOutput::adjust_gain(SampleVector& samples)
{
	for (unsigned int i = 0, n = samples.size(); i < n; i++) {
		samples[i] *= m_volume;
	}
}

void AudioOutput::close()
{
	if (isStreamOpen()) 
	{
		stopStream();
		closeStream();
	}
}

AudioOutput::~AudioOutput()
{
	close();
}

/*
 * Write data to audio buffer
 **/

bool AudioOutput::write(IQSampleVector& audiosamples)
{
	if (databuffer && isStreamOpen())
		databuffer->push(move(audiosamples));
	else
		audiosamples.clear();
	return true;
}

int	 AudioOutput::queued_samples()
{
	if (databuffer != nullptr)
		return databuffer->queued_samples();
	return 0;
}

/*
 * Code is copied from RTAUDIO to return a list of devices and their ID
 * The ID is card number + 1 
 *
 **/

unsigned int AudioOutput::getDevices()
{
	unsigned nDevices = 0;
	int result, subdevice, card;
	char name[64];
	snd_ctl_t *handle = 0;

	strcpy(name, "default");
	result = snd_ctl_open(&handle, "default", 0);
	if (result == 0)
	{
		nDevices++;
		snd_ctl_close(handle);
	}

	// Count cards and devices
	card = -1;
	snd_card_next(&card);
	while (card >= 0)
	{
		sprintf(name, "hw:%d", card);
		result = snd_ctl_open(&handle, name, 0);
		if (result < 0)
		{
			handle = 0;
			cout << "AudioOutput::getDevices: control open, card = " << card << ", " << snd_strerror(result) << "." << endl;
			goto nextcard;
		}
		subdevice = -1;
		// Get the device name
		if (strncmp(name, "default", 7) != 0)
		{
			char *cardname;
			result = snd_card_get_name(card, &cardname);
			if (result >= 0)
			{
				device_map[card+1] = string(cardname);
				free(cardname);
			}
		}
		while (1)
		{
			result = snd_ctl_pcm_next_device(handle, &subdevice);
			if (result < 0)
			{
				cout << "AudioOutput::getDevices: control next device, card = " << card << ", " << snd_strerror(result) << "." << endl;
				break;
			}
			if (subdevice < 0)
				break;
			nDevices++;
		}
	nextcard:
		if (handle)
			snd_ctl_close(handle);
		snd_card_next(&card);
	}
	return nDevices;
}


int lookup_id(snd_ctl_elem_id_t* id, snd_ctl_t* handle)
{
	int err;
	snd_ctl_elem_info_t* info;
	snd_ctl_elem_info_alloca(&info);

	snd_ctl_elem_info_set_id(info, id);
	if ((err = snd_ctl_elem_info(handle, info)) < 0) {
		fprintf(stderr, "Cannot find the given element from card\n");
		return err;
	}
	snd_ctl_elem_info_get_id(info, id);

	return 0;
}

int AudioOutput::controle_alsa(int element, int ivalue)
{
	char	str[80];
	int err;
	snd_ctl_t* handle;
	snd_ctl_elem_id_t* id;
	snd_ctl_elem_value_t* value;
	snd_ctl_elem_id_alloca(&id);
	snd_ctl_elem_value_alloca(&value);
	
	sprintf(str, "hw:%d", alsa_device);
	if ((err = snd_ctl_open(&handle, str, 0)) < 0) {
		fprintf(stderr, "Card open error: %s\n", snd_strerror(err));
		return err;
	}
	snd_ctl_elem_id_set_interface(id, SND_CTL_ELEM_IFACE_MIXER);
	snd_ctl_elem_id_set_numid(id, element);
	if (err = lookup_id(id, handle))
	{
		snd_ctl_close(handle);
		return err;
	}

	snd_ctl_elem_value_set_id(value, id);
	snd_ctl_elem_value_set_integer(value, 0, ivalue);
	snd_ctl_elem_value_set_integer(value, 1, ivalue);
	//snd_ctl_elem_value_set_integer(value, 1, 77);

	if ((err = snd_ctl_elem_write(handle, value)) < 0) {
		fprintf(stderr, "Control element write error: %s\n",
			snd_strerror(err));
		snd_ctl_close(handle);
		return err;
	}
	snd_ctl_close(handle);
	return 0;
}

/*
pi@pi3:~$ amixer --debug - c 3 controls
numid = 22, iface = MIXER, name = 'Headphone Playback Volume'
numid = 6, iface = MIXER, name = 'DSP Program'
numid = 28, iface = MIXER, name = 'ADC Left Capture Source'
numid = 23, iface = MIXER, name = 'ADC Left Input'
numid = 25, iface = MIXER, name = 'ADC Mic Bias'
numid = 29, iface = MIXER, name = 'ADC Right Capture Source'
numid = 24, iface = MIXER, name = 'ADC Right Input'
numid = 21, iface = MIXER, name = 'ADC Capture Volume'
numid = 3, iface = MIXER, name = 'Analogue Playback Boost Volume'
numid = 2, iface = MIXER, name = 'Analogue Playback Volume'
numid = 10, iface = MIXER, name = 'Auto Mute Mono Switch'
numid = 11, iface = MIXER, name = 'Auto Mute Switch'
numid = 8, iface = MIXER, name = 'Auto Mute Time Left'
numid = 9, iface = MIXER, name = 'Auto Mute Time Right'
numid = 7, iface = MIXER, name = 'Clock Missing Period'
numid = 5, iface = MIXER, name = 'Deemphasis Switch'
numid = 4, iface = MIXER, name = 'Digital Playback Switch'
numid = 1, iface = MIXER, name = 'Digital Playback Volume'
numid = 20, iface = MIXER, name = 'Max Overclock DAC'
numid = 19, iface = MIXER, name = 'Max Overclock DSP'
numid = 18, iface = MIXER, name = 'Max Overclock PLL'
numid = 26, iface = MIXER, name = 'PGA Gain Left'
numid = 27, iface = MIXER, name = 'PGA Gain Right'
numid = 16, iface = MIXER, name = 'Volume Ramp Down Emergency Rate'
numid = 17, iface = MIXER, name = 'Volume Ramp Down Emergency Step'
numid = 12, iface = MIXER, name = 'Volume Ramp Down Rate'
numid = 13, iface = MIXER, name = 'Volume Ramp Down Step'
numid = 14, iface = MIXER, name = 'Volume Ramp Up Rate'
numid = 15, iface = MIXER, name = 'Volume Ramp Up Step'
*/