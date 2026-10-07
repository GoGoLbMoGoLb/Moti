#pragma once
#include <Arduino.h>
#include "RobotEyes.h"

// --- ВСЕ СОСТОЯНИЯ РОБОТА ---
enum class DeviceState {
    BOOTING,      // Включение (можно показать лого)
    IDLE,         // Ждет действий, спокойно моргает
    LISTENING,    // Кнопка зажата, "внимательно слушает"
    SPEAKING,     // Проигрывает мелодию
    DIZZY         // Встряхнули, у робота "кружится голова"
};

// --- ВСЕ ВОЗМОЖНЫЕ СОБЫТИЯ ---
enum class DeviceEvent {
    BTN_PRESSED,
    BTN_RELEASED,
    SHAKE_DETECTED,
    AUDIO_FINISHED,
    TIMEOUT        // Системное событие по таймеру
};

class DeviceStateMachine {
private:
    DeviceState currentState;
    RobotEyes& eyes;
    
    uint32_t stateEnterTime; // Время входа в текущее состояние

    // Приватный метод для смены состояния с вызовом логики входа
    void changeState(DeviceState newState);

public:
    DeviceStateMachine(RobotEyes& eyesInstance);
    
    void init();
    
    // Главный роутер событий
    void dispatchEvent(DeviceEvent event);
    
    // Постоянное обновление (для обработки таймаутов)
    void update();
    
    DeviceState getState() const { return currentState; }
};