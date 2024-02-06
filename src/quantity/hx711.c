#include <inttypes.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_timer.h>
//#include "ets_sys.h"
#include "hx711.h"

#define CONFIG_EXAMPLE_DOUT_GPIO  47  // GPIO14 (IO14) pour DOUT
#define CONFIG_EXAMPLE_PD_SCK_GPIO 45  // GPIO15 (IO15) pour PD_SCK
#define CONFIG_EXAMPLE_AVG_TIMES 10  // Remplacez par le nombre de lectures moyennes souhaité

#define CHECK(x) do { esp_err_t __; if ((__ = x) != ESP_OK) { \
    ESP_LOGE(TAG, "Check failed at line %d", __LINE__); return __; } } while (0)

#define CHECK_ARG(VAL) do { if (!(VAL)) { \
    ESP_LOGE(TAG, "Invalid argument at line %d", __LINE__); return ESP_ERR_INVALID_ARG; } } while (0)

#if HELPER_TARGET_IS_ESP32
static portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
#endif

static const char *TAG = "hx711-example";

static uint32_t read_raw(gpio_num_t dout, gpio_num_t pd_sck, hx711_gain_t gain)
{
#if HELPER_TARGET_IS_ESP32
    portENTER_CRITICAL(&mux);
#elif HELPER_TARGET_IS_ESP8266
    portENTER_CRITICAL();
#endif

    // read data
    uint32_t data = 0;
    for (size_t i = 0; i < 24; i++)
    {
        gpio_set_level(pd_sck, 1);
        esp_rom_delay_us(1);
        data |= gpio_get_level(dout) << (23 - i);
        gpio_set_level(pd_sck, 0);
        esp_rom_delay_us(1);
    }

    // config gain + channel for next read
    for (size_t i = 0; i <= gain; i++)
    {
        gpio_set_level(pd_sck, 1);
        esp_rom_delay_us(1);
        gpio_set_level(pd_sck, 0);
        esp_rom_delay_us(1);
    }

#if HELPER_TARGET_IS_ESP32
    portEXIT_CRITICAL(&mux);
#elif HELPER_TARGET_IS_ESP8266
    portEXIT_CRITICAL();
#endif

    return data;
}

///////////////////////////////////////////////////////////////////////////////

esp_err_t hx711_init(hx711_t *dev)
{
    CHECK_ARG(dev);

    CHECK(gpio_set_direction(dev->dout, GPIO_MODE_INPUT));
    CHECK(gpio_set_direction(dev->pd_sck, GPIO_MODE_OUTPUT));

    CHECK(hx711_power_down(dev, false));

    ESP_LOGI(TAG, "HX711 initialized");

    return hx711_set_gain(dev, dev->gain);
}

esp_err_t hx711_power_down(hx711_t *dev, bool down)
{
    CHECK_ARG(dev);

    CHECK(gpio_set_level(dev->pd_sck, down));
    vTaskDelay(1);

    ESP_LOGI(TAG, "HX711 power %s", down ? "down" : "up");

    return ESP_OK;
}

esp_err_t hx711_set_gain(hx711_t *dev, hx711_gain_t gain)
{
    CHECK_ARG(dev && gain <= HX711_GAIN_A_64);

    CHECK(hx711_wait(dev, 200)); // 200 ms timeout

    read_raw(dev->dout, dev->pd_sck, gain);
    dev->gain = gain;

    ESP_LOGI(TAG, "HX711 gain set to %d", gain);

    return ESP_OK;
}

esp_err_t hx711_is_ready(hx711_t *dev, bool *ready)
{
    CHECK_ARG(dev && ready);

    *ready = !gpio_get_level(dev->dout);

    return ESP_OK;
}

esp_err_t hx711_wait(hx711_t *dev, size_t timeout_ms)
{
    uint64_t started = esp_timer_get_time() / 1000;
    while (esp_timer_get_time() / 1000 - started < timeout_ms)
    {
        if (!gpio_get_level(dev->dout))
            return ESP_OK;
        vTaskDelay(1);
    }

    ESP_LOGE(TAG, "HX711 wait timeout");
    return ESP_ERR_TIMEOUT;
}

esp_err_t hx711_read_data(hx711_t *dev, int32_t *data)
{
    CHECK_ARG(dev && data);

    uint32_t raw = read_raw(dev->dout, dev->pd_sck, dev->gain);
    if (raw & 0x800000)
        raw |= 0xff000000;
    *data = *((int32_t *)&raw);

    ESP_LOGI(TAG, "HX711 raw data: %" PRIi32, *data);

    return ESP_OK;
}

esp_err_t hx711_read_average(hx711_t *dev, size_t times, int32_t *data)
{
    CHECK_ARG(dev && times && data);

    int32_t v;
    *data = 0;
    for (size_t i = 0; i < times; i++)
    {
        CHECK(hx711_wait(dev, 200));
        CHECK(hx711_read_data(dev, &v));
        *data += v;
    }
    *data /= (int32_t) times;

    ESP_LOGI(TAG, "HX711 average data: %" PRIi32, *data);

    return ESP_OK;
}

void test(void *pvParameters)
{
    hx711_t dev = {
        .dout = CONFIG_EXAMPLE_DOUT_GPIO,
        .pd_sck = CONFIG_EXAMPLE_PD_SCK_GPIO,
        .gain = HX711_GAIN_A_64
    };

    // initialize device
    ESP_ERROR_CHECK(hx711_init(&dev));

    // read from device
    while (1)
    {
        esp_err_t r = hx711_wait(&dev, 500);
        if (r != ESP_OK)
        {
            ESP_LOGE(TAG, "Device not found: %d (%s)\n", r, esp_err_to_name(r));
            continue;
        }

        int32_t data;
        r = hx711_read_average(&dev, CONFIG_EXAMPLE_AVG_TIMES, &data);
        if (r != ESP_OK)
        {
            ESP_LOGE(TAG, "Could not read data: %d (%s)\n", r, esp_err_to_name(r));
            continue;
        }

        ESP_LOGI(TAG, "Raw data: %" PRIi32, data);

        vTaskDelay(pdMS_TO_TICKS(500));
    }
}
