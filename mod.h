#ifndef MOD_H
#define MOD_H
#include <device.h>
#include <server.h>
#include <future>
#include <mutex>
#include <queue>
#include <complex>
#include <cmath>
#include <bitset>

using namespace std::complex_literals;

struct internmessage_t {
    std::bitset<112> msg;
};


__int128 reverse(__int128 n, __int128 len) {
    __int128 x = 0;
    for(__int128 i = 0; i < len; i++) {
        x <<= 1;
        x |= (n & 1);
        n >>= 1;
    }
    return x;
}

void rv112(internmessage_t &m){
    for(size_t i = 0; i < 56; i++) {
        int b = m.msg[i];
        m.msg[i] = m.msg[m.msg.size() - (1+i)];
        m.msg[m.msg.size() - (1+i)] = b;
    }
}


unsigned long crc(__int128 msg) {
    unsigned long generator = 0b111111111111101000000101;
    msg = reverse(msg, 112);
    for(int i = 112; i > 24; i--)
        if((msg >> i) & 0x01) {
            //Shift down to 0-x
            __int128 imm1 = msg >> i;
            __int128 mask = (__int128)0xffffffffffffffff << 64;
            mask |= 0xffffffffffffffff;
            //AND off all the top bits
            imm1 &= 0b111111111111111111111111;
            //Generate a mask.
            imm1 ^= generator;
            for(int k = i; k > i-24; k--) {
                mask ^= 0x01 << k;
            }
            msg &= mask;
            //Apply the xor.
            msg |= imm1 << i;
            //Generate another mask

        }
    return reverse(msg & 0xffffff, 24);
}

class modulator_t {
public:
    void launch() {
        device.setcallback(std::bind(&modulator_t::mod_callback, this, std::placeholders::_1));
        server.setcallback(std::bind(&modulator_t::json_callback, this, std::placeholders::_1));
        device.launch();
        server.launch();
    }


    void mod_callback(std::complex<float> *buf) {
        internmessage_t msg;
        int length = 112;
        int nleft = 1000;
        {
            std::lock_guard<std::mutex> lock(m);
            if(!mailbox.size()) {
                goto skipmod;
            }
            msg = mailbox.back();
            mailbox.pop();
        }
        for(int i = 0; i < 32; i++) {
            buf[i].imag(0);
            buf[i].real(0);

        }
        //Synch pulses (
        buf[0].imag(1.0f);
        buf[0].real(1.0f);
        buf[1].imag(0);
        buf[1].real(0);
        buf[4].imag(1.0f);
        buf[4].real(1.0f);
        buf[5].imag(0);
        buf[5].real(0);
        buf[14].imag(1.0f);
        buf[14].real(1.0f);
        buf[15].imag(0);
        buf[15].real(0);
        buf[18].imag(1.0f);
        buf[18].real(1.0f);
        buf[19].imag(0);
        buf[19].real(0);
        nleft -= 32;
        //Encode message
        for(int i = 0; i < length; i++) {
            if(msg.msg.test(111-i)) {
                buf[32 + i*4].imag(1.0f);
                buf[32 + i*4].real(1.0f);
                buf[32 + i*4 + 1].imag(1.0f);
                buf[32 + i*4 + 1].real(1.0f);
                buf[32 + i*4 + 2].imag(0);
                buf[32 + i*4 + 2].real(0);
                buf[32 + i*4 + 3].imag(0);
                buf[32 + i*4 + 3].real(0);
            } else {
                buf[32 + i*4].imag(0);
                buf[32 + i*4].real(0);
                buf[32 + i*4 + 1].imag(0);
                buf[32 + i*4 + 1].real(0);
                buf[32 + i*4 + 2].imag(1.0f);
                buf[32 + i*4 + 2].real(1.0f);
                buf[32 + i*4 + 3].imag(1.0f);
                buf[32 + i*4 + 3].real(1.0f);
            }
            nleft -= 4;
        }
skipmod:
        for(int i = 1000 - nleft; i < 1000; i++) {
            buf[i].imag(0);
            buf[i].real(0);
        }
    }

