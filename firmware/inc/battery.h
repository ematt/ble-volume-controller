#ifndef _BATTERY_H_
#define _BATTERY_H_

enum BATTERY_STATUS_STATE {
    BATTERY_STATUS_STATE_CHARGING = 0,
    BATTERY_STATUS_STATE_DISCHARGING,
    BATTERY_STATUS_STATE_WARNING,
    BATTERY_STATUS_STATE_DEPLETED,
};

typedef struct {
    enum BATTERY_STATUS_STATE state;
    uint8_t proc;
    uint16_t mv;
} battery_status_t;

/**
 * @brief Pin event handler prototype.
 *
 * @param[in] pin    Pin that triggered this event.
 * @param[in] action Action that led to triggering this event.
 */
typedef void (*battery_evt_handler_t)(battery_status_t state);

void battery_init(battery_evt_handler_t evt_handler);

void battery_run(void);


#endif // _BATTERY_H_