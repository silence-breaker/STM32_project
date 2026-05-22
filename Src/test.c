#include "main.h"
#include "lcd.h"
#include "GUI.h"
#include "touch.h"
#include "tim.h"
#include "aht20.h"
#include "bmp280.h"
#include "delay.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// 全局变量定义
u8 current_page = PAGE_HOME;
int g_temp = 250;            // 默认25.0度
int g_humidity = 500;        // 默认50.0%
int g_pressure = 10130;      // 默认1013.0 hPa
Threshold_t g_temp_thresh = {TEMP_HIGH_DEFAULT, TEMP_LOW_DEFAULT, THRESH_EDIT_NONE};
Threshold_t g_hum_thresh = {HUM_HIGH_DEFAULT, HUM_LOW_DEFAULT, THRESH_EDIT_NONE};
Threshold_t g_press_thresh = {PRESS_HIGH_DEFAULT, PRESS_LOW_DEFAULT, THRESH_EDIT_NONE};
int g_temp_alert = 0;
int g_hum_alert = 0;
int g_press_alert = 0;

// 页面刷新标志
static u8 need_refresh = 1;
static u8 need_full_refresh = 1;
static u8 last_temp_alert = 0;
static u8 last_hum_alert = 0;
static u8 last_press_alert = 0;

// 阈值编辑临时值
static int edit_temp_high = 0;
static int edit_temp_low = 0;
static int edit_hum_high = 0;
static int edit_hum_low = 0;
static int edit_press_high = 0;
static int edit_press_low = 0;

// 传感器读取定时器
static u16 sensor_timer = 0;

// 按键状态
static u8 key_left_pressed = 0;
static u8 key_right_pressed = 0;
static u8 key_up_pressed = 0;
static u8 key_down_pressed = 0;
static u8 key_ok_pressed = 0;

static u8 led_alert_mode = 0;
static u8 led_breath_duty = 0;
static u8 led_breath_up = 1;
static u8 led_flash_state = 0;
static u16 led_flash_timer = 0;
static u16 led_breath_timer = 0;

#define WAVE_POINTS     80
#define WAVE_CHART_X    108
#define WAVE_CHART_W    360
#define WAVE_CHART_H    62
#define WAVE_TEMP_Y     48
#define WAVE_HUM_Y      124
#define WAVE_PRESS_Y    200

#define WAVE_TEMP_MIN   -100
#define WAVE_TEMP_MAX   500
#define WAVE_HUM_MIN    0
#define WAVE_HUM_MAX    1000
#define WAVE_PRESS_MIN  9000
#define WAVE_PRESS_MAX  11000

#define LED_PWM_PERIOD          999
#define LED_BREATH_UPDATE_MS    10
#define LED_BREATH_STEP_MS      20
#define LED_FLASH_INTERVAL_MS   250

#define UI_BG                   0xF7BE
#define UI_CARD                 WHITE
#define UI_SHADOW               0xD69A
#define UI_TEXT                 0x2104
#define UI_MUTED                0x6B4D
#define UI_ORANGE               0xFD20
#define UI_TEAL                 0x05B3
#define UI_PURPLE               0x781F
#define UI_LIME                 0x7FE0
#define UI_BLUE                 0x04BF
#define UI_NAV_OFF              0xDEFB
#define UI_HINT                 0xD71C

static s16 wave_temp[WAVE_POINTS];
static s16 wave_hum[WAVE_POINTS];
static s16 wave_press[WAVE_POINTS];
static u8 wave_count = 0;
static u8 wave_pos = 0;

static void LED_SetDuty(u16 duty);
static void LED_RefreshMode(void);
static void DrawWaveSeries(s16 *data, s16 min, s16 max, u16 x, u16 y, u16 w, u16 h, u16 color);

static void RequestPartialRefresh(void)
{
    need_refresh = 1;
}

static void RequestFullRefresh(void)
{
    need_full_refresh = 1;
    need_refresh = 1;
}

static void FinishRefresh(void)
{
    need_full_refresh = 0;
    need_refresh = 0;
}

// 图片尺寸定义
#define ICON_NORMAL_W       48
#define ICON_NORMAL_H       47
#define ICON_HIGHTEMP_W     48
#define ICON_HIGHTEMP_H     41
#define ICON_HIGHHUM_W      48
#define ICON_HIGHHUM_H      48
#define ICON_HIGHQIYA_W     48
#define ICON_HIGHQIYA_H     47

// big图标尺寸
#define ICON_PEIXIAO_W      49
#define ICON_PEIXIAO_H      80
#define ICON_TEMPBIG_W      ICON_HIGHTEMP_W
#define ICON_TEMPBIG_H      ICON_HIGHTEMP_H
#define ICON_HUMIDBIG_W     ICON_HIGHHUM_W
#define ICON_HUMIDBIG_H     ICON_HIGHHUM_H
#define ICON_PRESSBIG_W     ICON_HIGHQIYA_W
#define ICON_PRESSBIG_H     ICON_HIGHQIYA_H
#define ICON_NORMALBIG_W    ICON_NORMAL_W
#define ICON_NORMALBIG_H    ICON_NORMAL_H

/* ========== LED初始化 ========== */

void LED_Init(void)
{
    LED_SetDuty(0);
    if(HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_3) != HAL_OK)
    {
        Error_Handler();
    }
}

static void LED_SetDuty(u16 duty)
{
    if(duty > LED_PWM_PERIOD) duty = LED_PWM_PERIOD;
    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_3, duty);
}

static void LED_SetAlertMode(u8 alert)
{
    alert = alert ? 1 : 0;
    if(led_alert_mode == alert) return;

    led_alert_mode = alert;
    led_flash_state = 0;
    led_flash_timer = 0;
    if(alert)
    {
        LED_SetDuty(0);
    }
}

static void LED_RefreshMode(void)
{
    LED_SetAlertMode(g_temp_alert || g_hum_alert || g_press_alert);
}

static void LED_Update(void)
{
    if(led_alert_mode)
    {
        led_flash_timer += LED_BREATH_UPDATE_MS;
        if(led_flash_timer >= LED_FLASH_INTERVAL_MS)
        {
            led_flash_timer = 0;
            led_flash_state = !led_flash_state;
        }
        LED_SetDuty(led_flash_state ? LED_PWM_PERIOD : 0);
        return;
    }

    led_breath_timer += LED_BREATH_UPDATE_MS;
    if(led_breath_timer < LED_BREATH_STEP_MS)
    {
        return;
    }
    led_breath_timer = 0;

    if(led_breath_up)
    {
        if(led_breath_duty < 100)
        {
            led_breath_duty++;
        }
        else
        {
            led_breath_up = 0;
        }
    }
    else
    {
        if(led_breath_duty > 0)
        {
            led_breath_duty--;
        }
        else
        {
            led_breath_up = 1;
        }
    }

    LED_SetDuty(((u16)led_breath_duty * LED_PWM_PERIOD) / 100);
}

