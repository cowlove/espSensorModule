#ifndef __SENSORNETWORKESPNOW_H_
#define __SENSORNETWORKESPNOW_H_
#include "espNowMux.h"
#include "reliableStream.h"

#include <string>
#include <vector>
using std::string;
using std::vector;

class RemoteSensorArray;

class Sensor { 
friend RemoteSensorArray;
protected:
    bool isOutput = false;
public:    
    Sensor(RemoteSensorArray *parent = NULL, std::string n = "");
    virtual void begin() {}
    virtual string makeSchema() = 0;
    virtual string makeReport() = 0;
    virtual void setValue(const string &s) {}
    string name = "";
    string result = "";
    float asFloat() { return strtof(result.c_str(), NULL); }
    string asString() { return result; }

};

class SchemaParser { 
public:
    typedef Sensor *(*ParserFunc)(const string &);
    static vector<ParserFunc> parserList;

    static vector<Sensor *> parseSchema(const string &s) {
        vector<Sensor *> rval; 
        for (auto w : split(s, ' ')) { 
            string name = w.substr(0, w.find("="));
            string sch = w.substr(w.find("=") + 1); 
            for(auto i : parserList) { 
                Sensor *p = i(sch);
                if (p) {
                    p->name = name;
                    rval.push_back(p);
                    break;
                }
            }
        }
        return rval;
    }
    struct RegisterClass { 
        RegisterClass(ParserFunc a) { parserList.push_back(a); }
    };
    string hashSchema(const string &schema) { 
        return "hashSchema()HASH";
    }

};
vector<SchemaParser::ParserFunc> SchemaParser::parserList;
    
class RemoteSensorServer;

class RemoteSensorArray {
    vector<Sensor *> sensors;
    vector<const char*> requiredFields = {"SCHASH=SCHASH", "MAC=MAC"};
    string mac, schema;
protected:
    friend RemoteSensorServer;
public:
    RemoteSensorArray(const char *_mac, const char *_schema = "") : mac(_mac), schema(_schema) {
        for(auto p : requiredFields) { 
            if (strstr(schema.c_str(), p) == NULL)
                schema = string(p) + " " + schema;
        }
        sensors = SchemaParser::parseSchema(schema);
    }
    //RemoteSensorArray(vector<Sensor> &v) { for(auto p : v) { sensors.push_back(&p);}}
    RemoteSensorArray(vector<Sensor *> &v) { for(auto p : v) { sensors.push_back(p); }}
    void addSensor(Sensor *p) { sensors.push_back(p); } 
    float read(const char *key = "RESULT") { return 0; }
    float read(const string &k) { return this->read(k.c_str()); }
    //string makeAllSchema(); 
    //string makeAllResults();
    //void parseAllResults(const string &s);

    bool updateReady = false;
    bool updated() { 
        if (updateReady) { 
            updateReady = false;
            return true;
        }
        return false;
    }
    Sensor *findByName(const char *n) { 
        for(auto i : sensors) { 
            if (i->name == n) 
                return i;
        }
        return NULL;
    }
    string makeAllSchema() { 
        string r;
        for(auto i : sensors) { 
            r += i->name + "=" + i->makeSchema() + " ";
        }
        return r;
    }

    string makeAllResults() { 
        string r;
        findByName("MAC")->result = this->mac;
        findByName("SCHASH")->result = makeHash();
        for(auto i : sensors) { 
            i->result = i->makeReport();
            r += i->name + "=" + i->result + " ";
        }
        return r;
    }

    string makeHash() {
        string hash = makeAllSchema(); 
        return sfmt("%08x", hash.length());
    }
    void parseAllResults(const string &s) {
        for (auto w : split(s, ' ')) { 
            string name = w.substr(0, w.find("="));
            string v = w.substr(w.find("=") + 1); 
            for(auto i : sensors) { 
                if (name == i->name && i->isOutput == false) { 
                    i->result = v;
                    break;
                }
            }
        }
        if (s != "")
            updateReady = true;
    }

    void parseAllSetValues(const string &s) {
        vector<Sensor *> rval; 
        for (auto w : split(s, ' ')) { 
            string name = w.substr(0, w.find("="));
            string v = w.substr(w.find("=") + 1); 
            for(auto i : sensors) { 
                if (name == i->name) { 
                    i->setValue(v);
                    break;
                }
           }        
        } 
    }

    string makeAllSetValues() {
        string r;
        for(auto i : sensors) { 
            if (i->isOutput) {
                r += i->name + "=" + i->result + " ";
            }
        }
        return r;
    }

