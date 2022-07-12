# SoapyHifiBerry 
The SoapyHifiBerry driver is to use the HifiBerry DAC + ADC (Pro) as a SDR Tranceiver in combination with a Tayloe detector / Mixer.
The idea and design of the detector is from the book "Software Defined Radio Tranceiver" T41-EP by AC8GY and W8TEE
I adapted the design for use with Raspberry PI 4B. On the PCB there is a Tayloe mixer FST3253 / SN74CBT3252 and a Si5351 oscilator.
A SN74HC74 is used to devide the clock by 4 and deliver 90 degree phase shift.
The SoapyHifiBerry driver work with my SDR software SDRBERRY https://github.com/paulh002/sdrberry, I did not test it with other SDR software and all is an experiment.

ToDo:
- phase correction (if necessary)
- level correction

Done:
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

[sound]
device = "snd_rpi_hifiberry_dacplusadcpro"
```


![Tayloe](https://github.com/paulh002/SoapyHifiBerry/blob/master/Tayloe.jpg)

![sdrberry](https://github.com/paulh002/SoapyHifiBerry/blob/master/HifiBerry%20Tayloe.jpg)

![sdrberry](https://github.com/paulh002/sdrberry/blob/master/rb_tranceiver.jpg)