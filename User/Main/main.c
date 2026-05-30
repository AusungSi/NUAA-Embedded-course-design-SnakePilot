#include "stm32f10x.h"
#include "hw_config.h"
#include "lcd.h"

#define GRID_COLS       15
#define GRID_ROWS       20
#define CELL_SIZE       12
#define BOARD_X         30
#define BOARD_Y         72
#define MAX_SNAKE_LEN   (GRID_COLS * GRID_ROWS)
#define LEVEL_COUNT     5
#define MUSIC_TICK_MS   5
#define MUSIC_COUNT(a)  ((u8)(sizeof(a) / sizeof((a)[0])))

#define NOTE_REST       0
#define NOTE_G3         196
#define NOTE_C4         262
#define NOTE_D4         294
#define NOTE_E4         330
#define NOTE_F4         349
#define NOTE_G4         392
#define NOTE_A4         440
#define NOTE_B4         494
#define NOTE_C5         523
#define NOTE_D5         587
#define NOTE_E5         659
#define NOTE_F5         698
#define NOTE_G5         784
#define NOTE_A5         880
#define NOTE_B5         988
#define NOTE_C6         1047
#define NOTE_D6         1175
#define NOTE_E6         1319
#define NOTE_G6         1568

#define KEY_UP_MASK     0x01
#define KEY_DOWN_MASK   0x02
#define KEY_LEFT_MASK   0x04
#define KEY_RIGHT_MASK  0x08
#define KEY_PAUSE_MASK  (KEY_UP_MASK | KEY_DOWN_MASK)

#define FOOD_NORMAL     0
#define FOOD_POISON     1
#define FOOD_BONUS      2

#define STEP_ALIVE      1
#define STEP_DEAD       2
#define STEP_LEVEL_DONE 3

#define HOME_NAVY       0x000C
#define HOME_BLUE       0x01D1
#define HOME_PANEL      0x1084
#define HOME_TEAL       0x05B8
#define HOME_GOLD       0xFD20

typedef enum {
    DIR_UP = 0,
    DIR_DOWN,
    DIR_LEFT,
    DIR_RIGHT
} SnakeDir;

typedef struct {
    u16 freq;
    u16 ms;
} MusicNote;

typedef struct {
    const MusicNote *notes;
    u8 count;
} LevelMusic;

static const MusicNote music_level1[] = {
    {NOTE_C5, 110}, {NOTE_E5, 110}, {NOTE_G5, 110}, {NOTE_E5, 110},
    {NOTE_D5, 100}, {NOTE_F5, 100}, {NOTE_G5, 150}, {NOTE_REST, 60},
    {NOTE_E5, 110}, {NOTE_G5, 110}, {NOTE_C6, 160}, {NOTE_G5, 120}
};

static const MusicNote music_level2[] = {
    {NOTE_A4, 130}, {NOTE_C5, 130}, {NOTE_E5, 130}, {NOTE_A5, 150},
    {NOTE_G5, 120}, {NOTE_E5, 120}, {NOTE_D5, 140}, {NOTE_C5, 150},
    {NOTE_E5, 100}, {NOTE_G5, 100}, {NOTE_A5, 180}, {NOTE_REST, 80}
};

static const MusicNote music_level3[] = {
    {NOTE_G4, 90}, {NOTE_B4, 90}, {NOTE_D5, 90}, {NOTE_G5, 160},
    {NOTE_REST, 50}, {NOTE_D5, 90}, {NOTE_B4, 90}, {NOTE_G4, 120},
    {NOTE_A4, 100}, {NOTE_D5, 100}, {NOTE_F5, 140}, {NOTE_E5, 140}
};

static const MusicNote music_level4[] = {
    {NOTE_E5, 90}, {NOTE_REST, 30}, {NOTE_D5, 90}, {NOTE_E5, 90},
    {NOTE_G5, 120}, {NOTE_F5, 120}, {NOTE_D5, 100}, {NOTE_B4, 120},
    {NOTE_C5, 100}, {NOTE_E5, 100}, {NOTE_A5, 160}, {NOTE_G5, 160}
};

static const MusicNote music_level5[] = {
    {NOTE_C5, 90}, {NOTE_G5, 90}, {NOTE_A5, 90}, {NOTE_E5, 90},
    {NOTE_F5, 100}, {NOTE_A5, 100}, {NOTE_C6, 150}, {NOTE_D6, 120},
    {NOTE_E6, 120}, {NOTE_D6, 100}, {NOTE_C6, 170}, {NOTE_G6, 190}
};

