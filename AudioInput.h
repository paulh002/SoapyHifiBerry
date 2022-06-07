#pragma once
#include "RtAudio.h"
#include <string>
#include <iostream>
#include <cstdlib>
#include <cstring>
#include "Audiodefs.h"
#include "DataBuffer.h"

class AudioInput : public RtAudio
{
public:
  AudioInput(unsigned int pcmrate, bool stereo, DataBuffer<IQSample> *AudioBuffer, RtAudio::Api api = UNSPECIFIED);
  bool open(std::string device);
  void adjust_gain(IQSampleVector &samples);
  bool read(IQSampleVector &samples);
  void close();
  ~AudioInput();
  double get_volume() { return m_volume; }
  void set_volume(int vol);
  DataBuffer<IQSample> *get_databuffer() { return databuffer; };
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
	DataBuffer<IQSample>		*databuffer;
	string						m_error;
	long						asteps;
	bool						m_stereo;
	int							tune_tone;
	int							gaindb;
};

extern  AudioInput  *audio_input;
extern atomic_bool	audio_input_on;