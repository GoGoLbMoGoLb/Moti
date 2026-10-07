#include <Arduino.h>
#include <Wire.h>
#include <GyverOLED.h>
#include "RobotEyes.h"
#include "DeviceStateMachine.h"

// --- ПИНЫ ESP8266 (NodeMCU/Wemos) ---
#define PIN_SDA D2
#define PIN_SCL D1
#define PIN_BUTTON 13
#define PIN_BUZZER D5

// --- НАСТРОЙКИ ---
#define ADXL346_ADDRESS 0x53
#define SHAKE_THRESHOLD 1.5  // Порог "встряхивания" в G

GyverOLED<SSD1306_128x64, OLED_BUFFER> oled;
RobotEyes robot(oled);
DeviceStateMachine stateMachine(robot);

// --- СТРУКТУРА ДЛЯ ВЕКТОРА УСКОРЕНИЯ ---
struct Vector3 {
    float x, y, z;
    float magnitude() {
        return sqrt(x*x + y*y + z*z);
    }
};

Vector3 accelData;
float lastMagnitude = 1.0;

bool lastButtonState = false;
float lastAccelMag = 1.0;

// --- СТЕЙТ-МАШИНА МЕЛОДИИ (Неблокирующая) ---
const uint16_t melodyFreqs[] = {880, 1046, 1318, 1760}; // Ноты
const uint16_t melodyDurs[]  = {100, 100, 150,  200};  // Длительности
const uint8_t melodyLength = 4;

bool isPlaying = false;
uint8_t currentNote = 0;
uint32_t noteStartTime = 0;
bool buttonState = false;

// --- ИНИЦИАЛИЗАЦИЯ ADXL346 ---
void initADXL346() {
    Wire.beginTransmission(ADXL346_ADDRESS);
    Wire.write(0x2D); // Register POWER_CTL
    Wire.write(0x08); // Переводим в режим измерения (Measure mode)
    Wire.endTransmission();
    
    /* 
     * Совет: Для аппаратного прерывания по Tap (стук) 
     * можно сконфигурировать регистры 0x2A (TAP_AXES), 0x1D (THRESH_TAP)
     * и включить прерывание в регистре 0x2E (INT_ENABLE). 
     * Но по ТЗ используем векторную математику для shake-детекции.
     */
}

// --- ЧТЕНИЕ ADXL346 ---
void readADXL346() {
    Wire.beginTransmission(ADXL346_ADDRESS);
    Wire.write(0x32); // Начало данных (DATAX0)
    Wire.endTransmission(false);
    Wire.requestFrom(ADXL346_ADDRESS, 6, true);

    if (Wire.available() == 6) {
        int16_t x = Wire.read() | (Wire.read() << 8);
        int16_t y = Wire.read() | (Wire.read() << 8);
        int16_t z = Wire.read() | (Wire.read() << 8);

        // Преобразование сырых данных в G (грубо при +/- 2g)
        accelData.x = x * 0.0039;
        accelData.y = y * 0.0039;
        accelData.z = z * 0.0039;
    }
}

void handleHardwareAudio() {
    // Аудио играет только в состоянии SPEAKING
    if (stateMachine.getState() != DeviceState::SPEAKING) return;

    uint32_t now = millis();
    if (now - noteStartTime > (melodyDurs[currentNote] + 20)) {
        currentNote++;
        if (currentNote >= 4) { // Мелодия закончилась
            currentNote = 0;
            noTone(PIN_BUZZER);
            // ГЕНЕРИРУЕМ СОБЫТИЕ окончания аудио!
            stateMachine.dispatchEvent(DeviceEvent::AUDIO_FINISHED);
        } else {
            tone(PIN_BUZZER, melodyFreqs[currentNote], melodyDurs[currentNote]);
            noteStartTime = now;
        }
    }
}

void setup() {
    Serial.begin(115200);
    Wire.begin(PIN_SDA, PIN_SCL);
  	Wire.setClock(400000L);
    
    pinMode(PIN_BUTTON, INPUT);
    pinMode(PIN_BUZZER, OUTPUT);

    oled.init();
    oled.clear();
    oled.update();

    initADXL346();
    robot.init();
    stateMachine.init();
}

void loop() {
    uint32_t now = millis();
    // 2. ОПРОС АКСЕЛЕРОМЕТРА (Hardware Input - 50Hz)
    static uint32_t lastAccelUpdate = 0;
    if (now - lastAccelUpdate > 20) {
        lastAccelUpdate = now;
		// readADXL346();
        float currentMag = accelData.magnitude();
        if (abs(currentMag - lastAccelMag) > SHAKE_THRESHOLD) {
            stateMachine.dispatchEvent(DeviceEvent::SHAKE_DETECTED);
        }
        lastAccelMag = currentMag;
    }

    // 2. Опрос емкостной кнопки TTP223
	bool btnState = digitalRead(PIN_BUTTON);
    if (btnState && !lastButtonState) {
        stateMachine.dispatchEvent(DeviceEvent::BTN_PRESSED);
    } else if (!btnState && lastButtonState) {
        stateMachine.dispatchEvent(DeviceEvent::BTN_RELEASED);
    }
    lastButtonState = btnState;

    // 3. Обработка стейт-машин
    stateMachine.update();
    handleHardwareAudio();
    
    // 4. Обновление физики глаз и отрисовка буфера на дисплей
    robot.update();
    robot.draw();
}