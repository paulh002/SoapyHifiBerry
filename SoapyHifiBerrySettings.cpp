#include "SoapyHifiBerry.h"


/***********************************************************************
 * Device interface
 **********************************************************************/
const cfg::File::ConfigMap defaultOptions = {
	{"si5351", {{"correction", cfg::makeOption("0")}, {"rxdrive", cfg::makeOption("2")}, {"t.xdrive", cfg::makeOption("2")}}},
	{"sound", {{"device", cfg::makeOption("snd_rpi_hifiberry_dacplusadcpro")}, {"samplerate", cfg::makeOption("192000")}}}
};

int SoapyHifiBerry::get_int(string section, string key)
{
	auto option = uptr_cfg->getSection(section);
	auto s = option.find(key);
	if (s == option.end())
		return 0;
	string st = s->second;
	return atoi((const char *)st.c_str());
}

string SoapyHifiBerry::get_string(string section, string key)
{
	string st;
	auto option = uptr_cfg->getSection(section);
	auto s = option.find(key);
	if (s != option.end())
		st = s->second;
	return st;
}

size_t SoapyHifiBerry::getStreamMTU(SoapySDR::Stream *stream) const
{
	return 2048;
}

SoapyHifiBerry::SoapyHifiBerry(const SoapySDR::Kwargs &args)
{

	SoapySDR_setLogLevel(SOAPY_SDR_INFO);
	SoapySDR_log(SOAPY_SDR_INFO, "SoapyHifiBerry::SoapyHifiBerry  constructor called");
	no_channels = 1;
	txDrive  = rxDrive = SI5351_DRIVE_2MA;

	uptr_cfg = make_unique<cfg::File>();
	if (!uptr_cfg->loadFromFile("hifiberry.cfg"))
	{
		uptr_cfg->setDefaultOptions(defaultOptions);
		uptr_cfg->writeToFile("hifiberry.cfg");
	}
	int samplerate = SoapyHifiBerry::get_int("sound", "samplerate");

	int driveConfig = SoapyHifiBerry::get_int("si5351", "rxdrive");
	switch (driveConfig)
	{
	case 2:
		rxDrive = SI5351_DRIVE_2MA;
		break;
	case 4:
		rxDrive = SI5351_DRIVE_4MA;
		break;
	case 8:
		rxDrive = SI5351_DRIVE_8MA;
		break;
	default:
		rxDrive = SI5351_DRIVE_2MA;
		break;
	}
	
	driveConfig = SoapyHifiBerry::get_int("si5351", "txdrive");
	switch (driveConfig)
	{
	case 2:
		txDrive = SI5351_DRIVE_2MA;
		break;
	case 4:
		txDrive = SI5351_DRIVE_4MA;
		break;
	case 8:
		txDrive = SI5351_DRIVE_8MA;
		break;
	default:
		txDrive = SI5351_DRIVE_2MA;
		break;
	}

	uptr_HifiBerryAudioOutputput = make_unique<HifiBerryAudioOutputput>(samplerate, &source_buffer_tx, RtAudio::LINUX_ALSA);
	uptr_HifiBerryAudioInput = make_unique<HifiBerryAudioInput>(samplerate, true, &source_buffer_rx, RtAudio::LINUX_ALSA);

	std::vector<std::string> devices;
	uptr_HifiBerryAudioOutputput->listDevices(devices);
	for (int i = 0; i < devices.size(); i++)
	{
		if (devices[i].length() > 0)
		{
			printf("Audio device %s\n", devices[i].c_str());
		}
	}
	int corr = get_int("si5351", "correction");
	string dev = get_string("sound", "device");
	
	uptr_HifiBerryAudioOutputput->open(dev);
	uptr_HifiBerryAudioInput->open(uptr_HifiBerryAudioOutputput->get_device());

	pSI5351 = make_unique<Si5351>("/dev/i2c-1",SI5351_BUS_BASE_ADDR);
	if (pSI5351->init(SI5351_CRYSTAL_LOAD_8PF, 0, 0))
	{
		cout << "si5351 found" << endl;
		pSI5351->set_correction((long)corr, SI5351_PLL_INPUT_XO);
		pSI5351->drive_strength(CLK_VFO_RX, rxDrive);
		pSI5351->drive_strength(CLK_VFO_TX, txDrive);
		pSI5351->output_enable(CLK_VFO_RX, 1);
		pSI5351->output_enable(CLK_VFO_TX, 0);
		pSI5351->output_enable(CLK_NA, 0);
		pSI5351->update_status();
	}
	else
	{
		cout << "No si5351 found" << endl;
		pSI5351.reset(nullptr);
	}
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
	
	char version[100];
	snprintf(version, 100, "%u.%u", 0, 1); //0.1
	info["Version HifiBerry"] = version;
	return info;
}

size_t SoapyHifiBerry::getNumChannels(const int direction) const
{
	SoapySDR_log(SOAPY_SDR_INFO, "SoapyHifiBerry::getNumChannels called");

	return (1); //1 RX and 1 TX channel; making this for standalone radioberry!
}

bool SoapyHifiBerry::getFullDuplex(const int direction, const size_t channel) const
{
	SoapySDR_log(SOAPY_SDR_INFO, "SoapyHifiBerry::getFullDuplex called");

	return (true);
}

std::vector<double> SoapyHifiBerry::listBandwidths(const int direction, const size_t channel) const
{
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
		options.push_back(0.048e6);
		options.push_back(0.096e6);
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
		return (SoapySDR::Range(0, 100));
	return (SoapySDR::Range(0, 100));
}

void SoapyHifiBerry::setGain(const int direction, const size_t channel, const double value)
{
	char str[80];
	sprintf(str, "SoapyHifiBerry::setGain called %f", value * 2.4);
	SoapySDR_log(SOAPY_SDR_INFO, str);

	if (direction == SOAPY_SDR_RX)
		uptr_HifiBerryAudioOutputput->controle_alsa(21, (int) value); // numid = 21, iface = MIXER, name = 'ADC Capture Volume'

	if (direction == SOAPY_SDR_TX)
	{
		if (value < 99.9)
		{
			float factor = uptr_HifiBerryAudioOutputput->get_max_volume() / 100.0;
			uptr_HifiBerryAudioOutputput->controle_alsa(1, (int)(value * factor)); //numid = 1, iface = MIXER, name = 'Digital Playback Volume'
		}
		else
		{
			uptr_HifiBerryAudioOutputput->controle_alsa(1, uptr_HifiBerryAudioOutputput->get_max_volume()); //numid = 1, iface = MIXER, name = 'Digital Playback Volume'
		}
	}
}

/*******************************************************************
 * Frequency API 
 ******************************************************************/
void SoapyHifiBerry::setFrequency(const int direction, const size_t channel, const double frequency, const SoapySDR::Kwargs &args)
{

	

	if (direction == SOAPY_SDR_RX)
	{
		SoapySDR_log(SOAPY_SDR_INFO, "SoapyHifiBerry::setFrequency called RX");
		
		uint64_t freq = (uint64_t)frequency * SI5351_FREQ_MULT * 4;
		if (pSI5351)
			pSI5351->set_freq(freq, CLK_VFO_RX);
	}

	if (direction == SOAPY_SDR_TX)
	{
		SoapySDR_log(SOAPY_SDR_INFO, "SoapyHifiBerry::setFrequency called TX");
		
		uint64_t freq = (uint64_t)frequency * SI5351_FREQ_MULT * 4;
		if (pSI5351)
			pSI5351->set_freq(freq, CLK_VFO_TX);
	}

}
// end of source.

