#include "stm32f10x.h"
#include "hw_config.h"
#include "lcd.h"
#include "snake_uart.h"

#define GRID_COLS       15
#define GRID_ROWS       20
#define CELL_W          13
#define CELL_H          12
#define BOARD_X         22
#define BOARD_Y         72
#define OPEN_DUO_ROWS   9
#define OPEN_DUO_P1_Y   BOARD_Y
#define OPEN_DUO_P2_Y   204
#define OPEN_WORLD_COLS 40
#define OPEN_WORLD_ROWS 56
#define MAX_SNAKE_LEN   (OPEN_WORLD_COLS * OPEN_WORLD_ROWS)
#define LEVEL_COUNT     5
#define MUSIC_TICK_MS   5
#define MUSIC_COUNT(a)  ((u16)(sizeof(a) / sizeof((a)[0])))

#define NOTE_REST       0
#define NOTE_C3         131
#define NOTE_CS3        139
#define NOTE_D3         147
#define NOTE_DS3        156
#define NOTE_E3         165
#define NOTE_F3         175
#define NOTE_FS3        185
#define NOTE_G3         196
#define NOTE_GS3        208
#define NOTE_A3         220
#define NOTE_AS3        233
#define NOTE_B3         247
#define NOTE_C4         262
#define NOTE_CS4        277
#define NOTE_D4         294
#define NOTE_DS4        311
#define NOTE_E4         330
#define NOTE_F4         349
#define NOTE_FS4        370
#define NOTE_G4         392
#define NOTE_GS4        415
#define NOTE_A4         440
#define NOTE_AS4        466
#define NOTE_B4         494
#define NOTE_C5         523
#define NOTE_CS5        554
#define NOTE_D5         587
#define NOTE_DS5        622
#define NOTE_E5         659
#define NOTE_F5         698
#define NOTE_FS5        740
#define NOTE_G5         784
#define NOTE_GS5        831
#define NOTE_A5         880
#define NOTE_AS5        932
#define NOTE_B5         988
#define NOTE_C6         1047
#define NOTE_CS6        1109
#define NOTE_D6         1175
#define NOTE_DS6        1245
#define NOTE_E6         1319
#define NOTE_F6         1397
#define NOTE_FS6        1480
#define NOTE_G6         1568
#define NOTE_A6         1760

#define KEY_UP_MASK     0x01
#define KEY_DOWN_MASK   0x02
#define KEY_LEFT_MASK   0x04
#define KEY_RIGHT_MASK  0x08
#define KEY_PAUSE_MASK  (KEY_UP_MASK | KEY_DOWN_MASK)

#define KNOB_LEFT_EVENT   1
#define KNOB_RIGHT_EVENT  2
#define KNOB_LOW_TH       1400
#define KNOB_HIGH_TH      2700
#define KNOB_CENTER_LOW   1700
#define KNOB_CENTER_HIGH  2400
#define KNOB_TURN_DELTA   420
#define VOLUME_MAX        5
#define VOLUME_DEFAULT    4
#define AUDIO_WAVE_POINTS 32
#define AUDIO_DAC_CENTER  2047
#define AUDIO_DAC_DHR12R2 ((u32)0x40007414)

#define FOOD_NORMAL     0
#define FOOD_POISON     1
#define FOOD_BONUS      2

#define STEP_ALIVE      1
#define STEP_DEAD       2
#define STEP_LEVEL_DONE 3
#define STEP_WIN        4
#define STEP_SELF_HIT   5
#define STEP_SELF_GAME_OVER 6

#define GAME_MODE_STAGE   0
#define GAME_MODE_CLASSIC 1
#define GAME_MODE_DUO     2
#define GAME_MODE_OPEN    3
#define GAME_MODE_OPEN_DUO 4
#define GAME_MODE_COUNT   5

#define HOME_NAVY       0x000C
#define HOME_BLUE       0x01D1
#define HOME_PANEL      0x1084
#define HOME_TEAL       0x05B8
#define HOME_GOLD       0xFD20

#define KNOB_LEVEL_STEP (4096 / LEVEL_COUNT)

#define SNAKE_FLASH_PAGE_SIZE   0x800UL
#define SNAKE_FLASH_STORE_ADDR  0x0803F800UL
#define SNAKE_FLASH_MAGIC       0x534E4B50UL
#define SNAKE_FLASH_VERSION     0x0001UL
#define SNAKE_FLASH_XOR_KEY     0xA55A3CC3UL
#define SNAKE_NO_INDEX          0xFFFFU
#define SNAKE_FLASH_KEY1        0x45670123UL
#define SNAKE_FLASH_KEY2        0xCDEF89ABUL
#define SNAKE_FLASH_TIMEOUT     0x000B0000UL

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
    u16 count;
} LevelMusic;

typedef struct {
    u32 magic;
    u32 high_score;
    u32 settings;
    u32 checksum;
} SnakePersistRecord;

static const MusicNote music_level1[] = {
    {NOTE_C4, 260}, {NOTE_E4, 180}, {NOTE_G4, 260}, {NOTE_C5, 380},
    {NOTE_B4, 180}, {NOTE_G4, 220}, {NOTE_A4, 340}, {NOTE_REST, 70},
    {NOTE_D4, 240}, {NOTE_F4, 180}, {NOTE_A4, 260}, {NOTE_D5, 360},
    {NOTE_C5, 180}, {NOTE_A4, 220}, {NOTE_B4, 340}, {NOTE_REST, 70},
    {NOTE_E4, 240}, {NOTE_G4, 180}, {NOTE_C5, 300}, {NOTE_E5, 400},
    {NOTE_D5, 180}, {NOTE_C5, 220}, {NOTE_A4, 320}, {NOTE_REST, 80},
    {NOTE_F4, 220}, {NOTE_A4, 160}, {NOTE_D5, 300}, {NOTE_C5, 220},
    {NOTE_B4, 200}, {NOTE_G4, 220}, {NOTE_C5, 520}, {NOTE_REST, 120}
};

static const MusicNote music_level2[] = {
    {NOTE_D3, 520}, {NOTE_A3, 260}, {NOTE_D4, 520}, {NOTE_F4, 280},
    {NOTE_E4, 260}, {NOTE_C4, 360}, {NOTE_REST, 110},
    {NOTE_AS3, 460}, {NOTE_F4, 260}, {NOTE_AS4, 420}, {NOTE_A4, 220},
    {NOTE_G4, 340}, {NOTE_D4, 380}, {NOTE_REST, 100},
    {NOTE_C4, 360}, {NOTE_G3, 240}, {NOTE_C4, 260}, {NOTE_E4, 360},
    {NOTE_F4, 220}, {NOTE_E4, 260}, {NOTE_D4, 520}, {NOTE_REST, 120},
    {NOTE_D3, 240}, {NOTE_REST, 60}, {NOTE_D3, 240}, {NOTE_A3, 360},
    {NOTE_C4, 260}, {NOTE_F4, 360}, {NOTE_D4, 640}, {NOTE_REST, 160}
};

static const MusicNote music_level3[] = {
    {NOTE_E4, 360}, {NOTE_B4, 260}, {NOTE_FS5, 540}, {NOTE_REST, 80},
    {NOTE_G5, 180}, {NOTE_FS5, 180}, {NOTE_E5, 380}, {NOTE_B4, 300},
    {NOTE_CS4, 340}, {NOTE_GS4, 260}, {NOTE_DS5, 520}, {NOTE_REST, 90},
    {NOTE_E5, 180}, {NOTE_DS5, 180}, {NOTE_CS5, 360}, {NOTE_GS4, 320},
    {NOTE_A3, 320}, {NOTE_E4, 240}, {NOTE_B4, 480}, {NOTE_CS5, 220},
    {NOTE_E5, 300}, {NOTE_FS5, 520}, {NOTE_REST, 120},
    {NOTE_B3, 300}, {NOTE_FS4, 220}, {NOTE_CS5, 420}, {NOTE_B4, 240},
    {NOTE_GS4, 300}, {NOTE_E4, 620}, {NOTE_REST, 160}
};

static const MusicNote music_level4[] = {
    {NOTE_A3, 180}, {NOTE_REST, 40}, {NOTE_E4, 180}, {NOTE_A4, 220},
    {NOTE_C5, 180}, {NOTE_E5, 260}, {NOTE_D5, 140}, {NOTE_C5, 300},
    {NOTE_REST, 70},
    {NOTE_GS3, 180}, {NOTE_REST, 40}, {NOTE_E4, 180}, {NOTE_GS4, 220},
    {NOTE_B4, 180}, {NOTE_E5, 260}, {NOTE_D5, 140}, {NOTE_B4, 300},
    {NOTE_REST, 70},
    {NOTE_F3, 160}, {NOTE_C4, 160}, {NOTE_F4, 220}, {NOTE_A4, 180},
    {NOTE_C5, 300}, {NOTE_AS4, 150}, {NOTE_A4, 260}, {NOTE_REST, 60},
    {NOTE_E4, 160}, {NOTE_GS4, 160}, {NOTE_B4, 220}, {NOTE_E5, 340},
    {NOTE_GS5, 180}, {NOTE_E5, 180}, {NOTE_C5, 520}, {NOTE_REST, 130}
};

static const MusicNote music_level5[] = {
    {NOTE_C3, 180}, {NOTE_REST, 45}, {NOTE_C3, 180}, {NOTE_REST, 80},
    {NOTE_G3, 360}, {NOTE_DS4, 520}, {NOTE_D4, 260}, {NOTE_C4, 620},
    {NOTE_REST, 130},
    {NOTE_C3, 150}, {NOTE_REST, 40}, {NOTE_C3, 150}, {NOTE_REST, 60},
    {NOTE_G3, 300}, {NOTE_F4, 420}, {NOTE_DS4, 260}, {NOTE_D4, 620},
    {NOTE_REST, 110},
    {NOTE_C3, 130}, {NOTE_REST, 35}, {NOTE_C3, 130}, {NOTE_G3, 240},
    {NOTE_C4, 240}, {NOTE_G4, 480}, {NOTE_FS4, 180}, {NOTE_F4, 360},
    {NOTE_DS4, 300}, {NOTE_C4, 660}, {NOTE_REST, 140},
    {NOTE_C5, 180}, {NOTE_B4, 150}, {NOTE_AS4, 240}, {NOTE_G4, 320},
    {NOTE_DS5, 340}, {NOTE_D5, 300}, {NOTE_C5, 760}, {NOTE_REST, 180}
};

static const MusicNote music_home[] = {
    #include "music_home_data.inc"
};

