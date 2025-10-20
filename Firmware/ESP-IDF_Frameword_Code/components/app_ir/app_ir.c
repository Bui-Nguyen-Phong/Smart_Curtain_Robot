#include "app_ir.h"



uint32_t stop = 0xFF0043;
uint32_t foward = 0xFF0040;
uint32_t backward = 0xFF0044;

static uint16_t s_nec_code_address;
static uint16_t s_nec_code_command;
static const char *TAG = "IR";

// === HANLDLE & QUEUE ===
static rmt_channel_handle_t rx_chan = NULL;
static QueueHandle_t ir_raw_queue = NULL;
static QueueHandle_t motor_cmd_queue = NULL;

// === CALLBACK ===
// ISR callback: khi nhận được dữ liệu RMT sẽ gửi vào hàng đợi để xử lý

bool rmt_rx_done_callback(rmt_channel_handle_t channel, const rmt_rx_done_event_data_t *edata, void *user_data)
    {
        BaseType_t high_task_wakeup = pdFALSE;
        QueueHandle_t receive_queue = (QueueHandle_t)user_data;
        // send the received RMT symbols to the parser task
        xQueueSendFromISR(receive_queue, edata, &high_task_wakeup);
        return high_task_wakeup == pdTRUE;
    }
// === NEC PARSE ===
static inline bool nec_check_in_range(uint32_t signal_duration, uint32_t spec_duration)
    {
        return (signal_duration < (spec_duration + EXAMPLE_IR_NEC_DECODE_MARGIN)) &&
            (signal_duration > (spec_duration - EXAMPLE_IR_NEC_DECODE_MARGIN));  
    }
    // GIẢI MÃ TÍN HIỆU

static bool nec_parse_logic0(rmt_symbol_word_t *rmt_nec_symbols)
    {
        return nec_check_in_range(rmt_nec_symbols->duration0, NEC_PAYLOAD_ZERO_DURATION_0) &&
            nec_check_in_range(rmt_nec_symbols->duration1, NEC_PAYLOAD_ZERO_DURATION_1);
    }

static bool nec_parse_logic1(rmt_symbol_word_t *rmt_nec_symbols)
    {
        return nec_check_in_range(rmt_nec_symbols->duration0, NEC_PAYLOAD_ONE_DURATION_0) &&
            nec_check_in_range(rmt_nec_symbols->duration1, NEC_PAYLOAD_ONE_DURATION_1);
    }

static bool nec_parse_frame(rmt_symbol_word_t *rmt_nec_symbols)
    {
        rmt_symbol_word_t *cur = rmt_nec_symbols;
        uint16_t address = 0;
        uint16_t command = 0;
        // check thỏa mãn leading code 
        bool valid_leading_code = nec_check_in_range(cur->duration0, NEC_LEADING_CODE_DURATION_0) &&
                                nec_check_in_range(cur->duration1, NEC_LEADING_CODE_DURATION_1);
        if (!valid_leading_code) {
            return false;
        }
        cur++;
        for (int i = 0; i < 16; i++) {
            if (nec_parse_logic1(cur)) {
                address |= 1 << i;
            } else if (nec_parse_logic0(cur)) {
                address &= ~(1 << i);
            } else {
                return false;
            }
            cur++;
        }
        for (int i = 0; i < 16; i++) {
            if (nec_parse_logic1(cur)) {
                command |= 1 << i;
            } else if (nec_parse_logic0(cur)) {
                command &= ~(1 << i);
            } else {
                return false;
            }
            cur++;
        }
        // save address and command
        s_nec_code_address = address;
        s_nec_code_command = command;
        return true;
    }

static bool nec_parse_frame_repeat(rmt_symbol_word_t *rmt_nec_symbols)
    {
        return nec_check_in_range(rmt_nec_symbols->duration0, NEC_REPEAT_CODE_DURATION_0) &&
            nec_check_in_range(rmt_nec_symbols->duration1, NEC_REPEAT_CODE_DURATION_1);
    }
    
