#include "jimlib.h"
JStuff j;
#include "sensorNetworkEspNOW.h"
#include "serialLog.h"

CLI_VARIABLE_FLOAT(x, 800);

class RemoteSensorModuleDHT : public RemoteSensorModule {
public:
    RemoteSensorModuleDHT(const char *mac) : RemoteSensorModule(mac) {}
    SensorOutput gnd = SensorOutput(this, "GND", 27, 0);
    SensorDHT tempC = SensorDHT(this, "TEMP", 26);
    SensorOutput vcc = SensorOutput(this, "VCC", 25, 1);
    SensorADC battery = SensorADC(this, "LIPOBATTERY", 33, .0017);
    SensorOutput led = SensorOutput(this, "LED", 22, 0);
    SensorVariable v = SensorVariable(this, "RETRY", "X10");
    SensorMillis m = SensorMillis(this);
    ////} ambientTempSensor1("EC64C9986F2C");
} ambientTempSensor1("auto"), at2("auto"), at3("auto");

// TODO - these server-side instances also call SensorOutput::setValue and write pins, etc

// also maybe config by SCHEMA like
//RemoteSensorModule ambientTempSensor2("FFAACCEE01", "RESULT=DHT11 BAT=ADC13*.00023 TEMP=DHT11 LED=OUTPUT22,1 MILLIS=MILLIS");
//SensorWrapper ambientTemp(&ambientTempSensor2, "RESULT");
//SensorWrapper ambientTempBattery(&ambientTempSensor2, "BAT");

//float volts1 = ambientTempSensor2.read("BAT");
//float volts2 = ambientTempBattery.read();
//float volts3 = ambientTempSensor1.tempC.asFloat();

RemoteSensorServer server({ &ambientTempSensor1, &at2, &at3 });
        
RemoteSensorClient client1;
SPIFFSVariable<vector<string>> logFile("/logFile", {}); 

bool isServer() { 
    return getMacAddress() == "D48AFCA4AF20" || getMacAddress() == "083AF2B59110" 
        || getMacAddress() == "E4B063417040" || getMacAddress() == "FFEEDDAABBCC";
}
void setup() {
    //WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); //disable brownout 
    if (isServer() == false) { 
        j.jw.enabled = false;
    }
    j.begin();    
    j.run();
    SPIFFSVariableESP32Base::begin();
}
void loop() {
    j.run();
    if (isServer()) { 
        server.run();
        float sleepSec = server.getSleepRequest();
        if (j.secTick(10)) { 
            OUT("seen %d sleep req %.1f", server.countSeen(), sleepSec);
        }
        if (sleepSec > 0) {
            server.prepareSleep(sleepSec); 
            deepSleep(sleepSec * 1000);
        }
    } else { 
        client1.run();
    }
    delay(1);
}

#ifdef CSIM
RemoteSensorClient client2, client3;

class Csim : public ESP32sim_Module {
    string dummy;
    public:
    Csim() {
        ESPNOW_sendHandler = new ESPNOW_csimOneProg();
        csim_flags.OneProg = true;
    }
    void parseArg(char **&a, char **la) override {
        if (strcmp(*a, "--dummy") == 0) dummy = *(++a);
    }
    void setup() override {
        SPIFFSVariableESP32Base::begin(); // hush up artificial warnings about early access
        client1.csimOverrideMac("MAC1");
        client2.csimOverrideMac("MAC2");
        client3.csimOverrideMac("MAC3");
        csim_onDeepSleep([](uint64_t us) {
            client1.prepareSleep(us / 1000);
            client2.prepareSleep(us / 1000);
            client3.prepareSleep(us / 1000);
        });
    }
    void loop() override {
        client1.run();
        server.run();
	client2.run();
	server.run();
        client3.run();
	server.run();
    }

} csim;
#endif
