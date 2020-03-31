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
const sensor_reg_t reg_R_Bootcomplete = 0x01;
const sensor_reg_t reg_RW_Enable = 0x02;
const sensor_reg_t reg_RW_Frequency = 0x03;
const sensor_reg_t reg_RW_Area = 0x06;
const sensor_reg_t reg_R_Touch = 0x0A; // from 0x0A to 0x1A

#define TOUCH_BUFFER_SIZE 4 // must not exeed 4
uint8_t nTouches = 0;
#define MAX_REGS (reg_R_Touch + TOUCH_BUFFER_SIZE * 4)
sensor_val_t regs[MAX_REGS];


volatile bool newTouchDataFlag = false;
void dataReadyISR() { newTouchDataFlag = true; }
bool isDataReady() { return digitalRead(PIN_NN_DR) == HIGH; }
bool begin();
void config(uint8_t index = 0);
uint8_t updateTouch();
bool writeReg(sensor_reg_t addr, sensor_val_t val, bool sendToZforce = true);
bool writeReg(SensorReg reg) { return writeReg(reg.addr, reg.val); }
sensor_val_t readReg(sensor_reg_t addr);
bool readReg(SensorReg_t &reg);
bool sendAndGetFromZforce(sensor_reg_t addr);
void mapTouchdataToRegs(TouchData *touch, uint8_t index);
void getTouchdataFromRegs(TouchData &touch, uint8_t index);
void printTouchMessage();
void printOneReg(sensor_reg_t addr) { Serial << "regs[" << addr << "] = " << _HEX(regs[addr]) << endl; }
void printRegs();
} // namespace SensorHelper

bool SensorHelper::writeReg(sensor_reg_t addr, sensor_val_t val, bool sendToZforce)
{
    if (addr >= MAX_REGS)
        return false;
    regs[addr] = val;

    if(sendToZforce)
        return sendAndGetFromZforce(addr);
    else
        return true;
}

bool SensorHelper::sendAndGetFromZforce(sensor_reg_t addr)
{
    if (addr == reg_RW_Enable)
        zforce.Enable(regs[addr]);
    else if (addr == reg_RW_Frequency)
    {
        /* code */
    }
    else if (addr == reg_RW_Area)
    {
        zforce.TouchActiveArea(regs[addr], regs[addr + 1],
                               regs[addr + 2], regs[addr + 3]);
    }

    while (!isDataReady())
        ;
    Message *msg = zforce.GetMessage();
    if(msg == NULL)
        return false;
    zforce.DestroyMessage(msg);
    return true;
}

sensor_val_t SensorHelper::readReg(sensor_reg_t addr)
{
    if (addr >= MAX_REGS)
        return 0;

    return regs[addr];
}

bool SensorHelper::begin()
{
    zforce.Start(PIN_NN_DR);
    pinMode(PIN_NN_DR, INPUT_PULLDOWN);
    if (isDataReady())
    {
        Message *msg = zforce.GetMessage();
        if (msg->type == MessageType::BOOTCOMPLETETYPE)
            Serial << "Sensor connected" << endl;
        else
            Serial << "Unexpected senosr message" << endl;
        zforce.DestroyMessage(msg);
    }
    return true;
}

void SensorHelper::config(uint8_t index)
{
    detachInterrupt(digitalPinToInterrupt(PIN_NN_DR));
    writeReg(reg_RW_Enable, true);
    // writeReg(reg_RW_Area + 3, 100, false);
    // writeReg(reg_RW_Area + 2, 100, false);
    // writeReg(reg_RW_Area + 1, 0, false);
    // writeReg(reg_RW_Area, 0);
    attachInterrupt(digitalPinToInterrupt(PIN_NN_DR), dataReadyISR, RISING);
}

void SensorHelper::mapTouchdataToRegs(TouchData *touch, uint8_t index)
{
    auto i = index;
    regs[reg_R_Touch + i * 4 + 0] = touch->x;
    regs[reg_R_Touch + i * 4 + 1] = touch->y;
    regs[reg_R_Touch + i * 4 + 2] = touch->id;
    regs[reg_R_Touch + i * 4 + 3] = touch->event;
}

void SensorHelper::getTouchdataFromRegs(TouchData &touch, uint8_t index)
{
    auto i = index;
    touch.x = regs[reg_R_Touch + i * 4 + 0];
    touch.y = regs[reg_R_Touch + i * 4 + 1];
    touch.id = regs[reg_R_Touch + i * 4 + 2];
    touch.event = (TouchEvent)regs[reg_R_Touch + i * 4 + 3];
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
            nTouches = size >= TOUCH_BUFFER_SIZE ? TOUCH_BUFFER_SIZE : size;
            for (uint8_t i = 0; i < nTouches; i++)
            {
                mapTouchdataToRegs(&((TouchMessage *)touch)->touchData[i], i);
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
    TouchData touch;
    for (size_t i = 0; i < nTouches; i++)
    {
        getTouchdataFromRegs(touch, i);
        Serial << "(" << i << "/" << nTouches << ")\t["
               << touch.x << ", " << touch.y << "]\t("
               << touch.event << "/" << touch.id << ")\n";
    }
    Serial << endl;
}

void SensorHelper::printRegs()
{
    for (size_t i = 0; i < MAX_REGS; i++)
        printOneReg(i);
}

void setup()
{
    Serial.begin(115200);
    while (!Serial)
        ;
    SensorHelper::begin();
    SensorHelper::config();
    SensorHelper::printRegs();
    Serial.println("Sensor configured\n");
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