#include "ring_buffer.h"

static size_t RingBuffer_Next(const ring_buffer_t *rb, size_t value)
{
    value++;
    return (value >= rb->size) ? 0u : value;
}

void RingBuffer_Init(ring_buffer_t *rb, uint8_t *storage, size_t size)
{
    rb->data = storage;
    rb->size = size;
    rb->head = 0u;
    rb->tail = 0u;
}

size_t RingBuffer_Write(ring_buffer_t *rb, const uint8_t *data, size_t len)
{
    size_t written = 0u;

    while ((written < len) && (RingBuffer_Free(rb) > 0u)) {
        rb->data[rb->head] = data[written++];
        rb->head = RingBuffer_Next(rb, rb->head);
    }

    return written;
}

size_t RingBuffer_Read(ring_buffer_t *rb, uint8_t *data, size_t len)
{
    size_t read = 0u;

    while ((read < len) && !RingBuffer_IsEmpty(rb)) {
        data[read++] = rb->data[rb->tail];
        rb->tail = RingBuffer_Next(rb, rb->tail);
    }

    return read;
}

size_t RingBuffer_PeekLinear(const ring_buffer_t *rb, const uint8_t **data)
{
    size_t head = rb->head;
    size_t tail = rb->tail;

    if (head == tail) {
        *data = 0;
        return 0u;
    }

    *data = &rb->data[tail];
    return (head > tail) ? (head - tail) : (rb->size - tail);
}

void RingBuffer_Drop(ring_buffer_t *rb, size_t len)
{
    while ((len-- > 0u) && !RingBuffer_IsEmpty(rb)) {
        rb->tail = RingBuffer_Next(rb, rb->tail);
    }
}

size_t RingBuffer_Available(const ring_buffer_t *rb)
{
    size_t head = rb->head;
    size_t tail = rb->tail;

    return (head >= tail) ? (head - tail) : (rb->size - tail + head);
}

size_t RingBuffer_Free(const ring_buffer_t *rb)
{
    return (rb->size - 1u) - RingBuffer_Available(rb);
}

bool RingBuffer_IsEmpty(const ring_buffer_t *rb)
{
    return rb->head == rb->tail;
}