static const LevelMusic level_music[LEVEL_COUNT] = {
    {music_level1, MUSIC_COUNT(music_level1)},
    {music_level2, MUSIC_COUNT(music_level2)},
    {music_level3, MUSIC_COUNT(music_level3)},
    {music_level4, MUSIC_COUNT(music_level4)},
    {music_level5, MUSIC_COUNT(music_level5)}
};

static const char level_map[LEVEL_COUNT][GRID_ROWS][GRID_COLS + 1] = {
    {
        "...............",
        "...............",
        "...............",
        "...............",
        "...............",
        "...............",
        "...............",
        "...............",
        "...............",
        "...............",
        "...............",
        "...............",
        "...............",
        "...............",
        "...............",
        "...............",
        "...............",
        "...............",
        "...............",
        "..............."
    },
    {
        "...............",
        "...............",
        "....#####......",
        "...............",
        "...............",
        "...#.......#...",
        "...#.......#...",
        "...#.......#...",
        "...............",
        "...............",
        "...............",
        "...............",
        "......#####....",
        "...............",
        "...............",
        "..###.....###..",
        "...............",
        "...............",
        "...............",
        "..............."
    },
    {
        "...............",
        "..A............",
        "...............",
        ".....####......",
        "...............",
        "...............",
        "...............",
        "...............",
        "...............",
        "...............",
        "...............",
        "...............",
        "...............",
        "......####.....",
        "...............",
        "...............",
        "...............",
        "............B..",
        "...............",
        "..............."
    },
    {
        "...............",
        "...............",
        "...###.........",
        "...............",
        "...............",
        "...............",
        "...........###.",
        "...............",
        "...............",
        "...............",
        "...............",
        "...............",
        ".###...........",
        "...............",
        "...............",
        "...............",
        ".........###...",
        "...............",
        "...............",
        "..............."
    },
    {
        "...............",
        ".###.....###...",
        "...............",
        "...............",
        ".....###.......",
        "...............",
        "...............",
        "...#.......#...",
        "...#.......#...",
        "...............",
        "...............",
        "...#.......#...",
        "...#.......#...",
        "...............",
        ".......###.....",
        "...............",
        "...............",
        "...###.....###.",
        "...............",
        "..............."
    }
};

static const u8 level_target[LEVEL_COUNT] = {3, 4, 4, 6, 5};
static const u8 level_time_limit[LEVEL_COUNT] = {0, 0, 0, 0, 45};
static const char *level_name[LEVEL_COUNT] = {
    "BASIC",
    "WALL",
    "PORTAL",
    "ITEM",
    "TIME"
};

static const u16 home_author_glyphs[5][16] = {
    {
        0x0880, 0x1980, 0x11FC, 0x3380, 0x3280, 0x7480, 0x50F8, 0x1080,
        0x1080, 0x10FC, 0x1080, 0x1080, 0x1080, 0x0080, 0x0000, 0x0000
    },
    {
        0x0300, 0x0308, 0x1FD8, 0x0330, 0x0360, 0x7FFC, 0x0300, 0x0FF0,
        0x1810, 0x6FF0, 0x0810, 0x0810, 0x0FF0, 0x0000, 0x0000, 0x0000
    },
    {
        0x0000, 0x7FFC, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100,
        0x0100, 0x0100, 0x0100, 0x0100, 0x0700, 0x0000, 0x0000, 0x0000
    },
    {
        0x08C0, 0x1910, 0x1308, 0x33F4, 0x3110, 0x5318, 0x5684, 0x11F8,
        0x1190, 0x1690, 0x1060, 0x10E0, 0x179C, 0x1400, 0x0000, 0x0000
    },
    {
        0x0000, 0x33F0, 0x1130, 0x01E0, 0x60E0, 0x3F3C, 0x0440, 0x03F8,
        0x1040, 0x2040, 0x27FC, 0x6040, 0x6040, 0x0040, 0x0000, 0x0000
    }
};