/* ========== 按键初始化 ========== */

void KEY_Init(void)
{
}

/* ========== 按键扫描 ========== */

u8 KEY_Scan(void)
{
    u8 key_value = 0xFF;
    
    // 左键 PA2
    if(HAL_GPIO_ReadPin(KEY_LEFT_PORT, KEY_LEFT_PIN) == GPIO_PIN_RESET)
    {
        delay_ms(KEY_DEBOUNCE_MS);
        if(HAL_GPIO_ReadPin(KEY_LEFT_PORT, KEY_LEFT_PIN) == GPIO_PIN_RESET && !key_left_pressed)
        {
            key_left_pressed = 1;
            key_value = 0;  // 左
        }
    }
    else key_left_pressed = 0;
    
    // 右键 PA3
    if(HAL_GPIO_ReadPin(KEY_RIGHT_PORT, KEY_RIGHT_PIN) == GPIO_PIN_RESET)
    {
        delay_ms(KEY_DEBOUNCE_MS);
        if(HAL_GPIO_ReadPin(KEY_RIGHT_PORT, KEY_RIGHT_PIN) == GPIO_PIN_RESET && !key_right_pressed)
        {
            key_right_pressed = 1;
            key_value = 1;  // 右
        }
    }
    else key_right_pressed = 0;
    
    // 上键 PA4
    if(HAL_GPIO_ReadPin(KEY_UP_PORT, KEY_UP_PIN) == GPIO_PIN_RESET)
    {
        delay_ms(KEY_DEBOUNCE_MS);
        if(HAL_GPIO_ReadPin(KEY_UP_PORT, KEY_UP_PIN) == GPIO_PIN_RESET && !key_up_pressed)
        {
            key_up_pressed = 1;
            key_value = 2;  // 上
        }
    }
    else key_up_pressed = 0;
    
    // 下键 PA11
    if(HAL_GPIO_ReadPin(KEY_DOWN_PORT, KEY_DOWN_PIN) == GPIO_PIN_RESET)
    {
        delay_ms(KEY_DEBOUNCE_MS);
        if(HAL_GPIO_ReadPin(KEY_DOWN_PORT, KEY_DOWN_PIN) == GPIO_PIN_RESET && !key_down_pressed)
        {
            key_down_pressed = 1;
            key_value = 3;  // 下
        }
    }
    else key_down_pressed = 0;
    
    // 确认键 PA12
    if(HAL_GPIO_ReadPin(KEY_OK_PORT, KEY_OK_PIN) == GPIO_PIN_RESET)
    {
        delay_ms(KEY_DEBOUNCE_MS);
        if(HAL_GPIO_ReadPin(KEY_OK_PORT, KEY_OK_PIN) == GPIO_PIN_RESET && !key_ok_pressed)
        {
            key_ok_pressed = 1;
            key_value = 4;  // 确认
        }
    }
    else key_ok_pressed = 0;
    
    return key_value;
}

/* ========== 按键处理 ========== */

static void ApplyTempThreshold(void)
{
    g_temp_alert = (g_temp > g_temp_thresh.high || g_temp < g_temp_thresh.low) ? 1 : 0;
    if(g_temp_alert != last_temp_alert)
    {
        last_temp_alert = g_temp_alert;
        RequestPartialRefresh();
    }
    LED_RefreshMode();
}

static void ApplyHumThreshold(void)
{
    g_hum_alert = (g_humidity > g_hum_thresh.high || g_humidity < g_hum_thresh.low) ? 1 : 0;
    if(g_hum_alert != last_hum_alert)
    {
        last_hum_alert = g_hum_alert;
        RequestPartialRefresh();
    }
    LED_RefreshMode();
}

static void ApplyPressThreshold(void)
{
    g_press_alert = (g_pressure > g_press_thresh.high || g_pressure < g_press_thresh.low) ? 1 : 0;
    if(g_press_alert != last_press_alert)
    {
        last_press_alert = g_press_alert;
        RequestPartialRefresh();
    }
    LED_RefreshMode();
}

static void ExitEditAndApply(void)
{
    if(g_temp_thresh.edit_state != THRESH_EDIT_NONE)
    {
        g_temp_thresh.edit_state = THRESH_EDIT_NONE;
        ApplyTempThreshold();
    }
    if(g_hum_thresh.edit_state != THRESH_EDIT_NONE)
    {
        g_hum_thresh.edit_state = THRESH_EDIT_NONE;
        ApplyHumThreshold();
    }
    if(g_press_thresh.edit_state != THRESH_EDIT_NONE)
    {
        g_press_thresh.edit_state = THRESH_EDIT_NONE;
        ApplyPressThreshold();
    }
}

