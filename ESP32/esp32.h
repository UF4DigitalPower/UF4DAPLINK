#ifndef ESP32_H
#define ESP32_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

void ESP32_Init(void);
void ESP32_Reset(void);
void ESP32_EnterBootloader(void);
void ESP32_ExitBootloader(void);
void ESP32_SetEN(bool level);
void ESP32_SetIO0(bool level);
void ESP32_Task(void);

#ifdef __cplusplus
}
#endif

#endif /* ESP32_H */
