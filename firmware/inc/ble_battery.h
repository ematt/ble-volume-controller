#ifndef BLE_BATTERY_H_
#define BLE_BATTERY_H_

#include "battery.h"

void ble_battery_init(void);
void ble_battery_bas_init(void);
void ble_battery_bas_update(battery_status_t state);

#endif // BLE_BATTERY_H_