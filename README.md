# SoapyHifiBerry 
The SoapyHifiBerry driver is to use the HifiBerry DAC + ADC (Pro) as a SDR Tranceiver in combination with a Tayloe detector / Mixer.
The idea and design of the detector is from the book "Software Defined Radio Tranceiver" T41-EP by AC8GY and W8TEE
I adapted the design for use with Raspberry PI 4B. On the PCB there is a Tayloe mixer FST3253 / SN74CBT3252 and a Si5351 oscilator.
A SN74HC74 is used to devide the clock by 4 and deliver 90 degree phase shift.
The SoapyHifiBerry driver work with my SDR software SDRBERRY, I did not test it with other SDR software and all is an experiment.

ToDo:
- CMake compilation
- Transmit IQ interface 

Done:
- Si5351 I2C interface
- Gain interface
- Receive IQ signal

Installation of libraries is necessary:
- Liquid DSP
- Alsa audio
- SoapySDR
- pthread

# Installation


## Install and compile with VisualStudio and VisualGDB
Download the repository in pi home directory  
```
git clone https://github.com/paulh002/SoapyHifiBerry  
Open project in VisualStudio and compile with VisualGDB

```

![Tayloe](https://github.com/paulh002/SoapyHifiBerry/blob/master/Tayloe.jpg)

![sdrberry](https://github.com/paulh002/SoapyHifiBerry/blob/master/HifiBerry%20Tayloe.jpg)

![sdrberry](https://github.com/paulh002/sdrberry/blob/master/IMG_20211215_200645.jpg)

![sdrberry](https://github.com/paulh002/sdrberry/blob/master/IMG_20210909_183113.jpg)

[![Radioberry demo](https://img.youtube.com/vi/BMJiv3YGv-k/0.jpg)](https://youtu.be/PQ_Np5SfcxA)

# ESP32 Remote control for raspberry pi
![sdrberry](https://github.com/paulh002/sdrberry/blob/master/IMG_20210903_133827.jpg)