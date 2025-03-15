#include "jimlib.h"
JStuff j;
#include "sensorNetworkEspNOW.h"

CLI_VARIABLE_FLOAT(x, 800);

class RemoteSensorModuleDHT : public RemoteSensorArray {
public:
    RemoteSensorModuleDHT(const char *mac) : RemoteSensorArray(mac) {}
    SensorDHT tempC = SensorDHT(this, "TEMP", 11);
    SensorADC battery = SensorADC(this, "BATT", 33, .5023);
    SensorOutput led = SensorOutput(this, "LED", 14, 0);
    SensorVariable v = SensorVariable(this, "RETRY", "X10");
//} ambientTempSensor1("EC64C9986F2C");
} ambientTempSensor1("auto");
      
// also maybe config by SCHEMA like
RemoteSensorArray ambientTempSensor2("FFAACCEE01", "RESULT=DHT11 BAT=ADC13*.00023 TEMP=DHT11 LED=OUTPUT13,0 MILLIS=MILLIS");
SensorWrapper ambientTemp(&ambientTempSensor2, "RESULT");
SensorWrapper ambientTempBattery(&ambientTempSensor2, "BAT");

float volts1 = ambientTempSensor2.read("BAT");
float volts2 = ambientTempBattery.read();
float volts3 = ambientTempSensor1.tempC.asFloat();

RemoteSensorServer server(6, { &ambientTempSensor1, &ambientTempSensor2});
        
RemoteSensorClient client1;

void setup() {
    //WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); //disable brownout 
    if (getMacAddress() == "EC64C9986F2C") { 
        j.mqtt.active = j.jw.enabled = false;
    }
    j.begin();    
    j.run();
}

void loop() {
    j.run();
    if (getMacAddress() == "EC64C9986F2C") { 
        client1.run();
    } else { 
        server.run();
        ambientTempSensor1.v.result = sfmt("X%dY", millis() / 1000);
        if (ambientTempSensor1.updated()) {
            OUT("results: %f %f", ambientTempSensor1.tempC.asFloat(), ambientTempSensor1.battery.asFloat());
        }
    }
    delay(1);
}