static const LevelMusic level_music[LEVEL_COUNT] = {
    {music_level1, MUSIC_COUNT(music_level1)},
    {music_level2, MUSIC_COUNT(music_level2)},
    {music_level3, MUSIC_COUNT(music_level3)},
    {music_level4, MUSIC_COUNT(music_level4)},
    {music_level5, MUSIC_COUNT(music_level5)}
};

static const u16 audio_sine_base[AUDIO_WAVE_POINTS] = {
    2047, 2447, 2831, 3185, 3498, 3750, 3939, 4056,
    4095, 4056, 3939, 3750, 3495, 3185, 2831, 2447,
    2047, 1647, 1263, 909, 599, 344, 155, 38,
    0, 38, 155, 344, 599, 909, 1263, 1647
};

static u16 audio_sine_buffer[AUDIO_WAVE_POINTS];

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
static u16 prev_snake_len;
static u8 prev_snake_head_x;
static u8 prev_snake_head_y;
static u8 prev_snake_tail_x;
static u8 prev_snake_tail_y;
static u8 snake2_x[MAX_SNAKE_LEN];
static u8 snake2_y[MAX_SNAKE_LEN];
static u16 snake2_len;
static u16 prev_snake2_len;
static u8 prev_snake2_head_x;
static u8 prev_snake2_head_y;
static u8 prev_snake2_tail_x;
static u8 prev_snake2_tail_y;
static SnakeDir dir2;
static SnakeDir next_dir2;
static u8 turn_pending2;
static u8 duo_winner;
static u8 food_x;
static u8 food_y;
static u8 prev_food_x;
static u8 prev_food_y;
static u8 food_type;
static u16 score;
static u16 score2;
static u16 high_score;
static u8 lives;
static u8 level_index;
static u8 level_score;
static u16 classic_target_len;
static u8 viewport_x;
static u8 viewport_y;
static u8 prev_viewport_x;
static u8 prev_viewport_y;
static u8 viewport2_x;
static u8 viewport2_y;
static u8 prev_viewport2_x;
static u8 prev_viewport2_y;
static u8 time_left;
static u16 time_acc_ms;
static const MusicNote *music_notes;
static u16 music_count;
static u16 music_index;
static u16 music_left_ms;
static u16 music_gap_ms;
static u16 music_freq;
static u8 audio_playing;
static u8 audio_wave_volume = 0xff;
static u32 rng_state = 0x13572468;
static SnakeDir dir;
static SnakeDir next_dir;
static u8 key_last_raw;
static u8 key_stable;
static u8 key_stable_count;
static u8 key_press_latch;
static u8 knob_event_latch;
static u8 knob_adc_channel = ADC_Channel_3;
static u16 knob_last_value;
static u8 sound_volume = VOLUME_DEFAULT;
static u8 turn_pending;
static u8 paused;
static u8 pause_lock;
static u8 restart_request;
static u8 return_home_request;
static u8 start_level;
static u8 game_mode;
static u8 persist_dirty;
static const char *status_msg = "READY";

static void Snake_AudioStop(void);
static void Snake_AudioSet(u16 freq);
static void Snake_Beep(u16 ms);
static void Snake_SetStatus(const char *msg);
static void Snake_DrawHeader(void);
static void Snake_DrawPauseHint(void);
static FLASH_Status Snake_FlashWaitReady(void);
static void Snake_FlashClearStatus(void);
static void Snake_FlashUnlock(void);
static void Snake_FlashLock(void);
static FLASH_Status Snake_FlashErasePage(u32 page_addr);
static FLASH_Status Snake_FlashProgramHalfWord(u32 address, u16 data);
static FLASH_Status Snake_FlashProgramWord(u32 address, u32 data);
static void Snake_PersistLoad(void);
static void Snake_PersistSave(void);
static void Snake_Render(void);
static void Snake_StartLevel(u8 lv);

static u8 Snake_IsDuoMode(void)
{
    return (u8)(game_mode == GAME_MODE_DUO || game_mode == GAME_MODE_OPEN_DUO);
}

static u8 Snake_IsOpenMode(void)
{
    return (u8)(game_mode == GAME_MODE_OPEN || game_mode == GAME_MODE_OPEN_DUO);
}

static u8 Snake_IsOpenDuoMode(void)
{
    return (u8)(game_mode == GAME_MODE_OPEN_DUO);
}

static u32 Snake_PersistBuildSettings(void)
{
    return (u32)sound_volume | ((u32)game_mode << 8);
}

static u32 Snake_PersistBuildChecksum(u32 high, u32 settings)
{
    return SNAKE_FLASH_MAGIC ^ SNAKE_FLASH_VERSION ^
           high ^ settings ^ SNAKE_FLASH_XOR_KEY;
}

static void Snake_PersistMarkDirty(void)
{
    persist_dirty = 1;
}

static void Snake_PersistRemember(void)
{
    persist_dirty = 0;
}

static FLASH_Status Snake_FlashWaitReady(void)
{
    u32 timeout = SNAKE_FLASH_TIMEOUT;

    while (((FLASH->SR & FLASH_SR_BSY) != 0) && (timeout != 0)) {
        timeout--;
    }

    if ((FLASH->SR & FLASH_SR_BSY) != 0) {
        return FLASH_TIMEOUT;
    }
    if ((FLASH->SR & FLASH_SR_PGERR) != 0) {
        return FLASH_ERROR_PG;
    }
    if ((FLASH->SR & FLASH_SR_WRPRTERR) != 0) {
        return FLASH_ERROR_WRP;
    }

    return FLASH_COMPLETE;
}

static void Snake_FlashClearStatus(void)
{
    FLASH->SR = FLASH_SR_EOP | FLASH_SR_PGERR | FLASH_SR_WRPRTERR;
}

static void Snake_FlashUnlock(void)
{
    if ((FLASH->CR & FLASH_CR_LOCK) != 0) {
        FLASH->KEYR = SNAKE_FLASH_KEY1;
        FLASH->KEYR = SNAKE_FLASH_KEY2;
    }
}

static void Snake_FlashLock(void)
{
    FLASH->CR |= FLASH_CR_LOCK;
}

static FLASH_Status Snake_FlashErasePage(u32 page_addr)
{
    FLASH_Status status = Snake_FlashWaitReady();

    if (status != FLASH_COMPLETE) {
        return status;
    }

    Snake_FlashClearStatus();
    FLASH->CR |= FLASH_CR_PER;
    FLASH->AR = page_addr;
    FLASH->CR |= FLASH_CR_STRT;

    status = Snake_FlashWaitReady();
    FLASH->CR &= (u32)(~FLASH_CR_PER);
    Snake_FlashClearStatus();
    return status;
}

static FLASH_Status Snake_FlashProgramHalfWord(u32 address, u16 data)
{
    FLASH_Status status = Snake_FlashWaitReady();

    if (status != FLASH_COMPLETE) {
        return status;
    }

    Snake_FlashClearStatus();
    FLASH->CR |= FLASH_CR_PG;
    *(volatile u16 *)address = data;

    status = Snake_FlashWaitReady();
    FLASH->CR &= (u32)(~FLASH_CR_PG);
    Snake_FlashClearStatus();

    if (status != FLASH_COMPLETE) {
        return status;
    }

    if (*(volatile u16 *)address != data) {
        return FLASH_ERROR_PG;
    }

    return FLASH_COMPLETE;
}

static FLASH_Status Snake_FlashProgramWord(u32 address, u32 data)
{
    FLASH_Status status;

    status = Snake_FlashProgramHalfWord(address, (u16)(data & 0xffffu));
    if (status != FLASH_COMPLETE) {
        return status;
    }

    return Snake_FlashProgramHalfWord(address + 2,
                                      (u16)((data >> 16) & 0xffffu));
}

static void Snake_PersistLoad(void)
{
    const SnakePersistRecord *record =
        (const SnakePersistRecord *)SNAKE_FLASH_STORE_ADDR;
    u32 settings;
    u8 saved_volume;
    u8 saved_mode;

    if (record->magic != SNAKE_FLASH_MAGIC) {
        Snake_PersistRemember();
        return;
    }

    if (record->checksum !=
        Snake_PersistBuildChecksum(record->high_score, record->settings)) {
        Snake_PersistRemember();
        return;
    }

    settings = record->settings;
    saved_volume = (u8)(settings & 0xffu);
    saved_mode = (u8)((settings >> 8) & 0xffu);

    if (saved_volume > VOLUME_MAX) {
        saved_volume = VOLUME_DEFAULT;
    }
    if (saved_mode >= GAME_MODE_COUNT) {
        saved_mode = GAME_MODE_STAGE;
    }

    high_score = (u16)((record->high_score > 999u) ? 999u : record->high_score);
    sound_volume = saved_volume;
    game_mode = saved_mode;
    Snake_PersistRemember();
}

static void Snake_PersistSave(void)
{
    SnakePersistRecord record;
    const SnakePersistRecord *saved =
        (const SnakePersistRecord *)SNAKE_FLASH_STORE_ADDR;
    FLASH_Status status;
    u32 settings;

    if (!persist_dirty) {
        return;
    }

    settings = Snake_PersistBuildSettings();
    record.magic = SNAKE_FLASH_MAGIC;
    record.high_score = high_score;
    record.settings = settings;
    record.checksum = Snake_PersistBuildChecksum(record.high_score,
                                                 record.settings);

    if (saved->magic == record.magic &&
        saved->high_score == record.high_score &&
        saved->settings == record.settings &&
        saved->checksum == record.checksum) {
        Snake_PersistRemember();
        return;
    }

    Snake_FlashUnlock();
    Snake_FlashClearStatus();

    status = Snake_FlashErasePage(SNAKE_FLASH_STORE_ADDR);
    if (status == FLASH_COMPLETE) {
        status = Snake_FlashProgramWord(SNAKE_FLASH_STORE_ADDR + 0,
                                        record.magic);
    }
    if (status == FLASH_COMPLETE) {
        status = Snake_FlashProgramWord(SNAKE_FLASH_STORE_ADDR + 4,
                                        record.high_score);
    }
    if (status == FLASH_COMPLETE) {
        status = Snake_FlashProgramWord(SNAKE_FLASH_STORE_ADDR + 8,
                                        record.settings);
    }
    if (status == FLASH_COMPLETE) {
        status = Snake_FlashProgramWord(SNAKE_FLASH_STORE_ADDR + 12,
                                        record.checksum);
    }

    Snake_FlashLock();

    if (status == FLASH_COMPLETE) {
        Snake_PersistRemember();
    }
}

