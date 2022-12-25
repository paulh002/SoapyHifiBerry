#include "SoapyHifiBerry.h"
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <stdint.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <queue>

IQSampleVector iqsamples;

void SoapyHifiBerry::setSampleRate(const int direction, const size_t channel, const double rate)
{
	SoapySDR_log(SOAPY_SDR_INFO, "SoapyHifiBerry::setSampleRate called");

	int irate = floor(rate);

	if (direction == SOAPY_SDR_TX)
	{

	}
	if (direction == SOAPY_SDR_RX)
	{
	
	}


}

SoapySDR::RangeList SoapyHifiBerry::getSampleRateRange(const int direction, const size_t channel) const
{

	SoapySDR_log(SOAPY_SDR_INFO, "SoapyHifiBerry::getSampleRateRange called");

	SoapySDR::RangeList rangeList;

	if (direction == SOAPY_SDR_RX)
		rangeList.push_back(SoapySDR::Range(48000.0, 192000.0, 192000.0));
	if (direction == SOAPY_SDR_TX)
		rangeList.push_back(SoapySDR::Range(48000.0, 192000.0, 192000.0));

	return rangeList;
}

std::vector<std::string> SoapyHifiBerry::getStreamFormats(const int direction, const size_t channel) const
{
	SoapySDR_log(SOAPY_SDR_INFO, "SoapyHifiBerry::getStreamFormats called");

	std::vector<std::string> formats;

	formats.push_back(SOAPY_SDR_CF32);
	//formats.push_back(SOAPY_SDR_CS16);

	return formats;
}

std::string SoapyHifiBerry::getNativeStreamFormat(const int direction, const size_t channel, double &fullScale) const
{
	SoapySDR_log(SOAPY_SDR_INFO, "SoapyHifiBerry::getNativeStreamFormat called");

	if (direction == SOAPY_SDR_RX)
	{
		fullScale = 16777216.0; 
	}
	else if (direction == SOAPY_SDR_TX)
	{
		fullScale = 16777216.0; 
	}
	return SOAPY_SDR_CF32;
}

SoapySDR::ArgInfoList SoapyHifiBerry::getStreamArgsInfo(const int direction, const size_t channel) const
{
	SoapySDR::ArgInfoList streamArgs;

	SoapySDR::ArgInfo bufflenArg;
	bufflenArg.key = "bufflen";
	bufflenArg.value = "2048";
	bufflenArg.name = "Buffer Size";
	bufflenArg.description = "Number of bytes per buffer, multiples of 512 only.";
	bufflenArg.units = "bytes";
	bufflenArg.type = SoapySDR::ArgInfo::INT;

	streamArgs.push_back(bufflenArg);

	return streamArgs;
}

auto startTime = std::chrono::high_resolution_clock::now();

SoapySDR::Stream *SoapyHifiBerry::setupStream(
	const int direction,
	const std::string &format,
	const std::vector<size_t> &channels,
	const SoapySDR::Kwargs &args)
{
	SoapySDR_log(SOAPY_SDR_INFO, "SoapyHifiBerry::setupStream called");
	startTime = std::chrono::high_resolution_clock::now();
	//check the format
	sdr_stream *ptr;
	ptr = new sdr_stream(direction);

	if (format == SOAPY_SDR_CF32)
	{
		SoapySDR_log(SOAPY_SDR_INFO, "Using format CF32.");
		ptr->set_stream_format(HIFIBERRY_SDR_CF32);
	}
	else 
	{
		throw std::runtime_error(
			"setupStream invalid format '" + format + "' -- Only CF32 is supported by SoapyHifiBerrySDR module.");
	}

	if (direction == SOAPY_SDR_TX)
	{
		iqsamples.clear();
		pSI5351->output_enable(CLK_VFO_TX, 1);
	}

	streams.push_back(ptr);
	return (SoapySDR::Stream *)ptr;
}

int SoapyHifiBerry::deactivateStream(SoapySDR::Stream *stream, const int flags,  const long long timeNs)
{
	SoapySDR_log(SOAPY_SDR_INFO, "SoapyHifiBerry::deactivateStream called ");
	sdr_stream *ptr = (sdr_stream *)stream;

	if (ptr != nullptr)
	{
		if (ptr->get_direction() == SOAPY_SDR_TX)
		{
			SoapySDR_log(SOAPY_SDR_INFO, "SoapyHifiBerry::deactivateStream disable TX ");
			pSI5351->output_enable(CLK_VFO_TX, 0);
		}
	}
	return 0;
}

void SoapyHifiBerry::closeStream(SoapySDR::Stream *stream)
{
	int i = 0;
	for (auto con : streams)
	{
		if ((sdr_stream *)stream == con)
		{
			SoapySDR_log(SOAPY_SDR_INFO, "SoapyHifiBerry::closeStream delete ");
			delete ((sdr_stream *)stream);
			streams.erase(streams.begin() + i);
		}
		i++;
	}
}

std::queue<float> delay_queue;
const int delay = 0;

int SoapyHifiBerry::readStream(
	SoapySDR::Stream *handle,
	void *const *buffs,
	const size_t numElems,
	int &flags,
	long long &timeNs,
	const long timeoutUs)
{
	int nr_samples = 0;

	void *buff_base = buffs[0];
	IQSample *target_buffer = (IQSample *)buff_base;
	sdr_stream *ptr = (sdr_stream *)handle;
	IQSampleVector iqsamples;
	
	if (ptr->get_stream_format() == HIFIBERRY_SDR_CF32)
	{
		if (uptr_HifiBerryAudioInput->read(iqsamples))
		{
			for (auto con : iqsamples)
			{
				IQSample iq = con;
				//printf("Iqsample %d nr_samples %d size %lu\n", ii, nr_samples, iqsamples.size());
			
				if (delay > 0)
				{
					delay_queue.push(iq.real());
					iq.real(0.0);
					if (delay_queue.size() == delay)
					{
						iq.real(delay_queue.front());
						delay_queue.pop();
					}
				}
					target_buffer[nr_samples++] = iq;
				//printf("I %f, Q %f\n", con.real(), con.imag());
			}
		}
	}
	//printf("SoapyHifiBerry samples overflow %d size %lu\n", nr_samples, iqsamples.size());
	//printf("nr_samples %d sample: %d %d \n", nr_samples, left_sample, right_sample);
	return (nr_samples); //return the number of IQ samples
}


int SoapyHifiBerry::writeStream(SoapySDR::Stream *handle, const void *const *buffs, const size_t numElems, int &flags, const long long timeNs, const long timeoutUs)
{
	size_t ret;
	int nr_samples;

	void const *buff_base = buffs[0];
	IQSample *target_buffer = (IQSample *)buff_base;
	sdr_stream *ptr = (sdr_stream *)handle;

	if (ptr->get_stream_format() == HIFIBERRY_SDR_CF32)
	{
		for (int i = 0; i < numElems; i++)
		{
			iqsamples.push_back(target_buffer[i]);
			if (iqsamples.size() == hifiBerry_BufferSize)
				uptr_HifiBerryAudioOutputput->write(iqsamples);
		}
	}
	return numElems;
}