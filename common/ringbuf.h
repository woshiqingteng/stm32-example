/**
 * @file    ringbuf.h
 * @brief   Single-producer/single-consumer byte ring buffer (header-only).
 *
 * One slot is reserved to tell full from empty, so the usable capacity is
 * @c size - 1. The struct is deliberately transparent: a producer that fills
 * the backing store out of band (e.g. a circular DMA) can walk
 * @c buf[head] directly. A zero-size (or null buffer) ring is inert.
 */

#ifndef COMMON_RINGBUF_H
#define COMMON_RINGBUF_H

#include <stdint.h>
#include <stdbool.h>

typedef struct
{
    uint8_t          *buf;
    uint16_t          size;   /* capacity; usable = size - 1 */
    volatile uint16_t head;   /* produce index */
    volatile uint16_t tail;   /* consume index */
} ringbuf_t;

/** @brief Bind a ring to a caller-owned buffer. */
static inline void ringbuf_init(ringbuf_t *rb, uint8_t *buf, uint16_t size)
{
    rb->buf  = buf;
    rb->size = size;
    rb->head = 0U;
    rb->tail = 0U;
}

/** @brief True when no bytes are buffered. */
static inline bool ringbuf_empty(const ringbuf_t *rb)
{
    return rb->head == rb->tail;
}

/** @brief True when the ring cannot accept another byte. */
static inline bool ringbuf_full(const ringbuf_t *rb)
{
    if (rb->size == 0U)
    {
        return true;
    }
    return (uint16_t)((rb->head + 1U) % rb->size) == rb->tail;
}

/** @brief Number of buffered bytes. */
static inline uint16_t ringbuf_count(const ringbuf_t *rb)
{
    if (rb->size == 0U)
    {
        return 0U;
    }
    return (uint16_t)((rb->head + rb->size - rb->tail) % rb->size);
}

/** @brief Push one byte; false when full (or the ring has no storage). */
static inline bool ringbuf_put(ringbuf_t *rb, uint8_t byte)
{
    uint16_t next;

    if ((rb->buf == 0) || (rb->size == 0U))
    {
        return false;
    }
    next = (uint16_t)((rb->head + 1U) % rb->size);
    if (next == rb->tail)
    {
        return false;
    }
    rb->buf[rb->head] = byte;
    rb->head = next;
    return true;
}

/** @brief Pop one byte; false when empty. */
static inline bool ringbuf_get(ringbuf_t *rb, uint8_t *byte)
{
    if ((rb->buf == 0) || (rb->tail == rb->head))
    {
        return false;
    }
    *byte = rb->buf[rb->tail];
    rb->tail = (uint16_t)((rb->tail + 1U) % rb->size);
    return true;
}

#endif /* COMMON_RINGBUF_H */
