#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "driver/rmt_tx.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "led_strip_encoder.h"
#include "display.h"

#define LED_TEAM_1_GPIO_NUM 13
#define LED_TEAM_2_GPIO_NUM 12
#define BUZZER_GPIO_NUM 26
#define BTN_1_TEAM_GPIO_NUM 14
#define BTN_2_TEAM_GPIO_NUM 27

#define RMT_LED_STRIP_RESOLUTION_HZ 10000000 // 10MHz resolution, 1 tick = 0.1us
#define GPIO_OUTPUT_PIN_SEL ((1ULL << BUZZER_GPIO_NUM))
#define GPIO_INPUT_PIN_SEL ((1ULL << BTN_1_TEAM_GPIO_NUM) | (1ULL << BTN_2_TEAM_GPIO_NUM))

#define LED_NUMBERS 15
#define CHASE_SPEED_MS 100
#define DEBOUNCE_TIME_MS 100

#define LED_SET_GAME 14

static const char *TAG = "PETECA";
static SemaphoreHandle_t semaphore_btn_action = NULL;
static rmt_channel_handle_t led_team_1 = NULL;
static rmt_channel_handle_t led_team_2 = NULL;
static rmt_encoder_handle_t led_encoder = NULL;
static uint8_t led_strip_pixels[2][LED_NUMBERS * 3] = {0};
static uint8_t scoreboard_team_1 = 0, scoreboard_team_2 = 0;
static bool set_final_blue_team = false, set_final_red_team = false;
static bool set_blue_team = false, set_red_team = false;

static void led(rmt_channel_handle_t *team, int position, rgb color)
{
    int _team = *team == led_team_1 ? 0 : 1;
    led_strip_pixels[_team][position * 3 + 0] = color.green;
    led_strip_pixels[_team][position * 3 + 1] = color.red;
    led_strip_pixels[_team][position * 3 + 2] = color.blue;
    rmt_transmit_config_t tx_config = {
        .loop_count = 0, // no transfer loop
    };
    ESP_ERROR_CHECK(rmt_transmit(*team, led_encoder, led_strip_pixels[_team], sizeof(led_strip_pixels[_team]), &tx_config));
    ESP_ERROR_CHECK(rmt_tx_wait_all_done(*team, portMAX_DELAY));
}

static rgb check_color(uint8_t flag, rgb color)
{
    if (flag < 0)
    {
        return NO_COLOR;
    }
    return color;
}

static void display_reset(rmt_channel_handle_t *team)
{
    number _num = get_number(8);
    led(team, _num.top.led_1, NO_COLOR);
    led(team, _num.top.led_2, NO_COLOR);
    led(team, _num.top_left.led_1, NO_COLOR);
    led(team, _num.top_left.led_2, NO_COLOR);
    led(team, _num.top_right.led_1, NO_COLOR);
    led(team, _num.top_right.led_2, NO_COLOR);

    led(team, _num.mid.led_1, NO_COLOR);
    led(team, _num.mid.led_2, NO_COLOR);

    led(team, _num.bot.led_1, NO_COLOR);
    led(team, _num.bot.led_2, NO_COLOR);
    led(team, _num.bot_left.led_1, NO_COLOR);
    led(team, _num.bot_left.led_2, NO_COLOR);
    led(team, _num.bot_right.led_1, NO_COLOR);
    led(team, _num.bot_right.led_2, NO_COLOR);
}

static void display_number(rmt_channel_handle_t *team, uint8_t num, rgb color)
{
    display_reset(team);
    number _num = get_number(num);
    led(team, _num.top.led_1, check_color(_num.top.led_1, color));
    led(team, _num.top.led_2, check_color(_num.top.led_2, color));
    led(team, _num.top_left.led_1, check_color(_num.top_left.led_1, color));
    led(team, _num.top_left.led_2, check_color(_num.top_left.led_2, color));
    led(team, _num.top_right.led_1, check_color(_num.top_right.led_1, color));
    led(team, _num.top_right.led_2, check_color(_num.top_right.led_2, color));

    led(team, _num.mid.led_1, check_color(_num.mid.led_1, color));
    led(team, _num.mid.led_2, check_color(_num.mid.led_2, color));

    led(team, _num.bot.led_1, check_color(_num.bot.led_1, color));
    led(team, _num.bot.led_2, check_color(_num.bot.led_2, color));
    led(team, _num.bot_left.led_1, check_color(_num.bot_left.led_1, color));
    led(team, _num.bot_left.led_2, check_color(_num.bot_left.led_2, color));
    led(team, _num.bot_right.led_1, check_color(_num.bot_right.led_1, color));
    led(team, _num.bot_right.led_2, check_color(_num.bot_right.led_2, color));
}

