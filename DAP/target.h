#ifndef TARGET_H
#define TARGET_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    TARGET_G474 = 1,
    TARGET_F429 = 2
} target_t;

/** Initialize target routing with F429 selected and SWD idle. */
void Target_Init(void);

/** Switch to the selected target through the Target Manager safe sequence. */
bool Target_Select(target_t target);

/** Return the currently selected target. */
target_t Target_Get(void);

/** Assert and release the selected target reset using non-routing GPIO only. */
void Target_Reset(void);

/** Reset each routed target once during probe startup, then select F429. */
void Target_ResetAllAtStartup(void);

/** Detect whether the selected target has a valid VTREF. */
bool Target_Detect(void);

/** Read the selected target SW-DP IDCODE. */
uint32_t Target_ReadIDCODE(void);

/** Non-blocking target service hook for future SWO/reset sequencing. */
void Target_Task(void);

#ifdef __cplusplus
}
#endif

#endif /* TARGET_H */