static u8 snake_x[MAX_SNAKE_LEN];
static u8 snake_y[MAX_SNAKE_LEN];
static u16 snake_len;
static u8 food_x;
static u8 food_y;
static u8 food_type;
static u16 score;
static u16 high_score;
static u8 lives;
static u8 level_index;
static u8 level_score;
static u8 time_left;
static u16 time_acc_ms;
static u8 music_index;
static u16 music_left_ms;
static u16 music_freq;
static u32 rng_state = 0x13572468;
static SnakeDir dir;
static SnakeDir next_dir;
static u8 key_last_raw;
static u8 key_stable;
static u8 key_stable_count;
static u8 key_press_latch;
static u8 turn_pending;
static u8 paused;
static u8 pause_lock;
static const char *status_msg = "READY";

static u16 Snake_Rand(u16 max)
{
    rng_state = rng_state * 1664525u + 1013904223u;
    return (u16)((rng_state >> 16) % max);
}

static u8 Snake_KeyReadRaw(void)
{
    u8 keys = 0;

    if (GPIO_ReadInputDataBit(GPIOD, GPIO_Pin_11) == 0) keys |= KEY_UP_MASK;
    if (GPIO_ReadInputDataBit(GPIOD, GPIO_Pin_12) == 0) keys |= KEY_DOWN_MASK;
    if (GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_13) == 0) keys |= KEY_LEFT_MASK;
    if (GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_0) == 0)  keys |= KEY_RIGHT_MASK;

    return keys;
}

static void Snake_KeyReset(void)
{
    key_last_raw = Snake_KeyReadRaw();
    key_stable = key_last_raw;
    key_stable_count = 0;
    key_press_latch = 0;
    pause_lock = 0;
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
        u8 newly_pressed = (u8)(raw & (u8)(~key_stable));
        key_stable = raw;
        key_press_latch |= newly_pressed;
    }
}

static void Snake_DelayUs(u32 us)
{
    u32 i;

    for (i = 0; i < us * 8UL; i++) {
        __NOP();
    }
}

static void Snake_PlayTone(u16 freq, u16 ms)
{
    u16 half_period_us;
    u32 elapsed_us;
    u32 target_us;

    if (ms == 0) {
        return;
    }

    if (freq == NOTE_REST) {
        BEEP(0);
        Delay_ms(ms);
        return;
    }

    half_period_us = (u16)(500000UL / freq);
    if (half_period_us < 80) {
        half_period_us = 80;
    }

    elapsed_us = 0;
    target_us = (u32)ms * 1000UL;
    while (elapsed_us < target_us) {
        BEEP(1);
        Snake_DelayUs(half_period_us);
        BEEP(0);
        Snake_DelayUs(half_period_us);
        elapsed_us += (u32)half_period_us * 2UL;
    }
    BEEP(0);
}

static void Snake_MusicReset(void)
{
    music_index = 0;
    music_left_ms = 0;
    music_freq = NOTE_REST;
}

static void Snake_MusicLoadNextNote(void)
{
    const LevelMusic *music = &level_music[level_index];

    if (music->count == 0) {
        music_freq = NOTE_REST;
        music_left_ms = 80;
        return;
    }

    music_freq = music->notes[music_index].freq;
    music_left_ms = music->notes[music_index].ms;
    music_index++;
    if (music_index >= music->count) {
        music_index = 0;
    }

    if (music_left_ms == 0) {
        music_left_ms = MUSIC_TICK_MS;
    }
}

static void Snake_MusicTick(u16 ms)
{
    u16 chunk;

    while (ms > 0) {
        if (music_left_ms == 0) {
            Snake_MusicLoadNextNote();
        }

        chunk = ms;
        if (chunk > music_left_ms) {
            chunk = music_left_ms;
        }

        Snake_PlayTone(music_freq, chunk);
        music_left_ms = (u16)(music_left_ms - chunk);
        ms = (u16)(ms - chunk);
    }
}

static void Snake_Beep(u16 ms)
{
    Snake_PlayTone(NOTE_C6, ms);
}

static void Snake_BeepLevel(void)
{
    Snake_PlayTone(NOTE_G5, 90);
    Snake_PlayTone(NOTE_C6, 140);
}

static void Snake_ShowText(u16 x, u16 y, u8 size, const char *text,
                           u16 fc, u16 bc, u8 mode)
{
    POINT_COLOR = fc;
    BACK_COLOR = bc;
    LCD_ShowString(x, y, size, (u8 *)text, mode);
}

