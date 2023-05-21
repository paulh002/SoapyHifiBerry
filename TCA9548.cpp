#include "TCA9548.h"

//
//    FILE: TCA9548.cpp
//  AUTHOR: Rob Tillaart
// VERSION: 0.1.0
//    DATE: 2021-03-16
// PURPOSE: Library for TCA9548 I2C multiplexer
//
//  HISTORY:
//  0.1.0   2021-03-16  initial version

#include "TCA9548.h"
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
extern "C" {
#include <linux/i2c-dev.h>
}
#include <sys/ioctl.h>

TCA9548::TCA9548(const char *i2c_device_filepath, const uint8_t deviceAddress)
	: _address(deviceAddress), i2c_file(0), i2c_filepath(i2c_device_filepath)
{
	_mask = 0x00;
	_resetPin = -1;
	_forced = false;
}

bool TCA9548::begin(uint8_t mask)
{
	//Open i2c device
	int file;
	if ((file = open(i2c_filepath.data(), O_RDWR)) < 0)
	{
		return false;
	}
	i2c_file = file;

	//Set address
	if (ioctl(file, I2C_SLAVE_FORCE, _address) < 0)
	{
		return false;
	}
	
	if (!isConnected())
		return false;
	setChannelMask(mask);
	return true;
}

int TCA9548::i2c_read()
{
	uint8_t reg_val = 0;

	int result = write(i2c_file, &_address, 1);
	if (result < 0)
	{
		return result;
	}

	uint8_t data;
	result = read(i2c_file, &data, 1);
	if (result < 0)
	{
		return result;
	}
	else
	{
		reg_val = data;
	}

	return reg_val;
}

int TCA9548::i2c_write(uint8_t data)
{
	uint8_t buff[2];
	buff[0] = _address;
	buff[1] = data;
	int result = write(i2c_file, buff, 2);
	if (result < 0)
	{
		return result;
	}
	return 0;
}

bool TCA9548::isConnected()
{
	int retval = i2c_read();
	return (retval >= 0);
}

void TCA9548::enableChannel(uint8_t channel)
{
	if (isEnabled(channel))
		return;
	setChannelMask(_mask | (0x01 << channel));
}

void TCA9548::disableChannel(uint8_t channel)
{
	if (!isEnabled(channel))
		return;
	setChannelMask(_mask & ~(0x01 << channel));
}

void TCA9548::selectChannel(uint8_t channel)
{
	setChannelMask(0x01 << channel);
}

bool TCA9548::isEnabled(uint8_t channel)
{
	if (channel > 7)
		return false;
	return (_mask & (0x01 << channel));
}

void TCA9548::setChannelMask(uint8_t mask)
{
	if ((_mask == mask) && (!_forced))
		return;
	_mask = mask;
	i2c_write(mask);
}

uint8_t TCA9548::getChannelMask()
{
	return _mask;
}

int TCA9548::getError()
{
	int e = _error;
	_error = 0;
	return e;
}

// -- END OF FILE --