static u8 Snake_WorldCols(void)
{
    return Snake_IsOpenMode() ? OPEN_WORLD_COLS : GRID_COLS;
}

static u8 Snake_WorldRows(void)
{
    return Snake_IsOpenMode() ? OPEN_WORLD_ROWS : GRID_ROWS;
}

static u8 Snake_IsReverse(SnakeDir from, SnakeDir to)
{
    return (u8)((from == DIR_UP && to == DIR_DOWN) ||
                (from == DIR_DOWN && to == DIR_UP) ||
                (from == DIR_LEFT && to == DIR_RIGHT) ||
                (from == DIR_RIGHT && to == DIR_LEFT));
}

static u8 Snake_CommandToDir(SnakeUartCmdType type, SnakeDir *want)
{
    if (type == SNAKE_UART_CMD_UP || type == SNAKE_UART_CMD_P2_UP) {
        *want = DIR_UP;
    } else if (type == SNAKE_UART_CMD_DOWN || type == SNAKE_UART_CMD_P2_DOWN) {
        *want = DIR_DOWN;
    } else if (type == SNAKE_UART_CMD_LEFT || type == SNAKE_UART_CMD_P2_LEFT) {
        *want = DIR_LEFT;
    } else if (type == SNAKE_UART_CMD_RIGHT || type == SNAKE_UART_CMD_P2_RIGHT) {
        *want = DIR_RIGHT;
    } else {
        return 0;
    }

    return 1;
}

static u8 Snake_DuoKeypadToDir(u8 value, SnakeDir *want)
{
    if (value == 0) {
        *want = DIR_LEFT;   /* 1 */
    } else if (value == 1) {
        *want = DIR_DOWN;   /* 2 */
    } else if (value == 2) {
        *want = DIR_RIGHT;  /* 3 */
    } else if (value == 4) {
        *want = DIR_UP;     /* 5 */
    } else {
        return 0;
    }

    return 1;
}

static void Snake_SetP1Direction(SnakeDir want)
{
    if (paused || turn_pending || want == next_dir || Snake_IsReverse(dir, want)) {
        return;
    }

    next_dir = want;
    turn_pending = 1;
}

static void Snake_SetP2Direction(SnakeDir want)
{
    if (paused || turn_pending2 || want == next_dir2 ||
        Snake_IsReverse(dir2, want)) {
        return;
    }

    next_dir2 = want;
    turn_pending2 = 1;
}

static void Snake_TogglePause(void)
{
    paused = (u8)!paused;
    pause_lock = 1;
    key_press_latch = 0;
    if (paused) {
        Snake_AudioStop();
    } else if (music_left_ms != 0) {
        Snake_AudioSet(music_freq);
    }
    Snake_SetStatus(paused ? "PAUSED" : "RUNNING");
    Snake_DrawHeader();
    if (paused) {
        Snake_DrawPauseHint();
    } else {
        Snake_Render();
    }
    Snake_Beep(35);
}

static void Snake_SelectLevel(u8 next_level, const char *msg)
{
    Snake_StartLevel(next_level);
    paused = 1;
    Snake_AudioStop();
    Snake_SetStatus(msg);
    Snake_DrawHeader();
    Snake_Beep(35);
}

static void Snake_ApplySerialCommand(const SnakeUartCmd *cmd)
{
    SnakeDir want = next_dir;

    if (cmd->type == SNAKE_UART_CMD_PAUSE) {
        Snake_TogglePause();
        return;
    }

    if (cmd->type == SNAKE_UART_CMD_START) {
        Snake_TogglePause();
        return;
    }

    if (cmd->type == SNAKE_UART_CMD_RESET) {
        if (paused) {
            return_home_request = 1;
            Snake_SetStatus("EXIT HOME");
            Snake_DrawHeader();
            Snake_DrawPauseHint();
        } else {
            restart_request = 1;
            Snake_SetStatus("UART RESET");
            Snake_DrawHeader();
        }
        return;
    }

    if (cmd->type == SNAKE_UART_CMD_LEVEL) {
        if (Snake_IsDuoMode()) {
            if (Snake_DuoKeypadToDir(cmd->value, &want)) {
                Snake_SetP2Direction(want);
            }
        } else if (cmd->value < LEVEL_COUNT) {
            Snake_SelectLevel(cmd->value, "UART LEVEL");
        }
        return;
    }

    if (!Snake_CommandToDir(cmd->type, &want)) {
        return;
    }

    if (cmd->type == SNAKE_UART_CMD_P2_UP ||
        cmd->type == SNAKE_UART_CMD_P2_DOWN ||
        cmd->type == SNAKE_UART_CMD_P2_LEFT ||
        cmd->type == SNAKE_UART_CMD_P2_RIGHT) {
        Snake_SetP2Direction(want);
    } else {
        Snake_SetP1Direction(want);
    }
}

static void Snake_HandleSerialInput(void)
{
    SnakeUartCmd cmd;

    while (SnakeUart_PopCommand(&cmd)) {
        Snake_ApplySerialCommand(&cmd);
    }
}

static u8 Snake_HandleHomeSerialInput(u8 *selected_level, u8 *selected_mode,
                                      u8 *open_settings)
{
    SnakeUartCmd cmd;
    u8 should_start = 0;
    u8 selected = *selected_level;

    while (SnakeUart_PopCommand(&cmd)) {
        if (cmd.type == SNAKE_UART_CMD_LEVEL) {
            if (cmd.value < LEVEL_COUNT) {
                selected = cmd.value;
            }
        } else if (cmd.type == SNAKE_UART_CMD_LEFT) {
            selected = (selected == 0) ? (LEVEL_COUNT - 1) : (u8)(selected - 1);
        } else if (cmd.type == SNAKE_UART_CMD_RIGHT) {
            selected = (u8)((selected + 1) % LEVEL_COUNT);
        } else if (cmd.type == SNAKE_UART_CMD_START) {
            should_start = 1;
        } else if (cmd.type == SNAKE_UART_CMD_PAUSE) {
            *open_settings = 1;
        } else if (cmd.type == SNAKE_UART_CMD_UP ||
                   cmd.type == SNAKE_UART_CMD_DOWN) {
            *selected_mode = (u8)((*selected_mode + 1) % GAME_MODE_COUNT);
        }
    }

    *selected_level = selected;
    return should_start;
}

static u8 Snake_HandleSettingsSerialInput(void)
{
    SnakeUartCmd cmd;
    u8 should_return = 0;

    while (SnakeUart_PopCommand(&cmd)) {
        if (cmd.type == SNAKE_UART_CMD_LEFT) {
            if (sound_volume > 0) {
                sound_volume--;
            }
        } else if (cmd.type == SNAKE_UART_CMD_RIGHT) {
            if (sound_volume < VOLUME_MAX) {
                sound_volume++;
            }
        } else if (cmd.type == SNAKE_UART_CMD_START ||
                   cmd.type == SNAKE_UART_CMD_PAUSE ||
                   cmd.type == SNAKE_UART_CMD_RESET) {
            should_return = 1;
        }
    }

    return should_return;
}

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

static void Snake_ADCConfiguration(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    ADC_InitTypeDef ADC_InitStructure;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_AFIO |
                           RCC_APB2Periph_ADC1, ENABLE);
    RCC_ADCCLKConfig(RCC_PCLK2_Div6);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    ADC_DeInit(ADC1);
    ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;
    ADC_InitStructure.ADC_ScanConvMode = DISABLE;
    ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;
    ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
    ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
    ADC_InitStructure.ADC_NbrOfChannel = 1;
    ADC_Init(ADC1, &ADC_InitStructure);

    ADC_Cmd(ADC1, ENABLE);
    ADC_ResetCalibration(ADC1);
    while (ADC_GetResetCalibrationStatus(ADC1)) {
    }
    ADC_StartCalibration(ADC1);
    while (ADC_GetCalibrationStatus(ADC1)) {
    }
    ADC_SoftwareStartConvCmd(ADC1, ENABLE);
}

static u16 Snake_ReadAdcChannel(u8 channel)
{
    u8 i;
    u32 sum = 0;

    for (i = 0; i < 4; i++) {
        ADC_RegularChannelConfig(ADC1, channel, 1,
                                 ADC_SampleTime_239Cycles5);
        ADC_SoftwareStartConvCmd(ADC1, ENABLE);
        while (ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC) == RESET) {
        }
        sum += ADC_GetConversionValue(ADC1);
    }

    return (u16)(sum / 4);
}

static u16 Snake_KnobRead(void)
{
    return Snake_ReadAdcChannel(knob_adc_channel);
}

static u8 Snake_KnobLevel(void)
{
    u8 lv = (u8)(Snake_KnobRead() / KNOB_LEVEL_STEP);

    if (lv >= LEVEL_COUNT) {
        lv = LEVEL_COUNT - 1;
    }

    return lv;
}

static void Snake_KnobReset(void)
{
    knob_last_value = Snake_KnobRead();
    knob_event_latch = 0;
}

static void Snake_KnobScan(void)
{
    u16 value = Snake_KnobRead();

    if ((u16)(value + KNOB_TURN_DELTA) < knob_last_value) {
        knob_event_latch = KNOB_LEFT_EVENT;
        knob_last_value = value;
    } else if (value > (u16)(knob_last_value + KNOB_TURN_DELTA)) {
        knob_event_latch = KNOB_RIGHT_EVENT;
        knob_last_value = value;
    }
}

static u8 Snake_KnobPopEvent(void)
{
    u8 event = knob_event_latch;

    knob_event_latch = 0;
    return event;
}

static void Snake_AudioUpdateWave(void)
{
    u8 i;
    s32 delta;
    u16 value;

    if (audio_wave_volume == sound_volume) {
        return;
    }

    for (i = 0; i < AUDIO_WAVE_POINTS; i++) {
        delta = (s32)audio_sine_base[i] - AUDIO_DAC_CENTER;
        value = (u16)(AUDIO_DAC_CENTER +
                      (delta * sound_volume) / VOLUME_MAX);
        audio_sine_buffer[i] = value;
    }

    audio_wave_volume = sound_volume;
}

static void Snake_AudioStop(void)
{
    TIM_Cmd(TIM3, DISABLE);
    DAC_SetChannel2Data(DAC_Align_12b_R, AUDIO_DAC_CENTER);
    audio_playing = 0;
}

