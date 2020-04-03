#include <Arduino.h>
#include "Zforce.h"
#pragma once

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

void dataReadyISR();
bool isDataReady();
bool begin();
void config(uint8_t index = 0);
uint8_t updateTouch();
bool writeReg(sensor_reg_t addr, sensor_val_t val, bool sendToZforce = true);
// bool writeReg(SensorReg reg) { return writeReg(reg.addr, reg.val); }
sensor_val_t readReg(sensor_reg_t addr);
bool readReg(SensorReg_t &reg);
bool sendAndGetFromZforce(sensor_reg_t addr);
void mapTouchdataToRegs(TouchData *touch, uint8_t index);
void getTouchdataFromRegs(TouchData &touch, uint8_t index);
void printTouchMessage();
void printOneReg(sensor_reg_t addr);
void printRegs();

SensorReg_t decode(String str);
String encode(sensor_reg_t *regs, uint8_t length);
}; // namespace SensorHelper