void HandleKey(u8 key)
{
    switch(key)
    {
        case 0: // 左 - 上一页
            ExitEditAndApply();
            if(current_page > 0)
                SwitchPage(current_page - 1);
            else
                SwitchPage(PAGE_COUNT - 1);
            break;
            
        case 1: // 右 - 下一页
            ExitEditAndApply();
            SwitchPage((current_page + 1) % PAGE_COUNT);
            break;
            
        case 2: // 上 - 阈值+1
            if(current_page == PAGE_TEMP && g_temp_thresh.edit_state != THRESH_EDIT_NONE)
            {
                if(g_temp_thresh.edit_state == THRESH_EDIT_HIGH)
                {
                    edit_temp_high += 10;
                    if(edit_temp_high > 500) edit_temp_high = 500;
                }
                else
                {
                    edit_temp_low += 10;
                    if(edit_temp_low > 500) edit_temp_low = 500;
                }
                RequestPartialRefresh();
            }
            else if(current_page == PAGE_HUMIDITY && g_hum_thresh.edit_state != THRESH_EDIT_NONE)
            {
                if(g_hum_thresh.edit_state == THRESH_EDIT_HIGH)
                {
                    edit_hum_high += 100;  // +10.0%
                    if(edit_hum_high > 990) edit_hum_high = 990;
                }
                else
                {
                    edit_hum_low += 100;  // +10.0%
                    if(edit_hum_low > 990) edit_hum_low = 990;
                }
                RequestPartialRefresh();
            }
            else if(current_page == PAGE_PRESSURE && g_press_thresh.edit_state != THRESH_EDIT_NONE)
            {
                if(g_press_thresh.edit_state == THRESH_EDIT_HIGH)
                {
                    edit_press_high += 100;  // +10.0 hPa
                    if(edit_press_high > 11000) edit_press_high = 11000;
                }
                else
                {
                    edit_press_low += 100;  // +10.0 hPa
                    if(edit_press_low > 11000) edit_press_low = 11000;
                }
                RequestPartialRefresh();
            }
            break;
            
        case 3: // 下 - 阈值-1
            if(current_page == PAGE_TEMP && g_temp_thresh.edit_state != THRESH_EDIT_NONE)
            {
                if(g_temp_thresh.edit_state == THRESH_EDIT_HIGH)
                {
                    edit_temp_high -= 10;
                    if(edit_temp_high < -500) edit_temp_high = -500;
                }
                else
                {
                    edit_temp_low -= 10;
                    if(edit_temp_low < -500) edit_temp_low = -500;
                }
                RequestPartialRefresh();
            }
            else if(current_page == PAGE_HUMIDITY && g_hum_thresh.edit_state != THRESH_EDIT_NONE)
            {
                if(g_hum_thresh.edit_state == THRESH_EDIT_HIGH)
                {
                    edit_hum_high -= 100;  // -10.0%
                    if(edit_hum_high < 0) edit_hum_high = 0;
                }
                else
                {
                    edit_hum_low -= 100;  // -10.0%
                    if(edit_hum_low < 0) edit_hum_low = 0;
                }
                RequestPartialRefresh();
            }
            else if(current_page == PAGE_PRESSURE && g_press_thresh.edit_state != THRESH_EDIT_NONE)
            {
                if(g_press_thresh.edit_state == THRESH_EDIT_HIGH)
                {
                    edit_press_high -= 100;  // -10.0 hPa
                    if(edit_press_high < 8000) edit_press_high = 8000;
                }
                else
                {
                    edit_press_low -= 100;  // -10.0 hPa
                    if(edit_press_low < 8000) edit_press_low = 8000;
                }
                RequestPartialRefresh();
            }
            break;
            
        case 4: // 确认 - 切换编辑状态
            if(current_page == PAGE_TEMP)
            {
                if(g_temp_thresh.edit_state == THRESH_EDIT_NONE)
                {
                    edit_temp_high = g_temp_thresh.high;
                    g_temp_thresh.edit_state = THRESH_EDIT_HIGH;
                }
                else if(g_temp_thresh.edit_state == THRESH_EDIT_HIGH)
                {
                    g_temp_thresh.high = edit_temp_high;
                    edit_temp_low = g_temp_thresh.low;
                    g_temp_thresh.edit_state = THRESH_EDIT_LOW;
                }
                else
                {
                    g_temp_thresh.low = edit_temp_low;
                    g_temp_thresh.edit_state = THRESH_EDIT_NONE;
                    ApplyTempThreshold();
                }
                RequestPartialRefresh();
            }
            else if(current_page == PAGE_HUMIDITY)
            {
                if(g_hum_thresh.edit_state == THRESH_EDIT_NONE)
                {
                    edit_hum_high = g_hum_thresh.high;
                    g_hum_thresh.edit_state = THRESH_EDIT_HIGH;
                }
                else if(g_hum_thresh.edit_state == THRESH_EDIT_HIGH)
                {
                    g_hum_thresh.high = edit_hum_high;
                    edit_hum_low = g_hum_thresh.low;
                    g_hum_thresh.edit_state = THRESH_EDIT_LOW;
                }
                else
                {
                    g_hum_thresh.low = edit_hum_low;
                    g_hum_thresh.edit_state = THRESH_EDIT_NONE;
                    ApplyHumThreshold();
                }
                RequestPartialRefresh();
            }
            else if(current_page == PAGE_PRESSURE)
            {
                if(g_press_thresh.edit_state == THRESH_EDIT_NONE)
                {
                    edit_press_high = g_press_thresh.high;
                    g_press_thresh.edit_state = THRESH_EDIT_HIGH;
                }
                else if(g_press_thresh.edit_state == THRESH_EDIT_HIGH)
                {
                    g_press_thresh.high = edit_press_high;
                    edit_press_low = g_press_thresh.low;
                    g_press_thresh.edit_state = THRESH_EDIT_LOW;
                }
                else
                {
                    g_press_thresh.low = edit_press_low;
                    g_press_thresh.edit_state = THRESH_EDIT_NONE;
                    ApplyPressThreshold();
                }
                RequestPartialRefresh();
            }
            break;
            
        default:
            break;
    }
}

/* ========== 页面绘制函数 ========== */

static void UiFill(u16 x1, u16 y1, u16 x2, u16 y2, u16 color)
{
    if(lcddev.width == 0 || lcddev.height == 0) return;
    if(x1 > x2 || y1 > y2) return;
    if(x1 >= lcddev.width || y1 >= lcddev.height) return;
    if(x2 >= lcddev.width) x2 = lcddev.width - 1;
    if(y2 >= lcddev.height) y2 = lcddev.height - 1;
    if(x1 > x2 || y1 > y2) return;

    LCD_Fill(x1, y1, x2, y2, color);
}

static void UiTextCenter(u16 x1, u16 x2, u16 y, u16 fc, u16 bc, u8 *text, u8 size)
{
    u16 char_w = size / 2;
    u16 text_w;
    u16 x;

    if(char_w == 0 || text == NULL) return;
    text_w = (u16)(strlen((char*)text) * char_w);
    x = x1;
    if(x2 > x1 && text_w < (u16)(x2 - x1 + 1))
    {
        x = x1 + (u16)(((x2 - x1 + 1) - text_w) / 2);
    }
    LCD_ShowString(x, y, fc, bc, size, text, 1);
}

