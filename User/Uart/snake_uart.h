#ifndef __SNAKE_UART_H_
#define __SNAKE_UART_H_

#include "stm32f10x.h"

#define SNAKE_UART_BAUD 115200

typedef enum {
    SNAKE_UART_CMD_NONE = 0,
    SNAKE_UART_CMD_UP,
    SNAKE_UART_CMD_DOWN,
    SNAKE_UART_CMD_LEFT,
    SNAKE_UART_CMD_RIGHT,
    SNAKE_UART_CMD_PAUSE,
    SNAKE_UART_CMD_RESET,
    SNAKE_UART_CMD_LEVEL,
    SNAKE_UART_CMD_START,
    SNAKE_UART_CMD_P2_UP,
    SNAKE_UART_CMD_P2_DOWN,
    SNAKE_UART_CMD_P2_LEFT,
    SNAKE_UART_CMD_P2_RIGHT
} SnakeUartCmdType;

typedef struct {
    SnakeUartCmdType type;
    u8 value;
} SnakeUartCmd;

void SnakeUart_Init(void);
void SnakeUart_RxIrqHandler(void);
u8 SnakeUart_PopCommand(SnakeUartCmd *cmd);

#endif
