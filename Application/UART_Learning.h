#ifndef UART_LEARNING_H
#define UART_LEARNING_H

#include "main.h"

#define BUFFER_SIZE 1024

typedef struct {
    uint8_t buffer[BUFFER_SIZE];
    volatile uint16_t head;
    volatile uint16_t tail;
} CyclicBuffer;

void CyclicBuffer_Write(CyclicBuffer *buf, uint8_t data);
uint8_t CyclicBuffer_Read(CyclicBuffer *buf);

extern CyclicBuffer rxBuffer;
extern CyclicBuffer txBuffer;

#endif /* UART_LEARNING_H */
