/*---------------------------------------------------------------
 * Teaching module overview: Board support
 * This file groups the axp2101 responsibilities so learners can
 * follow the subsystem boundary before reading individual routines.
 *--------------------------------------------------------------*/

#ifndef __AXP2101_H__
#define __AXP2101_H__

#include "i2c_device.h"

class Axp2101 : public I2cDevice {
public:
    Axp2101(i2c_master_bus_handle_t i2c_bus, uint8_t addr);
    bool IsCharging();
    bool IsDischarging();
    bool IsChargingDone();
    int GetBatteryLevel();
    float GetTemperature();
    void PowerOff();

private:
    int GetBatteryCurrentDirection();
};

#endif