static void start_game()
{
    scoreboard_team_1 = 0;
    scoreboard_team_2 = 0;
    set_blue_team = false;
    set_red_team = false;
    set_final_blue_team = false;
    set_final_red_team = false;

    led(&led_team_1, LED_SET_GAME, NO_COLOR);
    led(&led_team_2, LED_SET_GAME, NO_COLOR);
    // display_reset(&led_team_1);
    // display_reset(&led_team_2);
    display_number(&led_team_1, scoreboard_team_1, COLOR_BLUE);
    display_number(&led_team_2, scoreboard_team_2, COLOR_RED);

    gpio_set_level(BUZZER_GPIO_NUM, 1);
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    gpio_set_level(BUZZER_GPIO_NUM, 0);
    vTaskDelay(100 / portTICK_PERIOD_MS);
    gpio_set_level(BUZZER_GPIO_NUM, 1);
    vTaskDelay(100 / portTICK_PERIOD_MS);
    gpio_set_level(BUZZER_GPIO_NUM, 0);
    vTaskDelay(100 / portTICK_PERIOD_MS);
    gpio_set_level(BUZZER_GPIO_NUM, 1);
    vTaskDelay(100 / portTICK_PERIOD_MS);
    gpio_set_level(BUZZER_GPIO_NUM, 0);
    xSemaphoreGive(semaphore_btn_action);
}

static void end_game()
{
    printf("ACABOUUUUUUUU!!!\n\n");
    display_reset(&led_team_1);
    display_reset(&led_team_2);
    scoreboard_team_1 = 0;
    scoreboard_team_2 = 0;
    set_blue_team = false;
    set_red_team = false;
    set_final_blue_team = false;
    set_final_red_team = false;

    led(&led_team_1, LED_SET_GAME, NO_COLOR);
    led(&led_team_2, LED_SET_GAME, NO_COLOR);

    gpio_set_level(BUZZER_GPIO_NUM, 1);
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    display_number(&led_team_1, 8, COLOR_PURPLE);
    display_number(&led_team_2, 8, COLOR_PURPLE);
    gpio_set_level(BUZZER_GPIO_NUM, 0);
    vTaskDelay(100 / portTICK_PERIOD_MS);
    gpio_set_level(BUZZER_GPIO_NUM, 1);
    vTaskDelay(100 / portTICK_PERIOD_MS);
    gpio_set_level(BUZZER_GPIO_NUM, 0);
    vTaskDelay(100 / portTICK_PERIOD_MS);
    gpio_set_level(BUZZER_GPIO_NUM, 1);
    vTaskDelay(100 / portTICK_PERIOD_MS);
    gpio_set_level(BUZZER_GPIO_NUM, 0);

    vTaskDelay(5000 / portTICK_PERIOD_MS);
    display_number(&led_team_1, scoreboard_team_1, COLOR_BLUE);
    display_number(&led_team_2, scoreboard_team_2, COLOR_RED);
    xSemaphoreGive(semaphore_btn_action);
}

