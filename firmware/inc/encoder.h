#ifndef ENCODER_H_
#define ENCODER_H_


typedef enum {
    ENCODER_DIR_CW = 0,
    ENCODER_DIR_CCW,
} encoder_direction_t;


/**
 * @brief Pin event handler prototype.
 *
 * @param[in] pin    Pin that triggered this event.
 * @param[in] action Action that led to triggering this event.
 */
typedef void (*enc_evt_handler_t)(encoder_direction_t dir);

void encoder_init(enc_evt_handler_t evt_handler);


#endif