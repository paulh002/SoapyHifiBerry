#pragma once
#include <cstdio>
#include <cstdarg>
#include <cstdlib>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <stdexcept>
#include <sys/ioctl.h>
#include <memory>
#include <unistd.h>
#include <string.h>
#include <mutex>
#include <cstring>
#include <chrono>
#include <thread>

#include <SoapySDR/Device.hpp>
#include <SoapySDR/Logger.hpp>
#include <SoapySDR/Formats.hpp>

#include "AudioInput.h"
#include "AudioOutput.h" 
#include "si5351.h"
#include "configfile.h"

/*-------------------------------------------------------
   Si5351
--------------------------------------------------------*/
#define SI5351_BUS_BASE_ADDR 0x60

#define CLK_VFO_RX SI5351_CLK0
#define CLK_VFO_TX SI5351_CLK1
#define CLK_NA SI5351_CLK2

const int hifiBerry_BufferSize = 2048;

typedef enum hifiberrysdrStreamFormat
{
	HIFIBERRY_SDR_CF32,
	HIFIBERRY_SDR_CS16
} hifiberrysdrStreamFormat;

class sdr_stream
{
  public:
	sdr_stream(int dir)
	{
		direction = dir;
	}
	int get_direction() { return direction; }
	void set_stream_format(hifiberrysdrStreamFormat sf) { streamFormat = sf; };
	hifiberrysdrStreamFormat get_stream_format() { return streamFormat; };

  private:
	int direction;
	hifiberrysdrStreamFormat streamFormat;
};

class SoapyHifiBerry : public SoapySDR::Device
{

  public:
	SoapyHifiBerry(const SoapySDR::Kwargs &args);
	~SoapyHifiBerry();

	/*******************************************************************
		 * Identification API
		 ******************************************************************/

	std::string getDriverKey(void) const;

	std::string getHardwareKey(void) const;

	SoapySDR::Kwargs getHardwareInfo(void) const;

	/*******************************************************************
		 * Channels API
		 ******************************************************************/

	size_t getNumChannels(const int direction) const;

	bool getFullDuplex(const int direction, const size_t channel) const;

	/*******************************************************************
		 * Stream API
		 ******************************************************************/
	SoapySDR::RangeList getSampleRateRange(const int direction, const size_t channel) const;

	std::vector<std::string> getStreamFormats(const int direction, const size_t channel) const;

	std::string getNativeStreamFormat(const int direction, const size_t channel, double &fullScale) const;

	SoapySDR::ArgInfoList getStreamArgsInfo(const int direction, const size_t channel) const;

	size_t getStreamMTU(SoapySDR::Stream *stream) const;
	void closeStream(SoapySDR::Stream *stream);
	int deactivateStream(SoapySDR::Stream *stream, const int flags = 0, const long long timeNs = 0);

	
	SoapySDR::Stream *setupStream(
		const int direction,
		const std::string &format,
		const std::vector<size_t> &channels = std::vector<size_t>(),
		const SoapySDR::Kwargs &args = SoapySDR::Kwargs());

	int readStream(
		SoapySDR::Stream *stream,
		void *const *buffs,
		const size_t numElems,
		int &flags,
		long long &timeNs,
		const long timeoutUs = 100000);

	int writeStream(
		SoapySDR::Stream *stream,
		const void *const *buffs,
		const size_t numElems,
		int &flags,
		const long long timeNs = 0,
		const long timeoutUs = 100000);

	/*******************************************************************
		 * Sample Rate API
		 ******************************************************************/

	void setSampleRate(const int direction, const size_t channel, const double rate);

	double getBandwidth(const int direction, const size_t channel) const;

	std::vector<double> listBandwidths(const int direction, const size_t channel) const;
	std::vector<double> listSampleRates(const int direction, const size_t channel) const;

	/*******************************************************************
		 * Frequency API
		 ******************************************************************/

	void setFrequency(
		const int direction,
		const size_t channel,
		const double frequency,
		const SoapySDR::Kwargs &args = SoapySDR::Kwargs());

	SoapySDR::RangeList getFrequencyRange(const int direction, const size_t channel) const;

	/*******************************************************************
		 * Antenna API
		 ******************************************************************/

	std::vector<std::string> listAntennas(const int direction, const size_t channel) const;

	/*******************************************************************
		 * Gain API
		 ******************************************************************/

	std::vector<std::string> listGains(const int direction, const size_t channel) const;

	void setGain(const int direction, const size_t channel, const double value);

	SoapySDR::Range getGainRange(const int direction, const size_t channel) const;

  private:
	int rx_frequency;
	int no_channels;
	std::mutex send_command;
	bool i2c_available;
	std::vector<sdr_stream *> streams;

	unique_ptr<HifiBerryAudioOutputput> uptr_HifiBerryAudioOutputput;
	unique_ptr<HifiBerryAudioInput> uptr_HifiBerryAudioInput;
	unique_ptr<cfg::File> uptr_cfg;

	SoapyHifiBerryDataBuffer<IQSample> source_buffer_rx;
	SoapyHifiBerryDataBuffer<IQSample> source_buffer_tx;
	unique_ptr<Si5351> pSI5351;

	int get_int(string section, string key);
	string get_string(string section, string key);
};
