#ifndef LED_H
#define LED_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    LED_OFF = 0,
    LED_USB_CONNECTED,
    LED_DAP_CONNECTED,
    LED_PROGRAMMING,
    LED_ERROR
} led_state_t;

/** Initialize the status LED pin from board_config.h. */
void LED_Init(void);

void LED_On(void);
void LED_Off(void);
void LED_Toggle(void);
void LED_SetState(led_state_t state);
void LED_Task(void);

#ifdef __cplusplus
}
#endif

#endif /* LED_H */