static motor_state_t nec_decode (rmt_symbol_word_t *rmt_nec_symbols, size_t symbol_num)
    {
        // decode RMT symbols
        switch (symbol_num) {
            // nếu gửi lần đầu thì sẽ là size = 32 gửi lặp lại thì sẽ là size = 2
        case 34: // NEC normal frame
            if (nec_parse_frame(rmt_nec_symbols)) {
                uint32_t nec_code = ((uint32_t)s_nec_code_address << 8) | (s_nec_code_command & 0xFF);
                ESP_LOGI(TAG,"NEC Code = 0x%X\r\n\r\n",(unsigned int) nec_code);
            }
            break;
        case 2: // NEC repeat frame
            if (nec_parse_frame_repeat(rmt_nec_symbols)) {
                uint32_t nec_code = ((uint32_t)s_nec_code_address << 8) | (s_nec_code_command & 0xFF);
            ESP_LOGI(TAG, "NEC Code = 0x%X (repeat)\r\n\r\n", (unsigned int) nec_code);
            }
            break;
        default:
            ESP_LOGI(TAG, "Unknown NEC frame\r\n\r\n");
            break;
        }
        uint32_t nec_code = ((uint32_t)s_nec_code_address << 8) | (s_nec_code_command & 0xFF);
        if (nec_code == foward){
            return MOTOR_FOWARD;
        }
        else if (nec_code == backward){
            return MOTOR_BACKWARD;
        }
        else if (nec_code == stop){
           return MOTOR_STOP;
        }return MOTOR_STOP;
        //ESP_LOGI(TAG, "State: %d", );
    }
//== NEC PARSER TASK ===
static void ir_parser_task(void *arg)
    {
        rmt_rx_done_event_data_t rx_data;
        motor_state_t motor_cmd;
        while (1) {
           if (xQueueReceive(ir_raw_queue, &rx_data, portMAX_DELAY)) {
            // Decode symbols
            motor_state_t state = nec_decode(rx_data.received_symbols,
                                             rx_data.num_symbols);

            // Tạo command gửi cho motor
            motor_cmd_t cmd = { .state = state };
            xQueueSend(motor_cmd_queue, &cmd, portMAX_DELAY);

            ESP_LOGI(TAG, "Decoded -> state = %d", state);

            // restart receive
            rmt_receive(rx_chan, NULL, 0, NULL);
            }
        }
    }

// === INIT ===

void app_ir_init(QueueHandle_t motor_queue){
    motor_cmd_queue = motor_queue;

// 1. Tạo queue nhận dữ liệu thô từ RMT
    ir_raw_queue = xQueueCreate(10, sizeof(rmt_rx_done_event_data_t));
    assert(ir_raw_queue); //debug
// 2. Cấu hình GPIO cho RMT RX
    rmt_rx_channel_config_t rx_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = 1 * 1000 * 1000, // 1MHz, 1 tick = 1us
        .mem_block_symbols = 64,           // Số symbol tối đa trong một khối bộ nhớ
        .gpio_num = ir_data,               // GPIO kết nối với tín hiệu IR
        
    };
    ESP_ERROR_CHECK(rmt_new_rx_channel(&rx_config, &rx_chan));
// 3. Cấu hình callback để nhận dữ liệu
    rmt_rx_event_callbacks_t cbs = {
        .on_recv_done = rmt_rx_done_callback, // Callback khi nhận xong dữ liệu
    };
    ESP_ERROR_CHECK(rmt_rx_register_event_callbacks(rx_chan, &cbs, ir_raw_queue));
// 4.Enable RMT RX channel
    ESP_ERROR_CHECK(rmt_enable(rx_chan));   
    ESP_ERROR_CHECK(rmt_receive(rx_chan, NULL, 0, NULL));
// 5. Tạo task để giải mã tín hiệu IR
    xTaskCreate(ir_parser_task, "ir_parser_task", 4096, NULL, 10, NULL);
    ESP_LOGI(TAG, "IR parser task created");
}