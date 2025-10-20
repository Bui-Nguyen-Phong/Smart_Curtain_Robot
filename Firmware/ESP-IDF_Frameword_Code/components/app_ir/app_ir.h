#pragma once 
#include "driver/rmt_rx.h"   
#include "ir_nec_encoder.h" 
#include "freertos/queue.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "app_common.h"
#define ir_data GPIO_NUM_26 
#define GPIO_WAKE  GPIO_NUM_27// RTC_17
#define EXAMPLE_IR_NEC_DECODE_MARGIN 200  // sai số do nhiễu thực tế khoảng ~200us


//NEC leading code  : 9ms mark & 4.5ms space
#define NEC_LEADING_CODE_DURATION_0  9000 
#define NEC_LEADING_CODE_DURATION_1  4500
// Mã 
#define NEC_PAYLOAD_ZERO_DURATION_0  560
#define NEC_PAYLOAD_ZERO_DURATION_1  560
#define NEC_PAYLOAD_ONE_DURATION_0   560
#define NEC_PAYLOAD_ONE_DURATION_1   1690

#define NEC_REPEAT_CODE_DURATION_0   9000
#define NEC_REPEAT_CODE_DURATION_1   2250


inline bool nec_check_in_range(uint32_t, uint32_t);
bool nec_parse_logic0(rmt_symbol_word_t *);
bool nec_parse_logic1(rmt_symbol_word_t *);
bool nec_parse_frame(rmt_symbol_word_t *);
bool nec_parse_frame_repeat(rmt_symbol_word_t *);  
void example_parse_nec_frame(rmt_symbol_word_t *, size_t);
bool rmt_rx_done_callback(rmt_channel_handle_t, const rmt_rx_done_event_data_t *, void *);
