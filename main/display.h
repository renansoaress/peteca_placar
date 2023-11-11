#ifndef _DISPLAY_H__
#define _DISPLAY_H__

#include <stdio.h>
#include "esp_log.h"

#define NO_COLOR make_rgb(0, 0, 0)
#define COLOR_RED make_rgb(255, 0, 0)
#define COLOR_GREEN make_rgb(0, 255, 0)
#define COLOR_BLUE make_rgb(0, 0, 255)
#define COLOR_ORANGE make_rgb(255, 165, 0)
#define COLOR_WHITE make_rgb(255, 255, 255)
#define COLOR_PURPLE make_rgb(185, 0, 255)

typedef struct
{
    uint8_t red;
    uint8_t green;
    uint8_t blue;
} rgb;

typedef struct
{
    uint8_t led_1;
    uint8_t led_2;
} block;

typedef struct
{
    block top;
    block top_left;
    block top_right;
    block mid;
    block bot;
    block bot_left;
    block bot_right;
} number;

#define DISABLE                  \
    {                            \
        .led_1 = -1, .led_2 = -1 \
    }
#define TOP                    \
    {                          \
        .led_1 = 0, .led_2 = 1 \
    }
#define TOP_LEFT                 \
    {                            \
        .led_1 = 10, .led_2 = 11 \
    }
#define TOP_RIGHT              \
    {                          \
        .led_1 = 2, .led_2 = 3 \
    }
#define MID                      \
    {                            \
        .led_1 = 12, .led_2 = 13 \
    }
#define BOT                    \
    {                          \
        .led_1 = 6, .led_2 = 7 \
    }
#define BOT_LEFT               \
    {                          \
        .led_1 = 8, .led_2 = 9 \
    }
#define BOT_RIGHT              \
    {                          \
        .led_1 = 4, .led_2 = 5 \
    }

static number numbers[10] = {
    // N = 0
    {.top = TOP,
     .top_left = TOP_LEFT,
     .top_right = TOP_RIGHT,
     .mid = DISABLE,
     .bot = BOT,
     .bot_left = BOT_LEFT,
     .bot_right = BOT_RIGHT}

    , // N = 1
    {.top = DISABLE,
     .top_left = DISABLE,
     .top_right = TOP_RIGHT,
     .mid = DISABLE,
     .bot = DISABLE,
     .bot_left = DISABLE,
     .bot_right = BOT_RIGHT}

    , // N = 2
    {.top = TOP,
     .top_left = DISABLE,
     .top_right = TOP_RIGHT,
     .mid = MID,
     .bot = BOT,
     .bot_left = BOT_LEFT,
     .bot_right = DISABLE}

    , // N = 3
    {.top = TOP,
     .top_left = DISABLE,
     .top_right = TOP_RIGHT,
     .mid = MID,
     .bot = BOT,
     .bot_left = DISABLE,
     .bot_right = BOT_RIGHT}

    , // N = 4
    {.top = DISABLE,
     .top_left = TOP_LEFT,
     .top_right = TOP_RIGHT,
     .mid = MID,
     .bot = DISABLE,
     .bot_left = DISABLE,
     .bot_right = BOT_RIGHT}

    , // N = 5
    {.top = TOP,
     .top_left = TOP_LEFT,
     .top_right = DISABLE,
     .mid = MID,
     .bot = BOT,
     .bot_left = DISABLE,
     .bot_right = BOT_RIGHT}

    , // N = 6
    {.top = TOP,
     .top_left = TOP_LEFT,
     .top_right = DISABLE,
     .mid = MID,
     .bot = BOT,
     .bot_left = BOT_LEFT,
     .bot_right = BOT_RIGHT}

    , // N = 7
    {.top = TOP,
     .top_left = DISABLE,
     .top_right = TOP_RIGHT,
     .mid = DISABLE,
     .bot = DISABLE,
     .bot_left = DISABLE,
     .bot_right = BOT_RIGHT}

    , // N = 8
    {.top = TOP,
     .top_left = TOP_LEFT,
     .top_right = TOP_RIGHT,
     .mid = MID,
     .bot = BOT,
     .bot_left = BOT_LEFT,
     .bot_right = BOT_RIGHT}

    , // N = 9
    {.top = TOP,
     .top_left = TOP_LEFT,
     .top_right = TOP_RIGHT,
     .mid = MID,
     .bot = BOT,
     .bot_left = DISABLE,
     .bot_right = BOT_RIGHT}};

number get_number(uint8_t num)
{
    return numbers[num];
}

static inline rgb make_rgb(uint8_t red, uint8_t green, uint8_t blue)
{
    return (rgb){.red = red, .green = green, .blue = blue};
}

#endif