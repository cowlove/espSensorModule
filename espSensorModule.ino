#include "jimlib.h"
JStuff j;
#include "sensorNetworkEspNOW.h"
#include "serialLog.h"

CLI_VARIABLE_FLOAT(x, 800);

class RemoteSensorModuleDHT : public RemoteSensorModule {
public:
    RemoteSensorModuleDHT(const char *mac) : RemoteSensorModule(mac) {}
    SensorDHT tempC = SensorDHT(this, "TEMP", 26);
    SensorOutput gnd = SensorOutput(this, "GND", 27, 0);
    SensorOutput vcc = SensorOutput(this, "VCC", 25, 1);
    SensorOutput gnd2 = SensorOutput(this, "GND2", 22, 0);
    SensorOutput vcc2 = SensorOutput(this, "VCC2", 19, 1);
    SensorADC battery = SensorADC(this, "LIPOBATTERY", 33, .0017);
    //SensorOutput led = SensorOutput(this, "LED", 22, 0);
    SensorVariable v = SensorVariable(this, "RETRY", "X10");
    SensorMillis m = SensorMillis(this);
    SensorHX711 grams = SensorHX711(this, "GRAMS", 23, 18);
    ////} ambientTempSensor1("EC64C9986F2C");
} at1("auto"), at2("auto"), at3("auto");


// TODO - these server-side instances also call SensorOutput::setValue and write pins, etc

// also maybe config by SCHEMA like
//RemoteSensorModule ambientTempSensor2("FFAACCEE01", "RESULT=DHT11 BAT=ADC13*.00023 TEMP=DHT11 LED=OUTPUT22,1 MILLIS=MILLIS");
//SensorWrapper ambientTemp(&ambientTempSensor2, "RESULT");
//SensorWrapper ambientTempBattery(&ambientTempSensor2, "BAT");

//float volts1 = ambientTempSensor2.read("BAT");
//float volts2 = ambientTempBattery.read();
//float volts3 = ambientTempSensor1.tempC.asFloat();

RemoteSensorServer server({ &at1, &at2, &at3 });
          
RemoteSensorClient client1;
SPIFFSVariable<vector<string>> logFile("/logFile", {}); 

bool isServer() { 
    return getMacAddress() == "0CB815C2412C" || getMacAddress() == "083AF2B59110" 
        || getMacAddress() == "MAC=CCBA9716E0D8" || getMacAddress() == "FFEEDDAABBCC";
}

void setup() {
    //WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); //disable brownout 
    j.jw.enabled = false;
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
Csim_RemoteSensorClientContext client2(0xffff1), client3(0xffff2);

class SketchCsim : public Csim_Module {
    CsimHx711 hx = CsimHx711(23, 18);
public:
    SketchCsim() {
        defaultContext.espnow = new ESPNOW_csimOneProg();
    }
    void setup() override {}
    void loop() override {
        hx.setResult(3333);
    }
} csim;
#endif