    void debugPrint() { 
        for(auto ci : sensors) { 
            printf("%s: %s\n", ci->name.c_str(), ci->result.c_str());
        }
    }
};

class SensorWrapper {
private:
    RemoteSensorArray *parent;
    string key;
public: 
    SensorWrapper(RemoteSensorArray *_parent, const string &_key) : parent(_parent), key(_key) {}  
    float read() { return parent->read(key); }  
};

Sensor::Sensor(RemoteSensorArray *parent /*= NULL*/, std::string n /*= ""*/) : name(n) { 
    if (parent) parent->addSensor(this);
}

    
class SensorSchemaHash : public Sensor { 
public:
    SensorSchemaHash(RemoteSensorArray *p = NULL) : Sensor(p, "SCHASH") { result = "NO_HASH"; }
    void begin() {}
    string makeSchema() { return "SCHASH"; }
    string makeReport() { return result; }
    static SchemaParser::RegisterClass reg;
};
SchemaParser::RegisterClass SensorSchemaHash::reg([](const string &s)->Sensor * { 
    return s == "SCHASH" ? new SensorSchemaHash(NULL) : NULL; 
});

class SensorMillis : public Sensor { 
    public:
        SensorMillis(RemoteSensorArray *p) : Sensor(p, "MILLIS") {}
        void begin() {}
        string makeSchema() { return "MILLIS"; }
        string makeReport() { return sfmt("%d", millis()); }
        static SchemaParser::RegisterClass reg;
};
SchemaParser::RegisterClass SensorMillis::reg([](const string &s)->Sensor * { 
    return s == "MILLIS" ? new SensorMillis(NULL) : NULL; 
});
    
class SensorMAC : public Sensor { 
public:
    SensorMAC(RemoteSensorArray *p) : Sensor(p, "MAC") { result = "NO_MAC"; }
    string makeSchema() { return "MAC"; }
    string makeReport() { return result; }
    static SchemaParser::RegisterClass reg;
};
SchemaParser::RegisterClass SensorMAC::reg([](const string &s)->Sensor * { 
    return s == "MAC" ? new SensorMAC(NULL) : NULL; 
});
    
class SensorGit : public Sensor { 
public:
    SensorGit(RemoteSensorArray *p) : Sensor(p, "GIT") {}
    string makeSchema() { return "GIT"; }
    string makeReport() { return string(GIT_VERSION); }
    static SchemaParser::RegisterClass reg;
};
SchemaParser::RegisterClass SensorGit::reg([](const string &s)->Sensor * { 
    return s == "GIT" ? new SensorGit(NULL) : NULL; 
});
    
class SensorADC : public Sensor { 
    int pin;
    float scale;
public:
    SensorADC(RemoteSensorArray *p, const char *n, int _pin, float s = 1.0) : Sensor(p, n), pin(_pin), scale(s) {}    
    string makeSchema() { return sfmt("ADC%d*%f", pin, scale); }
    string makeReport() { return sfmt("%f", avgAnalogRead(pin, 1024) * scale + (millis() % 971) / 1000.0); }
    static SchemaParser::RegisterClass reg;
};
SchemaParser::RegisterClass SensorADC::reg([](const string &s)->Sensor * { 
    float pin, scale;
    if (sscanf(s.c_str(), "ADC%f*%f", &pin, &scale) == 2) 
        return new SensorADC(NULL, "", pin);
    return NULL; 
});

class SensorDHT : public Sensor { 
    int pin;
public:
    SensorDHT(RemoteSensorArray *p, const char *n, int _pin) : Sensor(p, n), pin(_pin) {}    
    void begin() {}
    string makeSchema() { return sfmt("DHT%d", pin); }
    string makeReport() { return sfmt("%f", 12.34 + (millis() % 573) / 1000.0); }
    static SchemaParser::RegisterClass reg;
};

SchemaParser::RegisterClass SensorDHT::reg([](const string &s)->Sensor * { 
    int pin;
    if (sscanf(s.c_str(), "DHT%d", &pin) == 1) 
        return new SensorDHT(NULL, "", pin);
    return NULL; 
});

