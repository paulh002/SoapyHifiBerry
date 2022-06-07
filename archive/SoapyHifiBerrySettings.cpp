#include "SoapyHifiBerry.h"
#include "wiringPiI2C.h"

#define RADIOBERRY_BUFFER_SIZE 4096

/*-------------------------------------------------------
   Si5351
--------------------------------------------------------*/
#define SI5351_BUS_BASE_ADDR 0x60

#define CLK_VFO_RX SI5351_CLK0
#define CLK_VFO_TX SI5351_CLK1
#define CLK_NA SI5351_CLK2

/***********************************************************************
 * Device interface
 **********************************************************************/

SoapyHifiBerry::SoapyHifiBerry(const SoapySDR::Kwargs &args)
{

	SoapySDR_setLogLevel(SOAPY_SDR_INFO);
	SoapySDR_log(SOAPY_SDR_INFO, "SoapyHifiBerry::SoapyHifiBerry  constructor called");
	no_channels = 1;

	uptr_audiooutput = make_unique<AudioOutput>(192000, &source_buffer_tx, RtAudio::LINUX_ALSA);
	uptr_audioinput = make_unique<AudioInput>(192000, true, &source_buffer_rx, RtAudio::LINUX_ALSA);

	std::vector<std::string> devices;
	uptr_audiooutput->listDevices(devices);
	for (int i = 0; i < devices.size(); i++)
	{
		if (devices[i].length() > 0)
		{
			printf("Audio device %s\n", devices[i].c_str());
		}
	}
	uptr_audiooutput->open("snd_rpi_hifiberry_dacplusadcpro");
	uptr_audioinput->open(uptr_audiooutput->get_device());

	//si5351.set_correction(R.correction_si5351_no1, SI5351_PLL_INPUT_XO);
	pSI5351 = make_unique<si5351>(SI5351_BUS_BASE_ADDR);
	pSI5351->init(SI5351_CRYSTAL_LOAD_8PF, 0, 0);
	pSI5351->drive_strength(CLK_VFO_RX, SI5351_DRIVE_2MA);
	pSI5351->drive_strength(CLK_VFO_TX, SI5351_DRIVE_2MA);
	pSI5351->output_enable(CLK_VFO_RX, 1);
	pSI5351->output_enable(CLK_VFO_TX, 0);
	pSI5351->output_enable(CLK_NA, 0);
	pSI5351->set_freq(1400000000ULL, CLK_VFO_RX);
	pSI5351->update_status();
}

SoapyHifiBerry::~SoapyHifiBerry(void)
{
	SoapySDR_log(SOAPY_SDR_INFO, "SoapyHifiBerry::SoapyHifiBerry  destructor called");
	for (auto con : streams)
		delete (con);
}

std::string SoapyHifiBerry::getDriverKey(void) const
{
	SoapySDR_log(SOAPY_SDR_INFO, "SoapyHifiBerry::getDriverKey called");

	return "hifiberry";
}

std::string SoapyHifiBerry::getHardwareKey(void) const
{
	SoapySDR_log(SOAPY_SDR_INFO, "SoapyHifiBerry::getHardwareKey called");

	return "v0.1";
}

SoapySDR::Kwargs SoapyHifiBerry::getHardwareInfo(void) const
{

	SoapySDR_log(SOAPY_SDR_INFO, "SoapyHifiBerry::getHardwareInfo called");

	SoapySDR::Kwargs info;
	int count = 0;
	
	unsigned int major, minor;
	major = 1;
	minor = 1;

	char firmware_version[100];
	snprintf(firmware_version, 100, "%u.%u", 0, 1); //0.1
	info["firmwareVersion"] = firmware_version;

	char gateware_version[100];
	snprintf(gateware_version, 100, "%u.%u ", major, minor);
	info["gatewareVersion"] = gateware_version;

	char hardware_version[100];
	snprintf(hardware_version, 100, "%u.%u", 2, 4); //2.4 beta
	info["hardwareVersion"] = hardware_version;

	char protocol_version[100];
	snprintf(protocol_version, 100, "%u.%u ", 1, 58); //1.58 protocol 1
	info["protocolVersion"] = protocol_version;

	return info;
}

size_t SoapyHifiBerry::getNumChannels(const int direction) const
{
	SoapySDR_log(SOAPY_SDR_INFO, "SoapyHifiBerry::getNumChannels called");

	//if (direction == SOAPY_SDR_RX) return(4);

	return (1); //1 RX and 1 TX channel; making this for standalone radioberry!
}

bool SoapyHifiBerry::getFullDuplex(const int direction, const size_t channel) const
{
	SoapySDR_log(SOAPY_SDR_INFO, "SoapyHifiBerry::getFullDuplex called");

	return (true);
}

std::vector<double> SoapyHifiBerry::listBandwidths(const int direction, const size_t channel) const
{
	// radioberry does nor support bandwidth
	SoapySDR_log(SOAPY_SDR_INFO, "SoapyHifiBerry::listBandwidths called");

	std::vector<double> options;
	return (options);
}

