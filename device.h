#ifndef DEVICE_H
#define DEVICE_H

#include <SoapySDR/Device.hpp>
#include <SoapySDR/Formats.hpp>
#include <iostream>
#include <thread>
#include <functional>


class device_t {
public:
    device_t() {
        std::cout << "Found devices:" << std::endl;

        SoapySDR::KwargsList devices = SoapySDR::Device::enumerate();
        SoapySDR::Kwargs::iterator it;

        for(size_t i = 0; i < devices.size(); i++) {
            for(it = devices[i].begin(); it != devices[i].end(); it++)
                if(it->first == "label")
                    std::cout << i << ": " << it->second << std::endl;
        }
        int d = 0;
        std::cout << "Select device: ";
        std::cin >> d;
        dev = SoapySDR::Device::make(devices[d]);
        dev->setSampleRate(SOAPY_SDR_TX, 0, 4e6);
        dev->setFrequency(SOAPY_SDR_TX, 0, 1090e6);
        dev->setGainMode(SOAPY_SDR_TX, 0, false);
        dev->setGain(SOAPY_SDR_TX, 0, 60);
        dev->setDCOffsetMode(SOAPY_SDR_TX, 0, true);
    }



    void launch() {
        std::thread t(std::bind(&device_t::stream, this));
        t.detach();
    }

    void setcallback(std::function<void(std::complex<float>*)> func) {
        callback = func;
    }

    void fold() {
        if(!running)
            return;
        folding = true;
        while(folding)
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }

    ~device_t() {
        fold();
        //SoapySDR::Device::unmake(dev);
    }

private:
    bool running = false;
    bool folding = false;
    SoapySDR::Device *dev;
    void stream() {
        running = true;
        SoapySDR::Stream *tx_stream = dev->setupStream(SOAPY_SDR_TX, SOAPY_SDR_CF32);
        dev->activateStream(tx_stream);
        int activeBuf = 0;
        std::complex<float> buff[2][1000];
        if(!callback)
            goto quit;
        callback(buff[0]);
        while(!folding) {
            int flags;
            void *buffs[] = { buff[activeBuf] };
            dev->writeStream(tx_stream, buffs, 1000, flags);
            callback(buff[(activeBuf + 1) & 0x01]);
            activeBuf = (activeBuf + 1) & 0x01;
        }
quit:
        dev->deactivateStream(tx_stream);
        dev->closeStream(tx_stream);
        folding = false;
        running = false;
    }
     std::function<void(std::complex<float>*)> callback = nullptr;

};

#endif // DEVICE_H
