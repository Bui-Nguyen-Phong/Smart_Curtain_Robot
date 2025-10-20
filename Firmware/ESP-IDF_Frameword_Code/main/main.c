#include "app_common.h"
#include "app_ir.h"
#include "app_motor.h"

static const char *TAG = "MAIN";

int app_main(){
    ESP_LOGI (TAG, "== START ==");
    step_motor_gpio_init();
    // Tao queue de gui command cho motor
    QueueHandle_t motor_cmd_queue = xQueueCreate(10, sizeof(motor_cmd_t));
    if (motor_cmd_queue == NULL) {
        ESP_LOGE(TAG, "Failed to create motor command queue");
        return -1;
    }
    // Init IR whith queue
    app_ir_init(motor_cmd_queue);
    // Init step motor task
   if( xTaskCreatePinnedToCore(step_motor_task, "step_motor_task", 2048, NULL, 10, NULL,1) != pdPASS) {
        ESP_LOGE(TAG, "Failed to create step motor task");
        return -1;
    }

}