class SensorInput : public Sensor { 
    int pin, mode;
public:
    SensorInput(RemoteSensorArray *p, const char *n, int pi, int m) : Sensor(p, n), pin(pi), mode(m) {}    
    void begin() { pinMode(pin, mode); }
    string makeSchema() {
        const char *m = "";
        if (mode == INPUT_PULLUP) m = "PU";
        if (mode == INPUT_PULLDOWN) m = "PD";  
        return sfmt("INPUT%d%s", pin, m); 
    }
    string makeReport() { return sfmt("%d", digitalRead(pin)); }
    static SchemaParser::RegisterClass reg;
};
SchemaParser::RegisterClass SensorInput::reg([](const string &s)->Sensor * { 
    int pin, mode = INPUT;
    if (sscanf(s.c_str(), "INPUT%d", &pin) == 1) {
        if (strstr(s.c_str(), "PU") != NULL) mode = INPUT_PULLUP;
        if (strstr(s.c_str(), "PD") != NULL) mode = INPUT_PULLDOWN;
        return new SensorInput(NULL, "", pin, mode);
    }
    return NULL; 
});

class SensorOutput : public Sensor { 
    int pin, mode;
public:
    SensorOutput(RemoteSensorArray *p, const char *n, int pi, int m) : Sensor(p, n), pin(pi), mode(m) {
        isOutput = true;
        result = sfmt("%d", mode);
    }    
    void begin() { pinMode(pin, OUTPUT); digitalWrite(pin, mode); }
    string makeSchema() { return sfmt("OUTPUT%d,%d", pin, mode); }
    string makeReport() { return sfmt("%d", digitalRead(pin)); }
    void setValue(const string &s) { 
        sscanf(s.c_str(), "%d", &mode);
        OUT("setting pin %d to %d", pin, mode);
        digitalWrite(pin, mode);
    }
    static SchemaParser::RegisterClass reg;
};
SchemaParser::RegisterClass SensorOutput::reg([](const string &s)->Sensor * { 
    int pin, mode;
    if (sscanf(s.c_str(), "OUTPUT%d,%d", &pin, &mode) == 2) {
        return new SensorOutput(NULL, "", pin, mode);
    }
    return NULL; 
});

class SensorVariable : public Sensor { 
    string defaultValue;
public:
    string value;
    SensorVariable(RemoteSensorArray *p, const char *n, const char *v) : Sensor(p, n), defaultValue(v) {
        isOutput = true;
        result = defaultValue;
        value = defaultValue;
    }    
    string makeSchema() { return sfmt("VAR,%s", defaultValue.c_str()); }
    string makeReport() { return value; }
    void setValue(const string &s) { 
        value = s;
        OUT("setting variable %s to %s", name.c_str(), value.c_str());
    }
    static SchemaParser::RegisterClass reg;
};
SchemaParser::RegisterClass SensorVariable::reg([](const string &s)->Sensor * { 
    char txt[256];
    if (sscanf(s.c_str(), "VAR,%s", txt) == 1) {
        return new SensorVariable(NULL, "", txt);
    }
    return NULL; 
});

ReliableStreamESPNow fakeEspNow("SN", true);


class FakeEspNowFifo { 
public:
    string data;
    void write(const string &s) { data = s; }
    string read() { string rval = data; data = ""; return rval; }

} fakeEspNowNO;

class RemoteSensorServer { 
    vector<RemoteSensorArray *> modules;
public: 
    RemoteSensorServer(int espNowChannel) {}
    RemoteSensorServer(int espNowChannel, vector<RemoteSensorArray *> m) : modules(m) {}    
    void onReceive(const string &s) {
        OUT("%09.4f server <<<< %s", millis() / 1000.0, s.c_str());
        string incomingMac = "", incomingHash = "";
        for(auto w : split(s, ' ')) { 
            string name = w.substr(0, w.find("="));
            string val = w.substr(w.find("=") + 1);
            //OUT("%s == %s", name.c_str(), val.c_str());
            if (name == "MAC") { incomingMac = val; }
            if (name == "SCHASH") { incomingHash = val; }
        } 
        bool packetHandled = false;
        for(auto p : modules) { 
            if (p->mac == incomingMac) {
                packetHandled = true;
                string hash = p->makeHash();
                for(auto w : split(s, ' ')) { 
                    string name = w.substr(0, w.find("="));
                    string val = w.substr(w.find("=") + 1);
                    if (name == "SCHASH") {
                        if (hash != val) { 
                            string out = "MAC=" + p->mac + " NEWSCHEMA=1 " + p->makeAllSchema();
                            write(out);
                            return;
                        }
                    } else if (0) { 
                        string out = "MAC=" + p->mac + " UPDATENOW=1 ...";
                        write(out);
                    } else { 
                    }
                }
                if (hash == incomingHash) { 
                    p->parseAllResults(s);
                    string out = "MAC=" + p->mac + " SCHASH=" + hash + " SLEEP=60 " + p->makeAllSetValues();
                    write(out);
                } else {
                    OUT("%s != %s", incomingHash.c_str(), hash.c_str());
                }
            }
        }
        if (!packetHandled && incomingMac != "") { 
            OUT("Unknown MAC: %s", s.c_str());
            for(auto p : modules) { 
                if (p->mac == "auto") {
                    p->mac = incomingMac;
                    string schema = p->makeAllSchema();
                    OUT("Auto assigning incoming mac %s to sensor %s", p->mac.c_str(), schema.c_str());
                    break;
                }
            }
        }
    }