    void json_callback(rapidjson::Document &d) {
        std::lock_guard<std::mutex> lock(m);
        internmessage_t adsb;
        rapidjson::Value& message = d["message"];
        /*std::string df = message["format"].GetString();
        if(df == "transponder") {
            adsb.msg |= 0b10001;
        } else if(df == "ground") {
            adsb.msg |= 0b01001;
        }
        rapidjson::Value& capability = message["caps"];
        int level = capability["level"].GetInt();
        std::string location = capability["location"].GetString();
        if(level > 1) {
            if(location == "ground") {
                adsb.msg |= 0b001 << 5;
            } else if(location == "airborne") {
                adsb.msg |= 0b101 << 5;
            } else if(location == "either") {
                adsb.msg |= 0b011 << 5;
            }
        }
        rapidjson::Value& type_info = message["type"];
        if(type_info["what"] == "ident") {
            std::string category = "simple";
            if (message["planetype"] == "light") {
                category = "simple";
                adsb.msg |= (uint64_t)0b100 << 38;
            } else if (message["planetype"] == "mediumlight") {
                category = "simple";
                adsb.msg |= (uint64_t)0b010 << 38;
            } else if (message["planetype"] == "mediumhigh") {
                category = "simple";
                adsb.msg |= (uint64_t)0b110 << 38;
            } else if (message["planetype"] == "highvortex") {
                category = "simple";
                adsb.msg |= (uint64_t)0b001 << 38;
            } else if (message["planetype"] == "heavy") {
                category = "simple";
                adsb.msg |= (uint64_t)0b101 << 38;
            } else if (message["planetype"] == "highperf") {
                category = "simple";
                adsb.msg |= (uint64_t)0b011 << 38;
            } else if (message["planetype"] == "rotorcraft") {
                category = "simple";
                adsb.msg |= (uint64_t)0b111 << 38;
            } else if (message["planetype"] == "glider") {
                category = "esoteric";
                adsb.msg |= (uint64_t)0b100 << 38;
            } else if (message["planetype"] == "lifting") {
                category = "esoteric";
                adsb.msg |= (uint64_t)0b010 << 38;
            } else if (message["planetype"] == "skydiver") {
                category = "esoteric";
                adsb.msg |= (uint64_t)0b110 << 38;
            } else if (message["planetype"] == "ultralight") {
                category = "esoteric";
                adsb.msg |= (uint64_t)0b001 << 38;
            }  else if (message["planetype"] == "unmanned") {
                category = "esoteric";
                adsb.msg |= (uint64_t)0b011 << 38;
            } else if (message["planetype"] == "space") {
                category = "esoteric";
                adsb.msg |= (uint64_t)0b111 << 38;
            }
            if(category == "simple") {
                adsb.msg |= (uint64_t)0b00100 << 32;
            } else if (category== "esoteric") {
                adsb.msg |= (uint64_t)0b11000 << 32;
            } else if (category == "ground") {
                adsb.msg |= (uint64_t)0b01000 << 32;
            } else if (category == "reserved") {
                adsb.msg |= (uint64_t)0b10000 << 32;
            }
            rapidjson::Value& call = message["callsign"];
            for(int i = 0; i < 8; i++) {
                adsb.msg |= (uint64_t)(call.GetString()[i] & 0x3f) << (41 + (i*6));
            }

        } else if(type_info["what"] == "surfaceposition") {
            if(type_info["accuracy"] == "high") {
                adsb.msg |= (uint64_t)0b10100 << 32;
            } else if (type_info["accuracy"] == "medium") {
                adsb.msg |= (uint64_t)0b01100 << 32;
            } else if (type_info["accuracy"] == "low") {
                adsb.msg |= (uint64_t)0b11100 << 32;
            } else if (type_info["accuracy"] == "abysmal") {
                adsb.msg |= (uint64_t)0b00010 << 32;
            }
        } else if(type_info["what"] == "airborneposition") {
            if(type_info["barometric"] == "no") {
                if(type_info["accuracy"] == "high") {
                    adsb.msg |= (uint64_t)0b00101 << 32;
                } else if (type_info["accuracy"] == "medium") {
                    adsb.msg |= (uint64_t)0b10101 << 32;
                } else if (type_info["accuracy"] == "low") {
                    adsb.msg |= (uint64_t)0b01101 << 32;
                }
                goto nobaro;
            }
            if(type_info["accuracy"] == "high") {
                adsb.msg |= (uint64_t)0b10010 << 32;
            } else if (type_info["accuracy"] == "medium") {
                adsb.msg |= (uint64_t)0b01010 << 32;
            } else if (type_info["accuracy"] == "low") {
                adsb.msg |= (uint64_t)0b11010 << 32;
            } else if (type_info["accuracy"] == "abysmal") {
                adsb.msg |= (uint64_t)0b00110 << 32;
            } else if (type_info["accuracy"] == "pointone") {
                adsb.msg |= (uint64_t)0b10110 << 32;
            } else if (type_info["accuracy"] == "pointtwofive") {
                adsb.msg |= (uint64_t)0b01110 << 32;
            } else if (type_info["accuracy"] == "pointfive") {
                adsb.msg |= (uint64_t)0b11110 << 32;
            } else if (type_info["accuracy"] == "one") {
                adsb.msg |= (uint64_t)0b00001 << 32;
            } else if (type_info["accuracy"] == "five") {
                adsb.msg |= (uint64_t)0b10001 << 32;
            } else if (type_info["accuracy"] == "probabilityfield") {
                adsb.msg |= (uint64_t)0b01001 << 32;
            }
nobaro:
        }
        if(type_info["what"] == "test") {
            adsb.msg |= (uint64_t)0b11101 << 32;
        }
        if(type_info["what"] == "surfacestatus") {
            adsb.msg |= (uint64_t)0b00011 << 32;
        }
        if(type_info["what"] == "emergency") {
            adsb.msg |= (uint64_t)0b00111 << 32;
        }
        if(type_info["what"] == "velocity") {
            adsb.msg |= (uint64_t)0b01001 << 32;
            if(type_info["speed"] == "subsonic" && type_info["vtype"] == "heading")
                adsb.msg |= (uint64_t)0b011 << 37;
            else if(type_info["speed"] == "subsonic" && type_info["vtype"] == "ground")
                adsb.msg |= (uint64_t)0b001 << 37;
            else if(type_info["speed"] == "supersonic" && type_info["vtype"] == "heading")
                adsb.msg |= (uint64_t)0b010 << 37;
            else if(type_info["speed"] == "supersonic" && type_info["vtype"] == "ground")
                adsb.msg |= (uint64_t)0b100 << 37;
        }
        if(type_info["what"] == "opstatus") {
            adsb.msg |= (uint64_t)0b11111 << 32;
        }
        std::string address = message["address"].GetString();
        unsigned long x = std::stoul(address, nullptr, 16);
        std::cout << std::bitset<24>((int)reverse(x, 24)) << std::dec <<  std::endl;
        adsb.msg |= reverse(x, 24) << 8;

        //adsb.msg |= (unsigned long)0b10000 << 32;
        //adsb.msg |= (__int128)crc(adsb.msg) << 88;
        std::cout << "pushing packet!" << std::endl;
        //adsb.msg = reverse(adsb.msg, 112);
        rv112(adsb);
        std::cout << adsb.msg.to_string() << std::endl;
        adsb.msg = std::bitset<112>("1000110110101011110111110011111110011001000101000001000000111000011110000000100001010010010100100000000110001000");
        std::cout << adsb.msg.to_string() << std::endl;
        std::cout << adsb.msg.test(111) << std::endl;*/
        adsb.msg = std::bitset<112>(message["rawmsg"].GetString());
        mailbox.push(adsb);
    }
private:
    std::mutex m;
    std::queue<internmessage_t> mailbox;
    server_t server;
    device_t device;
};

#endif // MOD_H
