#pragma once
#include "RtAudio.h"
#include <iostream>
#include <cstdlib>
#include <cstring>
#include <climits>
#include <cmath>
#include <algorithm>
#include <string>
#include <map>
#include "Audiodefs.h"
#include "DataBuffer.h"
#include "alsa/asoundlib.h"

class HifiBerryAudioOutputput :
    public RtAudio
{
public:
  HifiBerryAudioOutputput(int pcmrate, SoapyHifiBerryDataBuffer<IQSample> *AudioBuffer, RtAudio::Api api = UNSPECIFIED);
  bool open(std::string device);
  bool write(IQSampleVector &samples);
  void adjust_gain(SampleVector &samples);
  void close();
  ~HifiBerryAudioOutputput();
  double get_volume() { return m_volume; }
  void set_volume(int vol);
  unsigned int get_framesize() { return bufferFrames; }
  int queued_samples();
  void listDevices(std::vector<std::string> &devices);
  int getDevices(std::string device);
  void inc_underrun() { underrun++; }
  void clear_underrun() { underrun = 0; }
  int  get_underrun() { return underrun.load(); }
  int get_channels() { return info.outputChannels; }
  unsigned int get_samplerate() { return m_sampleRate; }
  unsigned int find_device(std::string name);
  unsigned int get_device() { return parameters.deviceId;}
  int controle_alsa(int element, int ivalue);
  int lookup_id(snd_ctl_elem_id_t *id, snd_ctl_t *handle);

protected:
	void samplesToInt16(const SampleVector& samples,
		std::vector<std::uint8_t>& bytes);
	
private:
	RtAudio::StreamParameters	parameters;
	RtAudio::DeviceInfo			info;
	SoapyHifiBerryDataBuffer<IQSample>		*databuffer;
	unsigned int				m_sampleRate;
	unsigned int				bufferFrames;  // 256 sample frames
	double						m_volume;
	string						m_error;
	atomic<int>					underrun;
	int alsa_device;
	map<int, std::string> device_map;
};