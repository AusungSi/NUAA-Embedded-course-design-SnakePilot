#include "stm32f10x.h"
#include "hw_config.h"
#include "lcd.h"

#define GRID_COLS      15
#define GRID_ROWS      20
#define CELL_SIZE      12
#define BOARD_X        30
#define BOARD_Y        58
#define MAX_SNAKE_LEN  (GRID_COLS * GRID_ROWS)

typedef enum {
    DIR_UP = 0,
    DIR_DOWN,
    DIR_LEFT,
    DIR_RIGHT
} SnakeDir;

static u8 snake_x[MAX_SNAKE_LEN];
static u8 snake_y[MAX_SNAKE_LEN];
static u16 snake_len;
static u8 food_x;
static u8 food_y;
static u16 score;
static u32 rng_state = 0x13572468;
static SnakeDir dir;
static SnakeDir next_dir;
static u8 key_last_raw;
static u8 key_stable;
static u8 key_stable_count;
static u8 key_latched;
static u8 turn_pending;

static u16 Snake_Rand(u16 max)
{
    rng_state = rng_state * 1664525u + 1013904223u;
    return (u16)((rng_state >> 16) % max);
}

static u8 Snake_KeyReadRaw(void)
{
    if (GPIO_ReadInputDataBit(GPIOD, GPIO_Pin_11) == 0) return 1; /* KEY1: up */
    if (GPIO_ReadInputDataBit(GPIOD, GPIO_Pin_12) == 0) return 2; /* KEY2: down */
    if (GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_13) == 0) return 3; /* KEY3: left */
    if (GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_0) == 0)  return 4; /* KEY4: right */
    return 0;
}

static void Snake_KeyReset(void)
{
    key_last_raw = Snake_KeyReadRaw();
    key_stable = key_last_raw;
    key_stable_count = 0;
    key_latched = 0;
}

static void Snake_KeyScan(void)
{
    u8 raw = Snake_KeyReadRaw();

    if (raw == key_last_raw) {
        if (key_stable_count < 3) {
            key_stable_count++;
        }
    } else {
        key_last_raw = raw;
        key_stable_count = 0;
    }

    if (key_stable_count >= 2 && raw != key_stable) {
        key_stable = raw;
        if (key_stable != 0) {
            key_latched = key_stable;
        }
    }
}

static u8 Snake_KeyPeek(void)
{
    if (key_latched != 0) {
        return key_latched;
    }

    return key_stable;
}

static void Snake_KeyDrop(u8 key)
{
    if (key_latched == key) {
        key_latched = 0;
    }
}

static void Snake_Beep(u16 ms)
{
    BEEP(1);
    Delay_ms(ms);
    BEEP(0);
}

static void Snake_ShowDirection(void)
{
    LED1(1);
    LED2(1);
    LED3(1);
    LED4(1);

    if (dir == DIR_UP) {
        LED1(0);
    } else if (dir == DIR_DOWN) {
        LED2(0);
    } else if (dir == DIR_LEFT) {
        LED3(0);
    } else if (dir == DIR_RIGHT) {
        LED4(0);
    }
}

static u8 Snake_OnSnake(u8 x, u8 y, u16 limit)
{
    u16 i;

    for (i = 0; i < limit; i++) {
        if (snake_x[i] == x && snake_y[i] == y) {
            return 1;
        }
    }

    return 0;
}

static u8 Snake_IsOpposite(SnakeDir a, SnakeDir b)
{
    return ((a == DIR_UP && b == DIR_DOWN) ||
            (a == DIR_DOWN && b == DIR_UP) ||
            (a == DIR_LEFT && b == DIR_RIGHT) ||
            (a == DIR_RIGHT && b == DIR_LEFT));
}

static void Snake_ReverseBody(void)
{
    u16 left = 0;
    u16 right = (u16)(snake_len - 1);

    while (left < right) {
        u8 tx = snake_x[left];
        u8 ty = snake_y[left];

        snake_x[left] = snake_x[right];
        snake_y[left] = snake_y[right];
        snake_x[right] = tx;
        snake_y[right] = ty;

        left++;
        right--;
    }
}

static void Snake_PlaceFood(void)
{
    u16 tries = 0;

    do {
        food_x = (u8)Snake_Rand(GRID_COLS);
        food_y = (u8)Snake_Rand(GRID_ROWS);
        tries++;
    } while (Snake_OnSnake(food_x, food_y, snake_len) && tries < 600);
}

static void Snake_DrawCell(u8 x, u8 y, u16 color)
{
    u16 px = BOARD_X + (u16)x * CELL_SIZE;
    u16 py = BOARD_Y + (u16)y * CELL_SIZE;

    LCD_Fill(px + 1, py + 1, px + CELL_SIZE - 2, py + CELL_SIZE - 2, color);
}

static void Snake_DrawHeader(void)
{
    LCD_Fill(0, 0, LCD_W - 1, 48, DARKBLUE);
    POINT_COLOR = WHITE;
    BACK_COLOR = DARKBLUE;
    LCD_ShowString(6, 6, 16, (u8 *)"SnakePilot", 0);
    LCD_ShowString(6, 28, 16, (u8 *)"Score:", 0);
    LCD_ShowNum(62, 28, score, 3, 16);
    LCD_ShowString(124, 28, 16, (u8 *)"K1-4", 0);
}

