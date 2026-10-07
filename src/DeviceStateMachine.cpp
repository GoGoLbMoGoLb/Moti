#include "DeviceStateMachine.h"

DeviceStateMachine::DeviceStateMachine(RobotEyes& eyesInstance) 
    : eyes(eyesInstance), currentState(DeviceState::BOOTING), stateEnterTime(0) {}

void DeviceStateMachine::init() {
    changeState(DeviceState::IDLE);
}

// ---------------------------------------------------------
// ОБРАБОТЧИК СОБЫТИЙ (Event Router)
// ---------------------------------------------------------
void DeviceStateMachine::dispatchEvent(DeviceEvent event) {
    switch (currentState) {
        case DeviceState::IDLE:
            if (event == DeviceEvent::BTN_PRESSED) {
                changeState(DeviceState::LISTENING);
            } else if (event == DeviceEvent::SHAKE_DETECTED) {
                changeState(DeviceState::DIZZY);
            }
            break;

        case DeviceState::LISTENING:
            // Если отпустили кнопку - переходим к "ответу" (воспроизведение мелодии)
            if (event == DeviceEvent::BTN_RELEASED) {
                changeState(DeviceState::SPEAKING);
            } else if (event == DeviceEvent::SHAKE_DETECTED) {
                // Если встряхнули во время прослушивания - сбрасываем в DIZZY
                changeState(DeviceState::DIZZY);
            }
            break;

        case DeviceState::SPEAKING:
            if (event == DeviceEvent::AUDIO_FINISHED) {
                changeState(DeviceState::IDLE);
            } else if (event == DeviceEvent::SHAKE_DETECTED) {
                changeState(DeviceState::DIZZY);
            }
            break;

        case DeviceState::DIZZY:
            if (event == DeviceEvent::TIMEOUT) {
                changeState(DeviceState::IDLE);
            }
            break;
            
        default:
            break;
    }
}

// ---------------------------------------------------------
// ЛОГИКА СМЕНЫ СОСТОЯНИЙ (Enter State Actions)
// ---------------------------------------------------------
void DeviceStateMachine::changeState(DeviceState newState) {
    if (currentState == newState) return;
    
    currentState = newState;
    stateEnterTime = millis();

    // Применяем настройки UI и железа для нового состояния
    switch (currentState) {
        case DeviceState::IDLE:
            eyes.setMood(Mood::NORMAL);
            break;
            
        case DeviceState::LISTENING:
            eyes.setMood(Mood::LISTENING); // Зум глаз
            break;
            
        case DeviceState::SPEAKING:
            eyes.setMood(Mood::SPEAKING); // Специальная эмоция для разговора
            break;

        case DeviceState::DIZZY:
            eyes.setMood(Mood::DIZZY); // Сумасшедшие глаза
            break;
            
        default:
            break;
    }
}

// ---------------------------------------------------------
// ОБРАБОТКА ТАЙМАУТОВ И ФОНОВЫХ ПРОЦЕССОВ
// ---------------------------------------------------------
void DeviceStateMachine::update() {
    uint32_t now = millis();

    // Например, состояние DIZZY длится ровно 3 секунды, затем робот "очухивается"
    if (currentState == DeviceState::DIZZY) {
        if (now - stateEnterTime > 3000) {
            dispatchEvent(DeviceEvent::TIMEOUT);
        }
    }
    
    // Если нужно добавить переход в Deep Sleep после 5 минут в IDLE:
    // if (currentState == DeviceState::IDLE && (now - stateEnterTime > 300000)) {
    //     ESP.deepSleep(0);
    // }
}