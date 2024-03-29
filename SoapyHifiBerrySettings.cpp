#include "SoapyHifiBerry.h"


/***********************************************************************
 * Device interface
 **********************************************************************/

const cfg::File::ConfigMap defaultOptions = {
	{"si5351", {{"correction", cfg::makeOption("0")}, {"correction_tx", cfg::makeOption("0")}, {"disabletxoutput", cfg::makeOption("off")}, {"mode", cfg::makeOption("IQMULTI")}, {"multiplier", cfg::makeOption("1")}, {"rxdrive", cfg::makeOption("8")}, {"txdrive", cfg::makeOption("8")}}},
	{"sound", {{"device", cfg::makeOption("snd_rpi_hifiberry_dacplusadcpro")}, {"samplerate", cfg::makeOption("192000")}, {"samplerate_tx", cfg::makeOption("192000")}, {"input", cfg::makeOption("DIFF")}}}};

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
	txDrive  = rxDrive = SI5351_DRIVE_8MA;
	vfoIQMode = Single;
	disableOutput = true;
	uptr_cfg = make_unique<cfg::File>();
	multiplier = 0;
	currDirection = -1;
	iclk = SI5351_CLK0;
	qclk = SI5351_CLK1;
	iclkTx = SI5351_CLK4;
	qclkTx = SI5351_CLK5;
	
	if (!uptr_cfg->loadFromFile("hifiberry.cfg"))
	{
		uptr_cfg->setDefaultOptions(defaultOptions);
		uptr_cfg->writeToFile("hifiberry.cfg");
	}
	multiplier = SoapyHifiBerry::get_int("si5351", "multiplier");
	if (!multiplier)
		multiplier = 1;

	int samplerate = SoapyHifiBerry::get_int("sound", "samplerate");
	int samplerate_tx = SoapyHifiBerry::get_int("sound", "samplerate_tx");
	if (!samplerate_tx)
		samplerate_tx = samplerate;
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
		rxDrive = SI5351_DRIVE_8MA;
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
		txDrive = SI5351_DRIVE_8MA;
		break;
	}

	uptr_HifiBerryAudioOutputput = make_unique<HifiBerryAudioOutputput>(samplerate, &source_buffer_tx, RtAudio::LINUX_ALSA);
	uptr_HifiBerryAudioInput = make_unique<HifiBerryAudioInput>(samplerate_tx, true, &source_buffer_rx, RtAudio::LINUX_ALSA);

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
	int corrtx = get_int("si5351", "correctiontx");
	string dev = get_string("sound", "device");
	
	uptr_HifiBerryAudioOutputput->open(dev);
	uptr_HifiBerryAudioInput->open(uptr_HifiBerryAudioOutputput->get_device());

	string inputSetting = get_string("sound", "input");
	inputSetting = strlib::toUpper(inputSetting);
	if (inputSetting == "DIFF")
	{
		//numid = 24, iface = MIXER, name = 'ADC Right Input'
		//numid = 23, iface = MIXER, name = 'ADC Left Input'
		//uptr_HifiBerryAudioInput->SetInputLine(true);
		uptr_HifiBerryAudioOutputput->controle_alsa(24, 4);
		uptr_HifiBerryAudioOutputput->controle_alsa(23, 4);
	}
	else
	{
		uptr_HifiBerryAudioOutputput->controle_alsa(24, 1);
		uptr_HifiBerryAudioOutputput->controle_alsa(23, 1);
	}
	//numid = 25, iface = MIXER, name = 'ADC Mic Bias'
	uptr_HifiBerryAudioOutputput->controle_alsa(25, 0);
	if (strlib::toUpper(SoapyHifiBerry::get_string("si5351", "disabletxoutput")) == "ON")
		disableOutput = true;
	if (strlib::toUpper(SoapyHifiBerry::get_string("si5351", "disabletxoutput")) == "OFF")
		disableOutput = false;

	std::string IQMode = SoapyHifiBerry::get_string("si5351", "mode");
	IQMode = strlib::toUpper(IQMode);
	if (IQMode == "SINGLE")
		vfoIQMode = Single;
	else if (IQMode == "IQSINGLE")
		vfoIQMode = IQSingle;
	else if (IQMode == "IQMULTI")
		vfoIQMode = IQMulti;
	else
		vfoIQMode = IQSingleMultiPort;
	
	cout << IQMode << " ";
	
	if (vfoIQMode == Single || vfoIQMode == IQSingle || vfoIQMode == IQSingleMultiPort)
	{
		cout << "IQ mode single multiplier = " << multiplier << endl;

		pSI5351 = make_unique<Si5351>("/dev/i2c-1", SI5351_BUS_BASE_ADDR);
		if (!pSI5351)
		{
			cout << "No si5351 found" << endl;
			pSI5351.reset(nullptr);
			return;
		}
		if (!pSI5351->init(SI5351_CRYSTAL_LOAD_8PF, 0, 0))
		{
			cout << "No si5351 found" << endl;
			pSI5351.reset(nullptr);
			return;
		}
	}
	if (vfoIQMode == Single)
	{
		if (!multiplier)
			multiplier = 4;
		cout << "si5351 found" << endl;
		pSI5351->set_correction((long)corr, SI5351_PLL_INPUT_XO);
		pSI5351->drive_strength(iclk, rxDrive);
		pSI5351->drive_strength(qclk, txDrive);
		pSI5351->output_enable(iclk, 1);
		pSI5351->output_enable(qclk, disableOutput ? (0) : (1));
		pSI5351->output_enable(SI5351_CLK2, 0);
		pSI5351->update_status();
	}
	if (vfoIQMode == IQSingle)
	{
		cout << "si5351 found" << endl;
		pSI5351->set_correction((long)corr, SI5351_PLL_INPUT_XO);
		pSI5351->drive_strength(iclk, rxDrive);
		pSI5351->drive_strength(qclk, rxDrive);
		pSI5351->output_enable(iclk, 1);
		pSI5351->output_enable(qclk, 1);
		pSI5351->output_enable(SI5351_CLK2, 0);
		pSI5351->update_status();
	}
	if (vfoIQMode == IQSingleMultiPort)
	{
		cout << "si5351 found" << endl;
		pSI5351->set_correction((long)corr, SI5351_PLL_INPUT_XO);
		pSI5351->drive_strength(iclk, rxDrive);
		pSI5351->drive_strength(qclk, rxDrive);
		pSI5351->output_enable(iclk, 1);
		pSI5351->output_enable(qclk, 1);
		pSI5351->drive_strength(iclkTx, txDrive);
		pSI5351->drive_strength(qclkTx, txDrive);
		pSI5351->output_enable(iclkTx, 1);
		pSI5351->output_enable(qclkTx, 1);
		pSI5351->update_status();
	}
	if (vfoIQMode == IQMulti)
	{
		cout << "IQMulti mode multiplier = " << multiplier << endl;
		pTCA9548 = std::make_unique<TCA9548>("/dev/i2c-1", 0x70);
		pTCA9548->begin(1);
		pSI5351 = make_unique<Si5351>("/dev/i2c-1", SI5351_BUS_BASE_ADDR);
		if (pSI5351->init(SI5351_CRYSTAL_LOAD_8PF, 0, 0))
		{
			pSI5351->set_correction((long)corr, SI5351_PLL_INPUT_XO);
			pSI5351->drive_strength(iclk, rxDrive);
			pSI5351->drive_strength(qclk, rxDrive);
			pSI5351->output_enable(iclk, 1);
			pSI5351->output_enable(qclk, 1);
			pSI5351->output_enable(SI5351_CLK2, 0);
			pSI5351->update_status();
		}
		pTCA9548->setChannelMask(2);
		pSI5351tx = make_unique<Si5351>("/dev/i2c-1", SI5351_BUS_BASE_ADDR);
		if (pSI5351tx->init(SI5351_CRYSTAL_LOAD_8PF, 0, 0, pSI5351->getFileHandle()))
		{
			pSI5351tx->set_correction((long)corrtx, SI5351_PLL_INPUT_XO);
			pSI5351tx->drive_strength(iclk, txDrive);
			pSI5351tx->drive_strength(qclk, txDrive);
			pSI5351tx->output_enable(iclk, disableOutput ? (0) : (1));
			pSI5351tx->output_enable(qclk, disableOutput ? (0) : (1));
			pSI5351tx->output_enable(SI5351_CLK2, 0);
			pSI5351tx->update_status();
		}
		if (pSI5351 == nullptr || pSI5351tx == nullptr)
		{
			cout << "No si5351 found" << endl;
			pSI5351.reset(nullptr);
			pSI5351tx.reset(nullptr);
		}
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

	return "v0.2";
}