static void Snake_Render(void)
{
    u16 i;

    Snake_DrawHeader();

    LCD_Fill(BOARD_X, BOARD_Y, BOARD_X + GRID_COLS * CELL_SIZE - 1,
             BOARD_Y + GRID_ROWS * CELL_SIZE - 1, BLACK);
    POINT_COLOR = GRAY;
    LCD_DrawRectangle(BOARD_X - 1, BOARD_Y - 1,
                      BOARD_X + GRID_COLS * CELL_SIZE,
                      BOARD_Y + GRID_ROWS * CELL_SIZE);

    Snake_DrawCell(food_x, food_y, RED);

    for (i = 0; i < snake_len; i++) {
        if (i == 0) {
            Snake_DrawCell(snake_x[i], snake_y[i], YELLOW);
        } else {
            Snake_DrawCell(snake_x[i], snake_y[i], GREEN);
        }
    }
}

static void Snake_HandleInput(void)
{
    u8 key;
    SnakeDir want;

    Snake_KeyScan();
    key = Snake_KeyPeek();

    if (key == 0) {
        return;
    }

    if (key == 1) {
        want = DIR_UP;
    } else if (key == 2) {
        want = DIR_DOWN;
    } else if (key == 3) {
        want = DIR_LEFT;
    } else if (key == 4) {
        want = DIR_RIGHT;
    } else {
        Snake_KeyDrop(key);
        return;
    }

    if (want == dir || want == next_dir) {
        Snake_KeyDrop(key);
        return;
    }

    if (turn_pending) {
        return;
    }

    if (Snake_IsOpposite(dir, want)) {
        Snake_ReverseBody();
        dir = want;
    }

    next_dir = want;
    turn_pending = 1;
    Snake_KeyDrop(key);
}

static u16 Snake_StepDelay(void)
{
    u16 delay = 220;

    if (score < 20) {
        delay = (u16)(220 - score * 6);
    } else {
        delay = 100;
    }

    return delay;
}

static void Snake_WaitStep(u16 ms)
{
    u16 elapsed = 0;

    while (elapsed < ms) {
        Snake_HandleInput();
        Delay_ms(5);
        elapsed += 5;
    }
}

static u8 Snake_Step(void)
{
    signed char nx = (signed char)snake_x[0];
    signed char ny = (signed char)snake_y[0];
    u8 old_head_x = snake_x[0];
    u8 old_head_y = snake_y[0];
    u8 old_tail_x = snake_x[snake_len - 1];
    u8 old_tail_y = snake_y[snake_len - 1];
    u8 eat;
    u16 check_len;
    int i;

    dir = next_dir;
    turn_pending = 0;

    if (dir == DIR_UP) ny--;
    else if (dir == DIR_DOWN) ny++;
    else if (dir == DIR_LEFT) nx--;
    else if (dir == DIR_RIGHT) nx++;

    if (nx < 0 || nx >= GRID_COLS || ny < 0 || ny >= GRID_ROWS) {
        return 0;
    }

    eat = ((u8)nx == food_x && (u8)ny == food_y);
    check_len = eat ? snake_len : (u16)(snake_len - 1);

    if (Snake_OnSnake((u8)nx, (u8)ny, check_len)) {
        return 0;
    }

    if (eat && snake_len < MAX_SNAKE_LEN) {
        snake_len++;
    }

    for (i = (int)snake_len - 1; i > 0; i--) {
        snake_x[i] = snake_x[i - 1];
        snake_y[i] = snake_y[i - 1];
    }

    snake_x[0] = (u8)nx;
    snake_y[0] = (u8)ny;

    Snake_DrawCell(old_head_x, old_head_y, GREEN);

    if (!eat && (old_tail_x != (u8)nx || old_tail_y != (u8)ny)) {
        Snake_DrawCell(old_tail_x, old_tail_y, BLACK);
    }

    Snake_DrawCell(snake_x[0], snake_y[0], YELLOW);

    if (eat) {
        score++;
        Snake_PlaceFood();
        Snake_DrawHeader();
        Snake_DrawCell(food_x, food_y, RED);
        Snake_Beep(30);
    }

    Snake_ShowDirection();
    return 1;
}

static void Snake_Init(void)
{
    score = 0;
    snake_len = 4;
    dir = DIR_RIGHT;
    next_dir = DIR_RIGHT;
    turn_pending = 0;

    snake_x[0] = 7;
    snake_y[0] = 10;
    snake_x[1] = 6;
    snake_y[1] = 10;
    snake_x[2] = 5;
    snake_y[2] = 10;
    snake_x[3] = 4;
    snake_y[3] = 10;

    rng_state ^= 0x5a5a0000u + score + GPIO_ReadInputData(GPIOA);
    Snake_KeyReset();
    Snake_PlaceFood();
    Snake_ShowDirection();
    Snake_Render();
}

static void Snake_GameOver(void)
{
    Snake_Beep(150);
    LCD_Fill(20, 124, LCD_W - 21, 196, BLACK);
    POINT_COLOR = RED;
    BACK_COLOR = BLACK;
    LCD_ShowString(62, 138, 24, (u8 *)"GAME OVER", 0);
    POINT_COLOR = WHITE;
    LCD_ShowString(42, 170, 16, (u8 *)"Press any key", 0);

    while (Snake_KeyReadRaw() != 0) {
        Delay_ms(20);
    }

    while (Snake_KeyReadRaw() == 0) {
        Delay_ms(20);
    }

    Delay_ms(250);
}

int main(void)
{
    SystemInit();
    GPIO_Configuration();
    LCD_Init();
    LCD_Clear(BLACK);

    BEEP(0);
    Snake_Init();

    while (1) {
        Snake_HandleInput();
        Snake_WaitStep(Snake_StepDelay());

        if (!Snake_Step()) {
            Snake_GameOver();
            Snake_Init();
        } else {
            Snake_Render();
        }
    }
}
