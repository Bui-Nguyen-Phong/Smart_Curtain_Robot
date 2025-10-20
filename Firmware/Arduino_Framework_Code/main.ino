#include <IRremote.h>
#include <Stepper.h>
#include "esp_sleep.h"

// ==== Cấu hình chân GPIO ====
#define IR_WAKE_PIN    27      // Chân wake-up từ cảm biến IR
#define IR_RECV_PIN    26      // Chân nhận tín hiệu remote IR
#define IN1            22
#define IN2            21
#define IN3            17
#define IN4            16

// ==== Cấu hình Stepper ====
const int stepsPerRevolution = 2048;
Stepper myStepper(stepsPerRevolution, IN1, IN3, IN2, IN4);

// ==== Mã IR điều khiển ====
const uint32_t CMD_STOP     = 0xBC43FF00;
const uint32_t CMD_FORWARD  = 0xBF40FF00;
const uint32_t CMD_BACKWARD = 0xBB44FF00;

// ==== Biến trạng thái ====
enum MotorState {STOP, FORWARD, BACKWARD};
MotorState currentState = STOP;

// ==== Biến timer ====
unsigned long lastCommandTime = 0;
const unsigned long TIMEOUT_MS = 60000; // 60 giây

IRrecv irrecv(IR_RECV_PIN);
decode_results results;

void setup() {
  Serial.begin(115200);
  delay(500);

  // Nếu đây là lần wake-up từ deep sleep
  esp_sleep_wakeup_cause_t wakeReason = esp_sleep_get_wakeup_cause();
  if (wakeReason == ESP_SLEEP_WAKEUP_EXT0) {
    Serial.println("Woke up from IR wake pin (IO27)");
  }

  // Khởi tạo động cơ
  myStepper.setSpeed(15); // RPM

  // Khởi tạo IR
  IrReceiver.begin(IR_RECV_PIN, ENABLE_LED_FEEDBACK);

  // Khởi tạo thời gian
  lastCommandTime = millis();

  // Khởi động từ trạng thái trước đó nếu cần
  // Nếu bạn muốn lưu vào RTC memory thì dùng RTC_DATA_ATTR
  // Tạm thời, khởi động mặc định dừng
  currentState = STOP;
}

void loop() {
  if (IrReceiver.decode()) {
    uint32_t irCode = IrReceiver.decodedIRData.decodedRawData;
    Serial.print("IR Received: 0x");
    Serial.println(irCode, HEX);

    lastCommandTime = millis(); // reset timeout

    // Xử lý lệnh
    switch (irCode) {
      case CMD_FORWARD:
        currentState = FORWARD;
        Serial.println("Motor FORWARD");
        break;
      case CMD_BACKWARD:
        currentState = BACKWARD;
        Serial.println("Motor BACKWARD");
        break;
      case CMD_STOP:
        currentState = STOP;
        Serial.println("Motor STOP");
        stopMotor();
        break;
    }
    IrReceiver.resume();
  }

  // Điều khiển động cơ theo trạng thái hiện tại
  if (currentState == FORWARD) {
    myStepper.step(1); // Quay phải
  } else if (currentState == BACKWARD) {
    myStepper.step(-1); // Quay trái
  }

  // Kiểm tra timeout
  if ((millis() - lastCommandTime) > TIMEOUT_MS) {
    Serial.println("No IR command for 60s -> Going to sleep");
    stopMotor();
    delay(100);

    // Đưa ESP32 vào deep sleep, đánh thức bằng GPIO27 (LOW)
    esp_sleep_enable_ext0_wakeup(GPIO_NUM_27, 0); // 0 = LOW
    esp_deep_sleep_start();
  }
}

// ==== Hàm dừng động cơ hoàn toàn ====
void stopMotor() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
}