SoapySDR::Kwargs SoapyHifiBerry::getHardwareInfo(void) const
{	

	SoapySDR_log(SOAPY_SDR_INFO, "SoapyHifiBerry::getHardwareInfo called");

	SoapySDR::Kwargs info;
	info["library_version"] = "0.2.0";
	info["backend_version"] = "0.2.0";
	info["manufacturer"] = "PA0PHH";
	info["label"] = "HifiBerry";
	info["origin"] = "https://github.com/paulh002/SoapyHifiBerry";
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
		options.push_back(0.384e6);
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
		return rxGain;
	return (SoapySDR::Range(0, 100));
}

void SoapyHifiBerry::setGain(const int direction, const size_t channel, const double value)
{
	char str[80];
	sprintf(str, "SoapyHifiBerry::setGain called %f %f", value, (int)value + abs(rxGain.minimum()));
	SoapySDR_log(SOAPY_SDR_INFO, str);

	if (direction == SOAPY_SDR_RX)
		uptr_HifiBerryAudioOutputput->controle_alsa(21, 2 * ((int)value + abs(rxGain.minimum()))); // numid = 21, iface = MIXER, name = 'ADC Capture Volume'

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
		switch (vfoIQMode)
		{
		case Single:
			if (pSI5351)
				pSI5351->set_freq((uint64_t)frequency * SI5351_FREQ_MULT * multiplier, iclk);
			break;
		case IQSingle:
		case IQSingleMultiPort: {
			if (pSI5351)
				pSI5351->setIQFrequency((long)frequency * (long)multiplier, iclk, qclk);
		}
			break;
		case IQMulti:
			if (pSI5351)
			{
				pTCA9548->setChannelMask(1);
				pSI5351->setIQFrequency((long)frequency * (long)multiplier, iclk, qclk);
			}
			break;
		}
	}

	if (direction == SOAPY_SDR_TX)
	{
		switch (vfoIQMode)
		{
		case Single:
			if (pSI5351)
				pSI5351->set_freq((uint64_t)frequency * SI5351_FREQ_MULT * multiplier, qclk);
			break;
		case IQSingle:
			if (pSI5351)
				pSI5351->setIQFrequency((long)frequency * (long)multiplier, iclk, qclk);
			break;
		case IQSingleMultiPort:
			if (pSI5351)
				pSI5351->setIQFrequency((long)frequency * (long)multiplier, iclkTx, qclkTx);
			break;
		case IQMulti:
			if (pSI5351tx)
			{
				pTCA9548->setChannelMask(2);
				pSI5351tx->setIQFrequency((long)frequency * (long)multiplier, iclk, qclk);
			}
			break;
		}
	}

}
// end of source.

