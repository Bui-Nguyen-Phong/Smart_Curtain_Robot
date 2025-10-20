#pragma once 
#include "driver/gpio.h"
#include "freertos/task.h"
#include "app_common.h"
#include "freertos/FreeRTOS.h"  
#include "freertos/queue.h"

#define IN1_GPIO GPIO_NUM_22
#define IN2_GPIO GPIO_NUM_21
#define IN3_GPIO GPIO_NUM_17
#define IN4_GPIO GPIO_NUM_16

typedef struct {
        int step_index;
        int interval_us;
} step_motor_t;
    
step_motor_t motor;

void step_motor_gpio_init(void);
void stepper_set_step(int);
void step_motor_stop (void);
void step_motor_foward (void);
void step_motor_backward (void);
void step_motor_task(void *arg);