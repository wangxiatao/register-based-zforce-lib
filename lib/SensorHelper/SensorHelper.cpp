#include "SensorHelper.h"
#include "Streaming.h"

namespace SensorHelper
{
#define TOUCH_BUFFER_SIZE 2 // must not exeed 4
uint8_t nTouches = 0;
#define MAX_REGS (reg_R_Touch + TOUCH_BUFFER_SIZE * 4)
sensor_val_t regs[MAX_REGS];
volatile bool newTouchDataFlag = false;

void dataReadyISR() { newTouchDataFlag = true; }

bool isDataReady() { return digitalRead(PIN_NN_DR) == HIGH; }

bool writeReg(sensor_reg_t addr, sensor_val_t val, bool sendToZforce)
{
    if (addr >= MAX_REGS)
        return false;
    regs[addr] = val;

    if (sendToZforce)
        return sendAndGetFromZforce(addr);
    else
        return true;
}

bool sendAndGetFromZforce(sensor_reg_t addr)
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
    if (msg == NULL)
        return false;
    zforce.DestroyMessage(msg);
    return true;
}

sensor_val_t readReg(sensor_reg_t addr)
{
    if (addr >= MAX_REGS)
        return 0;

    return regs[addr];
}

bool begin()
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

void config(uint8_t index)
{
    detachInterrupt(digitalPinToInterrupt(PIN_NN_DR));
    writeReg(reg_RW_Enable, true);
    attachInterrupt(digitalPinToInterrupt(PIN_NN_DR), dataReadyISR, RISING);
    SensorHelper::printRegs();
    Serial << "Sensor configured" << endl << endl;
}

void mapTouchdataToRegs(TouchData *touch, uint8_t index)
{
    auto i = index;
    regs[reg_R_Touch + i * 4 + 0] = touch->x;
    regs[reg_R_Touch + i * 4 + 1] = touch->y;
    regs[reg_R_Touch + i * 4 + 2] = touch->id;
    regs[reg_R_Touch + i * 4 + 3] = touch->event;
}

void getTouchdataFromRegs(TouchData &touch, uint8_t index)
{
    auto i = index;
    touch.x = regs[reg_R_Touch + i * 4 + 0];
    touch.y = regs[reg_R_Touch + i * 4 + 1];
    touch.id = regs[reg_R_Touch + i * 4 + 2];
    touch.event = (TouchEvent)regs[reg_R_Touch + i * 4 + 3];
}

uint8_t updateTouch()
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

void printTouchMessage()
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

void printOneReg(sensor_reg_t addr)
{
    Serial << "regs[" << addr << "] = " << _HEX(regs[addr]) << endl;
}

void printRegs()
{
    for (size_t i = 0; i < MAX_REGS; i++)
        printOneReg(i);
}

SensorReg_t decode(String input)
{
    int commaIndexes[4];
    String params[4];
    size_t nParams = 4;
    SensorReg_t reg = {0, 0};

    for (size_t i = 0; i < 4; i++)
    {
        if (i == 0)
        {
            commaIndexes[i] = input.indexOf(',');
            params[i] = input.substring(0, commaIndexes[i]);
            if (commaIndexes[i] == -1)
            {
                nParams = -1;
                break;
            }
        }
        else
        {
            commaIndexes[i] = input.indexOf(',', commaIndexes[i - 1] + 1);
            params[i] = input.substring(commaIndexes[i - 1] + 1, commaIndexes[i]);
            if (commaIndexes[i] == -1)
            {
                nParams = i + 1;
                break;
            }
        }
    }

    if (nParams < 2)
    {
        Serial.println("# of parameters must be more than 1");
    }
    else
    {
        reg.addr = params[0].toInt();
        bool readOrWrite = params[1] == "R" ? 0 : 1;
        if (nParams > 2 && readOrWrite == 1)
        {
            reg.val = params[2].toInt();
            if (reg.addr < MAX_REGS)
                regs[reg.addr] = reg.val;
        }
        else
        {
            if(reg.addr < MAX_REGS)
                reg.val = regs[reg.addr];
        }
        Serial << reg.addr << ", " << _HEX(reg.val) << endl;
    }
    return reg;
}

String encode(sensor_reg_t *regs, uint8_t length)
{

}

}; // namespace SensorHelper
