#include "app.h"

#include <stdio.h>
#include <string.h>

#include "board.h"
#include "spi.h"
#include "lcd.h"
#include "lcd_init.h"
#include "ws2812.h"
#include "keypad.h"
#include "encoder.h"
#include "audio.h"

/* ---------------- 界面颜色 ---------------- */
#define UI_BG       0x0861U
#define UI_LINE     0x4208U
#define UI_CELL     0x2124U
#define UI_FG       0xFFFFU
#define UI_DIM      0x8410U
#define UI_OK       0x07E0U
#define UI_WARN     0xFFE0U
#define UI_BAD      0xF800U
#define UI_ACCENT   0x07FFU

#define FONT_H      12U
#define FONT_W      6U

/* ---------------- 文字槽（局部刷新，避免整屏重绘） ---------------- */
typedef struct
{
    uint16_t x;
    uint16_t y;
    uint16_t w;
    uint16_t color;
    char     text[24];
} ui_slot_t;

static ui_slot_t s_slots[] = {
    { 88U, 16U,  70U, UI_FG,   "" },   /* 编码器计数 */
    {200U, 16U,  44U, UI_OK,   "" },   /* 编码器按键 */
    { 88U, 28U,  70U, UI_FG,   "" },   /* 按键数量/键号 */
    {200U, 28U,  44U, UI_FG,   "" },   /* 编码器 A/B */
    { 88U, 40U, 110U, UI_FG,   "" },   /* RGB 模式 */
    { 88U, 52U, 110U, UI_FG,   "" },   /* 音频状态 */
    {230U, 64U,  54U, UI_DIM,  "" },   /* 运行时间 */
};

#define SLOT_ENC        0
#define SLOT_SW         1
#define SLOT_KEY        2
#define SLOT_AB         3
#define SLOT_RGB        4
#define SLOT_AUDIO      5
#define SLOT_UPTIME     6
#define SLOT_NUM        (sizeof(s_slots) / sizeof(s_slots[0]))

/* ---------------- RGB 模式 ---------------- */
#define RGB_MODE_NUM    6U

static const char *const s_rgb_name[RGB_MODE_NUM] = {
    "RAINBOW", "RED", "GREEN", "BLUE", "WHITE", "OFF"
};
static const uint8_t s_rgb_value[RGB_MODE_NUM][3] = {
    {  0,   0,   0},   /* RAINBOW 由渐变函数生成 */
    {255,   0,   0},
    {  0, 255,   0},
    {  0,   0, 255},
    {255, 255, 255},
    {  0,   0,   0}
};

/* ---------------- 音频测试状态 ---------------- */
typedef enum
{
    AUDIO_IDLE = 0,
    AUDIO_SEND,
    AUDIO_WAIT
} audio_state_t;

typedef enum
{
    AUDIO_RESULT_NONE = 0,
    AUDIO_RESULT_PASS,
    AUDIO_RESULT_FAIL
} audio_result_t;

static audio_state_t  s_audio_state = AUDIO_IDLE;
static audio_result_t s_audio_result = AUDIO_RESULT_NONE;
static uint8_t        s_audio_probe;
static uint8_t        s_audio_idle_level;
static uint32_t       s_audio_time;
static uint32_t       s_audio_auto_at;
static uint8_t        s_audio_auto_done;

/* ---------------- 应用状态 ---------------- */
static uint8_t  s_rgb_mode;
static uint16_t s_rgb_phase;
static uint8_t  s_rgb_needs_show;
static uint32_t s_last_rgb_ms;

static int32_t  s_last_detent;

static uint32_t s_last_enc_poll;
static uint32_t s_last_key_scan;
static uint32_t s_last_ui;
static uint8_t  s_ui_dirty;

static uint8_t  s_cell_drawn[KEY_ROW_NUM];