static void FillRoundRect(u16 x1, u16 y1, u16 x2, u16 y2, u16 r, u16 color)
{
    u32 w, h, min_size;

    if(lcddev.width == 0 || lcddev.height == 0) return;
    if(x1 > x2 || y1 > y2) return;
    if(x1 >= lcddev.width || y1 >= lcddev.height) return;
    if(x2 >= lcddev.width) x2 = lcddev.width - 1;
    if(y2 >= lcddev.height) y2 = lcddev.height - 1;
    if(x1 > x2 || y1 > y2) return;

    w = (u32)x2 - x1 + 1;
    h = (u32)y2 - y1 + 1;
    min_size = (u32)r * 2 + 1;
    if(r == 0 || w < min_size || h < min_size)
    {
        UiFill(x1, y1, x2, y2, color);
        return;
    }

    UiFill(x1 + r, y1, x2 - r, y2, color);
    UiFill(x1, y1 + r, x2, y2 - r, color);
    gui_circle(x1 + r, y1 + r, color, r, 1);
    gui_circle(x2 - r, y1 + r, color, r, 1);
    gui_circle(x1 + r, y2 - r, color, r, 1);
    gui_circle(x2 - r, y2 - r, color, r, 1);
}

static void DrawCard(u16 x1, u16 y1, u16 x2, u16 y2, u16 r, u16 accent)
{
    FillRoundRect(x1 + 3, y1 + 4, x2 + 3, y2 + 4, r, UI_SHADOW);
    FillRoundRect(x1, y1, x2, y2, r, UI_CARD);
    UiFill(x1, y1, x2, y1 + 30, accent);
}

static void DrawMiniIcon(u16 cx, u16 cy, u8 type, u16 color)
{
    POINT_COLOR = color;
    if(type == 0)
    {
        LCD_DrawLine(cx, cy - 16, cx, cy + 7);
        LCD_DrawLine(cx - 4, cy - 16, cx + 4, cy - 16);
        LCD_DrawLine(cx - 4, cy - 16, cx - 4, cy + 7);
        LCD_DrawLine(cx + 4, cy - 16, cx + 4, cy + 7);
        gui_circle(cx, cy + 10, color, 8, 0);
        gui_circle(cx, cy + 10, color, 4, 1);
        LCD_Fill(cx - 2, cy - 7, cx + 2, cy + 9, color);
        LCD_DrawLine(cx + 8, cy - 12, cx + 14, cy - 12);
        LCD_DrawLine(cx + 8, cy - 5, cx + 13, cy - 5);
        LCD_DrawLine(cx + 8, cy + 2, cx + 14, cy + 2);
    }
    else if(type == 1)
    {
        gui_circle(cx, cy + 2, color, 17, 0);
        LCD_DrawLine(cx, cy - 19, cx - 14, cy + 7);
        LCD_DrawLine(cx, cy - 19, cx + 14, cy + 7);
        LCD_DrawLine(cx - 7, cy + 11, cx + 7, cy + 11);
        LCD_DrawLine(cx - 8, cy + 4, cx - 2, cy + 10);
    }
    else
    {
        gui_circle(cx, cy, color, 19, 0);
        gui_circle(cx, cy, color, 14, 0);
        LCD_DrawLine(cx, cy, cx + 8, cy - 9);
        LCD_DrawLine(cx, cy, cx - 10, cy + 5);
        gui_circle(cx, cy, color, 3, 1);
        LCD_DrawLine(cx - 12, cy + 12, cx + 12, cy + 12);
    }
}

static void DrawValueCard(u16 x1, u16 y1, u16 x2, u16 y2, u16 accent, u8 *title)
{
    DrawCard(x1, y1, x2, y2, 16, accent);
    UiTextCenter(x1, x2, y1 + 8, WHITE, accent, title, 16);
}

static void DrawPressureCard(void)
{
    DrawCard(20, 198, 400, 252, 14, UI_LIME);
    UiTextCenter(20, 400, 207, WHITE, UI_LIME, (u8*)"PRESSURE", 16);
}

static void DrawHintBar(void)
{
    UiFill(0, 258, 479, 279, UI_HINT);
    LCD_ShowString(42, 262, BLACK, UI_HINT, 16, (u8*)"Key Left/Right: switch", 1);
    LCD_ShowString(232, 262, BLACK, UI_HINT, 16, (u8*)"Up/Down: adjust  OK: confirm", 1);
}

void DrawTitleBar(u8 *title)
{
    u16 right = (lcddev.width > 0) ? (lcddev.width - 1) : 479;

    UiFill(0, 0, right, 13, 0x045F);
    UiFill(0, 14, right, 27, UI_BLUE);
    UiFill(0, 28, right, 40, UI_PURPLE);
    UiTextCenter(0, right, 12, WHITE, UI_BLUE, title, 16);
}

void DrawNavBar(void)
{
    u16 i;
    u8 *btn_texts[PAGE_COUNT] = {(u8*)"Home", (u8*)"Temp", (u8*)"Hum", (u8*)"Press", (u8*)"Wave"};
    u16 nav_width;
    u16 nav_bottom;

    if(lcddev.width == 0 || lcddev.height == 0) return;
    nav_width = lcddev.width / PAGE_COUNT;
    nav_bottom = (NAV_BTN_Y_END >= lcddev.height) ? (lcddev.height - 1) : NAV_BTN_Y_END;

    UiFill(0, NAV_BTN_Y_START, lcddev.width - 1, nav_bottom, UI_BG);
    for(i = 0; i < PAGE_COUNT; i++)
    {
        u16 x1 = i * nav_width;
        u16 x2 = (i + 1) * nav_width - 1;
        u16 bg = (i == current_page) ? GREEN : UI_NAV_OFF;
        u16 fg = (i == current_page) ? WHITE : UI_TEXT;
        if(i == PAGE_COUNT - 1) x2 = lcddev.width - 1;

        FillRoundRect(x1 + 3, NAV_BTN_Y_START + 3, x2 - 3, nav_bottom - 2, 14, bg);
        u16 text_x = x1 + (nav_width - strlen((char*)btn_texts[i]) * 8) / 2;
        u16 text_y = NAV_BTN_Y_START + 10;
        LCD_ShowString(text_x, text_y, fg, bg, 16, btn_texts[i], 1);
    }
}

static void DrawHomeDynamic(void)
{
    char buf[32];
    u16 temp_color = g_temp_alert ? RED : BLACK;
    u16 hum_color = g_hum_alert ? BLUE : BLACK;
    u16 press_color = g_press_alert ? MAGENTA : BLACK;

    sprintf(buf, "%2d.%d C", g_temp / 10, abs(g_temp % 10));
    LCD_Fill(52, 109, 170, 128, UI_CARD);
    LCD_ShowString(52, 109, temp_color, UI_CARD, 16, (u8*)buf, 1);
    DrawMiniIcon(109, 168, 0, temp_color);

    sprintf(buf, "%2d.%d %%", g_humidity / 10, abs(g_humidity % 10));
    LCD_Fill(254, 109, 372, 128, UI_CARD);
    LCD_ShowString(254, 109, hum_color, UI_CARD, 16, (u8*)buf, 1);
    DrawMiniIcon(311, 168, 1, hum_color);

    sprintf(buf, "%4d.%d hPa", g_pressure / 10, abs(g_pressure % 10));
    LCD_Fill(104, 229, 228, 248, UI_CARD);
    LCD_ShowString(104, 229, press_color, UI_CARD, 16, (u8*)buf, 1);
    DrawMiniIcon(334, 233, 2, press_color);
}

