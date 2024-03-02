# SoapyHifiBerry 
The SoapyHifiBerry driver is to use the HifiBerry DAC + ADC (Pro) as a SDR Tranceiver in combination with a Tayloe detector / Mixer.
The idea and design of the detector is from the book "Software Defined Radio Tranceiver" T41-EP by AC8GY and W8TEE
I adapted the design for use with Raspberry PI 4B. On the PCB there is a Tayloe mixer FST3253 / SN74CBT3252 and a Si5351 oscilator.
A SN74HC74 is used to devide the clock by 4 and deliver 90 degree phase shift. Alternative 2 si5351 can be used clock 0 and clock 1 will deliver 90 deg phase shift. A TCA9548 is used to switch the i2c bus.
The SoapyHifiBerry driver work with my SDR software SDRBERRY https://github.com/paulh002/sdrberry, I did not test it with other SDR software and all is an experiment.
(SoapyHifiBerry relies on RtAudio library, which is included staticly. This can give problems with applications using also RtAudio but a different version, 
in that case remove the RtAudio.cpp from SoapyHifiBerry and use correct RtAudio.h)

The sdr41-rxtx board uses a Si5351A-B-GM which has 8 outputs 2 outputs are used in quadrature for receive and 2 outputs for receive. Unfortunaly 160 M is not possible in this design because of limitations of the si5351.
An option would be to add a divide by 2 chip. The board compresses the whole design to a single pcb. SoapyHifiBerry can be configured by setting the mode to IQSINGLEMULTI and the multiplier to 1.

Done:
- Support 2 times Si5351 (RX and TX)
- Si5351 I2C interface
- Gain interface
- Receive IQ signal
- Transmit IQ interface
- CMake compilation

Installation of libraries is necessary:
- Liquid DSP
- Alsa audio
- SoapySDR
- pthread

# Installation

## Install libraries

git clone https://github.com/pothosware/SoapySDR.git
cd SoapySDR
git pull origin master
mkdir build
cd build
cmake ..
make -j4
sudo make install
sudo ldconfig

cd ~
sudo apt-get install -y fftw3-dev
git clone https://github.com/jgaeddert/liquid-dsp
git checkout v1.4.0
sudo apt-get install -y automake autoconf
cd liquid-dsp
./bootstrap.sh
./configure
make -j4
sudo make install
sudo ldconfig


## Install and compile with VisualStudio and VisualGDB
Download the repository in pi home directory  
```
git clone https://github.com/paulh002/SoapyHifiBerry  
Open project in VisualStudio and compile with VisualGDB

```

## Install and compile with CMake
```
git clone https://github.com/paulh002/SoapyHifiBerry  
cd SoapyHifiBerry
mkdir build
cd build
cmake ..
make
sudo make install
```

SoapyHifiBerry will create config file HifiBerry.cfg Si5351 correction can be configured and audio adc/dac alsa device name set
```
[si5351]
correction = "50000"
mode = "single"

[sound]
device = "snd_rpi_hifiberry_dacplusadcpro"
```

To enable 2 time si5351

```
[si5351]
mode = "IQ"
```


To enable single Si5351A-B-GM

```
[si5351]
correction = "121000"
correctiontx = "132500"
rxdrive = "2"
txdrive = "4"
mode = "IQSINGLEMULTI"
multiplier = 1
disabletxoutput = "on"

```

## QSD and QSE combined board (sdr41-rxtx)
![Tayloe](https://github.com/paulh002/SoapyHifiBerry/blob/master/sdr-txrx.jpg)

## Seperate QSD and QSE boards (Kicad rx and tx v3)
![sdrberry](https://github.com/paulh002/SoapyHifiBerry/blob/master/HifiBerry%20Tayloe.jpg)

![sdrberry](https://github.com/paulh002/sdrberry/blob/master/rb_tranceiver.jpg)