#include "Streaming.h"
#include "Zforce.h"
#include <Arduino.h>

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
const sensor_reg_t reg_R_Bootcomplete = 0x08;
const sensor_reg_t reg_RW_Enable = 0x11;
const sensor_reg_t reg_RW_Frequency = 0x13;
const sensor_reg_t reg_RW_Area = 0x14;
const sensor_reg_t reg_R_Touch = 0x22;

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
void printTouchMessage();
} // namespace SensorHelper

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
        return false;
    }

    while (!isDataReady())
        ;
    Message *msg = zforce.GetMessage();
    zforce.DestroyMessage(msg);
    return true;
}

sensor_val_t SensorHelper::readReg(sensor_reg_t addr)
{
    sensor_val_t val = 0;

    switch (addr)
    {
    case reg_R_Status:
    case reg_RW_Enable:
    case reg_R_Touch:
    case reg_RW_Frequency:
    case reg_RW_Area:
        val = regs[addr];
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
    if (isDataReady())
    {
        Message *msg = zforce.GetMessage();
        Serial << (int)msg->type << endl;
        zforce.DestroyMessage(msg);
    }
    return true;
}

void SensorHelper::config(uint8_t index)
{
    detachInterrupt(digitalPinToInterrupt(PIN_NN_DR));
    writeReg(reg_RW_Enable, true);
    attachInterrupt(digitalPinToInterrupt(PIN_NN_DR), dataReadyISR, RISING);
}

uint8_t SensorHelper::updateTouch()
{
    if (newTouchDataFlag == false)
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

void SensorHelper::printTouchMessage()
{
    for (size_t i = 0; i < nTouches; i++)
    {
        Serial << "(" << touches[i].id << ")\t[" << touches[i].x << ", " << touches[i].y << "]\t(" << touches[i].event << ")\n";
    }
    Serial << endl;
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
    if (result > 0)
    {
        SensorHelper::printTouchMessage();
    }
}