void SoapyHifiBerry::setOutput(const int direction)
{
	if (disableOutput)
	{
		if (currDirection != direction)
		{
			if (direction == SOAPY_SDR_RX)
			{
				switch (vfoIQMode)
				{
				case Single:
					if (pSI5351)
					{
						pSI5351->output_enable(iclk, 1);
						pSI5351->output_enable(qclk, 0);
					}
					break;
				case IQSingle:
					break;
				case IQSingleMultiPort:
					if (pSI5351)
					{
						pSI5351->output_enable(iclk, 1);
						pSI5351->output_enable(qclk, 1);
						pSI5351->output_enable(iclkTx, 0);
						pSI5351->output_enable(qclkTx, 0);
					}
					break;
				case IQMulti:
					if (pSI5351tx)
					{
						pTCA9548->setChannelMask(1);
						pSI5351->output_enable(iclk, 1);
						pSI5351->output_enable(qclk, 1);
						pTCA9548->setChannelMask(2);
						pSI5351->output_enable(iclk, 0);
						pSI5351->output_enable(qclk, 0);
					}
					break;
				}
			}

			if (direction == SOAPY_SDR_TX)
			{
				switch (vfoIQMode)
				{
				case Single:
					if (pSI5351)
					{
						pSI5351->output_enable(iclk, 0);
						pSI5351->output_enable(qclk, 1);
					}
					break;
				case IQSingle:
					break;
				case IQSingleMultiPort:
					if (pSI5351)
					{
						pSI5351->output_enable(iclk, 0);
						pSI5351->output_enable(qclk, 0);
						pSI5351->output_enable(iclkTx, 1);
						pSI5351->output_enable(qclkTx, 1);
					}
					break;
				case IQMulti:
					if (pSI5351tx)
					{
						pTCA9548->setChannelMask(1);
						pSI5351->output_enable(iclk, 0);
						pSI5351->output_enable(qclk, 0);
						pTCA9548->setChannelMask(2);
						pSI5351->output_enable(iclk, 1);
						pSI5351->output_enable(qclk, 1);
					}
					break;
				}
			}
			currDirection = direction;
		}
	}
}