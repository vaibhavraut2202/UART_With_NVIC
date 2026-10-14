#include "Uart_Learning.h"

CyclicBuffer rxBuffer = { .head = 0, .tail = 0 };
CyclicBuffer txBuffer = { .head = 0, .tail = 0 };

void CyclicBuffer_Write(CyclicBuffer *buf, uint8_t data) {
    uint16_t next = (buf->head + 1) % BUFFER_SIZE;
    if (next != buf->tail) 
			{
        buf->buffer[buf->head] = data;
        buf->head = next;
      } 
		else 
			{
        /* Handle buffer overrun */
      }
}

uint8_t CyclicBuffer_Read(CyclicBuffer *buf) {
    if (buf->head == buf->tail) 
			{
        return 0; /* Buffer empty */
      }
    uint8_t data = buf->buffer[buf->tail];
    buf->tail = (buf->tail + 1) % BUFFER_SIZE;
    return data;
}
