#include <Arduino.h>
#include "Streaming.h"
#include "Zforce.h"

typedef uint8_t sensor_reg_t;
typedef int32_t sensor_val_t;
typedef struct SensorReg
{
    sensor_reg_t addr;
    sensor_val_t val;
} SensorReg_t;

namespace SensorHelper
{
const sensor_reg_t reg_R_Status = 0x00;
const sensor_reg_t reg_RW_Enable = 0x01;
const sensor_reg_t reg_R_Touch = 0x02;
const sensor_reg_t reg_RW_Frequency = 0x03;
const sensor_reg_t reg_RW_Area = 0x04;
const sensor_reg_t reg_R_Bootcomplete = 0x08;

#define MAX_REGS 20
sensor_val_t regs[MAX_REGS];

#define TOUCH_BUFFER_SIZE 4
TouchData touches[TOUCH_BUFFER_SIZE];
uint8_t nTouches = 0;

volatile bool newTouchDataFlag = false;
void dataReadyISR() { newTouchDataFlag = true; }
bool isDataReady() { return digitalRead(PIN_NN_DR) == HIGH; }
bool begin();
void config(uint8_t index = 0);
uint8_t updateTouch();
bool writeReg(sensor_reg_t addr, sensor_val_t val);
bool writeReg(SensorReg reg) { return writeReg(reg.addr, reg.val); }
sensor_val_t readReg(sensor_reg_t addr);
bool readReg(SensorReg_t &reg);
}

bool SensorHelper::writeReg(sensor_reg_t addr, sensor_val_t val)
{
    switch (addr)
    {
    case reg_RW_Enable:
        zforce.Enable(val);
        regs[reg_RW_Enable] = val;
        break;
    case reg_RW_Frequency:
        break;
    case reg_RW_Area:
        break;

    default:
        break;
    }
}

sensor_val_t SensorHelper::readReg(sensor_reg_t addr)
{
    sensor_val_t val;

    switch (addr)
    {
    case reg_R_Status:
        break;
    case reg_RW_Enable:
        val = regs[addr];
        break;
    case reg_R_Touch:
        break;
    case reg_RW_Frequency:
        break;
    case reg_RW_Area:
        break;

    default:
        break;
    }
    return val;
}

bool SensorHelper::begin()
{
    zforce.Start(PIN_NN_DR);
    pinMode(PIN_NN_DR, INPUT_PULLDOWN);
    attachInterrupt(digitalPinToInterrupt(PIN_NN_DR), dataReadyISR, RISING);
    Message *msg = zforce.GetMessage();
}

void SensorHelper::config(uint8_t index)
{
}

uint8_t SensorHelper::updateTouch()
{
    if(newTouchDataFlag == false)
    {
        return 0;
    }

    newTouchDataFlag = false;
    Message *touch = zforce.GetMessage();
    if (touch != NULL)
    {
        if (touch->type == MessageType::TOUCHTYPE)
        {
            auto size = ((TouchMessage *)touch)->touchCount;
            size = size >= TOUCH_BUFFER_SIZE ? TOUCH_BUFFER_SIZE : size;
            nTouches = size;
            for (uint8_t i = 0; i < size; i++)
            {
                memcpy(touches + i, &((TouchMessage *)touch)->touchData[i], sizeof(TouchData));
            }
            zforce.DestroyMessage(touch);
            return nTouches;
        }
        zforce.DestroyMessage(touch);
    }
    return -1;
}

void setup()
{
    Serial.begin(115200);
    while (!Serial)
        ;
    SensorHelper::begin();
    SensorHelper::config();
    Serial.println("zforce start");
}

void loop() 
{
    auto result = SensorHelper::updateTouch();
}