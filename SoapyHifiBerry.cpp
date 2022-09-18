#include <SoapySDR/Device.hpp>
#include <SoapySDR/Registry.hpp>

#include "SoapyHifiBerry.h"

/***********************************************************************
 * Find available devices
 **********************************************************************/

SoapySDR::KwargsList findMyHifiBerry(const SoapySDR::Kwargs &args)
{
	SoapySDR::Kwargs options;

	static std::vector<SoapySDR::Kwargs> results;

	options["driver"] = "hifiberry";
	results.push_back(options);

	return results;
}

/***********************************************************************
 * Make device instance
 **********************************************************************/
SoapySDR::Device *makeMyHifiBerry(const SoapySDR::Kwargs &args)
{
	return new SoapyHifiBerry(args);
}

/***********************************************************************
 * Registration
 **********************************************************************/
static SoapySDR::Registry registerHifiberry("hifiberry", &findMyHifiBerry, &makeMyHifiBerry, SOAPY_SDR_ABI_VERSION);
