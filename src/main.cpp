#include "SensorHelper.h"

void setup()
{
    Serial.begin(115200);
    while (!Serial)
        ;
    SensorHelper::begin();
    SensorHelper::config();
}

void loop()
{
    auto result = SensorHelper::updateTouch();
    if (result > 0)
    {
        SensorHelper::printTouchMessage();

        if (SensorHelper::readReg(SensorHelper::reg_R_Touch + 3) == 2)
        {
            SensorHelper::printRegs();
        }
    }
}