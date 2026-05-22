#include "main.h"
#include "lcd.h"
#include "GUI.h"
#include "touch.h"
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

#define WAVE_POINTS     80
#define WAVE_CHART_X    108
#define WAVE_CHART_W    360
#define WAVE_CHART_H    62
#define WAVE_TEMP_Y     42
#define WAVE_HUM_Y      120
#define WAVE_PRESS_Y    198

#define WAVE_TEMP_MIN   -100
#define WAVE_TEMP_MAX   500
#define WAVE_HUM_MIN    0
#define WAVE_HUM_MAX    1000
#define WAVE_PRESS_MIN  9000
#define WAVE_PRESS_MAX  11000

static s16 wave_temp[WAVE_POINTS];
static s16 wave_hum[WAVE_POINTS];
static s16 wave_press[WAVE_POINTS];
static u8 wave_count = 0;
static u8 wave_pos = 0;

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
    HAL_GPIO_WritePin(LED_GPIO_PORT, LED_GREEN_PIN | LED_RED_PIN, GPIO_PIN_SET);
}

void LED_Control(u8 green_on, u8 red_on)
{
    if(green_on)
        HAL_GPIO_WritePin(LED_GPIO_PORT, LED_GREEN_PIN, GPIO_PIN_RESET);
    else
        HAL_GPIO_WritePin(LED_GPIO_PORT, LED_GREEN_PIN, GPIO_PIN_SET);
    
    if(red_on)
        HAL_GPIO_WritePin(LED_GPIO_PORT, LED_RED_PIN, GPIO_PIN_RESET);
    else
        HAL_GPIO_WritePin(LED_GPIO_PORT, LED_RED_PIN, GPIO_PIN_SET);
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
}

static void ApplyHumThreshold(void)
{
    g_hum_alert = (g_humidity > g_hum_thresh.high || g_humidity < g_hum_thresh.low) ? 1 : 0;
    if(g_hum_alert != last_hum_alert)
    {
        last_hum_alert = g_hum_alert;
        RequestPartialRefresh();
    }
}