static void Snake_AudioSet(u16 freq)
{
    u32 period;

    if (freq == NOTE_REST || freq == 0 || sound_volume == 0) {
        Snake_AudioStop();
        return;
    }

    Snake_AudioUpdateWave();

    period = (SystemCoreClock / (AUDIO_WAVE_POINTS * (u32)freq));
    if (period < 2) {
        period = 2;
    }

    TIM_Cmd(TIM3, DISABLE);
    TIM_SetAutoreload(TIM3, (u16)(period - 1));
    TIM_SetCounter(TIM3, 0);
    TIM_Cmd(TIM3, ENABLE);
    audio_playing = 1;
}

static void Snake_AudioInit(void)
{
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
    GPIO_InitTypeDef GPIO_InitStructure;
    DAC_InitTypeDef DAC_InitStructure;
    DMA_InitTypeDef DMA_InitStructure;

    RCC_APB1PeriphClockCmd(RCC_APB1Periph_DAC | RCC_APB1Periph_TIM3, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_AFIO, ENABLE);
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA2, ENABLE);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    Snake_AudioUpdateWave();

    DMA_DeInit(DMA2_Channel4);
    DMA_InitStructure.DMA_PeripheralBaseAddr = AUDIO_DAC_DHR12R2;
    DMA_InitStructure.DMA_MemoryBaseAddr = (u32)audio_sine_buffer;
    DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralDST;
    DMA_InitStructure.DMA_BufferSize = AUDIO_WAVE_POINTS;
    DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
    DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;
    DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;
    DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;
    DMA_InitStructure.DMA_Priority = DMA_Priority_High;
    DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
    DMA_Init(DMA2_Channel4, &DMA_InitStructure);
    DMA_Cmd(DMA2_Channel4, ENABLE);

    DAC_DeInit();
    DAC_InitStructure.DAC_Trigger = DAC_Trigger_T3_TRGO;
    DAC_InitStructure.DAC_WaveGeneration = DAC_WaveGeneration_None;
    DAC_InitStructure.DAC_LFSRUnmask_TriangleAmplitude = DAC_LFSRUnmask_Bit0;
    DAC_InitStructure.DAC_OutputBuffer = DAC_OutputBuffer_Disable;
    DAC_Init(DAC_Channel_2, &DAC_InitStructure);
    DAC_Cmd(DAC_Channel_2, ENABLE);
    DAC_DMACmd(DAC_Channel_2, ENABLE);
    DAC_SetChannel2Data(DAC_Align_12b_R, AUDIO_DAC_CENTER);

    TIM_DeInit(TIM3);
    TIM_InternalClockConfig(TIM3);

    TIM_TimeBaseStructure.TIM_Period = 1000 - 1;
    TIM_TimeBaseStructure.TIM_Prescaler = 0;
    TIM_TimeBaseStructure.TIM_ClockDivision = 0;
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure);
    TIM_ARRPreloadConfig(TIM3, ENABLE);
    TIM_SelectOutputTrigger(TIM3, TIM_TRGOSource_Update);

    Snake_AudioStop();
}

static void Snake_PlayTone(u16 freq, u16 ms)
{
    u16 saved_freq = music_freq;

    if (ms == 0 || sound_volume == 0) {
        return;
    }

    Snake_AudioUpdateWave();
    Snake_AudioSet(freq);
    Delay_ms(ms);
    Snake_AudioSet(saved_freq);
}

static void Snake_MusicSelect(const MusicNote *notes, u16 count)
{
    music_notes = notes;
    music_count = count;
    music_index = 0;
    music_left_ms = 0;
    music_gap_ms = 0;
    music_freq = NOTE_REST;
    Snake_AudioStop();
}

static void Snake_MusicSetLevel(u8 lv)
{
    if (lv >= LEVEL_COUNT) {
        lv = LEVEL_COUNT - 1;
    }

    Snake_MusicSelect(level_music[lv].notes, level_music[lv].count);
}

