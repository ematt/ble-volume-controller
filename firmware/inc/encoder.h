#ifndef ENCODER_H_
#define ENCODER_H_

#include <stdint.h>

typedef enum {
    ENCODER_DIR_CW = 0,
    ENCODER_DIR_CCW,
} encoder_direction_t;


typedef struct {
    encoder_direction_t direction;
    uint8_t steps;
} encoder_event_t;

/**
 * @brief Pin event handler prototype.
 *
 * @param[in] pin    Pin that triggered this event.
 * @param[in] action Action that led to triggering this event.
 */
typedef void (*enc_evt_handler_t)(encoder_event_t dir);

int encoder_init(enc_evt_handler_t evt_handler);


#endif