/* ---------------- 基础绘制 ---------------- */
static void ui_set(uint8_t idx, const char *text, uint16_t color)
{
    ui_slot_t *slot = &s_slots[idx];

    if ((slot->color == color) && (strcmp(slot->text, text) == 0))
    {
        return;
    }

    slot->color = color;
    strncpy(slot->text, text, sizeof(slot->text) - 1U);
    slot->text[sizeof(slot->text) - 1U] = '\0';

    LCD_Fill(slot->x, slot->y,
             (uint16_t)(slot->x + slot->w), (uint16_t)(slot->y + FONT_H),
             UI_BG);
    LCD_ShowString(slot->x, slot->y, slot->text, slot->color, UI_BG,
                   FONT_H, 0U);
}

static void draw_cell(uint8_t row, uint8_t col, uint8_t pressed)
{
    uint16_t x = (uint16_t)(2U + col * 13U);
    uint16_t y = (uint16_t)(18U + row * 8U);

    LCD_Fill(x, y, (uint16_t)(x + 10U), (uint16_t)(y + 6U),
             pressed ? UI_OK : UI_CELL);
}

static void draw_static_ui(void)
{
    uint8_t r, c;

    LCD_Fill(0U, 0U, LCD_W, LCD_H, UI_BG);

    /* 标题与时钟源 */
    LCD_ShowString(2U, 0U, "TICKBOARD SELFTEST", UI_ACCENT, UI_BG, FONT_H, 0U);
    LCD_ShowString(200U, 0U, g_clock_hse_ok ? "HSE 64M" : "HSI 64M",
                   g_clock_hse_ok ? UI_OK : UI_WARN, UI_BG, FONT_H, 0U);

    /* 分隔线 */
    LCD_Fill(0U, 14U, LCD_W, 15U, UI_LINE);
    LCD_Fill(84U, 16U, 85U, LCD_H, UI_LINE);

    /* 提示行 */
    LCD_ShowString(88U, 64U, "TURN=RGB PUSH=AUD", UI_DIM, UI_BG, FONT_H, 0U);

    /* 按键矩阵格 */
    for (r = 0U; r < KEY_ROW_NUM; r++)
    {
        s_cell_drawn[r] = 0U;
        for (c = 0U; c < KEY_COL_NUM; c++)
        {
            draw_cell(r, c, 0U);
        }
    }

    /* 强制刷新所有文字槽 */
    for (r = 0U; r < SLOT_NUM; r++)
    {
        s_slots[r].text[0] = '\0';
        s_slots[r].color = 0xFFFFU ^ s_slots[r].color;
    }
}

/* ---------------- 按键矩阵显示 ---------------- */
static void update_matrix(void)
{
    uint32_t mask = Keypad_GetMask();
    uint8_t r, c;

    for (r = 0U; r < KEY_ROW_NUM; r++)
    {
        uint8_t row_bits = 0U;

        for (c = 0U; c < KEY_COL_NUM; c++)
        {
            uint8_t key = 0U;
            uint8_t pressed;

            if (r < 5U)
            {
                key = (uint8_t)(r * KEY_COL_NUM + c + 1U);
            }
            else if (c == 0U)
            {
                key = (r == 5U) ? 31U : 32U;
            }

            pressed = (uint8_t)((key != 0U) &&
                                ((mask & (1UL << (key - 1U))) != 0U));
            if (pressed)
            {
                row_bits |= (uint8_t)(1U << c);
            }
        }

        if (row_bits != s_cell_drawn[r])
        {
            for (c = 0U; c < KEY_COL_NUM; c++)
            {
                draw_cell(r, c, (uint8_t)(row_bits & (1U << c)));
            }
            s_cell_drawn[r] = row_bits;
        }
    }
}

