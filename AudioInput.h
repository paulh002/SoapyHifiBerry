#pragma once
#include "AudioInput.h"
#include "RtAudio.h"
#include <string>
#include <iostream>
#include <cstdlib>
#include <cstring>
#include "Audiodefs.h"
#include "DataBuffer.h"

class HifiBerryAudioInput : public RtAudio
{
public:
  HifiBerryAudioInput(unsigned int pcmrate, bool stereo, SoapyHifiBerryDataBuffer<IQSample> *AudioBuffer, RtAudio::Api api = UNSPECIFIED);
  bool open(std::string device);
  void adjust_gain(IQSampleVector &samples);
  bool read(IQSampleVector &samples);
  void close();
  ~HifiBerryAudioInput();
  double get_volume() { return m_volume; }
  void set_volume(int vol);
  SoapyHifiBerryDataBuffer<IQSample> *get_databuffer() { return databuffer; };
  bool get_stereo() { return m_stereo; };
  int queued_samples();
  int getDevices(std::string device);
  void listDevices(std::vector<std::string> &devices);
  operator bool() const { return m_error.empty();}
  void clear() { databuffer->clear();}
  std::vector<RtAudio::Api> listApis();
  bool open(unsigned int device);
  void set_gain(int g) { gaindb = g; }

private:
	RtAudio::StreamParameters	parameters;
	RtAudio::DeviceInfo			info;
	unsigned int				sampleRate;
	unsigned int				bufferFrames;
	double						m_volume;
	SoapyHifiBerryDataBuffer<IQSample>		*databuffer;
	string						m_error;
	long						asteps;
	bool						m_stereo;
	int							tune_tone;
	int							gaindb;
};