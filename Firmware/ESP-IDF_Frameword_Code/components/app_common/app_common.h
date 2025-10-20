#pragma once
typedef enum {
        MOTOR_STOP,
        MOTOR_FORWARD,
        MOTOR_BACKWARD
} motor_state_t;
typedef struct {
    motor_state_t state;
} motor_cmd_t;