static void DrawTempDynamic(void)
{
    char buf[32];
    u16 high_bg = UI_CARD, low_bg = UI_CARD;
    int high_val = g_temp_thresh.high;
    int low_val = g_temp_thresh.low;

    if(g_temp_thresh.edit_state == THRESH_EDIT_HIGH)
    {
        high_bg = UI_LIME;
        high_val = edit_temp_high;
    }
    else if(g_temp_thresh.edit_state == THRESH_EDIT_LOW)
    {
        low_bg = UI_LIME;
        low_val = edit_temp_low;
    }

    LCD_Fill(65, 102, 210, 126, UI_CARD);
    POINT_COLOR = g_temp_alert ? RED : BLACK;
    sprintf(buf, "%2d.%d", g_temp / 10, abs(g_temp % 10));
    LCD_ShowString(65, 102, POINT_COLOR, UI_CARD, 16, (u8*)buf, 1);
    LCD_ShowString(105, 102, BLACK, UI_CARD, 16, (u8*)"C", 1);
    gui_circle(103, 104, BLACK, 2, 0);

    FillRoundRect(65, 153, 305, 176, 8, high_bg);
    sprintf(buf, "High: >%d.%d C", high_val / 10, abs(high_val % 10));
    LCD_ShowString(75, 157, BLACK, high_bg, 16, (u8*)buf, 1);

    FillRoundRect(65, 184, 305, 207, 8, low_bg);
    sprintf(buf, "Low:  <%d.%d C", low_val / 10, abs(low_val % 10));
    LCD_ShowString(75, 188, BLACK, low_bg, 16, (u8*)buf, 1);

    LCD_Fill(340, 100, 390, 150, UI_CARD);
    if(g_temp_alert)
        Gui_Drawbmp16_Custom(340, 100, ICON_TEMPBIG_W, ICON_TEMPBIG_H, gImage_high_temp);
    else
        Gui_Drawbmp16_Custom(340, 100, ICON_NORMALBIG_W, ICON_NORMALBIG_H, gImage_normal);
}

static void DrawHumidityDynamic(void)
{
    char buf[32];
    u16 high_bg = UI_CARD, low_bg = UI_CARD;
    int high_val = g_hum_thresh.high;
    int low_val = g_hum_thresh.low;

    if(g_hum_thresh.edit_state == THRESH_EDIT_HIGH)
    {
        high_bg = UI_LIME;
        high_val = edit_hum_high;
    }
    else if(g_hum_thresh.edit_state == THRESH_EDIT_LOW)
    {
        low_bg = UI_LIME;
        low_val = edit_hum_low;
    }

    LCD_Fill(65, 102, 190, 126, UI_CARD);
    POINT_COLOR = g_hum_alert ? BLUE : BLACK;
    sprintf(buf, "%2d.%d", g_humidity / 10, abs(g_humidity % 10));
    LCD_ShowString(65, 102, POINT_COLOR, UI_CARD, 16, (u8*)buf, 1);
    LCD_ShowString(105, 102, BLACK, UI_CARD, 16, (u8*)"%", 1);

    FillRoundRect(65, 153, 305, 176, 8, high_bg);
    sprintf(buf, "High: >%d.%d %%", high_val / 10, abs(high_val % 10));
    LCD_ShowString(75, 157, BLACK, high_bg, 16, (u8*)buf, 1);

    FillRoundRect(65, 184, 305, 207, 8, low_bg);
    sprintf(buf, "Low:  <%d.%d %%", low_val / 10, abs(low_val % 10));
    LCD_ShowString(75, 188, BLACK, low_bg, 16, (u8*)buf, 1);

    LCD_Fill(340, 100, 390, 150, UI_CARD);
    if(g_hum_alert)
        Gui_Drawbmp16_Custom(340, 100, ICON_HUMIDBIG_W, ICON_HUMIDBIG_H, gImage_high_humidity);
    else
        Gui_Drawbmp16_Custom(340, 100, ICON_NORMALBIG_W, ICON_NORMALBIG_H, gImage_normal);
}

static void DrawPressureDynamic(void)
{
    char buf[32];
    u16 high_bg = UI_CARD, low_bg = UI_CARD;
    int high_val = g_press_thresh.high;
    int low_val = g_press_thresh.low;

    if(g_press_thresh.edit_state == THRESH_EDIT_HIGH)
    {
        high_bg = UI_LIME;
        high_val = edit_press_high;
    }
    else if(g_press_thresh.edit_state == THRESH_EDIT_LOW)
    {
        low_bg = UI_LIME;
        low_val = edit_press_low;
    }

    LCD_Fill(65, 102, 235, 126, UI_CARD);
    POINT_COLOR = g_press_alert ? MAGENTA : BLACK;
    sprintf(buf, "%4d.%d", g_pressure / 10, abs(g_pressure % 10));
    LCD_ShowString(65, 102, POINT_COLOR, UI_CARD, 16, (u8*)buf, 1);
    LCD_ShowString(120, 102, BLACK, UI_CARD, 16, (u8*)"hPa", 1);

    FillRoundRect(65, 153, 305, 176, 8, high_bg);
    sprintf(buf, "High: >%d.%d hPa", high_val / 10, abs(high_val % 10));
    LCD_ShowString(75, 157, BLACK, high_bg, 16, (u8*)buf, 1);

    FillRoundRect(65, 184, 305, 207, 8, low_bg);
    sprintf(buf, "Low:  <%d.%d hPa", low_val / 10, abs(low_val % 10));
    LCD_ShowString(75, 188, BLACK, low_bg, 16, (u8*)buf, 1);

    LCD_Fill(340, 100, 390, 150, UI_CARD);
    if(g_press_alert)
        Gui_Drawbmp16_Custom(340, 100, ICON_PRESSBIG_W, ICON_PRESSBIG_H, gImage_highQIYA);
    else
        Gui_Drawbmp16_Custom(340, 100, ICON_NORMALBIG_W, ICON_NORMALBIG_H, gImage_normal);
}