/* ---------------- RGB ---------------- */
static void rgb_update(uint32_t now)
{
    if (s_rgb_mode == 0U)
    {
        if ((now - s_last_rgb_ms) >= 40U)
        {
            uint16_t i;

            for (i = 0U; i < RGB_LED_COUNT; i++)
            {
                uint8_t r, g, b;
                WS2812_ColorWheel(i, s_rgb_phase, &r, &g, &b);
                WS2812_SetPixel(i, r, g, b);
            }
            s_rgb_phase = (uint16_t)(s_rgb_phase + 4U);
            WS2812_Show();
            s_last_rgb_ms = now;
        }
    }
    else if (s_rgb_needs_show != 0U)
    {
        WS2812_Fill(s_rgb_value[s_rgb_mode][0],
                    s_rgb_value[s_rgb_mode][1],
                    s_rgb_value[s_rgb_mode][2]);
        WS2812_Show();
        s_rgb_needs_show = 0U;
    }
}

/* ---------------- 音频 ---------------- */
static void audio_start_test(void)
{
    s_audio_probe = 0U;
    s_audio_result = AUDIO_RESULT_NONE;
    s_audio_state = AUDIO_SEND;
    s_ui_dirty = 1U;
}

static void audio_poll(uint32_t now)
{
    if ((s_audio_auto_done == 0U) && ((int32_t)(now - s_audio_auto_at) >= 0))
    {
        s_audio_auto_done = 1U;
        audio_start_test();
    }

    switch (s_audio_state)
    {
        case AUDIO_SEND:
            /* 记录当前空闲电平，再发送最高音量 + 探测地址 */
            s_audio_idle_level = Audio_IsBusy();
            Audio_SetVolume(0xE7U);
            Audio_SendCommand(s_audio_probe);
            s_audio_time = now;
            s_audio_state = AUDIO_WAIT;
            break;

        case AUDIO_WAIT:
            if (Audio_IsBusy() != s_audio_idle_level)
            {
                s_audio_result = AUDIO_RESULT_PASS;
                s_audio_state = AUDIO_IDLE;
                s_ui_dirty = 1U;
            }
            else if ((now - s_audio_time) > 1500U)
            {
                if (s_audio_probe < 2U)
                {
                    s_audio_probe++;
                    s_audio_state = AUDIO_SEND;
                }
                else
                {
                    s_audio_result = AUDIO_RESULT_FAIL;
                    s_audio_state = AUDIO_IDLE;
                    s_ui_dirty = 1U;
                }
            }
            break;

        default:
            break;
    }
}

/* ---------------- 界面刷新 ---------------- */
static void ui_update(uint32_t now)
{
    char buf[24];

    snprintf(buf, sizeof(buf), "ENC:%+05ld",
             (long)Encoder_GetDetents());
    ui_set(SLOT_ENC, buf, UI_FG);

    ui_set(SLOT_SW, Encoder_SwitchIsDown() ? "SW:DOWN" : "SW:UP",
           Encoder_SwitchIsDown() ? UI_BAD : UI_OK);

    snprintf(buf, sizeof(buf), "KEY:%02u K%02u",
             (unsigned)Keypad_GetPressedCount(),
             (unsigned)Keypad_GetLastKey());
    ui_set(SLOT_KEY, buf, Keypad_GetPressedCount() ? UI_ACCENT : UI_FG);

    snprintf(buf, sizeof(buf), "AB:%u%u",
             (unsigned)Board_PinRead(ENC_A_PORT, ENC_A_PIN),
             (unsigned)Board_PinRead(ENC_B_PORT, ENC_B_PIN));
    ui_set(SLOT_AB, buf, UI_FG);

    snprintf(buf, sizeof(buf), "RGB:%s", s_rgb_name[s_rgb_mode]);
    ui_set(SLOT_RGB, buf, UI_ACCENT);

    switch (s_audio_state)
    {
        case AUDIO_SEND:
        case AUDIO_WAIT:
            ui_set(SLOT_AUDIO, "AUD:TEST", UI_WARN);
            break;
        default:
            if (s_audio_result == AUDIO_RESULT_PASS)
            {
                ui_set(SLOT_AUDIO, "AUD:PASS", UI_OK);
            }
            else if (s_audio_result == AUDIO_RESULT_FAIL)
            {
                ui_set(SLOT_AUDIO, "AUD:NO RSP", UI_BAD);
            }
            else
            {
                ui_set(SLOT_AUDIO, "AUD:IDLE", UI_FG);
            }
            break;
    }

    snprintf(buf, sizeof(buf), "%lus", (unsigned long)(now / 1000U));
    ui_set(SLOT_UPTIME, buf, UI_DIM);

    update_matrix();
}

