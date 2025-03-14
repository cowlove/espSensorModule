#include "jimlib.h"
#include "sensorNetworkEspNOW.h"

JStuff j;
CLI_VARIABLE_FLOAT(x, 800);

void setup() {
    WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); //disable brownout 
    j.begin();
}

void loop() {
    j.run();
    delay(1);
}