static void Snake_ShowTextCenter(u16 y, u8 size, const char *text,
                                 u16 fc, u16 bc, u8 mode)
{
    u16 len = 0;
    const char *p = text;

    while (*p >= ' ' && *p <= '~') {
        len++;
        p++;
    }

    Snake_ShowText((u16)((LCD_W - len * (size / 2)) / 2),
                   y, size, text, fc, bc, mode);
}

static void Snake_DrawHomeFrame(u16 x1, u16 y1, u16 x2, u16 y2, u16 color)
{
    POINT_COLOR = color;
    LCD_DrawRectangle(x1, y1, x2, y2);
    LCD_DrawRectangle((u16)(x1 + 2), (u16)(y1 + 2),
                      (u16)(x2 - 2), (u16)(y2 - 2));
}

static void Snake_DrawHomeGlyph(u16 x, u16 y, u8 glyph, u16 fc, u16 bc)
{
    u8 row;
    u8 col;
    u16 bits;

    for (row = 0; row < 16; row++) {
        bits = home_author_glyphs[glyph][row];
        for (col = 0; col < 16; col++) {
            GUI_DrawPoint((u16)(x + col), (u16)(y + row),
                          (bits & (u16)(0x8000 >> col)) ? fc : bc);
        }
    }
}

static void Snake_DrawHomeAuthor(u16 y)
{
    u8 i;
    u16 x = 77;

    for (i = 0; i < 5; i++) {
        Snake_DrawHomeGlyph((u16)(x + i * 18), y, i, LGRAY, HOME_NAVY);
    }
}

static void Snake_DrawHomeSnake(void)
{
    u8 i;
    const u16 sx[9] = {38, 54, 70, 86, 102, 118, 134, 150, 166};
    const u16 sy[9] = {104, 104, 104, 92, 92, 92, 104, 104, 104};

    for (i = 0; i < 9; i++) {
        LCD_Fill(sx[i], sy[i], (u16)(sx[i] + 13), (u16)(sy[i] + 13),
                 (i == 8) ? YELLOW : GREEN);
        POINT_COLOR = BLACK;
        LCD_DrawRectangle(sx[i], sy[i], (u16)(sx[i] + 13), (u16)(sy[i] + 13));
    }

    LCD_Fill(176, 108, 180, 112, RED);
    LCD_Fill(170, 98, 172, 100, BLACK);
}

static void Snake_DrawHomeLevels(void)
{
    u8 i;
    u16 x;
    u16 color;

    LCD_Fill(18, 142, 221, 236, HOME_PANEL);
    Snake_DrawHomeFrame(18, 142, 221, 236, HOME_TEAL);

    Snake_ShowTextCenter(154, 16, "FIVE STAGE ROUTE", WHITE, HOME_PANEL, 1);
    Snake_ShowTextCenter(176, 12, "obstacles  speed  score", LGRAY, HOME_PANEL, 1);

    for (i = 0; i < LEVEL_COUNT; i++) {
        x = (u16)(30 + i * 36);
        if (i == 0) {
            color = GREEN;
        } else if (i == 1) {
            color = CYAN;
        } else if (i == 2) {
            color = HOME_GOLD;
        } else if (i == 3) {
            color = MAGENTA;
        } else {
            color = RED;
        }

        LCD_Fill(x, 200, (u16)(x + 24), 222, color);
        POINT_COLOR = WHITE;
        LCD_DrawRectangle(x, 200, (u16)(x + 24), 222);
        BACK_COLOR = color;
        POINT_COLOR = BLACK;
        LCD_ShowNum((u16)(x + 8), 204, (u32)(i + 1), 1, 16);
    }
}

static void Snake_DrawHomePrompt(u8 visible)
{
    LCD_Fill(22, 278, 217, 309, HOME_NAVY);
    Snake_DrawHomeFrame(22, 278, 217, 309, visible ? HOME_GOLD : HOME_BLUE);

    if (visible) {
        Snake_ShowTextCenter(286, 16, "PRESS ANY KEY", YELLOW, HOME_NAVY, 1);
    }
}

