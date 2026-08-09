#ifndef RING_BUFFER_H
#define RING_BUFFER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    uint8_t *data;
    size_t size;
    volatile size_t head;
    volatile size_t tail;
} ring_buffer_t;

void RingBuffer_Init(ring_buffer_t *rb, uint8_t *storage, size_t size);
size_t RingBuffer_Write(ring_buffer_t *rb, const uint8_t *data, size_t len);
size_t RingBuffer_Read(ring_buffer_t *rb, uint8_t *data, size_t len);
size_t RingBuffer_PeekLinear(const ring_buffer_t *rb, const uint8_t **data);
void RingBuffer_Drop(ring_buffer_t *rb, size_t len);
size_t RingBuffer_Available(const ring_buffer_t *rb);
size_t RingBuffer_Free(const ring_buffer_t *rb);
bool RingBuffer_IsEmpty(const ring_buffer_t *rb);

#ifdef __cplusplus
}
#endif

#endif /* RING_BUFFER_H */
