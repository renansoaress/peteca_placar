#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/rmt_tx.h"
#include "led_strip_encoder.h"

#define RMT_LED_STRIP_RESOLUTION_HZ 10000000 // 10MHz resolution, 1 tick = 0.1us (led strip needs a high resolution)
#define RMT_LED_STRIP_GPIO_NUM 13

#define LED_NUMBERS 10
#define CHASE_SPEED_MS 100

static const char *TAG = "PETECA";
static rmt_channel_handle_t led_team_1 = NULL;
static rmt_encoder_handle_t led_encoder = NULL;

void led(rmt_channel_handle_t *team, int position, uint8_t red, uint8_t green, uint8_t blue)
{
    uint8_t led_strip_pixels[LED_NUMBERS * 3] = {0};
    led_strip_pixels[position * 3 + 0] = green;
    led_strip_pixels[position * 3 + 1] = red;
    led_strip_pixels[position * 3 + 2] = blue;
    rmt_transmit_config_t tx_config = {
        .loop_count = 0, // no transfer loop
    };
    ESP_ERROR_CHECK(rmt_transmit(*team, led_encoder, led_strip_pixels, sizeof(led_strip_pixels), &tx_config));
    ESP_ERROR_CHECK(rmt_tx_wait_all_done(*team, portMAX_DELAY));
}

void app_main(void)
{

    ESP_LOGI(TAG, "Start!!!");
    rmt_tx_channel_config_t tx_chan_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT, // select source clock
        .gpio_num = RMT_LED_STRIP_GPIO_NUM,
        .mem_block_symbols = 64, // increase the block size can make the LED less flickering
        .resolution_hz = RMT_LED_STRIP_RESOLUTION_HZ,
        .trans_queue_depth = 4, // set the number of transactions that can be pending in the background
    };
    ESP_ERROR_CHECK(rmt_new_tx_channel(&tx_chan_config, &led_team_1));

    ESP_LOGI(TAG, "Install led strip encoder");
    led_strip_encoder_config_t encoder_config = {
        .resolution = RMT_LED_STRIP_RESOLUTION_HZ,
    };
    ESP_ERROR_CHECK(rmt_new_led_strip_encoder(&encoder_config, &led_encoder));

    ESP_LOGI(TAG, "Enable RMT TX channel");
    ESP_ERROR_CHECK(rmt_enable(led_team_1));

    // led(&led_team_1, 0, 255, 0, 255);
    // vTaskDelay(pdMS_TO_TICKS(CHASE_SPEED_MS));
    // led(&led_team_1, 1, 0, 255, 0);
    // vTaskDelay(pdMS_TO_TICKS(CHASE_SPEED_MS));
    // led(&led_team_1, 2, 0, 0, 255);
    // vTaskDelay(pdMS_TO_TICKS(CHASE_SPEED_MS));

    while (1)
    {
        for (uint8_t i = 0; i < LED_NUMBERS; i++)
        {
            uint8_t next1 = i >= LED_NUMBERS ? 0 : i + 1;
            uint8_t next2 = next1 + 1;
            led(&led_team_1, i, 255, 0, 0);
            vTaskDelay(pdMS_TO_TICKS(CHASE_SPEED_MS));
            led(&led_team_1, next1, 0, 255, 0);
            vTaskDelay(pdMS_TO_TICKS(CHASE_SPEED_MS));
            led(&led_team_1, next2, 0, 0, 255);
            vTaskDelay(pdMS_TO_TICKS(CHASE_SPEED_MS));
            led(&led_team_1, i, 0, 0, 0);
            vTaskDelay(pdMS_TO_TICKS(CHASE_SPEED_MS));
            led(&led_team_1, next1, 0, 0, 0);
            vTaskDelay(pdMS_TO_TICKS(CHASE_SPEED_MS));
            led(&led_team_1, next2, 0, 0, 0);
            vTaskDelay(pdMS_TO_TICKS(CHASE_SPEED_MS));
        }
    }
}
