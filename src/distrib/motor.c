#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/mcpwm_prelude.h"

static const char *TAG = "servo";
#define MOTOR_PULSE_GPIO  4
#define MOTOR_MIN_PULSEWIDTH_US 500
#define MOTOR_MAX_PULSEWIDTH_US 2500
#define MOTOR_MIN_DEGREE  -90
#define MOTOR_MAX_DEGREE  90
#define MOTOR_TIMEBASE_RESOLUTION_HZ 1000000
#define MOTOR_TIMEBASE_PERIOD 20000

static mcpwm_cmpr_handle_t comparator = NULL;

static inline uint32_t motor_angle_to_compare(int angle)
{
    return (angle - MOTOR_MIN_DEGREE) * (MOTOR_MAX_PULSEWIDTH_US - MOTOR_MIN_PULSEWIDTH_US) / (MOTOR_MAX_DEGREE - MOTOR_MIN_DEGREE) + MOTOR_MIN_PULSEWIDTH_US;
}

void motor_init(void)
{
    ESP_LOGI(TAG, "Initializing motor");

    mcpwm_timer_handle_t timer = NULL;
    mcpwm_timer_config_t timer_config = {
        .group_id = 0,
        .clk_src = MCPWM_TIMER_CLK_SRC_DEFAULT,
        .resolution_hz = MOTOR_TIMEBASE_RESOLUTION_HZ,
        .period_ticks = MOTOR_TIMEBASE_PERIOD,
        .count_mode = MCPWM_TIMER_COUNT_MODE_UP,
    };
    ESP_ERROR_CHECK(mcpwm_new_timer(&timer_config, &timer));

    mcpwm_oper_handle_t oper = NULL;
    mcpwm_operator_config_t operator_config = {
        .group_id = 0,
    };
    ESP_ERROR_CHECK(mcpwm_new_operator(&operator_config, &oper));

    ESP_ERROR_CHECK(mcpwm_operator_connect_timer(oper, timer));

    mcpwm_comparator_config_t comparator_config = {
        .flags.update_cmp_on_tez = true,
    };
    ESP_ERROR_CHECK(mcpwm_new_comparator(oper, &comparator_config, &comparator));

    mcpwm_gen_handle_t generator = NULL;
    mcpwm_generator_config_t generator_config = {
        .gen_gpio_num = MOTOR_PULSE_GPIO,
    };
    ESP_ERROR_CHECK(mcpwm_new_generator(oper, &generator_config, &generator));

    ESP_ERROR_CHECK(mcpwm_comparator_set_compare_value(comparator, motor_angle_to_compare(0)));

    ESP_ERROR_CHECK(mcpwm_generator_set_action_on_timer_event(generator,
                                                              MCPWM_GEN_TIMER_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, MCPWM_TIMER_EVENT_EMPTY, MCPWM_GEN_ACTION_HIGH)));

    ESP_ERROR_CHECK(mcpwm_generator_set_action_on_compare_event(generator,
                                                                MCPWM_GEN_COMPARE_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, comparator, MCPWM_GEN_ACTION_LOW)));

    ESP_ERROR_CHECK(mcpwm_timer_enable(timer));
    ESP_ERROR_CHECK(mcpwm_timer_start_stop(timer, MCPWM_TIMER_START_NO_STOP));
}

void motor_set_angle(int angle)
{
    ESP_ERROR_CHECK(mcpwm_comparator_set_compare_value(comparator, motor_angle_to_compare(angle)));
    vTaskDelay(pdMS_TO_TICKS(500)); // Delay to allow the servo to reach the desired position
}