static void Snake_ShowHome(void)
{
    u8 blink = 1;
    u8 i;

    LCD_Clear(BLACK);
    LCD_Fill(0, 0, LCD_W - 1, 78, HOME_NAVY);
    LCD_Fill(0, 78, LCD_W - 1, 124, HOME_BLUE);
    LCD_Fill(0, 124, LCD_W - 1, LCD_H - 1, BLACK);

    POINT_COLOR = GRAYBLUE;
    for (i = 0; i < 6; i++) {
        LCD_DrawLine((u16)(i * 46), 76, (u16)(i * 46 + 24), 122);
    }

    Snake_ShowTextCenter(20, 16, "SnakePilot", WHITE, HOME_NAVY, 1);
    Snake_ShowTextCenter(42, 12, "STM32 SMART SNAKE", CYAN, HOME_NAVY, 1);
    Snake_DrawHomeAuthor(61);

    Snake_DrawHomeSnake();
    Snake_DrawHomeLevels();

    Snake_ShowTextCenter(250, 12, "K1/K2/K3/K4  Control Direction",
                         LGRAY, BLACK, 1);
    Snake_DrawHomePrompt(blink);

    while (Snake_KeyReadRaw() != 0) {
        Delay_ms(20);
    }

    while (1) {
        for (i = 0; i < 25; i++) {
            if (Snake_KeyReadRaw() != 0) {
                Snake_Beep(60);
                while (Snake_KeyReadRaw() != 0) {
                    Delay_ms(20);
                }
                Delay_ms(120);
                LCD_Clear(BLACK);
                Snake_KeyReset();
                return;
            }
            Delay_ms(20);
        }

        blink = (u8)!blink;
        Snake_DrawHomePrompt(blink);
    }
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
    } else {
        LED4(0);
    }
}

