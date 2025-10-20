#include "app_motor.h"
#include "app_common.h"
void step_motor_gpio_init(){
        gpio_config_t io_config = {
            .pin_bit_mask = (1ULL << IN1_GPIO) | (1ULL << IN2_GPIO) | (1ULL << IN3_GPIO)| (1ULL << IN4_GPIO),          //!< GPIO pin: set with bit mask, each bit maps to a GPIO 
            .mode = GPIO_MODE_OUTPUT,          //!< GPIO mode: set input/output mode                     
            .pull_up_en = 0,       //!< GPIO pull-up                                         
            .pull_down_en = 0,   //!< GPIO pull-down                                       
            .intr_type = GPIO_INTR_DISABLE
        };
        gpio_config(&io_config);
    }    
    static int FULL_STEP[4][4] = {
        {1, 0, 1, 0},
        {0, 1, 1, 0},
        {0, 1, 0, 1},
        {1, 0, 0, 1},
    };

    void stepper_set_step(int step)
    {
        gpio_set_level(IN1_GPIO, FULL_STEP[step][0]);
        gpio_set_level(IN2_GPIO, FULL_STEP[step][1]);
        gpio_set_level(IN3_GPIO, FULL_STEP[step][2]);
        gpio_set_level(IN4_GPIO, FULL_STEP[step][3]);
    }
    void step_motor_stop (){
        gpio_set_level (IN1_GPIO, 0);
        gpio_set_level (IN2_GPIO, 0);
        gpio_set_level (IN3_GPIO, 0);
        gpio_set_level (IN4_GPIO, 0);
    }
    void step_motor_foward (){
        stepper_set_step(motor.step_index);
        motor.step_index = (motor.step_index +1) % 4;
    }
    void step_motor_backward (){
        motor.step_index= (motor.step_index-1 + 4) % 4;
        stepper_set_step(motor.step_index);
    }
    void step_motor_task(void *arg){
        esp_task_wdt_add (NULL);
        while(1){  
            switch(current_state){
                case MOTOR_FOWARD:
                    step_motor_foward();
                    break;
                case MOTOR_BACKWARD:
                    step_motor_backward();
                    break;
                case MOTOR_STOP:
                    step_motor_stop();
                    break; 
            }  
            esp_task_wdt_reset ();
            vTaskDelay(pdMS_TO_TICKS(2));
            
        }
    }