std::vector<double> SoapyHifiBerry::listSampleRates(const int direction, const size_t channel) const
{

	SoapySDR_log(SOAPY_SDR_INFO, "SoapyHifiBerry::listSampleRates called");

	std::vector<double> options;

	if (direction == SOAPY_SDR_RX)
	{
		options.push_back(0.048e6);
		options.push_back(0.096e6);
		options.push_back(0.192e6);
	}
	if (direction == SOAPY_SDR_TX)
	{
		options.push_back(0.192e6);
	}
	return (options);
}

double SoapyHifiBerry::getBandwidth(const int direction, const size_t channel) const
{

	SoapySDR_log(SOAPY_SDR_INFO, "SoapyHifiBerry::getBandwidth called");

	if (direction == SOAPY_SDR_RX)
	{

		//depends on settings.. TODO
	}

	else if (direction == SOAPY_SDR_TX)
	{
		
	}

	return double(0.192e6);
}

SoapySDR::RangeList SoapyHifiBerry::getFrequencyRange(const int direction, const size_t channel) const
{
	SoapySDR_log(SOAPY_SDR_INFO, "SoapyHifiBerry::getFrequencyRange called");

	SoapySDR::RangeList rangeList;

	rangeList.push_back(SoapySDR::Range(10000.0, 30000000.0, 1.0));

	return rangeList;
}

std::vector<std::string> SoapyHifiBerry::listAntennas(const int direction, const size_t channel) const
{

	SoapySDR_log(SOAPY_SDR_INFO, "SoapyHifiBerry::listAntennas called");

	std::vector<std::string> options;
	if (direction == SOAPY_SDR_RX)
		options.push_back("ANTENNA RX");
	if (direction == SOAPY_SDR_TX)
		options.push_back("ANTENNA TX");
	return (options);
}

/*******************************************************************
 * Gain API
 ******************************************************************/

std::vector<std::string> SoapyHifiBerry::listGains(const int direction, const size_t channel) const
{
	SoapySDR_log(SOAPY_SDR_INFO, "SoapyHifiBerry::listGains called");

	std::vector<std::string> options;
	//options.push_back("PGA"); in pihpsdr no additional gain settings.
	return (options);
}

SoapySDR::Range SoapyHifiBerry::getGainRange(const int direction, const size_t channel) const
{
	SoapySDR_log(SOAPY_SDR_INFO, "SoapyHifiBerry::getGainRange called");

	if (direction == SOAPY_SDR_RX)
		return (SoapySDR::Range(-12, 48));
	return (SoapySDR::Range(0, 15));
}

void SoapyHifiBerry::setGain(const int direction, const size_t channel, const double value)
{

	SoapySDR_log(SOAPY_SDR_INFO, "SoapyHifiBerry::setGain called");

}

/*******************************************************************
 * Frequency API 
 ******************************************************************/
void SoapyHifiBerry::setFrequency(const int direction, const size_t channel, const double frequency, const SoapySDR::Kwargs &args)
{

	SoapySDR_log(SOAPY_SDR_INFO, "SoapyHifiBerry::setFrequency called");

	if (direction == SOAPY_SDR_RX)
	{
		uint64_t freq = (uint64_t)frequency * SI5351_FREQ_MULT * 4;
		pSI5351->set_freq(freq, CLK_VFO_RX);
	}

	if (direction == SOAPY_SDR_TX)
	{
		uint64_t freq = (uint64_t)frequency * SI5351_FREQ_MULT * 4;
		pSI5351->set_freq(freq, CLK_VFO_TX);
	}

}

uint8_t si5351::si5351_begin()
{
	int ret;
	
	printf("setup I2C %x\n", _address);
	if ((fd = wiringPiI2CSetup(_address)) < 0)
	{
		printf("Wiring Pi I2C error\n");
		return false;
	}
	return 0;
}

bool si5351::isConnected()
{
	int ret = wiringPiI2CRead(fd);
	return (ret != -1);
}


uint8_t si5351::si5351_write_bulk(uint8_t reg, uint8_t size , uint8_t *value)
{

	int ret = wiringPiI2CWriteBulk(fd, reg, value, size);
	if (ret < 0)
	{
		printf("i2c error \n");
		return 1; // last value
	}
	// success
	return 0;
}

uint8_t si5351::si5351_write(uint8_t reg, uint8_t value)
{
	int ret = wiringPiI2CWriteReg8(fd, reg, value);
	if (ret < 0)
	{
		printf("i2c write error %d\n", ret);
		return 1 << 7; // last value
	}
	return ret & 0xFF;
}

uint8_t si5351::si5351_read(uint8_t addr)
{
	int ret = wiringPiI2CReadReg8(fd, addr);
	if (ret < 0)
	{
		printf("i2c read error %d\n",ret);
		return 1 << 7; // last value
	}
	return ret & 0xFF;
}

// end of source.

