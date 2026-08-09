#ifndef CDC_BRIDGE_H
#define CDC_BRIDGE_H

#ifdef __cplusplus
extern "C" {
#endif

/** Initialize USB CDC bridge state. */
void CDC_Bridge_Init(void);

/** Non-blocking pump between TinyUSB CDC instances and UART manager rings. */
void CDC_Task(void);

#ifdef __cplusplus
}
#endif

#endif /* CDC_BRIDGE_H */