static void ApplyPressThreshold(void)
{
    g_press_alert = (g_pressure > g_press_thresh.high || g_pressure < g_press_thresh.low) ? 1 : 0;
    if(g_press_alert != last_press_alert)
    {
        last_press_alert = g_press_alert;
        RequestPartialRefresh();
    }
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

void DrawTitleBar(u8 *title)
{
    LCD_Fill(0, 0, 480, 35, DARKBLUE);
    POINT_COLOR = WHITE;
    BACK_COLOR = DARKBLUE;
    Gui_StrCenter(0, 8, WHITE, DARKBLUE, title, 16, 1);
}

void DrawNavBar(void)
{
    u16 i;
    u16 btn_colors[PAGE_COUNT] = {LIGHTGRAY, LIGHTGRAY, LIGHTGRAY, LIGHTGRAY, LIGHTGRAY};
    u8 *btn_texts[PAGE_COUNT] = {(u8*)"Home", (u8*)"Temp", (u8*)"Hum", (u8*)"Press", (u8*)"Wave"};
    u16 nav_width = lcddev.width / PAGE_COUNT;
    
    btn_colors[current_page] = GREEN;
    
    for(i = 0; i < PAGE_COUNT; i++)
    {
        u16 x1 = i * nav_width;
        u16 x2 = (i + 1) * nav_width - 1;
        if(i == PAGE_COUNT - 1) x2 = lcddev.width - 1;
        
        LCD_Fill(x1, NAV_BTN_Y_START, x2, NAV_BTN_Y_END, btn_colors[i]);
        POINT_COLOR = BLACK;
        BACK_COLOR = btn_colors[i];
        
        u16 text_x = x1 + (nav_width - strlen((char*)btn_texts[i]) * 8) / 2;
        u16 text_y = NAV_BTN_Y_START + 10;
        LCD_ShowString(text_x, text_y, BLACK, btn_colors[i], 16, btn_texts[i], 1);
    }
    
    for(i = 1; i < PAGE_COUNT; i++)
    {
        LCD_DrawLine(nav_width * i, NAV_BTN_Y_START, nav_width * i, NAV_BTN_Y_END);
    }
}

static void DrawHomeDynamic(void)
{
    char buf[32];

    LCD_Fill(140, 40, 260, 64, WHITE);
    sprintf(buf, "%2d.%d", g_temp / 10, abs(g_temp % 10));
    POINT_COLOR = g_temp_alert ? RED : BLACK;
    LCD_ShowString(140, 40, POINT_COLOR, WHITE, 16, (u8*)buf, 1);
    LCD_ShowString(195, 40, BLACK, WHITE, 16, (u8*)"C", 1);
    gui_circle(193, 42, BLACK, 2, 0);
    LCD_Fill(380, 30, 430, 80, WHITE);
    if(g_temp_alert)
        Gui_Drawbmp16_Custom(380, 30, ICON_HIGHTEMP_W, ICON_HIGHTEMP_H, gImage_high_temp);
    else
        Gui_Drawbmp16_Custom(380, 30, ICON_NORMAL_W, ICON_NORMAL_H, gImage_normal);

    LCD_Fill(140, 90, 260, 114, WHITE);
    sprintf(buf, "%2d.%d %%", g_humidity / 10, abs(g_humidity % 10));
    POINT_COLOR = g_hum_alert ? BLUE : BLACK;
    LCD_ShowString(140, 90, POINT_COLOR, WHITE, 16, (u8*)buf, 1);
    LCD_Fill(380, 80, 430, 130, WHITE);
    if(g_hum_alert)
        Gui_Drawbmp16_Custom(380, 80, ICON_HIGHHUM_W, ICON_HIGHHUM_H, gImage_high_humidity);
    else
        Gui_Drawbmp16_Custom(380, 80, ICON_NORMAL_W, ICON_NORMAL_H, gImage_normal);

    LCD_Fill(140, 140, 280, 164, WHITE);
    sprintf(buf, "%4d.%d hPa", g_pressure / 10, abs(g_pressure % 10));
    POINT_COLOR = g_press_alert ? MAGENTA : BLACK;
    LCD_ShowString(140, 140, POINT_COLOR, WHITE, 16, (u8*)buf, 1);
    LCD_Fill(380, 130, 430, 180, WHITE);
    if(g_press_alert)
        Gui_Drawbmp16_Custom(380, 130, ICON_HIGHQIYA_W, ICON_HIGHQIYA_H, gImage_highQIYA);
    else
        Gui_Drawbmp16_Custom(380, 130, ICON_NORMAL_W, ICON_NORMAL_H, gImage_normal);
}

static void DrawTempDynamic(void)
{
    char buf[32];
    u16 high_bg = WHITE, low_bg = WHITE;
    int high_val = g_temp_thresh.high;
    int low_val = g_temp_thresh.low;

    if(g_temp_thresh.edit_state == THRESH_EDIT_HIGH)
    {
        high_bg = GREEN;
        high_val = edit_temp_high;
    }
    else if(g_temp_thresh.edit_state == THRESH_EDIT_LOW)
    {
        low_bg = GREEN;
        low_val = edit_temp_low;
    }

    LCD_Fill(60, 100, 200, 124, WHITE);
    POINT_COLOR = g_temp_alert ? RED : BLACK;
    sprintf(buf, "%2d.%d", g_temp / 10, abs(g_temp % 10));
    LCD_ShowString(60, 100, POINT_COLOR, WHITE, 16, (u8*)buf, 1);
    LCD_ShowString(100, 100, BLACK, WHITE, 16, (u8*)"C", 1);
    gui_circle(98, 102, BLACK, 2, 0);

    LCD_Fill(60, 150, 300, 166, high_bg);
    sprintf(buf, "High: >%d.%d C", high_val / 10, abs(high_val % 10));
    LCD_ShowString(60, 150, BLACK, high_bg, 16, (u8*)buf, 1);

    LCD_Fill(60, 180, 300, 196, low_bg);
    sprintf(buf, "Low:  <%d.%d C", low_val / 10, abs(low_val % 10));
    LCD_ShowString(60, 180, BLACK, low_bg, 16, (u8*)buf, 1);

    LCD_Fill(340, 100, 390, 150, WHITE);
    if(g_temp_alert)
        Gui_Drawbmp16_Custom(340, 100, ICON_TEMPBIG_W, ICON_TEMPBIG_H, gImage_high_temp);
    else
        Gui_Drawbmp16_Custom(340, 100, ICON_NORMALBIG_W, ICON_NORMALBIG_H, gImage_normal);
}

static void DrawHumidityDynamic(void)
{
    char buf[32];
    u16 high_bg = WHITE, low_bg = WHITE;
    int high_val = g_hum_thresh.high;
    int low_val = g_hum_thresh.low;

    if(g_hum_thresh.edit_state == THRESH_EDIT_HIGH)
    {
        high_bg = GREEN;
        high_val = edit_hum_high;
    }
    else if(g_hum_thresh.edit_state == THRESH_EDIT_LOW)
    {
        low_bg = GREEN;
        low_val = edit_hum_low;
    }

    LCD_Fill(60, 100, 180, 124, WHITE);
    POINT_COLOR = g_hum_alert ? BLUE : BLACK;
    sprintf(buf, "%2d.%d", g_humidity / 10, abs(g_humidity % 10));
    LCD_ShowString(60, 100, POINT_COLOR, WHITE, 16, (u8*)buf, 1);
    LCD_ShowString(100, 100, BLACK, WHITE, 16, (u8*)"%", 1);

    LCD_Fill(60, 150, 300, 166, high_bg);
    sprintf(buf, "High: >%d.%d %%", high_val / 10, abs(high_val % 10));
    LCD_ShowString(60, 150, BLACK, high_bg, 16, (u8*)buf, 1);

    LCD_Fill(60, 180, 300, 196, low_bg);
    sprintf(buf, "Low:  <%d.%d %%", low_val / 10, abs(low_val % 10));
    LCD_ShowString(60, 180, BLACK, low_bg, 16, (u8*)buf, 1);

    LCD_Fill(340, 100, 390, 150, WHITE);
    if(g_hum_alert)
        Gui_Drawbmp16_Custom(340, 100, ICON_HUMIDBIG_W, ICON_HUMIDBIG_H, gImage_high_humidity);
    else
        Gui_Drawbmp16_Custom(340, 100, ICON_NORMALBIG_W, ICON_NORMALBIG_H, gImage_normal);
}

static void DrawPressureDynamic(void)
{
    char buf[32];
    u16 high_bg = WHITE, low_bg = WHITE;
    int high_val = g_press_thresh.high;
    int low_val = g_press_thresh.low;

    if(g_press_thresh.edit_state == THRESH_EDIT_HIGH)
    {
        high_bg = GREEN;
        high_val = edit_press_high;
    }
    else if(g_press_thresh.edit_state == THRESH_EDIT_LOW)
    {
        low_bg = GREEN;
        low_val = edit_press_low;
    }

    LCD_Fill(60, 100, 220, 124, WHITE);
    POINT_COLOR = g_press_alert ? MAGENTA : BLACK;
    sprintf(buf, "%4d.%d", g_pressure / 10, abs(g_pressure % 10));
    LCD_ShowString(60, 100, POINT_COLOR, WHITE, 16, (u8*)buf, 1);
    LCD_ShowString(115, 100, BLACK, WHITE, 16, (u8*)"hPa", 1);

    LCD_Fill(60, 150, 300, 166, high_bg);
    sprintf(buf, "High: >%d.%d hPa", high_val / 10, abs(high_val % 10));
    LCD_ShowString(60, 150, BLACK, high_bg, 16, (u8*)buf, 1);

    LCD_Fill(60, 180, 300, 196, low_bg);
    sprintf(buf, "Low:  <%d.%d hPa", low_val / 10, abs(low_val % 10));
    LCD_ShowString(60, 180, BLACK, low_bg, 16, (u8*)buf, 1);

    LCD_Fill(340, 100, 390, 150, WHITE);
    if(g_press_alert)
        Gui_Drawbmp16_Custom(340, 100, ICON_PRESSBIG_W, ICON_PRESSBIG_H, gImage_highQIYA);
    else
        Gui_Drawbmp16_Custom(340, 100, ICON_NORMALBIG_W, ICON_NORMALBIG_H, gImage_normal);
}

static void PushWaveSample(void)
{
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
    POINT_COLOR = BLACK;
    BACK_COLOR = WHITE;
    LCD_ShowString(8, y + 2, BLACK, WHITE, 16, name, 1);
    LCD_ShowString(8, y + 22, color, WHITE, 16, (u8*)value, 1);
    LCD_ShowString(8, y + 44, GRAY, WHITE, 12, range, 1);
    LCD_DrawRectangle(WAVE_CHART_X - 1, y - 1, WAVE_CHART_X + WAVE_CHART_W, y + WAVE_CHART_H);
}

static void DrawWaveSeries(s16 *data, s16 min, s16 max, u16 x, u16 y, u16 w, u16 h, u16 color)
{
    u8 i;
    u16 last_x, last_y, px, py;

    if(wave_count == 0) return;

    POINT_COLOR = color;
    if(wave_count == 1)
    {
        px = x + w - 1;
        py = ScaleWaveY(data[WaveDataIndex(0)], min, max, y, h);
        LCD_DrawLine(px, py, px + 1, py);
        return;
    }

    last_x = x;
    last_y = ScaleWaveY(data[WaveDataIndex(0)], min, max, y, h);
    for(i = 1; i < wave_count; i++)
    {
        px = x + ((u32)i * (w - 1)) / (WAVE_POINTS - 1);
        py = ScaleWaveY(data[WaveDataIndex(i)], min, max, y, h);
        LCD_DrawLine(last_x, last_y, px, py);
        last_x = px;
        last_y = py;
    }
}

static void DrawWaveDynamic(void)
{
    char value[20];

    LCD_Fill(0, WAVE_TEMP_Y, WAVE_CHART_X + WAVE_CHART_W, WAVE_TEMP_Y + WAVE_CHART_H + 1, WHITE);
    FormatValue(value, (s16)g_temp, 0);
    DrawWaveFrame(WAVE_TEMP_Y, (u8*)"Temp", (u8*)"-10..50", RED, value);
    DrawWaveSeries(wave_temp, WAVE_TEMP_MIN, WAVE_TEMP_MAX, WAVE_CHART_X, WAVE_TEMP_Y, WAVE_CHART_W, WAVE_CHART_H, RED);

    LCD_Fill(0, WAVE_HUM_Y, WAVE_CHART_X + WAVE_CHART_W, WAVE_HUM_Y + WAVE_CHART_H + 1, WHITE);
    FormatValue(value, (s16)g_humidity, 1);
    DrawWaveFrame(WAVE_HUM_Y, (u8*)"Hum", (u8*)"0..100%", BLUE, value);
    DrawWaveSeries(wave_hum, WAVE_HUM_MIN, WAVE_HUM_MAX, WAVE_CHART_X, WAVE_HUM_Y, WAVE_CHART_W, WAVE_CHART_H, BLUE);

    LCD_Fill(0, WAVE_PRESS_Y, WAVE_CHART_X + WAVE_CHART_W, WAVE_PRESS_Y + WAVE_CHART_H + 1, WHITE);
    FormatValue(value, (s16)g_pressure, 2);
    DrawWaveFrame(WAVE_PRESS_Y, (u8*)"Press", (u8*)"900..1100", MAGENTA, value);
    DrawWaveSeries(wave_press, WAVE_PRESS_MIN, WAVE_PRESS_MAX, WAVE_CHART_X, WAVE_PRESS_Y, WAVE_CHART_W, WAVE_CHART_H, MAGENTA);
}

void DrawPage_Home(void)
{
    LCD_Clear(WHITE);
    DrawTitleBar((u8*)"Environment Monitoring System");
    
    POINT_COLOR = BLACK;
    BACK_COLOR = WHITE;
    LCD_ShowString(20, 40, BLACK, WHITE, 16, (u8*)"Temperature:", 1);
    LCD_ShowString(20, 90, BLACK, WHITE, 16, (u8*)"Humidity:", 1);
    LCD_ShowString(20, 140, BLACK, WHITE, 16, (u8*)"Pressure:", 1);
    
    POINT_COLOR = RED;
    LCD_ShowString(260, 90, RED, WHITE, 16, (u8*)"GROUP 11", 1);
    Gui_Drawbmp16_Custom(330, 190, ICON_PEIXIAO_W, ICON_PEIXIAO_H, gImage_peixiao);
    
    POINT_COLOR = GRAY;
    LCD_ShowString(60, 200, GRAY, WHITE, 16, (u8*)"Key Left/Right: switch", 1);
    LCD_ShowString(60, 220, GRAY, WHITE, 16, (u8*)"Key Up/Down: adjust", 1);
    LCD_ShowString(60, 240, GRAY, WHITE, 16, (u8*)"Key OK: confirm", 1);
    
    DrawHomeDynamic();
    DrawNavBar();
}

void DrawPage_Temp(void)
{
    LCD_Clear(WHITE);
    DrawTitleBar((u8*)"Temperature");

    POINT_COLOR = GRAY;
    LCD_ShowString(60, 230, GRAY, WHITE, 16, (u8*)"Up/Down: adjust", 1);
    LCD_ShowString(60, 250, GRAY, WHITE, 16, (u8*)"OK: switch/confirm", 1);
    
    DrawTempDynamic();
    DrawNavBar();
}

void DrawPage_Humidity(void)
{
    LCD_Clear(WHITE);
    DrawTitleBar((u8*)"Humidity");

    POINT_COLOR = GRAY;
    LCD_ShowString(60, 230, GRAY, WHITE, 16, (u8*)"Up/Down: adjust", 1);
    LCD_ShowString(60, 250, GRAY, WHITE, 16, (u8*)"OK: switch/confirm", 1);
    
    DrawHumidityDynamic();
    DrawNavBar();
}

void DrawPage_Pressure(void)
{
    LCD_Clear(WHITE);
    DrawTitleBar((u8*)"Pressure");

    POINT_COLOR = GRAY;
    LCD_ShowString(60, 230, GRAY, WHITE, 16, (u8*)"Up/Down: adjust", 1);
    LCD_ShowString(60, 250, GRAY, WHITE, 16, (u8*)"OK: switch/confirm", 1);
    
    DrawPressureDynamic();
    DrawNavBar();
}

void DrawPage_Wave(void)
{
    LCD_Clear(WHITE);
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
    
    // LED报警控制
    if(g_temp_alert || g_hum_alert || g_press_alert)
    {
        LED_Control(0, 1);  // 红灯亮
    }
    else
    {
        LED_Control(1, 0);  // 绿灯亮
    }
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
        
        // 3. 定时读取传感器（每2秒），只在首页自动刷新
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
        
        delay_ms(10);
    }
}
