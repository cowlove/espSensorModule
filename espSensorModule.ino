#include "jimlib.h"
JStuff j;
#include "sensorNetworkEspNOW.h"

CLI_VARIABLE_FLOAT(x, 800);

class RemoteSensorModuleDHT : public RemoteSensorModule {
public:
    RemoteSensorModuleDHT(const char *mac) : RemoteSensorModule(mac) {}
    SensorDHT tempC = SensorDHT(this, "TEMP", 11);
    SensorADC battery = SensorADC(this, "BATT", 33, .5023);
    SensorOutput led = SensorOutput(this, "LIGHTX", 22, 0);
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

RemoteSensorServer server(6, { &ambientTempSensor1, &at2, &at3 });
        
RemoteSensorClient client1, client2, client3;

void setup() {
    //WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); //disable brownout 
    if (getMacAddress() != "D48AFCA4AF20") { 
        j.mqtt.active = j.jw.enabled = false;
    }
    j.begin();    
    j.run();
#if 0 
    for (int i = 0; i < 2; i++) { 
        OUT("toggle");
        pinMode(22, OUTPUT);
        digitalWrite(22, 0);
        delay(1000);
        digitalWrite(22, 1);
        delay(1000);
    }
#endif
#ifdef CSIM
    client1.csimOverrideMac("MAC1");
    client2.csimOverrideMac("MAC2");
    client3.csimOverrideMac("MAC3");
#endif

}

void loop() {
    j.run();
#ifdef CSIM
    client1.run();
    client2.run();
    client3.run();
    server.run();
#endif

if (getMacAddress() == "D48AFCA4AF20") { 
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
