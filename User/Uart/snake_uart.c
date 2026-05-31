#include "snake_uart.h"

#define SNAKE_UART_CMD_BUF_SIZE 8

static volatile SnakeUartCmd uart_cmd_buf[SNAKE_UART_CMD_BUF_SIZE];
static volatile u8 uart_cmd_head;
static volatile u8 uart_cmd_tail;

static void SnakeUart_PushCommand(SnakeUartCmdType type, u8 value)
{
    u8 next = (u8)((uart_cmd_head + 1) % SNAKE_UART_CMD_BUF_SIZE);

    if (next != uart_cmd_tail) {
        uart_cmd_buf[uart_cmd_head].type = type;
        uart_cmd_buf[uart_cmd_head].value = value;
        uart_cmd_head = next;
    }
}

static void SnakeUart_ParseByte(u8 ch)
{
    if (ch >= 'a' && ch <= 'z') {
        ch = (u8)(ch - 'a' + 'A');
    }

    if (ch == 'W') {
        SnakeUart_PushCommand(SNAKE_UART_CMD_UP, 0);
    } else if (ch == 'S') {
        SnakeUart_PushCommand(SNAKE_UART_CMD_DOWN, 0);
    } else if (ch == 'A') {
        SnakeUart_PushCommand(SNAKE_UART_CMD_LEFT, 0);
    } else if (ch == 'D') {
        SnakeUart_PushCommand(SNAKE_UART_CMD_RIGHT, 0);
    } else if (ch == 'P') {
        SnakeUart_PushCommand(SNAKE_UART_CMD_PAUSE, 0);
    } else if (ch == ' ') {
        SnakeUart_PushCommand(SNAKE_UART_CMD_START, 0);
    } else if (ch == 'R') {
        SnakeUart_PushCommand(SNAKE_UART_CMD_RESET, 0);
    } else if (ch >= '1' && ch <= '5') {
        SnakeUart_PushCommand(SNAKE_UART_CMD_LEVEL, (u8)(ch - '1'));
    }
}

void SnakeUart_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    USART_InitTypeDef USART_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_AFIO |
                           RCC_APB2Periph_USART1, ENABLE);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    USART_InitStructure.USART_BaudRate = SNAKE_UART_BAUD;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    USART_Init(USART1, &USART_InitStructure);

    USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);

    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
    NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    USART_Cmd(USART1, ENABLE);
}

void SnakeUart_RxIrqHandler(void)
{
    if (USART_GetITStatus(USART1, USART_IT_RXNE) != RESET) {
        SnakeUart_ParseByte((u8)USART_ReceiveData(USART1));
        USART_ClearITPendingBit(USART1, USART_IT_RXNE);
    }
}

u8 SnakeUart_PopCommand(SnakeUartCmd *cmd)
{
    if (uart_cmd_head == uart_cmd_tail) {
        return 0;
    }

    cmd->type = uart_cmd_buf[uart_cmd_tail].type;
    cmd->value = uart_cmd_buf[uart_cmd_tail].value;
    uart_cmd_tail = (u8)((uart_cmd_tail + 1) % SNAKE_UART_CMD_BUF_SIZE);
    return 1;
}