    void write(const string &s) { 
        OUT("%09.4f server >>>> %s", millis() / 1000.0, s.c_str());
        fakeEspNow.write(s);
    }

    void begin() { 
        // set up ESPNOW, register this->onReceive() listener 
    }
    void run() {
        string in = fakeEspNow.read();
        if (in != "") 
            onReceive(in);
        // TODO: have we heard from all sensors?  we can sleep; 
    }
};

class RemoteSensorClient { 
    RemoteSensorArray *array = NULL;
    string myMac() { return getMacAddress().c_str(); }
    static SPIFFSVariable<int> lastChannel;
    static SPIFFSVariable<string> lastSchema;
public:
    RemoteSensorClient() { 
        string s = lastSchema;
        espNowMux.defaultChannel = lastChannel; 
        init(s);
    }
    void init(const string &schema) { 
        if (array != NULL) delete array;
        array = new RemoteSensorArray(myMac().c_str(), schema.c_str());
    }
    void updateFirmware() {
        // TODO
    }
    bool updateFirmwareNow = false, updateSchemaNow = false;
    uint32_t lastReceive = 0;
    void onReceive(const string &s) { 
        OUT("%09.4f client <<<< %s", millis() / 1000.0, s.c_str());
        string schash, newSchema;
        bool updatingSchema = false;   
        int sleepTime = -1;     
        for(auto w : split(s, ' ')) { 
            if (updatingSchema) {
                newSchema += w + " ";
            } else { 
                string name = w.substr(0, w.find("="));
                string val = w.substr(w.find("=") + 1);
                if (name == "MAC") {
                    if (val != myMac()) return;
                    lastChannel = espNowMux.defaultChannel;
                    lastReceive = millis();
                }
                else if (name == "NEWSCHEMA") updatingSchema = true;
                else if (name == "UPDATENOW") updateFirmware();
                else if (name == "SLEEP") sscanf(val.c_str(), "%d", &sleepTime);
                // TODO: process setValues
            }
        }
        if (updatingSchema) { 
            OUT("Got new schema: %s", newSchema.c_str());
            init(newSchema);
            lastSchema = newSchema;
            string out = array->makeAllResults();
            write(out);
            return;
        }
        if (array)
            array->parseAllSetValues(s);
        // TODO:  Write schema and shit to SPIFF
        if (sleepTime > 0) { 
            OUT("%08.4f: Sleeping %d seconds...", millis() / 1000.0, sleepTime);
            WiFi.disconnect(true);  // Disconnect from the network
            WiFi.mode(WIFI_OFF);    // Switch WiFi off
            int rc = esp_sleep_enable_timer_wakeup(1000000LL * sleepTime);
            Serial.flush();
            //esp_light_sleep_start();                                                                 
            esp_deep_sleep_start();
            //delay(1000 * sleepTime);                                                                 
            ESP.restart();                                  
        }
    }
    void write(const string &s) { 
        OUT("%09.4f client >>>> %s", millis() / 1000.0, s.c_str());
        fakeEspNow.write(s);
    }
    void run() {
        string in = fakeEspNow.read();
        if (in != "") 
            onReceive(in); 
        if (array != NULL && (j.once() || j.secTick(1))) { 
            string out = array->makeAllResults();
            write(out);
        }
        if (millis() - lastReceive > 5000) {
            espNowMux.defaultChannel = (espNowMux.defaultChannel + 1) % 14;
            espNowMux.stop();
            espNowMux.firstInit = true;
            lastReceive = millis();
        }

    }
};

SPIFFSVariable<int> RemoteSensorClient::lastChannel 
    = SPIFFSVariable<int>("/sensorNetwork_lastChannel2", 1);
SPIFFSVariable<string> RemoteSensorClient::lastSchema 
    = SPIFFSVariable<string>("/sensorNetwork_lastSchema", "MAC=MAC SKHASH=SKHASH GIT=GIT MILLIS=MILLIS");


//static SchemaList::Register();
#endif //__SENSORNETWORKESPNOW_H_