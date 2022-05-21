#ifndef CUSTOM_BOARD_H_
#define CUSTOM_BOARD_H_

#ifdef __cplusplus
extern "C" {
#endif


#define LEDS_NUMBER    2

#define LED_1          7
#define LED_2          14
#define LED_START      LED_1
#define LED_STOP       LED_2

#define LEDS_ACTIVE_STATE 1

#define LEDS_LIST { LED_1, LED_2 }

#define LEDS_INV_MASK  LEDS_MASK

#define BSP_LED_0      7
#define BSP_LED_1      7

#define BUTTONS_NUMBER 2

#define BUTTON_1       13
#define BUTTON_2       21
#define BUTTON_PULL    3

#define BUTTONS_ACTIVE_STATE 0

#define BUTTONS_LIST { BUTTON_1, BUTTON_2 }

#define BSP_BUTTON_0        BUTTON_1
#define BSP_BUTTON_1        BUTTON_2
#define BSP_BUTTON_BOARD    0
#define BSP_BUTTON_ENC      1

#define BSP_ENC_PIN_A   22
#define BSP_ENC_PIN_B   23

#ifdef __cplusplus
}
#endif

#endif // CUSTOM_BOARD_H_