static void invert_game()
{
    gpio_set_level(BUZZER_GPIO_NUM, 1);
    vTaskDelay(500 / portTICK_PERIOD_MS);
    gpio_set_level(BUZZER_GPIO_NUM, 0);
    display_reset(&led_team_1);
    display_reset(&led_team_2);
    vTaskDelay(200 / portTICK_PERIOD_MS);
    display_number(&led_team_1, scoreboard_team_2, COLOR_RED);
    display_number(&led_team_2, scoreboard_team_1, COLOR_BLUE);
    gpio_set_level(BUZZER_GPIO_NUM, 1);
    display_reset(&led_team_1);
    display_reset(&led_team_2);
    vTaskDelay(200 / portTICK_PERIOD_MS);
    display_number(&led_team_1, scoreboard_team_2, COLOR_RED);
    display_number(&led_team_2, scoreboard_team_1, COLOR_BLUE);
    gpio_set_level(BUZZER_GPIO_NUM, 0);
    display_reset(&led_team_1);
    display_reset(&led_team_2);
    vTaskDelay(200 / portTICK_PERIOD_MS);
    display_number(&led_team_1, scoreboard_team_2, COLOR_RED);
    display_number(&led_team_2, scoreboard_team_1, COLOR_BLUE);
    gpio_set_level(BUZZER_GPIO_NUM, 1);
    display_reset(&led_team_1);
    display_reset(&led_team_2);
    vTaskDelay(200 / portTICK_PERIOD_MS);
    display_number(&led_team_1, scoreboard_team_2, COLOR_RED);
    display_number(&led_team_2, scoreboard_team_1, COLOR_BLUE);
    gpio_set_level(BUZZER_GPIO_NUM, 0);
    display_reset(&led_team_1);
    display_reset(&led_team_2);
    vTaskDelay(200 / portTICK_PERIOD_MS);
    display_number(&led_team_1, scoreboard_team_2, COLOR_RED);
    display_number(&led_team_2, scoreboard_team_1, COLOR_BLUE);
    gpio_set_level(BUZZER_GPIO_NUM, 1);
    vTaskDelay(500 / portTICK_PERIOD_MS);
    gpio_set_level(BUZZER_GPIO_NUM, 0);
}