/* ---------------- 启动自检 ---------------- */
void APP_Init(void)
{
    Board_Init();

    /* LCD 启动画面 */
    LCD_Init();
    LCD_Fill(0U, 0U, LCD_W, LCD_H, WHITE);
    LCD_ShowString(88U, 20U, "TICKBOARD", BLUE, WHITE, 24U, 0U);
    LCD_ShowString(115U, 48U, "SELF TEST", RED, WHITE, FONT_H, 0U);

    /* RGB 灯链基础颜色自检 */
    WS2812_Init();
    WS2812_Fill(255U, 0U, 0U);     WS2812_Show(); HAL_Delay(200U);
    WS2812_Fill(0U, 255U, 0U);     WS2812_Show(); HAL_Delay(200U);
    WS2812_Fill(0U, 0U, 255U);     WS2812_Show(); HAL_Delay(200U);
    WS2812_Fill(255U, 255U, 255U); WS2812_Show(); HAL_Delay(200U);
    WS2812_Clear();                WS2812_Show();

    Keypad_Init();
    Encoder_Init();
    Audio_Init();

    /* LCD 颜色条自检 */
    LCD_Fill(0U, 0U, LCD_W, LCD_H, RED);   HAL_Delay(180U);
    LCD_Fill(0U, 0U, LCD_W, LCD_H, GREEN); HAL_Delay(180U);
    LCD_Fill(0U, 0U, LCD_W, LCD_H, BLUE);  HAL_Delay(180U);
    LCD_Fill(0U, 0U, LCD_W, LCD_H, WHITE); HAL_Delay(180U);

    draw_static_ui();

    s_rgb_mode       = 0U;
    s_rgb_phase      = 0U;
    s_rgb_needs_show = 1U;
    s_last_detent    = Encoder_GetDetents();
    s_ui_dirty       = 1U;
    s_audio_auto_at  = HAL_GetTick() + 500U;
}

/* ---------------- 主循环（非阻塞） ---------------- */
void APP_Run(void)
{
    uint32_t now = HAL_GetTick();

    if ((now - s_last_enc_poll) >= 2U)
    {
        Encoder_Poll();
        s_last_enc_poll = now;
    }

    if ((now - s_last_key_scan) >= 10U)
    {
        if (Keypad_Scan() != 0U)
        {
            s_ui_dirty = 1U;
        }
        s_last_key_scan = now;
    }

    /* 编码器旋转：切换 RGB 模式 */
    {
        int32_t detent = Encoder_GetDetents();

        if (detent != s_last_detent)
        {
            if (detent > s_last_detent)
            {
                s_rgb_mode = (uint8_t)((s_rgb_mode + 1U) % RGB_MODE_NUM);
            }
            else
            {
                s_rgb_mode = (uint8_t)((s_rgb_mode + RGB_MODE_NUM - 1U) %
                                       RGB_MODE_NUM);
            }
            s_last_detent = detent;
            s_rgb_needs_show = 1U;
            s_ui_dirty = 1U;
        }
    }

    /* 编码器按下：重新测试语音芯片 */
    if (Encoder_TakePressEvent() != 0U)
    {
        audio_start_test();
    }

    audio_poll(now);
    rgb_update(now);

    if ((s_ui_dirty != 0U) || ((now - s_last_ui) >= 100U))
    {
        ui_update(now);
        s_ui_dirty = 0U;
        s_last_ui = now;
    }
}