static void PushWaveSample(void)
{
    if(current_page == PAGE_WAVE && !need_full_refresh && wave_count > 0)
    {
        DrawWaveSeries(wave_temp, WAVE_TEMP_MIN, WAVE_TEMP_MAX, WAVE_CHART_X, WAVE_TEMP_Y, WAVE_CHART_W, WAVE_CHART_H, UI_CARD);
        DrawWaveSeries(wave_hum, WAVE_HUM_MIN, WAVE_HUM_MAX, WAVE_CHART_X, WAVE_HUM_Y, WAVE_CHART_W, WAVE_CHART_H, UI_CARD);
        DrawWaveSeries(wave_press, WAVE_PRESS_MIN, WAVE_PRESS_MAX, WAVE_CHART_X, WAVE_PRESS_Y, WAVE_CHART_W, WAVE_CHART_H, UI_CARD);
    }

    wave_temp[wave_pos] = (s16)g_temp;
    wave_hum[wave_pos] = (s16)g_humidity;
    wave_press[wave_pos] = (s16)g_pressure;
    wave_pos++;
    if(wave_pos >= WAVE_POINTS) wave_pos = 0;
    if(wave_count < WAVE_POINTS) wave_count++;
}

static s16 ClampWaveValue(s16 value, s16 min, s16 max)
{
    if(value < min) return min;
    if(value > max) return max;
    return value;
}

static u16 ScaleWaveY(s16 value, s16 min, s16 max, u16 y, u16 h)
{
    s32 range = max - min;
    s32 scaled;

    value = ClampWaveValue(value, min, max);
    scaled = ((s32)(value - min) * (h - 1)) / range;
    return y + h - 1 - (u16)scaled;
}

static u8 WaveDataIndex(u8 point)
{
    if(wave_count < WAVE_POINTS) return point;
    return (wave_pos + point) % WAVE_POINTS;
}

static void FormatValue(char *buf, s16 value, u8 unit)
{
    if(unit == 0)
    {
        sprintf(buf, "%d.%d C", value / 10, abs(value % 10));
    }
    else if(unit == 1)
    {
        sprintf(buf, "%d.%d %%", value / 10, abs(value % 10));
    }
    else
    {
        sprintf(buf, "%d.%d hPa", value / 10, abs(value % 10));
    }
}

static void DrawWaveFrame(u16 y, u8 *name, u8 *range, u16 color, char *value)
{
    FillRoundRect(5, y - 4, 474, y + WAVE_CHART_H + 4, 10, UI_CARD);
    POINT_COLOR = BLACK;
    BACK_COLOR = UI_CARD;
    LCD_ShowString(12, y + 2, BLACK, UI_CARD, 16, name, 1);
    LCD_ShowString(12, y + 22, color, UI_CARD, 16, (u8*)value, 1);
    LCD_ShowString(12, y + 44, UI_MUTED, UI_CARD, 12, range, 1);
    LCD_DrawRectangle(WAVE_CHART_X - 1, y - 1, WAVE_CHART_X + WAVE_CHART_W, y + WAVE_CHART_H);
}

static void DrawWaveSeries(s16 *data, s16 min, s16 max, u16 x, u16 y, u16 w, u16 h, u16 color)
{
    u8 i;
    u8 first_slot;
    u16 last_x, last_y, px, py;

    if(wave_count == 0) return;
    first_slot = WAVE_POINTS - wave_count;

    POINT_COLOR = color;
    if(wave_count == 1)
    {
        px = x + ((u32)(WAVE_POINTS - 1) * (w - 1)) / (WAVE_POINTS - 1);
        py = ScaleWaveY(data[WaveDataIndex(0)], min, max, y, h);
        LCD_DrawLine(px - 1, py, px, py);
        return;
    }

    last_x = x + ((u32)first_slot * (w - 1)) / (WAVE_POINTS - 1);
    last_y = ScaleWaveY(data[WaveDataIndex(0)], min, max, y, h);
    for(i = 1; i < wave_count; i++)
    {
        px = x + ((u32)(first_slot + i) * (w - 1)) / (WAVE_POINTS - 1);
        py = ScaleWaveY(data[WaveDataIndex(i)], min, max, y, h);
        LCD_DrawLine(last_x, last_y, px, py);
        last_x = px;
        last_y = py;
    }
}

static void DrawWaveDynamic(void)
{
    char value[20];

    LCD_Fill(8, WAVE_TEMP_Y, WAVE_CHART_X - 2, WAVE_TEMP_Y + WAVE_CHART_H, UI_CARD);
    FormatValue(value, (s16)g_temp, 0);
    DrawWaveFrame(WAVE_TEMP_Y, (u8*)"Temp", (u8*)"-10..50", RED, value);
    DrawWaveSeries(wave_temp, WAVE_TEMP_MIN, WAVE_TEMP_MAX, WAVE_CHART_X, WAVE_TEMP_Y, WAVE_CHART_W, WAVE_CHART_H, RED);

    LCD_Fill(8, WAVE_HUM_Y, WAVE_CHART_X - 2, WAVE_HUM_Y + WAVE_CHART_H, UI_CARD);
    FormatValue(value, (s16)g_humidity, 1);
    DrawWaveFrame(WAVE_HUM_Y, (u8*)"Hum", (u8*)"0..100%", BLUE, value);
    DrawWaveSeries(wave_hum, WAVE_HUM_MIN, WAVE_HUM_MAX, WAVE_CHART_X, WAVE_HUM_Y, WAVE_CHART_W, WAVE_CHART_H, BLUE);

    LCD_Fill(8, WAVE_PRESS_Y, WAVE_CHART_X - 2, WAVE_PRESS_Y + WAVE_CHART_H, UI_CARD);
    FormatValue(value, (s16)g_pressure, 2);
    DrawWaveFrame(WAVE_PRESS_Y, (u8*)"Press", (u8*)"900..1100", MAGENTA, value);
    DrawWaveSeries(wave_press, WAVE_PRESS_MIN, WAVE_PRESS_MAX, WAVE_CHART_X, WAVE_PRESS_Y, WAVE_CHART_W, WAVE_CHART_H, MAGENTA);
}