static void debounce_btn_team_task(int BTN_GPIO)
{
    int level = gpio_get_level(BTN_GPIO);
    int last_level = level;

    while (1)
    {

        vTaskDelay(pdMS_TO_TICKS(DEBOUNCE_TIME_MS));

        level = gpio_get_level(BTN_GPIO);

        if (level != last_level)
        {
            // O estado do pino mudou, aguarde para confirmar a mudança
            vTaskDelay(pdMS_TO_TICKS(DEBOUNCE_TIME_MS));
            level = gpio_get_level(BTN_GPIO);

            if (level != last_level)
            {
                if (level)
                {
                    if (xSemaphoreTake(semaphore_btn_action, pdMS_TO_TICKS(5000)) == pdTRUE)
                    {
                        // printf("PONTO (%d) GPIO: %d\n", level, BTN_GPIO);
                        // BTN 1
                        if (BTN_GPIO == BTN_1_TEAM_GPIO_NUM)
                        {
                            if (set_blue_team || set_red_team)
                            {
                                if (set_final_red_team)
                                {
                                    if (set_final_blue_team && scoreboard_team_1 == 1)
                                    {
                                        scoreboard_team_1 = 0;
                                        scoreboard_team_2 = 0;
                                        display_number(&led_team_1, scoreboard_team_2, COLOR_RED);
                                        display_number(&led_team_2, scoreboard_team_1, COLOR_BLUE);
                                    }
                                    else
                                    {
                                        scoreboard_team_2++;
                                        display_number(&led_team_1, scoreboard_team_2, COLOR_RED);
                                        if (scoreboard_team_2 > 1)
                                        {
                                            // GG
                                            last_level = level;
                                            end_game();
                                            continue;
                                        }
                                    }
                                }
                                else
                                {
                                    //// PASSO 3 RED
                                    if (scoreboard_team_2 == 9)
                                    {
                                        scoreboard_team_2 = 0;

                                        if (set_red_team)
                                        {
                                            set_final_red_team = true;
                                            led(&led_team_1, LED_SET_GAME, COLOR_WHITE);
                                            if (set_final_blue_team && scoreboard_team_1 == 1)
                                            {
                                                scoreboard_team_1 = 0;
                                                display_number(&led_team_2, scoreboard_team_1, COLOR_BLUE);
                                            }
                                        }
                                        else
                                        {
                                            set_red_team = true;
                                            led(&led_team_1, LED_SET_GAME, COLOR_GREEN);
                                        }
                                        display_number(&led_team_1, scoreboard_team_2, COLOR_RED);
                                    }
                                    else
                                    {
                                        scoreboard_team_2++;
                                        display_number(&led_team_1, scoreboard_team_2, COLOR_RED);
                                    }
                                }
                            }
                            else
                            {
                                // PONTO NORMAL
                                if (scoreboard_team_1 == 9)
                                {
                                    //// PASSO 2 FIZ 9 PONTOS INVERTIR
                                    scoreboard_team_1 = 0;
                                    set_blue_team = true;
                                    led(&led_team_2, LED_SET_GAME, COLOR_GREEN);
                                    display_number(&led_team_1, scoreboard_team_2, COLOR_RED);
                                    display_number(&led_team_2, scoreboard_team_1, COLOR_BLUE);
                                    invert_game();
                                }
                                else
                                {
                                    // PASSO 1
                                    scoreboard_team_1++;
                                    display_number(&led_team_1, scoreboard_team_1, COLOR_BLUE);
                                }
                            }
                        }

                        // BTN 2
                        else
                        {
                            if (set_blue_team || set_red_team)
                            {
                                if (set_final_blue_team)
                                {
                                    if (set_final_red_team && scoreboard_team_2 == 1)
                                    {
                                        scoreboard_team_1 = 0;
                                        scoreboard_team_2 = 0;
                                        display_number(&led_team_1, scoreboard_team_2, COLOR_RED);
                                        display_number(&led_team_2, scoreboard_team_1, COLOR_BLUE);
                                    }
                                    else
                                    {
                                        scoreboard_team_1++;
                                        display_number(&led_team_2, scoreboard_team_1, COLOR_BLUE);
                                        if (scoreboard_team_1 > 1)
                                        {
                                            // GG
                                            last_level = level;
                                            end_game();
                                            continue;
                                        }
                                    }
                                }
                                else
                                {
                                    //// PASSO 3 AZUL
                                    if (scoreboard_team_1 == 9)
                                    {
                                        scoreboard_team_1 = 0;

                                        if (set_blue_team)
                                        {
                                            set_final_blue_team = true;
                                            led(&led_team_2, LED_SET_GAME, COLOR_WHITE);
                                            if (set_final_red_team && scoreboard_team_2 == 1)
                                            {
                                                scoreboard_team_2 = 0;
                                                display_number(&led_team_1, scoreboard_team_2, COLOR_RED);
                                            }
                                        }
                                        else
                                        {
                                            set_blue_team = true;
                                            led(&led_team_2, LED_SET_GAME, COLOR_GREEN);
                                        }
                                        display_number(&led_team_2, scoreboard_team_1, COLOR_BLUE);
                                    }
                                    else
                                    {
                                        scoreboard_team_1++;
                                        display_number(&led_team_2, scoreboard_team_1, COLOR_BLUE);
                                    }
                                }
                            }
                            else
                            {
                                // PONTO NORMAL
                                if (scoreboard_team_2 == 9)
                                {
                                    //// PASSO 2 FIZ 9 PONTOS INVERTIR
                                    scoreboard_team_2 = 0;
                                    set_red_team = true;
                                    led(&led_team_1, LED_SET_GAME, COLOR_GREEN);
                                    display_number(&led_team_1, scoreboard_team_2, COLOR_RED);
                                    display_number(&led_team_2, scoreboard_team_1, COLOR_BLUE);
                                    invert_game();
                                }
                                else
                                {
                                    // PASSO 1
                                    scoreboard_team_2++;
                                    display_number(&led_team_2, scoreboard_team_2, COLOR_RED);
                                }
                            }
                        }

                        printf("SCORE 1(%d) - SCORE 2(%d) | set_blue_team(%s) - set_red_team(%s) | set_final_blue_team(%s) - set_final_red_team(%s)\n\n",
                               scoreboard_team_1, scoreboard_team_2,
                               (set_blue_team ? "true" : "false"),
                               (set_red_team ? "true" : "false"),
                               (set_final_blue_team ? "true" : "false"),
                               (set_final_red_team ? "true" : "false"));

                        gpio_set_level(BUZZER_GPIO_NUM, 1);
                        vTaskDelay(150 / portTICK_PERIOD_MS);
                        gpio_set_level(BUZZER_GPIO_NUM, 0);
                        vTaskDelay(30 / portTICK_PERIOD_MS);
                        gpio_set_level(BUZZER_GPIO_NUM, 1);
                        vTaskDelay(600 / portTICK_PERIOD_MS);
                        gpio_set_level(BUZZER_GPIO_NUM, 0);

                        vTaskDelay(2000 / portTICK_PERIOD_MS);

                        xSemaphoreGive(semaphore_btn_action);
                    }
                }
            }

            last_level = level;
        }
    }
}