static char Snake_MapCell(u8 x, u8 y)
{
    return level_map[level_index][y][x];
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

static u8 Snake_CellCanHoldFood(u8 x, u8 y)
{
    if (Snake_MapCell(x, y) != '.') {
        return 0;
    }

    return (u8)(!Snake_OnSnake(x, y, snake_len));
}

static void Snake_DrawCell(u8 x, u8 y, u16 color)
{
    u16 px = BOARD_X + (u16)x * CELL_SIZE;
    u16 py = BOARD_Y + (u16)y * CELL_SIZE;

    LCD_Fill(px + 1, py + 1, px + CELL_SIZE - 2, py + CELL_SIZE - 2, color);
}

static void Snake_DrawMapCell(u8 x, u8 y)
{
    char c = Snake_MapCell(x, y);

    if (c == '#') {
        Snake_DrawCell(x, y, GRAY);
    } else if (c == 'A') {
        Snake_DrawCell(x, y, CYAN);
    } else if (c == 'B') {
        Snake_DrawCell(x, y, MAGENTA);
    } else {
        Snake_DrawCell(x, y, BLACK);
    }
}

static void Snake_DrawFood(void)
{
    if (food_type == FOOD_POISON) {
        Snake_DrawCell(food_x, food_y, MAGENTA);
    } else if (food_type == FOOD_BONUS) {
        Snake_DrawCell(food_x, food_y, CYAN);
    } else {
        Snake_DrawCell(food_x, food_y, RED);
    }
}

static void Snake_SetStatus(const char *msg)
{
    status_msg = msg;
}

static void Snake_DrawHeader(void)
{
    LCD_Fill(0, 0, LCD_W - 1, BOARD_Y - 8, DARKBLUE);
    POINT_COLOR = WHITE;
    BACK_COLOR = DARKBLUE;

    LCD_ShowString(6, 4, 16, (u8 *)"SnakeLab", 0);
    LCD_ShowString(88, 4, 16, (u8 *)"L", 0);
    LCD_ShowNum(102, 4, (u32)(level_index + 1), 1, 16);
    LCD_ShowString(122, 4, 16, (u8 *)level_name[level_index], 0);

    LCD_ShowString(6, 24, 16, (u8 *)"S:", 0);
    LCD_ShowNum(28, 24, score, 3, 16);
    LCD_ShowString(70, 24, 16, (u8 *)"B:", 0);
    LCD_ShowNum(92, 24, high_score, 3, 16);
    LCD_ShowString(134, 24, 16, (u8 *)"HP:", 0);
    LCD_ShowNum(166, 24, lives, 1, 16);
    LCD_ShowString(190, 24, 16, (u8 *)"T:", 0);
    if (level_time_limit[level_index] == 0) {
        LCD_ShowString(212, 24, 16, (u8 *)"--", 0);
    } else {
        LCD_ShowNum(212, 24, time_left, 2, 16);
    }

    LCD_ShowString(6, 48, 16, (u8 *)status_msg, 0);
}

static void Snake_Render(void)
{
    u16 i;
    u8 x;
    u8 y;

    Snake_DrawHeader();
    LCD_Fill(BOARD_X, BOARD_Y, BOARD_X + GRID_COLS * CELL_SIZE - 1,
             BOARD_Y + GRID_ROWS * CELL_SIZE - 1, BLACK);

    for (y = 0; y < GRID_ROWS; y++) {
        for (x = 0; x < GRID_COLS; x++) {
            Snake_DrawMapCell(x, y);
        }
    }

    POINT_COLOR = GRAY;
    LCD_DrawRectangle(BOARD_X - 1, BOARD_Y - 1,
                      BOARD_X + GRID_COLS * CELL_SIZE,
                      BOARD_Y + GRID_ROWS * CELL_SIZE);

    Snake_DrawFood();

    for (i = 0; i < snake_len; i++) {
        if (i == 0) {
            Snake_DrawCell(snake_x[i], snake_y[i], YELLOW);
        } else {
            Snake_DrawCell(snake_x[i], snake_y[i], GREEN);
        }
    }
}

static void Snake_ShowCenter(const char *line1, const char *line2)
{
    LCD_Fill(20, 132, LCD_W - 21, 198, BLACK);
    POINT_COLOR = YELLOW;
    BACK_COLOR = BLACK;
    LCD_ShowString(48, 144, 24, (u8 *)line1, 0);
    POINT_COLOR = WHITE;
    LCD_ShowString(42, 176, 16, (u8 *)line2, 0);
}

static void Snake_WaitAnyKey(void)
{
    while (Snake_KeyReadRaw() != 0) {
        Delay_ms(20);
    }

    while (Snake_KeyReadRaw() == 0) {
        Delay_ms(20);
    }

    Delay_ms(250);
    Snake_KeyReset();
}

static void Snake_PlaceFood(void)
{
    u16 tries;
    u8 x;
    u8 y;

    for (tries = 0; tries < 700; tries++) {
        x = (u8)Snake_Rand(GRID_COLS);
        y = (u8)Snake_Rand(GRID_ROWS);
        if (Snake_CellCanHoldFood(x, y)) {
            food_x = x;
            food_y = y;
            break;
        }
    }

    if (tries >= 700) {
        for (y = 0; y < GRID_ROWS; y++) {
            for (x = 0; x < GRID_COLS; x++) {
                if (Snake_CellCanHoldFood(x, y)) {
                    food_x = x;
                    food_y = y;
                    y = GRID_ROWS;
                    break;
                }
            }
        }
    }

    if (level_index == 3) {
        u8 r = (u8)Snake_Rand(10);
        if (r < 2) {
            food_type = FOOD_POISON;
        } else if (r == 9) {
            food_type = FOOD_BONUS;
        } else {
            food_type = FOOD_NORMAL;
        }
    } else {
        food_type = FOOD_NORMAL;
    }
}

static void Snake_ResetSnake(void)
{
    snake_len = 4;
    dir = DIR_RIGHT;
    next_dir = DIR_RIGHT;
    turn_pending = 0;
    paused = 0;
    time_acc_ms = 0;

    snake_x[0] = 7;
    snake_y[0] = 10;
    snake_x[1] = 6;
    snake_y[1] = 10;
    snake_x[2] = 5;
    snake_y[2] = 10;
    snake_x[3] = 4;
    snake_y[3] = 10;

    Snake_KeyReset();
    Snake_ShowDirection();
}

static void Snake_StartLevel(u8 lv)
{
    level_index = lv;
    level_score = 0;
    time_left = level_time_limit[level_index];
    Snake_SetStatus("K1+K2 Pause");
    Snake_MusicReset();
    Snake_ResetSnake();
    rng_state ^= 0x5a5a0000u + score + GPIO_ReadInputData(GPIOA);
    Snake_PlaceFood();
    Snake_Render();
}

static void Snake_StartGame(void)
{
    score = 0;
    lives = 3;
    Snake_StartLevel(0);
}

static SnakeDir Snake_KeyToDir(u8 key_bit)
{
    if (key_bit == KEY_UP_MASK) return DIR_UP;
    if (key_bit == KEY_DOWN_MASK) return DIR_DOWN;
    if (key_bit == KEY_LEFT_MASK) return DIR_LEFT;
    return DIR_RIGHT;
}

static u8 Snake_PopDirectionKey(void)
{
    u8 key = 0;

    if (key_press_latch & KEY_UP_MASK) {
        key = KEY_UP_MASK;
    } else if (key_press_latch & KEY_DOWN_MASK) {
        key = KEY_DOWN_MASK;
    } else if (key_press_latch & KEY_LEFT_MASK) {
        key = KEY_LEFT_MASK;
    } else if (key_press_latch & KEY_RIGHT_MASK) {
        key = KEY_RIGHT_MASK;
    }

    if (key != 0) {
        key_press_latch &= (u8)(~key);
    }

    return key;
}

static void Snake_HandleInput(void)
{
    u8 key;
    SnakeDir want;

    Snake_KeyScan();

    if ((key_stable & KEY_PAUSE_MASK) == KEY_PAUSE_MASK) {
        if (!pause_lock) {
            paused = (u8)!paused;
            pause_lock = 1;
            key_press_latch = 0;
            Snake_SetStatus(paused ? "PAUSED" : "RUNNING");
            Snake_DrawHeader();
            Snake_Beep(35);
        }
        return;
    }

    pause_lock = 0;
    if (paused) {
        return;
    }

    if (turn_pending) {
        return;
    }

    key = Snake_PopDirectionKey();
    if (key == 0) {
        return;
    }

    want = Snake_KeyToDir(key);
    if (want == next_dir) {
        return;
    }

    next_dir = want;
    turn_pending = 1;
}

static u16 Snake_StepDelay(void)
{
    u16 delay = 300;
    u16 speed_score = score;

    if (speed_score > 12) {
        speed_score = 12;
    }

    delay = (u16)(300 - speed_score * 14);
    if (level_index >= 3 && delay > 40) {
        delay = (u16)(delay - 30);
    }
    if (delay < 110) {
        delay = 110;
    }

    return delay;
}

static u8 Snake_TimerTick(u16 ms)
{
    if (level_time_limit[level_index] == 0) {
        return 1;
    }

    time_acc_ms = (u16)(time_acc_ms + ms);
    while (time_acc_ms >= 1000) {
        time_acc_ms = (u16)(time_acc_ms - 1000);
        if (time_left > 0) {
            time_left--;
            Snake_DrawHeader();
        }
        if (time_left == 0) {
            return 0;
        }
    }

    return 1;
}

static u8 Snake_WaitStep(u16 ms)
{
    u16 elapsed = 0;

    while (elapsed < ms) {
        Snake_HandleInput();

        if (paused) {
            Delay_ms(20);
            continue;
        }

        Snake_MusicTick(MUSIC_TICK_MS);
        elapsed = (u16)(elapsed + MUSIC_TICK_MS);
        if (!Snake_TimerTick(MUSIC_TICK_MS)) {
            return 0;
        }
    }

    return 1;
}

static u8 Snake_FindOtherPortal(char portal, u8 *x, u8 *y)
{
    u8 ix;
    u8 iy;
    char other = (portal == 'A') ? 'B' : 'A';

    for (iy = 0; iy < GRID_ROWS; iy++) {
        for (ix = 0; ix < GRID_COLS; ix++) {
            if (Snake_MapCell(ix, iy) == other) {
                *x = ix;
                *y = iy;
                return 1;
            }
        }
    }

    return 0;
}

static void Snake_UpdateBest(void)
{
    if (score > high_score) {
        high_score = score;
    }
}

static u8 Snake_ApplyFood(void)
{
    if (food_type == FOOD_POISON) {
        if (score > 0) {
            score--;
        }
        if (lives > 0) {
            lives--;
        }
        Snake_SetStatus("POISON");
        Snake_PlayTone(NOTE_D4, 120);
        if (lives == 0) {
            return STEP_DEAD;
        }
    } else if (food_type == FOOD_BONUS) {
        score = (u16)(score + 2);
        level_score = (u8)(level_score + 2);
        if (lives < 5) {
            lives++;
        }
        Snake_SetStatus("BONUS");
        Snake_BeepLevel();
    } else {
        score++;
        level_score++;
        Snake_SetStatus("FOOD");
        Snake_PlayTone(NOTE_C6, 30);
    }

    Snake_UpdateBest();
    if (level_score >= level_target[level_index]) {
        return STEP_LEVEL_DONE;
    }

    Snake_PlaceFood();
    return STEP_ALIVE;
}

static u8 Snake_Step(void)
{
    signed char nx = (signed char)snake_x[0];
    signed char ny = (signed char)snake_y[0];
    u8 eat;
    u16 check_len;
    int i;
    char cell;
    u8 result;

    dir = next_dir;
    turn_pending = 0;

    if (dir == DIR_UP) ny--;
    else if (dir == DIR_DOWN) ny++;
    else if (dir == DIR_LEFT) nx--;
    else nx++;

    if (nx < 0 || nx >= GRID_COLS || ny < 0 || ny >= GRID_ROWS) {
        return STEP_DEAD;
    }

    cell = Snake_MapCell((u8)nx, (u8)ny);
    if (cell == '#') {
        return STEP_DEAD;
    }

    if (cell == 'A' || cell == 'B') {
        u8 tx = (u8)nx;
        u8 ty = (u8)ny;
        if (Snake_FindOtherPortal(cell, &tx, &ty)) {
            nx = (signed char)tx;
            ny = (signed char)ty;
            Snake_SetStatus("PORTAL");
            Snake_PlayTone(NOTE_D6, 60);
        }
    }

    eat = ((u8)nx == food_x && (u8)ny == food_y);
    check_len = eat ? snake_len : (u16)(snake_len - 1);

    if (Snake_OnSnake((u8)nx, (u8)ny, check_len)) {
        return STEP_DEAD;
    }

    if (eat && food_type != FOOD_POISON && snake_len < MAX_SNAKE_LEN) {
        snake_len++;
    }

    for (i = (int)snake_len - 1; i > 0; i--) {
        snake_x[i] = snake_x[i - 1];
        snake_y[i] = snake_y[i - 1];
    }

    snake_x[0] = (u8)nx;
    snake_y[0] = (u8)ny;

    result = STEP_ALIVE;
    if (eat) {
        result = Snake_ApplyFood();
    } else {
        Snake_SetStatus("RUNNING");
    }

    Snake_ShowDirection();
    Snake_Render();
    return result;
}

static u8 Snake_LoseLife(const char *reason)
{
    if (lives > 0) {
        lives--;
    }
    Snake_UpdateBest();

    Snake_SetStatus(reason);
    Snake_Render();
    Snake_PlayTone(NOTE_C4, 150);
    Snake_PlayTone(NOTE_G3, 180);

    if (lives == 0) {
        return 0;
    }

    Snake_ShowCenter("LIFE LOST", "Resume same level");
    Delay_ms(800);
    time_left = level_time_limit[level_index];
    Snake_ResetSnake();
    Snake_PlaceFood();
    Snake_SetStatus("TRY AGAIN");
    Snake_Render();
    return 1;
}

static void Snake_GameOver(void)
{
    Snake_UpdateBest();
    Snake_ShowCenter("GAME OVER", "Press any key");
    Snake_PlayTone(NOTE_C4, 160);
    Snake_PlayTone(NOTE_G3, 230);
    Snake_WaitAnyKey();
}

static void Snake_NextLevel(void)
{
    if (level_index + 1 >= LEVEL_COUNT) {
        Snake_UpdateBest();
        Snake_ShowCenter("YOU WIN", "Press any key");
        Snake_BeepLevel();
        Snake_WaitAnyKey();
        Snake_StartGame();
    } else {
        Snake_ShowCenter("LEVEL CLEAR", "Next level");
        Snake_BeepLevel();
        Delay_ms(900);
        Snake_StartLevel((u8)(level_index + 1));
    }
}

int main(void)
{
    u8 step_result;

    SystemInit();
    GPIO_Configuration();
    LCD_Init();
    LCD_Clear(BLACK);
    BEEP(0);

    Snake_ShowHome();
    Snake_StartGame();

    while (1) {
        if (!Snake_WaitStep(Snake_StepDelay())) {
            if (!Snake_LoseLife("TIME OUT")) {
                Snake_GameOver();
                Snake_StartGame();
            }
            continue;
        }

        step_result = Snake_Step();
        if (step_result == STEP_DEAD) {
            if (!Snake_LoseLife("CRASH")) {
                Snake_GameOver();
                Snake_StartGame();
            }
        } else if (step_result == STEP_LEVEL_DONE) {
            Snake_NextLevel();
        }
    }
}