void DrawPage_Home(void)
{
    LCD_Clear(UI_BG);
    DrawTitleBar((u8*)"ENVIRONMENT MONITORING SYSTEM");
    DrawValueCard(20, 60, 198, 188, UI_ORANGE, (u8*)"TEMPERATURE");
    DrawValueCard(222, 60, 400, 188, UI_TEAL, (u8*)"HUMIDITY");
    DrawPressureCard();
    Gui_Drawbmp16_Custom(417, 48, ICON_PEIXIAO_W, ICON_PEIXIAO_H, gImage_peixiao);
    LCD_ShowString(408, 130, UI_PURPLE, UI_BG, 16, (u8*)"GROUP 11", 1);
    LCD_ShowString(410, 156, UI_TEXT, UI_BG, 16, (u8*)"Sensor 1", 1);
    LCD_ShowString(410, 186, UI_TEXT, UI_BG, 16, (u8*)"Sensor 2", 1);
    LCD_ShowString(410, 216, UI_TEXT, UI_BG, 16, (u8*)"Sensor 3", 1);
    DrawHomeDynamic();
    DrawHintBar();
    DrawNavBar();
}

void DrawPage_Temp(void)
{
    LCD_Clear(UI_BG);
    DrawTitleBar((u8*)"Temperature");
    DrawCard(40, 65, 425, 222, 18, UI_ORANGE);
    LCD_ShowString(65, 78, WHITE, UI_ORANGE, 16, (u8*)"THRESHOLD PANEL", 1);
    DrawMiniIcon(380, 170, 0, UI_ORANGE);
    DrawHintBar();
     
    DrawTempDynamic();
    DrawNavBar();
}

void DrawPage_Humidity(void)
{
    LCD_Clear(UI_BG);
    DrawTitleBar((u8*)"Humidity");
    DrawCard(40, 65, 425, 222, 18, UI_TEAL);
    LCD_ShowString(65, 78, WHITE, UI_TEAL, 16, (u8*)"THRESHOLD PANEL", 1);
    DrawMiniIcon(380, 170, 1, UI_TEAL);
    DrawHintBar();
     
    DrawHumidityDynamic();
    DrawNavBar();
}

void DrawPage_Pressure(void)
{
    LCD_Clear(UI_BG);
    DrawTitleBar((u8*)"Pressure");
    DrawCard(40, 65, 425, 222, 18, UI_LIME);
    LCD_ShowString(65, 78, WHITE, UI_LIME, 16, (u8*)"THRESHOLD PANEL", 1);
    DrawMiniIcon(380, 170, 2, UI_LIME);
    DrawHintBar();
     
    DrawPressureDynamic();
    DrawNavBar();
}

void DrawPage_Wave(void)
{
    LCD_Clear(UI_BG);
    DrawTitleBar((u8*)"Wave Monitor");
    DrawWaveDynamic();
    DrawNavBar();
}

void SwitchPage(u8 page)
{
    if(page >= PAGE_COUNT) return;
    current_page = page;
    RequestFullRefresh();
}

void RefreshPage(void)
{
    if(!need_refresh) return;

    if(!need_full_refresh)
    {
        switch(current_page)
        {
            case PAGE_HOME:
                DrawHomeDynamic();
                break;
            case PAGE_TEMP:
                DrawTempDynamic();
                break;
            case PAGE_HUMIDITY:
                DrawHumidityDynamic();
                break;
            case PAGE_PRESSURE:
                DrawPressureDynamic();
                break;
            case PAGE_WAVE:
                DrawWaveDynamic();
                break;
            default:
                break;
        }
        FinishRefresh();
        return;
    }
    
    switch(current_page)
    {
        case PAGE_HOME:
            DrawPage_Home();
            break;
        case PAGE_TEMP:
            DrawPage_Temp();
            break;
        case PAGE_HUMIDITY:
            DrawPage_Humidity();
            break;
        case PAGE_PRESSURE:
            DrawPage_Pressure();
            break;
        case PAGE_WAVE:
            DrawPage_Wave();
            break;
        default:
            break;
    }
    FinishRefresh();
}

/* ========== 触摸处理（保留兼容） ========== */

u8 HandleNavTouch(u16 x, u16 y)
{
    u16 nav_width = lcddev.width / PAGE_COUNT;
    u8 page;

    if(y < NAV_BTN_Y_START || y > NAV_BTN_Y_END) return 0xFF;

    page = x / nav_width;
    if(page >= PAGE_COUNT) page = PAGE_COUNT - 1;
    return page;
}

