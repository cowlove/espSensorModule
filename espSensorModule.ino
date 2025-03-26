#include "jimlib.h"
JStuff j;
#include "sensorNetworkEspNOW.h"

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
        
RemoteSensorClient client1, client2, client3;
SPIFFSVariable<vector<string>> logFile("/logFile", {}); 

bool isServer() { 
    return getMacAddress() == "D48AFCA4AF20" || getMacAddress() == "083AF2B59110" 
        || getMacAddress() == "E4B063417040";
}
void setup() {
    //WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); //disable brownout 
    if (isServer() == false) { 
        j.jw.enabled = false;
    }
    j.begin();    
    j.run();

#ifdef CSIM
    if (csim_flags.OneProg) { 
        client1.csimOverrideMac("MAC1");
        client2.csimOverrideMac("MAC2");
        client3.csimOverrideMac("MAC3");
    }
#endif
}

void loop() {
    j.run();
#ifdef CSIM
    if (csim_flags.OneProg) { 
        client1.run();
        client2.run();
        client3.run();
        server.run();
    }
#endif

    if (isServer()) { 
        server.run();
//        ambientTempSensor1.v.result = sfmt("X%dY", millis() / 1000);
//        if (ambientTempSensor1.updated()) {
//            OUT("results: %f %f", ambientTempSensor1.tempC.asFloat(), ambientTempSensor1.battery.asFloat());
//        }
    } else { 
        client1.run();
    }
    delay(1);
}