static void Snake_MusicLoadNextNote(void)
{
    if (music_notes == 0 || music_count == 0) {
        music_freq = NOTE_REST;
        music_left_ms = 80;
        music_gap_ms = 0;
        Snake_AudioStop();
        return;
    }

    music_freq = music_notes[music_index].freq;
    music_left_ms = music_notes[music_index].ms;
    music_gap_ms = 0;
    if (music_freq != NOTE_REST) {
        music_gap_ms = (music_left_ms >= 300) ? 24 : 10;
    }
    Snake_AudioSet(music_freq);
    music_index++;
    if (music_index >= music_count) {
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

        if (music_gap_ms != 0 && music_left_ms <= music_gap_ms) {
            Snake_AudioStop();
        }
        Delay_ms(chunk);
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

static void Snake_DrawHomeLevels(u8 selected)
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

    x = (u16)(30 + selected * 36);
    Snake_DrawHomeFrame((u16)(x - 4), 196, (u16)(x + 28), 226, HOME_GOLD);
}

static void Snake_DrawHomePrompt(u8 visible)
{
    LCD_Fill(22, 278, 217, 309, HOME_NAVY);
    Snake_DrawHomeFrame(22, 278, 217, 309, visible ? HOME_GOLD : HOME_BLUE);

    if (visible) {
        Snake_ShowTextCenter(286, 16, "PRESS ANY KEY", YELLOW, HOME_NAVY, 1);
    }
}

static void Snake_DrawHomeMode(u8 selected_mode)
{
    LCD_Fill(18, 236, 122, 270, HOME_PANEL);
    Snake_DrawHomeFrame(18, 236, 122, 270,
                        selected_mode == GAME_MODE_STAGE ? HOME_TEAL : HOME_GOLD);
    if (selected_mode == GAME_MODE_CLASSIC) {
        Snake_ShowText(31, 244, 12, "MODE CLASSIC", WHITE, HOME_PANEL, 1);
    } else if (selected_mode == GAME_MODE_DUO) {
        Snake_ShowText(43, 244, 12, "MODE DUO", WHITE, HOME_PANEL, 1);
    } else if (selected_mode == GAME_MODE_OPEN) {
        Snake_ShowText(39, 244, 12, "MODE OPEN", WHITE, HOME_PANEL, 1);
    } else if (selected_mode == GAME_MODE_OPEN_DUO) {
        Snake_ShowText(25, 244, 12, "MODE OPENDUO", WHITE, HOME_PANEL, 1);
    } else {
        Snake_ShowText(37, 244, 12, "MODE STAGE", WHITE, HOME_PANEL, 1);
    }
}

static void Snake_DrawHomeSettingsButton(void)
{
    LCD_Fill(122, 236, 221, 270, HOME_PANEL);
    Snake_DrawHomeFrame(122, 236, 221, 270, HOME_TEAL);
    Snake_ShowText(132, 244, 12, "KEY4 SETTINGS", WHITE, HOME_PANEL, 1);
}

static void Snake_DrawVolumeBar(void)
{
    u8 i;
    u16 x;

    LCD_Fill(42, 154, 197, 210, BLACK);
    for (i = 0; i < VOLUME_MAX; i++) {
        x = (u16)(50 + i * 30);
        LCD_Fill(x, 176, (u16)(x + 20), 202,
                 (i < sound_volume) ? HOME_GOLD : HOME_PANEL);
        POINT_COLOR = WHITE;
        LCD_DrawRectangle(x, 176, (u16)(x + 20), 202);
    }

    POINT_COLOR = WHITE;
    BACK_COLOR = BLACK;
    LCD_ShowString(78, 154, 16, (u8 *)"Volume", 0);
    LCD_ShowNum(150, 154, sound_volume, 1, 16);
}

static void Snake_ShowSettings(void)
{
    u8 last_volume = 0xff;
    u8 raw;
    u16 knob_value;
    u16 last_knob_value;

    LCD_Clear(BLACK);
    LCD_Fill(0, 0, LCD_W - 1, 76, HOME_NAVY);
    Snake_ShowTextCenter(22, 16, "SETTINGS", WHITE, HOME_NAVY, 1);
    Snake_ShowTextCenter(48, 12, "A/D or knob volume", CYAN, HOME_NAVY, 1);
    Snake_DrawVolumeBar();
    Snake_ShowTextCenter(250, 12, "Space/P return", LGRAY, BLACK, 1);

    while (Snake_KeyReadRaw() != 0) {
        Delay_ms(20);
    }

    last_knob_value = Snake_KnobRead();

    while (1) {
        knob_value = Snake_KnobRead();
        if ((u16)(knob_value + KNOB_TURN_DELTA) < last_knob_value ||
            knob_value > (u16)(last_knob_value + KNOB_TURN_DELTA)) {
            sound_volume = (u8)((knob_value * (VOLUME_MAX + 1UL)) / 4096UL);
            if (sound_volume > VOLUME_MAX) {
                sound_volume = VOLUME_MAX;
            }
            last_knob_value = knob_value;
        }

        if (Snake_HandleSettingsSerialInput()) {
            Snake_PersistSave();
            LCD_Clear(BLACK);
            Snake_KeyReset();
            Snake_KnobReset();
            return;
        }

        if (sound_volume != last_volume) {
            Snake_PersistMarkDirty();
            Snake_DrawVolumeBar();
            last_volume = sound_volume;
            Snake_AudioUpdateWave();
            if (audio_playing && music_freq != NOTE_REST) {
                Snake_AudioSet(music_freq);
            }
            if (sound_volume != 0) {
                Snake_PlayTone(NOTE_C6, 25);
            }
        }

        raw = Snake_KeyReadRaw();
        if (raw != 0) {
            while (Snake_KeyReadRaw() != 0) {
                Delay_ms(20);
            }
            Snake_PersistSave();
            LCD_Clear(BLACK);
            Snake_KeyReset();
            Snake_KnobReset();
            return;
        }

        Delay_ms(40);
    }
}

static void Snake_ShowHome(void)
{
    u8 blink = 1;
    u8 i;
    u8 selected = Snake_KnobLevel();
    u8 selected_mode = game_mode;
    u8 last_selected = 0xff;
    u8 last_mode = 0xff;
    u16 adc_value;
    u16 last_adc_value;
    u8 raw_key;
    u8 open_settings;

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
    Snake_DrawHomeLevels(selected);
    Snake_DrawHomeMode(selected_mode);

    Snake_ShowTextCenter(250, 12, "A/D level  W/S mode  Space start",
                         LGRAY, BLACK, 1);
    Snake_DrawHomeSettingsButton();
    Snake_DrawHomePrompt(blink);
    Snake_MusicSelect(music_home, MUSIC_COUNT(music_home));

    while (Snake_KeyReadRaw() != 0) {
        Snake_MusicTick(20);
    }

    last_adc_value = Snake_KnobRead();

    while (1) {
        for (i = 0; i < 25; i++) {
            adc_value = Snake_KnobRead();
            if ((u16)(adc_value + KNOB_TURN_DELTA) < last_adc_value ||
                adc_value > (u16)(last_adc_value + KNOB_TURN_DELTA)) {
                selected = (u8)(adc_value / KNOB_LEVEL_STEP);
                if (selected >= LEVEL_COUNT) {
                    selected = LEVEL_COUNT - 1;
                }
                last_adc_value = adc_value;
            }

            open_settings = 0;
            if (Snake_HandleHomeSerialInput(&selected, &selected_mode,
                                            &open_settings)) {
                start_level = selected;
                if (game_mode != selected_mode) {
                    Snake_PersistMarkDirty();
                }
                game_mode = selected_mode;
                Snake_PersistSave();
                Snake_Beep(60);
                Delay_ms(120);
                LCD_Clear(BLACK);
                Snake_AudioStop();
                Snake_KeyReset();
                return;
            }

            if (open_settings) {
                Snake_AudioStop();
                Snake_ShowSettings();
                LCD_Clear(BLACK);
                Snake_ShowHome();
                return;
            }

            if (selected != last_selected) {
                Snake_DrawHomeLevels(selected);
                last_selected = selected;
            }

            if (selected_mode != last_mode) {
                Snake_DrawHomeMode(selected_mode);
                last_mode = selected_mode;
            }

            raw_key = Snake_KeyReadRaw();
            if (raw_key & KEY_RIGHT_MASK) {
                Snake_AudioStop();
                Snake_ShowSettings();
                LCD_Clear(BLACK);
                Snake_ShowHome();
                return;
            }

            if (raw_key != 0) {
                Snake_Beep(60);
                while (Snake_KeyReadRaw() != 0) {
                    Snake_MusicTick(20);
                }
                start_level = selected;
                if (game_mode != selected_mode) {
                    Snake_PersistMarkDirty();
                }
                game_mode = selected_mode;
                Snake_PersistSave();
                Delay_ms(120);
                LCD_Clear(BLACK);
                Snake_AudioStop();
                Snake_KeyReset();
                return;
            }
            Snake_MusicTick(20);
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
    if (Snake_IsOpenMode()) {
        if (x >= OPEN_WORLD_COLS || y >= OPEN_WORLD_ROWS) {
            return '#';
        }

        if (x == 0 || y == 0 ||
            x == (OPEN_WORLD_COLS - 1) || y == (OPEN_WORLD_ROWS - 1)) {
            return '#';
        }

        if (((x == 8 || x == 31) && y > 5 && y < 50 && (y % 7) != 0) ||
            ((y == 12 || y == 39) && x > 4 && x < 35 && (x % 8) != 0)) {
            return '#';
        }

        if (((x > 4 && x < 14) && (y == 24 || y == 25) && x != 9) ||
            ((x > 25 && x < 36) && (y == 28 || y == 29) && x != 31)) {
            return '#';
        }

        if (((x > 14 && x < 25) && (y == 6 || y == 49) && (x % 5) != 0) ||
            ((y > 16 && y < 23) && (x == 17 || x == 22) && y != 19)) {
            return '#';
        }

        if (((x + y) % 17) == 0 && x > 4 && x < 36 && y > 5 && y < 51) {
            return '#';
        }

        if ((x == 12 && y == 18) || (x == 27 && y == 43)) {
            return 'A';
        }
        if ((x == 34 && y == 9) || (x == 5 && y == 46)) {
            return 'B';
        }

        return '.';
    }

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

static u8 Snake2_OnSnake(u8 x, u8 y, u16 limit)
{
    u16 i;

    for (i = 0; i < limit; i++) {
        if (snake2_x[i] == x && snake2_y[i] == y) {
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

    if (Snake_OnSnake(x, y, snake_len)) {
        return 0;
    }

    if (Snake_IsDuoMode() && Snake2_OnSnake(x, y, snake2_len)) {
        return 0;
    }

    return 1;
}

static u16 Snake_CountWalkableCells(void)
{
    u8 x;
    u8 y;
    u16 count = 0;
    u8 cols = Snake_WorldCols();
    u8 rows = Snake_WorldRows();

    for (y = 0; y < rows; y++) {
        for (x = 0; x < cols; x++) {
            if (Snake_MapCell(x, y) == '.') {
                count++;
            }
        }
    }

    return count;
}

static void Snake_DrawCell(u8 x, u8 y, u16 color)
{
    u16 px = BOARD_X + (u16)x * CELL_W;
    u16 py = BOARD_Y + (u16)y * CELL_H;

    LCD_Fill(px + 1, py + 1, px + CELL_W - 2, py + CELL_H - 2, color);
}

static void Snake_DrawCellAt(u16 board_x, u16 board_y, u8 x, u8 y, u16 color)
{
    u16 px = board_x + (u16)x * CELL_W;
    u16 py = board_y + (u16)y * CELL_H;

    LCD_Fill(px + 1, py + 1, px + CELL_W - 2, py + CELL_H - 2, color);
}

static u8 Snake_WorldToViewAt(u8 wx, u8 wy, u8 base_x, u8 base_y,
                              u8 view_rows, u8 *vx, u8 *vy)
{
    if (wx < base_x || wy < base_y) {
        return 0;
    }

    wx = (u8)(wx - base_x);
    wy = (u8)(wy - base_y);
    if (wx >= GRID_COLS || wy >= view_rows) {
        return 0;
    }

    *vx = wx;
    *vy = wy;
    return 1;
}

static u8 Snake_WorldToView(u8 wx, u8 wy, u8 *vx, u8 *vy)
{
    return Snake_WorldToViewAt(wx, wy, viewport_x, viewport_y,
                               GRID_ROWS, vx, vy);
}

static void Snake_DrawWorldCell(u8 x, u8 y, u16 color)
{
    u8 vx;
    u8 vy;

    if (Snake_WorldToView(x, y, &vx, &vy)) {
        Snake_DrawCell(vx, vy, color);
    }
}

static void Snake_DrawWorldCellAt(u16 board_x, u16 board_y, u8 view_x,
                                  u8 view_y, u8 view_rows,
                                  u8 x, u8 y, u16 color)
{
    u8 vx;
    u8 vy;

    if (Snake_WorldToViewAt(x, y, view_x, view_y, view_rows, &vx, &vy)) {
        Snake_DrawCellAt(board_x, board_y, vx, vy, color);
    }
}

static void Snake_DrawMapCell(u8 x, u8 y)
{
    char c = Snake_MapCell(x, y);

    if (c == '#') {
        Snake_DrawWorldCell(x, y, GRAY);
    } else if (c == 'A') {
        Snake_DrawWorldCell(x, y, CYAN);
    } else if (c == 'B') {
        Snake_DrawWorldCell(x, y, MAGENTA);
    } else {
        Snake_DrawWorldCell(x, y, BLACK);
    }
}

static void Snake_DrawMapCellAt(u16 board_x, u16 board_y, u8 view_x,
                                u8 view_y, u8 view_rows, u8 x, u8 y)
{
    char c = Snake_MapCell(x, y);
    u16 color = BLACK;

    if (c == '#') {
        color = GRAY;
    } else if (c == 'A') {
        color = CYAN;
    } else if (c == 'B') {
        color = MAGENTA;
    }

    Snake_DrawWorldCellAt(board_x, board_y, view_x, view_y, view_rows,
                          x, y, color);
}

static void Snake_DrawFood(void)
{
    if (food_type == FOOD_POISON) {
        Snake_DrawWorldCell(food_x, food_y, MAGENTA);
    } else if (food_type == FOOD_BONUS) {
        Snake_DrawWorldCell(food_x, food_y, CYAN);
    } else {
        Snake_DrawWorldCell(food_x, food_y, RED);
    }
}

static void Snake_DrawFoodAt(u16 board_x, u16 board_y, u8 view_x,
                             u8 view_y, u8 view_rows)
{
    u16 color = RED;

    if (food_type == FOOD_POISON) {
        color = MAGENTA;
    } else if (food_type == FOOD_BONUS) {
        color = CYAN;
    }

    Snake_DrawWorldCellAt(board_x, board_y, view_x, view_y, view_rows,
                          food_x, food_y, color);
}

static void Snake_SaveRenderState(void)
{
    prev_snake_len = snake_len;
    if (snake_len != 0) {
        prev_snake_head_x = snake_x[0];
        prev_snake_head_y = snake_y[0];
        prev_snake_tail_x = snake_x[snake_len - 1];
        prev_snake_tail_y = snake_y[snake_len - 1];
    }
    prev_snake2_len = snake2_len;
    if (snake2_len != 0) {
        prev_snake2_head_x = snake2_x[0];
        prev_snake2_head_y = snake2_y[0];
        prev_snake2_tail_x = snake2_x[snake2_len - 1];
        prev_snake2_tail_y = snake2_y[snake2_len - 1];
    }
    prev_food_x = food_x;
    prev_food_y = food_y;
    prev_viewport_x = viewport_x;
    prev_viewport_y = viewport_y;
    prev_viewport2_x = viewport2_x;
    prev_viewport2_y = viewport2_y;
}

static void Snake_UpdateOneViewport(u8 head_x, u8 head_y, u8 view_rows,
                                    u8 *view_x, u8 *view_y)
{
    u8 cols = Snake_WorldCols();
    u8 rows = Snake_WorldRows();
    u8 max_x = (cols > GRID_COLS) ? (u8)(cols - GRID_COLS) : 0;
    u8 max_y = (rows > view_rows) ? (u8)(rows - view_rows) : 0;
    u8 y_margin = (view_rows > 10) ? 5 : 2;

    if (head_x < *view_x + 4) {
        *view_x = (head_x > 4) ? (u8)(head_x - 4) : 0;
    } else if (head_x >= *view_x + GRID_COLS - 4) {
        *view_x = (u8)(head_x - GRID_COLS + 5);
    }

    if (head_y < *view_y + y_margin) {
        *view_y = (head_y > y_margin) ? (u8)(head_y - y_margin) : 0;
    } else if (head_y >= *view_y + view_rows - y_margin) {
        *view_y = (u8)(head_y - view_rows + y_margin + 1);
    }

    if (*view_x > max_x) {
        *view_x = max_x;
    }
    if (*view_y > max_y) {
        *view_y = max_y;
    }
}

static void Snake_UpdateViewport(void)
{
    if (!Snake_IsOpenMode()) {
        viewport_x = 0;
        viewport_y = 0;
        viewport2_x = 0;
        viewport2_y = 0;
        return;
    }

    Snake_UpdateOneViewport(snake_x[0], snake_y[0],
                            Snake_IsOpenDuoMode() ? OPEN_DUO_ROWS : GRID_ROWS,
                            &viewport_x, &viewport_y);
    if (Snake_IsOpenDuoMode()) {
        Snake_UpdateOneViewport(snake2_x[0], snake2_y[0],
                                OPEN_DUO_ROWS, &viewport2_x, &viewport2_y);
    } else {
        viewport2_x = viewport_x;
        viewport2_y = viewport_y;
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

    if (Snake_IsDuoMode()) {
        LCD_ShowString(6, 24, 16, (u8 *)"P1:", 0);
        LCD_ShowNum(38, 24, score, 3, 16);
        LCD_ShowString(88, 24, 16, (u8 *)"P2:", 0);
        LCD_ShowNum(120, 24, score2, 3, 16);
    } else {
        LCD_ShowString(6, 24, 16, (u8 *)"S:", 0);
        LCD_ShowNum(28, 24, score, 3, 16);
        LCD_ShowString(70, 24, 16, (u8 *)"B:", 0);
        LCD_ShowNum(92, 24, high_score, 3, 16);
        LCD_ShowString(134, 24, 16, (u8 *)"HP:", 0);
        LCD_ShowNum(166, 24, lives, 1, 16);
    }
    if (Snake_IsOpenMode()) {
        LCD_ShowString(176, 24, 16, (u8 *)(Snake_IsOpenDuoMode() ? "V1:" : "M:"), 0);
        LCD_ShowNum(204, 24, viewport_x, 2, 16);
        LCD_ShowString(220, 24, 16, (u8 *)",", 0);
        LCD_ShowNum(228, 24, viewport_y, 2, 16);
    } else {
        LCD_ShowString(190, 24, 16, (u8 *)"T:", 0);
        if (game_mode == GAME_MODE_CLASSIC || Snake_IsDuoMode() ||
            level_time_limit[level_index] == 0) {
            LCD_ShowString(212, 24, 16, (u8 *)"--", 0);
        } else {
            LCD_ShowNum(212, 24, time_left, 2, 16);
        }
    }

    LCD_ShowString(6, 48, 16, (u8 *)status_msg, 0);
}

static void Snake_DrawPauseHint(void)
{
    LCD_Fill(12, 148, LCD_W - 13, 186, BLACK);
    Snake_ShowTextCenter(160, 16, "Press KEY1 or R to exit", WHITE, BLACK, 0);
}

static void Snake_RenderViewportAt(u16 board_x, u16 board_y, u8 view_x,
                                   u8 view_y, u8 view_rows)
{
    u8 x;
    u8 y;
    u8 wx;
    u8 wy;

    for (y = 0; y < view_rows; y++) {
        for (x = 0; x < GRID_COLS; x++) {
            wx = (u8)(view_x + x);
            wy = (u8)(view_y + y);
            Snake_DrawMapCellAt(board_x, board_y, view_x, view_y,
                                view_rows, wx, wy);
        }
    }

    POINT_COLOR = GRAY;
    LCD_DrawRectangle(board_x - 1, board_y - 1,
                      board_x + GRID_COLS * CELL_W,
                      board_y + view_rows * CELL_H);
}

static void Snake_RenderBoard(void)
{
    u8 x;
    u8 y;
    u8 wx;
    u8 wy;

    if (!Snake_IsOpenMode()) {
        LCD_Fill(BOARD_X, BOARD_Y, BOARD_X + GRID_COLS * CELL_W - 1,
                 BOARD_Y + GRID_ROWS * CELL_H - 1, BLACK);
    }

    for (y = 0; y < GRID_ROWS; y++) {
        for (x = 0; x < GRID_COLS; x++) {
            wx = (u8)(viewport_x + x);
            wy = (u8)(viewport_y + y);
            Snake_DrawMapCell(wx, wy);
        }
    }

    POINT_COLOR = GRAY;
    LCD_DrawRectangle(BOARD_X - 1, BOARD_Y - 1,
                      BOARD_X + GRID_COLS * CELL_W,
                      BOARD_Y + GRID_ROWS * CELL_H);
}

static void Snake_DrawOpenDuoLabels(void)
{
    POINT_COLOR = WHITE;
    BACK_COLOR = BLACK;
    LCD_ShowString(4, OPEN_DUO_P1_Y + 38, 16, (u8 *)"P1", 0);
    LCD_ShowString(4, OPEN_DUO_P2_Y + 38, 16, (u8 *)"P2", 0);
}

static void Snake_DrawSnakeOnViewport(u16 board_x, u16 board_y, u8 view_x,
                                      u8 view_y, u8 view_rows,
                                      u8 *sx, u8 *sy, u16 len,
                                      u16 head_color, u16 body_color)
{
    u16 i;

    for (i = 0; i < len; i++) {
        Snake_DrawWorldCellAt(board_x, board_y, view_x, view_y, view_rows,
                              sx[i], sy[i], (i == 0) ? head_color : body_color);
    }
}

static void Snake_RenderOpenDuo(void)
{
    Snake_UpdateViewport();
    Snake_DrawHeader();
    LCD_Fill(0, BOARD_Y - 4, LCD_W - 1, LCD_H - 1, BLACK);

    Snake_RenderViewportAt(BOARD_X, OPEN_DUO_P1_Y, viewport_x, viewport_y,
                           OPEN_DUO_ROWS);
    Snake_DrawFoodAt(BOARD_X, OPEN_DUO_P1_Y, viewport_x, viewport_y,
                     OPEN_DUO_ROWS);
    Snake_DrawSnakeOnViewport(BOARD_X, OPEN_DUO_P1_Y, viewport_x, viewport_y,
                              OPEN_DUO_ROWS, snake_x, snake_y, snake_len,
                              YELLOW, GREEN);
    Snake_DrawSnakeOnViewport(BOARD_X, OPEN_DUO_P1_Y, viewport_x, viewport_y,
                              OPEN_DUO_ROWS, snake2_x, snake2_y, snake2_len,
                              MAGENTA, CYAN);

    Snake_RenderViewportAt(BOARD_X, OPEN_DUO_P2_Y, viewport2_x, viewport2_y,
                           OPEN_DUO_ROWS);
    Snake_DrawFoodAt(BOARD_X, OPEN_DUO_P2_Y, viewport2_x, viewport2_y,
                     OPEN_DUO_ROWS);
    Snake_DrawSnakeOnViewport(BOARD_X, OPEN_DUO_P2_Y, viewport2_x, viewport2_y,
                              OPEN_DUO_ROWS, snake_x, snake_y, snake_len,
                              YELLOW, GREEN);
    Snake_DrawSnakeOnViewport(BOARD_X, OPEN_DUO_P2_Y, viewport2_x, viewport2_y,
                              OPEN_DUO_ROWS, snake2_x, snake2_y, snake2_len,
                              MAGENTA, CYAN);
    Snake_DrawOpenDuoLabels();
    Snake_SaveRenderState();
}

static void Snake_Render(void)
{
    u16 i;

    if (Snake_IsOpenDuoMode()) {
        Snake_RenderOpenDuo();
        return;
    }

    Snake_UpdateViewport();
    Snake_DrawHeader();
    Snake_RenderBoard();
    Snake_DrawFood();

    for (i = 0; i < snake_len; i++) {
        if (i == 0) {
            Snake_DrawWorldCell(snake_x[i], snake_y[i], YELLOW);
        } else {
            Snake_DrawWorldCell(snake_x[i], snake_y[i], GREEN);
        }
    }

    if (Snake_IsDuoMode()) {
        for (i = 0; i < snake2_len; i++) {
            if (i == 0) {
                Snake_DrawWorldCell(snake2_x[i], snake2_y[i], MAGENTA);
            } else {
                Snake_DrawWorldCell(snake2_x[i], snake2_y[i], CYAN);
            }
        }
    }

    Snake_SaveRenderState();
}

static void Snake_RestoreCell(u8 x, u8 y)
{
    Snake_DrawMapCell(x, y);
    if (x == food_x && y == food_y) {
        Snake_DrawFood();
    }
}

static void Snake_RestoreCellAt(u16 board_x, u16 board_y, u8 view_x,
                                u8 view_y, u8 view_rows, u8 x, u8 y)
{
    Snake_DrawMapCellAt(board_x, board_y, view_x, view_y, view_rows, x, y);
    if (x == food_x && y == food_y) {
        Snake_DrawFoodAt(board_x, board_y, view_x, view_y, view_rows);
    }
}

static void Snake_RenderStepOnViewport(u16 board_x, u16 board_y, u8 view_x,
                                       u8 view_y, u8 view_rows,
                                       u8 food_changed)
{
    if (prev_snake_len != 0) {
        if (snake_len <= prev_snake_len) {
            Snake_RestoreCellAt(board_x, board_y, view_x, view_y, view_rows,
                                prev_snake_tail_x, prev_snake_tail_y);
        }
        Snake_RestoreCellAt(board_x, board_y, view_x, view_y, view_rows,
                            prev_snake_head_x, prev_snake_head_y);
        if (Snake_IsDuoMode() && prev_snake2_len != 0) {
            if (snake2_len <= prev_snake2_len) {
                Snake_RestoreCellAt(board_x, board_y, view_x, view_y, view_rows,
                                    prev_snake2_tail_x, prev_snake2_tail_y);
            }
            Snake_RestoreCellAt(board_x, board_y, view_x, view_y, view_rows,
                                prev_snake2_head_x, prev_snake2_head_y);
        }
    }

    if (food_changed) {
        Snake_RestoreCellAt(board_x, board_y, view_x, view_y, view_rows,
                            prev_food_x, prev_food_y);
        Snake_DrawFoodAt(board_x, board_y, view_x, view_y, view_rows);
    }

    if (snake_len > 1) {
        Snake_DrawWorldCellAt(board_x, board_y, view_x, view_y, view_rows,
                              snake_x[1], snake_y[1], GREEN);
    }
    Snake_DrawWorldCellAt(board_x, board_y, view_x, view_y, view_rows,
                          snake_x[0], snake_y[0], YELLOW);
    if (Snake_IsDuoMode()) {
        if (snake2_len > 1) {
            Snake_DrawWorldCellAt(board_x, board_y, view_x, view_y, view_rows,
                                  snake2_x[1], snake2_y[1], CYAN);
        }
        Snake_DrawWorldCellAt(board_x, board_y, view_x, view_y, view_rows,
                              snake2_x[0], snake2_y[0], MAGENTA);
    }
}

static void Snake_RenderStep(u8 food_changed)
{
    Snake_UpdateViewport();
    if (Snake_IsOpenDuoMode()) {
        if (viewport_x != prev_viewport_x || viewport_y != prev_viewport_y ||
            viewport2_x != prev_viewport2_x || viewport2_y != prev_viewport2_y ||
            prev_snake_len == 0) {
            Snake_Render();
            return;
        }

        Snake_RenderStepOnViewport(BOARD_X, OPEN_DUO_P1_Y, viewport_x,
                                   viewport_y, OPEN_DUO_ROWS, food_changed);
        Snake_RenderStepOnViewport(BOARD_X, OPEN_DUO_P2_Y, viewport2_x,
                                   viewport2_y, OPEN_DUO_ROWS, food_changed);
        Snake_DrawOpenDuoLabels();
        Snake_DrawHeader();
        Snake_SaveRenderState();
        return;
    }

    if (viewport_x != prev_viewport_x || viewport_y != prev_viewport_y) {
        Snake_Render();
        return;
    }

    if (prev_snake_len != 0) {
        if (snake_len <= prev_snake_len) {
            Snake_RestoreCell(prev_snake_tail_x, prev_snake_tail_y);
        }
        Snake_RestoreCell(prev_snake_head_x, prev_snake_head_y);
        if (Snake_IsDuoMode() && prev_snake2_len != 0) {
            if (snake2_len <= prev_snake2_len) {
                Snake_RestoreCell(prev_snake2_tail_x, prev_snake2_tail_y);
            }
            Snake_RestoreCell(prev_snake2_head_x, prev_snake2_head_y);
        }
    } else {
        Snake_Render();
        return;
    }

    if (food_changed) {
        Snake_RestoreCell(prev_food_x, prev_food_y);
        Snake_DrawFood();
    }

    if (snake_len > 1) {
        Snake_DrawWorldCell(snake_x[1], snake_y[1], GREEN);
    }
    Snake_DrawWorldCell(snake_x[0], snake_y[0], YELLOW);
    if (Snake_IsDuoMode()) {
        if (snake2_len > 1) {
            Snake_DrawWorldCell(snake2_x[1], snake2_y[1], CYAN);
        }
        Snake_DrawWorldCell(snake2_x[0], snake2_y[0], MAGENTA);
    }
    Snake_DrawHeader();
    Snake_SaveRenderState();
}

static void Snake_ShowCenter(const char *line1, const char *line2)
{
    LCD_Fill(12, 124, LCD_W - 13, 206, BLACK);
    Snake_ShowTextCenter(136, 24, line1, YELLOW, BLACK, 0);
    Snake_ShowTextCenter(176, 16, line2, WHITE, BLACK, 0);
}

static u8 Snake_ResultReturnRequested(void)
{
    SnakeUartCmd cmd;

    if (GPIO_ReadInputDataBit(GPIOD, GPIO_Pin_11) == 0) {
        return 1;
    }

    while (SnakeUart_PopCommand(&cmd)) {
        if (cmd.type == SNAKE_UART_CMD_RESET) {
            return 1;
        }
    }

    return 0;
}

static u16 Snake_FindOnSnakeIndex(u8 x, u8 y, u16 limit)
{
    u16 i;

    for (i = 0; i < limit; i++) {
        if (snake_x[i] == x && snake_y[i] == y) {
            return i;
        }
    }

    return SNAKE_NO_INDEX;
}

static void Snake_ClearPendingUartCommands(void)
{
    SnakeUartCmd cmd;

    while (SnakeUart_PopCommand(&cmd)) {
    }
}

static void Snake_WaitReturnHome(void)
{
    Snake_AudioStop();
    restart_request = 0;
    Snake_ClearPendingUartCommands();

    while (GPIO_ReadInputDataBit(GPIOD, GPIO_Pin_11) == 0) {
        Delay_ms(20);
    }

    while (!Snake_ResultReturnRequested()) {
        Delay_ms(20);
    }

    while (GPIO_ReadInputDataBit(GPIOD, GPIO_Pin_11) == 0) {
        Delay_ms(20);
    }

    Delay_ms(250);
    Snake_KeyReset();
    LCD_Clear(BLACK);
    Snake_ShowHome();
}

static void Snake_PlaceFood(void)
{
    u16 tries;
    u8 x;
    u8 y;
    u8 cols = Snake_WorldCols();
    u8 rows = Snake_WorldRows();

    for (tries = 0; tries < 1000; tries++) {
        x = (u8)Snake_Rand(cols);
        y = (u8)Snake_Rand(rows);
        if (Snake_CellCanHoldFood(x, y)) {
            food_x = x;
            food_y = y;
            break;
        }
    }

    if (tries >= 1000) {
        for (y = 0; y < rows; y++) {
            for (x = 0; x < cols; x++) {
                if (Snake_CellCanHoldFood(x, y)) {
                    food_x = x;
                    food_y = y;
                    y = rows;
                    break;
                }
            }
        }
    }

    if (Snake_IsOpenMode()) {
        u8 r = (u8)Snake_Rand(12);
        if (r == 0) {
            food_type = FOOD_POISON;
        } else if (r == 11 || r == 10) {
            food_type = FOOD_BONUS;
        } else {
            food_type = FOOD_NORMAL;
        }
    } else if (level_index == 3) {
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
    snake2_len = 4;
    prev_snake_len = 0;
    prev_snake2_len = 0;
    dir = DIR_RIGHT;
    next_dir = DIR_RIGHT;
    dir2 = DIR_LEFT;
    next_dir2 = DIR_LEFT;
    turn_pending = 0;
    turn_pending2 = 0;
    duo_winner = 0;
    paused = 0;
    restart_request = 0;
    return_home_request = 0;
    time_acc_ms = 0;
    viewport_x = 0;
    viewport_y = 0;
    prev_viewport_x = 0xff;
    prev_viewport_y = 0xff;
    viewport2_x = 0;
    viewport2_y = 0;
    prev_viewport2_x = 0xff;
    prev_viewport2_y = 0xff;

    if (Snake_IsOpenMode()) {
        snake_x[0] = 20;
        snake_y[0] = 28;
        snake_x[1] = 19;
        snake_y[1] = 28;
        snake_x[2] = 18;
        snake_y[2] = 28;
        snake_x[3] = 17;
        snake_y[3] = 28;

        snake2_x[0] = 24;
        snake2_y[0] = 28;
        snake2_x[1] = 25;
        snake2_y[1] = 28;
        snake2_x[2] = 26;
        snake2_y[2] = 28;
        snake2_x[3] = 27;
        snake2_y[3] = 28;
    } else {
        snake_x[0] = 7;
        snake_y[0] = 10;
        snake_x[1] = 6;
        snake_y[1] = 10;
        snake_x[2] = 5;
        snake_y[2] = 10;
        snake_x[3] = 4;
        snake_y[3] = 10;

        snake2_x[0] = 11;
        snake2_y[0] = 10;
        snake2_x[1] = 12;
        snake2_y[1] = 10;
        snake2_x[2] = 13;
        snake2_y[2] = 10;
        snake2_x[3] = 14;
        snake2_y[3] = 10;
    }

    Snake_KeyReset();
    Snake_KnobReset();
    Snake_ShowDirection();
}

static void Snake_StartLevel(u8 lv)
{
    level_index = lv;
    level_score = 0;
    time_left = (game_mode == GAME_MODE_CLASSIC || Snake_IsOpenMode()) ?
                0 : level_time_limit[level_index];
    classic_target_len = Snake_CountWalkableCells();
    if (Snake_IsDuoMode()) {
        Snake_SetStatus(Snake_IsOpenMode() ? "OPEN DUO" : "P1 WASD P2 1235");
    } else if (Snake_IsOpenMode()) {
        Snake_SetStatus("OPEN MAP");
    } else {
        Snake_SetStatus(game_mode == GAME_MODE_CLASSIC ? "CLASSIC" : "K1+K2 Pause");
    }
    Snake_MusicSetLevel(level_index);
    Snake_ResetSnake();
    rng_state ^= 0x5a5a0000u + score + GPIO_ReadInputData(GPIOA);
    Snake_PlaceFood();
    Snake_Render();
}

static void Snake_StartGame(void)
{
    score = 0;
    score2 = 0;
    lives = 3;
    Snake_StartLevel(start_level);
}

static SnakeDir Snake_KeyToDir(u8 key_bit)
{
    if (key_bit == KEY_UP_MASK) return DIR_UP;
    if (key_bit == KEY_DOWN_MASK) return DIR_DOWN;
    if (key_bit == KEY_LEFT_MASK) return DIR_LEFT;
    return DIR_RIGHT;
}

static SnakeDir Snake_RelativeTurn(SnakeDir base, u8 knob_event)
{
    if (knob_event == KNOB_LEFT_EVENT) {
        if (base == DIR_UP) return DIR_LEFT;
        if (base == DIR_DOWN) return DIR_RIGHT;
        if (base == DIR_LEFT) return DIR_DOWN;
        return DIR_UP;
    }

    if (base == DIR_UP) return DIR_RIGHT;
    if (base == DIR_DOWN) return DIR_LEFT;
    if (base == DIR_LEFT) return DIR_UP;
    return DIR_DOWN;
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
    u8 knob_event;
    u8 next_level;
    SnakeDir want;

    Snake_HandleSerialInput();
    Snake_KeyScan();
    Snake_KnobScan();
    knob_event = Snake_KnobPopEvent();

    if (paused && (key_press_latch & KEY_UP_MASK)) {
        key_press_latch &= (u8)(~KEY_UP_MASK);
        return_home_request = 1;
        Snake_SetStatus("EXIT HOME");
        Snake_DrawHeader();
        Snake_DrawPauseHint();
        return;
    }

    if ((key_stable & KEY_PAUSE_MASK) == KEY_PAUSE_MASK) {
        if (!pause_lock) {
            Snake_TogglePause();
        }
        return;
    }

    pause_lock = 0;
    if (paused) {
        if (knob_event != 0) {
            if (knob_event == KNOB_LEFT_EVENT) {
                next_level = (level_index == 0) ? (LEVEL_COUNT - 1) : (level_index - 1);
            } else {
                next_level = (u8)((level_index + 1) % LEVEL_COUNT);
            }

            Snake_SelectLevel(next_level, "KNOB LEVEL");
        }
        return;
    }

    if (turn_pending) {
        return;
    }

    if (knob_event != 0) {
        want = Snake_RelativeTurn(next_dir, knob_event);
        next_dir = want;
        turn_pending = 1;
        Snake_SetStatus(knob_event == KNOB_LEFT_EVENT ? "KNOB LEFT" : "KNOB RIGHT");
        Snake_DrawHeader();
        return;
    }

    key = Snake_PopDirectionKey();
    if (key == 0) {
        return;
    }

    want = Snake_KeyToDir(key);
    if (want == next_dir || Snake_IsReverse(dir, want)) {
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
    if (game_mode == GAME_MODE_CLASSIC || Snake_IsOpenMode()) {
        return 1;
    }

    if (level_time_limit[level_index] == 0) {
        return 1;
    }

    if (Snake_IsDuoMode()) {
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

        if (return_home_request) {
            return_home_request = 0;
            Snake_KeyReset();
            LCD_Clear(BLACK);
            Snake_ShowHome();
            Snake_StartGame();
            return 1;
        }

        if (restart_request) {
            return 1;
        }

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
        Snake_PersistMarkDirty();
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
    if (Snake_IsOpenMode()) {
        /* Open map has no fixed clear condition. */
    } else if (game_mode == GAME_MODE_CLASSIC) {
        if (snake_len >= classic_target_len) {
            return STEP_WIN;
        }
    } else if (level_score >= level_target[level_index]) {
        return STEP_LEVEL_DONE;
    }

    Snake_PlaceFood();
    return STEP_ALIVE;
}

static u8 Snake_HandleSelfCollision(u16 hit_index)
{
    u16 removed_len;

    if (hit_index == SNAKE_NO_INDEX || hit_index >= snake_len) {
        return STEP_DEAD;
    }

    removed_len = (u16)(snake_len - hit_index);

    Snake_UpdateBest();
    if (lives > 0) {
        lives--;
    }

    if (score > removed_len) {
        score = (u16)(score - removed_len);
    } else {
        score = 0;
    }

    if (level_score > removed_len) {
        level_score = (u8)(level_score - removed_len);
    } else {
        level_score = 0;
    }

    snake_len = hit_index;
    Snake_SetStatus("SELF HIT");
    Snake_Render();
    Snake_PlayTone(NOTE_D4, 150);
    Snake_PlayTone(NOTE_G3, 120);

    if (lives == 0) {
        return STEP_SELF_GAME_OVER;
    }

    return STEP_SELF_HIT;
}

static u8 Snake_Step(void)
{
    short nx = (short)snake_x[0];
    short ny = (short)snake_y[0];
    u8 eat;
    u8 food_changed = 0;
    u16 check_len;
    u16 self_index;
    int i;
    char cell;
    u8 result;
    u8 cols = Snake_WorldCols();
    u8 rows = Snake_WorldRows();

    dir = next_dir;
    turn_pending = 0;

    if (dir == DIR_UP) ny--;
    else if (dir == DIR_DOWN) ny++;
    else if (dir == DIR_LEFT) nx--;
    else nx++;

    if (nx < 0 || nx >= cols || ny < 0 || ny >= rows) {
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
            nx = (short)tx;
            ny = (short)ty;
            Snake_SetStatus("PORTAL");
            Snake_PlayTone(NOTE_D6, 60);
        }
    }

    eat = ((u8)nx == food_x && (u8)ny == food_y);
    check_len = eat ? snake_len : (u16)(snake_len - 1);

    self_index = Snake_FindOnSnakeIndex((u8)nx, (u8)ny, check_len);
    if (self_index != SNAKE_NO_INDEX) {
        return Snake_HandleSelfCollision(self_index);
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
        food_changed = 1;
    } else {
        Snake_SetStatus("RUNNING");
    }

    Snake_ShowDirection();
    Snake_RenderStep(food_changed);
    return result;
}

static void Snake_AdvancePoint(SnakeDir move_dir, short *x, short *y)
{
    if (move_dir == DIR_UP) (*y)--;
    else if (move_dir == DIR_DOWN) (*y)++;
    else if (move_dir == DIR_LEFT) (*x)--;
    else (*x)++;
}

static void Snake_MoveBody(u8 *x, u8 *y, u16 len,
                           short nx, short ny)
{
    int i;

    for (i = (int)len - 1; i > 0; i--) {
        x[i] = x[i - 1];
        y[i] = y[i - 1];
    }

    x[0] = (u8)nx;
    y[0] = (u8)ny;
}

static u8 Snake_DuoHitWallOrMap(short x, short y)
{
    u8 cols = Snake_WorldCols();
    u8 rows = Snake_WorldRows();

    if (x < 0 || x >= cols || y < 0 || y >= rows) {
        return 1;
    }

    return (u8)(Snake_MapCell((u8)x, (u8)y) == '#');
}

static u8 Snake_DuoStep(void)
{
    short nx1 = (short)snake_x[0];
    short ny1 = (short)snake_y[0];
    short nx2 = (short)snake2_x[0];
    short ny2 = (short)snake2_y[0];
    u8 p1_dead = 0;
    u8 p2_dead = 0;
    u8 eat1;
    u8 eat2;
    u16 check_len1;
    u16 check_len2;
    u8 food_changed = 0;

    dir = next_dir;
    dir2 = next_dir2;
    turn_pending = 0;
    turn_pending2 = 0;

    Snake_AdvancePoint(dir, &nx1, &ny1);
    Snake_AdvancePoint(dir2, &nx2, &ny2);

    p1_dead = Snake_DuoHitWallOrMap(nx1, ny1);
    p2_dead = Snake_DuoHitWallOrMap(nx2, ny2);

    eat1 = (u8)(!p1_dead && (u8)nx1 == food_x && (u8)ny1 == food_y);
    eat2 = (u8)(!p2_dead && (u8)nx2 == food_x && (u8)ny2 == food_y);
    check_len1 = eat1 ? snake_len : (u16)(snake_len - 1);
    check_len2 = eat2 ? snake2_len : (u16)(snake2_len - 1);

    if (!p1_dead && Snake_OnSnake((u8)nx1, (u8)ny1, check_len1)) {
        p1_dead = 1;
    }
    if (!p2_dead && Snake2_OnSnake((u8)nx2, (u8)ny2, check_len2)) {
        p2_dead = 1;
    }
    if (!p1_dead && Snake2_OnSnake((u8)nx1, (u8)ny1, check_len2)) {
        p1_dead = 1;
    }
    if (!p2_dead && Snake_OnSnake((u8)nx2, (u8)ny2, check_len1)) {
        p2_dead = 1;
    }
    if (!p1_dead && !p2_dead && nx1 == nx2 && ny1 == ny2) {
        p1_dead = 1;
        p2_dead = 1;
    }

    if (p1_dead || p2_dead) {
        if (p1_dead && p2_dead) duo_winner = 3;
        else duo_winner = p1_dead ? 2 : 1;
        Snake_SetStatus(duo_winner == 1 ? "P1 WINS" :
                        (duo_winner == 2 ? "P2 WINS" : "DRAW"));
        Snake_Render();
        return STEP_DEAD;
    }

    if (eat1 && snake_len < MAX_SNAKE_LEN) {
        snake_len++;
        score++;
        food_changed = 1;
    }
    if (eat2 && snake2_len < MAX_SNAKE_LEN) {
        snake2_len++;
        score2++;
        food_changed = 1;
    }

    Snake_MoveBody(snake_x, snake_y, snake_len, nx1, ny1);
    Snake_MoveBody(snake2_x, snake2_y, snake2_len, nx2, ny2);

    if (food_changed) {
        Snake_SetStatus(eat1 && eat2 ? "BOTH" : (eat1 ? "P1 +" : "P2 +"));
        Snake_PlaceFood();
    } else {
        Snake_SetStatus("DUO");
    }

    Snake_ShowDirection();
    Snake_RenderStep(food_changed);
    return STEP_ALIVE;
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
    Snake_PersistSave();
    Snake_ShowCenter("GAME OVER", "Press KEY1 or R to return");
    Snake_PlayTone(NOTE_C4, 160);
    Snake_PlayTone(NOTE_G3, 230);
    Snake_WaitReturnHome();
}

static void Snake_WinGame(void)
{
    Snake_UpdateBest();
    Snake_PersistSave();
    Snake_ShowCenter("YOU WIN", "Press KEY1 or R to return");
    Snake_BeepLevel();
    Snake_WaitReturnHome();
}

static void Snake_NextLevel(void)
{
    if (level_index + 1 >= LEVEL_COUNT) {
        Snake_UpdateBest();
        Snake_PersistSave();
        Snake_ShowCenter("YOU WIN", "Press KEY1 or R to return");
        Snake_BeepLevel();
        Snake_WaitReturnHome();
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

    high_score = 0;
    game_mode = GAME_MODE_STAGE;
    Snake_PersistLoad();

    SystemInit();
    GPIO_Configuration();
    Snake_ADCConfiguration();
    SnakeUart_Init();
    Snake_AudioInit();
    LCD_Init();
    LCD_Clear(BLACK);
    BEEP(0);

    Snake_ShowHome();
    Snake_StartGame();

    while (1) {
        if (restart_request) {
            restart_request = 0;
            Snake_StartGame();
            continue;
        }

        if (!Snake_WaitStep(Snake_StepDelay())) {
            if (!Snake_LoseLife("TIME OUT")) {
                Snake_GameOver();
                Snake_StartGame();
            }
            continue;
        }

        if (restart_request) {
            restart_request = 0;
            Snake_StartGame();
            continue;
        }

        step_result = Snake_IsDuoMode() ? Snake_DuoStep() : Snake_Step();
        if (step_result == STEP_DEAD) {
            if (Snake_IsDuoMode()) {
                Snake_PersistSave();
                Snake_ShowCenter(duo_winner == 1 ? "P1 WINS" :
                                (duo_winner == 2 ? "P2 WINS" : "DRAW"),
                                "Press KEY1 or R to return");
                Snake_BeepLevel();
                Snake_WaitReturnHome();
                Snake_StartGame();
            } else {
                if (!Snake_LoseLife("CRASH")) {
                    Snake_GameOver();
                    Snake_StartGame();
                }
            }
        } else if (step_result == STEP_LEVEL_DONE) {
            Snake_NextLevel();
        } else if (step_result == STEP_WIN) {
            Snake_WinGame();
            Snake_StartGame();
        } else if (step_result == STEP_SELF_GAME_OVER) {
            Snake_GameOver();
            Snake_StartGame();
        } else if (step_result == STEP_SELF_HIT) {
            continue;
        }
    }
}