void HandleTouch(u16 x, u16 y)
{
    u8 new_page;
    
    // 1. 检查导航栏
    new_page = HandleNavTouch(x, y);
    if(new_page != 0xFF && new_page != current_page)
    {
        ExitEditAndApply();
        SwitchPage(new_page);
        return;
    }
    
    // 2. 首页：点击数据区域跳转到对应页面
    if(current_page == PAGE_HOME)
    {
        if(y >= 40 && y <= 70)        // 温度区域
        {
            ExitEditAndApply();
            SwitchPage(PAGE_TEMP);
        }
        else if(y >= 90 && y <= 120)  // 湿度区域
        {
            ExitEditAndApply();
            SwitchPage(PAGE_HUMIDITY);
        }
        else if(y >= 140 && y <= 170) // 气压区域
        {
            ExitEditAndApply();
            SwitchPage(PAGE_PRESSURE);
        }
    }
    // 3. 具体页面：点击 High/Low 行进入对应编辑状态
    else if(current_page == PAGE_TEMP)
    {
        if(y >= 150 && y <= 170)      // High 行
        {
            if(g_temp_thresh.edit_state == THRESH_EDIT_NONE)
            {
                edit_temp_high = g_temp_thresh.high;
                g_temp_thresh.edit_state = THRESH_EDIT_HIGH;
            }
            else if(g_temp_thresh.edit_state == THRESH_EDIT_HIGH)
            {
                g_temp_thresh.high = edit_temp_high;
                edit_temp_low = g_temp_thresh.low;
                g_temp_thresh.edit_state = THRESH_EDIT_LOW;
            }
            else
            {
                g_temp_thresh.low = edit_temp_low;
                g_temp_thresh.edit_state = THRESH_EDIT_NONE;
                ApplyTempThreshold();
            }
            RequestPartialRefresh();
        }
        else if(y >= 180 && y <= 200) // Low 行
        {
            if(g_temp_thresh.edit_state == THRESH_EDIT_NONE)
            {
                edit_temp_low = g_temp_thresh.low;
                g_temp_thresh.edit_state = THRESH_EDIT_LOW;
            }
            else if(g_temp_thresh.edit_state == THRESH_EDIT_LOW)
            {
                g_temp_thresh.low = edit_temp_low;
                edit_temp_high = g_temp_thresh.high;
                g_temp_thresh.edit_state = THRESH_EDIT_HIGH;
            }
            else
            {
                g_temp_thresh.high = edit_temp_high;
                g_temp_thresh.edit_state = THRESH_EDIT_NONE;
                ApplyTempThreshold();
            }
            RequestPartialRefresh();
        }
    }
    else if(current_page == PAGE_HUMIDITY)
    {
        if(y >= 150 && y <= 170)      // High 行
        {
            if(g_hum_thresh.edit_state == THRESH_EDIT_NONE)
            {
                edit_hum_high = g_hum_thresh.high;
                g_hum_thresh.edit_state = THRESH_EDIT_HIGH;
            }
            else if(g_hum_thresh.edit_state == THRESH_EDIT_HIGH)
            {
                g_hum_thresh.high = edit_hum_high;
                edit_hum_low = g_hum_thresh.low;
                g_hum_thresh.edit_state = THRESH_EDIT_LOW;
            }
            else
            {
                g_hum_thresh.low = edit_hum_low;
                g_hum_thresh.edit_state = THRESH_EDIT_NONE;
                ApplyHumThreshold();
            }
            RequestPartialRefresh();
        }
        else if(y >= 180 && y <= 200) // Low 行
        {
            if(g_hum_thresh.edit_state == THRESH_EDIT_NONE)
            {
                edit_hum_low = g_hum_thresh.low;
                g_hum_thresh.edit_state = THRESH_EDIT_LOW;
            }
            else if(g_hum_thresh.edit_state == THRESH_EDIT_LOW)
            {
                g_hum_thresh.low = edit_hum_low;
                edit_hum_high = g_hum_thresh.high;
                g_hum_thresh.edit_state = THRESH_EDIT_HIGH;
            }
            else
            {
                g_hum_thresh.high = edit_hum_high;
                g_hum_thresh.edit_state = THRESH_EDIT_NONE;
                ApplyHumThreshold();
            }
            RequestPartialRefresh();
        }
    }
    else if(current_page == PAGE_PRESSURE)
    {
        if(y >= 150 && y <= 170)      // High 行
        {
            if(g_press_thresh.edit_state == THRESH_EDIT_NONE)
            {
                edit_press_high = g_press_thresh.high;
                g_press_thresh.edit_state = THRESH_EDIT_HIGH;
            }
            else if(g_press_thresh.edit_state == THRESH_EDIT_HIGH)
            {
                g_press_thresh.high = edit_press_high;
                edit_press_low = g_press_thresh.low;
                g_press_thresh.edit_state = THRESH_EDIT_LOW;
            }
            else
            {
                g_press_thresh.low = edit_press_low;
                g_press_thresh.edit_state = THRESH_EDIT_NONE;
                ApplyPressThreshold();
            }
            RequestPartialRefresh();
        }
        else if(y >= 180 && y <= 200) // Low 行
        {
            if(g_press_thresh.edit_state == THRESH_EDIT_NONE)
            {
                edit_press_low = g_press_thresh.low;
                g_press_thresh.edit_state = THRESH_EDIT_LOW;
            }
            else if(g_press_thresh.edit_state == THRESH_EDIT_LOW)
            {
                g_press_thresh.low = edit_press_low;
                edit_press_high = g_press_thresh.high;
                g_press_thresh.edit_state = THRESH_EDIT_HIGH;
            }
            else
            {
                g_press_thresh.high = edit_press_high;
                g_press_thresh.edit_state = THRESH_EDIT_NONE;
                ApplyPressThreshold();
            }
            RequestPartialRefresh();
        }
    }
}

/* ========== 传感器处理 ========== */

void UpdateSensorData(void)
{
    float real_temp, real_humidity, real_pressure;
    u8 alert_changed = 0;
    
    if(AHT20_GetData(&real_temp, &real_humidity) == 0)
    {
        g_temp = (int)(real_temp * 10);
        g_humidity = (int)(real_humidity * 10);
        
        // 只在非编辑状态下更新报警
        if(g_temp_thresh.edit_state == THRESH_EDIT_NONE)
        {
            g_temp_alert = (g_temp > g_temp_thresh.high || g_temp < g_temp_thresh.low) ? 1 : 0;
            if(g_temp_alert != last_temp_alert)
            {
                alert_changed = 1;
                last_temp_alert = g_temp_alert;
            }
        }
        
        if(g_hum_thresh.edit_state == THRESH_EDIT_NONE)
        {
            g_hum_alert = (g_humidity > g_hum_thresh.high || g_humidity < g_hum_thresh.low) ? 1 : 0;
            if(g_hum_alert != last_hum_alert)
            {
                alert_changed = 1;
                last_hum_alert = g_hum_alert;
            }
        }
    }
    
    if(BMP280_GetPressure(&real_pressure) == 0)
    {
        g_pressure = (int)(real_pressure * 10);
        
        if(g_press_thresh.edit_state == THRESH_EDIT_NONE)
        {
            g_press_alert = (g_pressure > g_press_thresh.high || g_pressure < g_press_thresh.low) ? 1 : 0;
            if(g_press_alert != last_press_alert)
            {
                alert_changed = 1;
                last_press_alert = g_press_alert;
            }
        }
    }

    PushWaveSample();
    
    if(alert_changed)
    {
        RequestPartialRefresh();
    }
    
    LED_RefreshMode();
}

/* ========== 主函数 ========== */

void main_test(void)
{
    u8 key;

    TP_Init();
    KEY_Init();         // 初始化物理按键
    LED_Init();         // 初始化LED
    BMP280_Init();
    
    UpdateSensorData();
    
    DrawPage_Home();
    FinishRefresh();
    
    while(1)
    {
        // 1. 扫描物理按键
        key = KEY_Scan();
        if(key != 0xFF)
        {
            HandleKey(key);
        }
        
        // 2. 扫描触摸（保留兼容）
        tp_dev.scan(0);
        if(tp_dev.sta & TP_PRES_DOWN)
        {
            if(tp_dev.x < lcddev.width && tp_dev.y < lcddev.height)
            {
                HandleTouch(tp_dev.x, tp_dev.y);
            }
        }
        
        // 3. 定时读取传感器（每2秒），只在首页和波形页自动刷新
        sensor_timer += 10;
        if(sensor_timer >= 2000)
        {
            UpdateSensorData();
            sensor_timer = 0;
            if(current_page == PAGE_HOME || current_page == PAGE_WAVE)
            {
                RequestPartialRefresh();
            }
        }
        
        // 4. 刷新页面
        RefreshPage();
        LED_Update();
        
        delay_ms(10);
    }
}