void app_main(void)
{

    ESP_LOGI(TAG, "Start!!!");
    semaphore_btn_action = xSemaphoreCreateBinary();
    xSemaphoreGive(semaphore_btn_action);

    gpio_config_t buzzer_config;
    buzzer_config.intr_type = GPIO_INTR_DISABLE;
    buzzer_config.mode = GPIO_MODE_OUTPUT;
    buzzer_config.pin_bit_mask = GPIO_OUTPUT_PIN_SEL;
    buzzer_config.pull_down_en = GPIO_PULLDOWN_DISABLE;
    buzzer_config.pull_up_en = GPIO_PULLUP_DISABLE;
    gpio_config(&buzzer_config);

    gpio_config_t btn_teams_config;
    btn_teams_config.intr_type = GPIO_INTR_DISABLE;
    btn_teams_config.mode = GPIO_MODE_INPUT;
    btn_teams_config.pin_bit_mask = GPIO_INPUT_PIN_SEL;
    btn_teams_config.pull_down_en = GPIO_PULLDOWN_DISABLE;
    btn_teams_config.pull_up_en = GPIO_PULLUP_ENABLE;
    gpio_config(&btn_teams_config);

    rmt_tx_channel_config_t tx_chan_blue_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT, // select source clock
        .gpio_num = LED_TEAM_1_GPIO_NUM,
        .mem_block_symbols = 64, // increase the block size can make the LED less flickering
        .resolution_hz = RMT_LED_STRIP_RESOLUTION_HZ,
        .trans_queue_depth = 4, // set the number of transactions that can be pending in the background
    };
    ESP_ERROR_CHECK(rmt_new_tx_channel(&tx_chan_blue_config, &led_team_1));

    rmt_tx_channel_config_t tx_chan_red_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT, // select source clock
        .gpio_num = LED_TEAM_2_GPIO_NUM,
        .mem_block_symbols = 64, // increase the block size can make the LED less flickering
        .resolution_hz = RMT_LED_STRIP_RESOLUTION_HZ,
        .trans_queue_depth = 4, // set the number of transactions that can be pending in the background
    };
    ESP_ERROR_CHECK(rmt_new_tx_channel(&tx_chan_red_config, &led_team_2));

    ESP_LOGI(TAG, "Install led strip encoder");
    led_strip_encoder_config_t encoder_config = {
        .resolution = RMT_LED_STRIP_RESOLUTION_HZ,
    };
    ESP_ERROR_CHECK(rmt_new_led_strip_encoder(&encoder_config, &led_encoder));

    ESP_LOGI(TAG, "Enable RMT TX channel");
    ESP_ERROR_CHECK(rmt_enable(led_team_1));
    ESP_ERROR_CHECK(rmt_enable(led_team_2));

    start_game();

    xTaskCreate(debounce_btn_team_task, "debounce_t_1", 2048, BTN_1_TEAM_GPIO_NUM, 10, NULL);
    xTaskCreate(debounce_btn_team_task, "debounce_t_2", 2048, BTN_2_TEAM_GPIO_NUM, 10, NULL);

    // while (1)
    // {

    //     for (uint8_t i = 0, j = 9; i < 10 && j >= 0; i++, j--)
    //     {
    //         // printf("i=%d - j=%d\n", i, j);
    //         display_number(&led_team_1, i, COLOR_BLUE);
    //         display_number(&led_team_2, j, COLOR_RED);
    //         vTaskDelay(pdMS_TO_TICKS(500));
    //         display_reset(&led_team_1);
    //         display_reset(&led_team_2);
    //         vTaskDelay(pdMS_TO_TICKS(100));
    //     }
    // }
}
