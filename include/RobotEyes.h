#pragma once
#include <GyverOLED.h>
#include "PCG.h"

// Состояния настроения
enum class Mood {
    NORMAL,
    LISTENING,
	SPEAKING,
	DIZZY,
    BITMAP_MODE
};

// Внутренние состояния анимации моргания
enum class BlinkState {
    IDLE,       // Открыты, ждем
    CLOSING,    // Вектор сжимается (закрываем)
    OPENING     // Вектор расширяется (открываем)
};

class RobotEyes {
private:
    GyverOLED<SSD1306_128x64, OLED_BUFFER>& oled;
    PCG32 rng;

    Mood currentMood;
    BlinkState blinkState;

    // Таймеры моргания
    uint32_t lastBlinkTime;
    uint32_t nextBlinkDelay;

    // --- МАТЕМАТИЧЕСКАЯ МОДЕЛЬ (Текущие значения) ---
    float eyeWidth;
    float eyeHeight;
    float eyeDist;

    // --- ЦЕЛЕВЫЕ ЗНАЧЕНИЯ (Для алгоритма Easing) ---
    float targetWidth;
    float targetHeight;
    float targetEyeDist;
    
    // Базовая высота открытого глаза для текущего настроения
    float normalHeight; 

    // Скорости анимации (от 0.0 до 1.0)
    float blinkSpeed;
    float moodSpeed;

    // Функция Easing (плавное приближение)
    void approach(float &current, float target, float speed);

public:
    RobotEyes(GyverOLED<SSD1306_128x64, OLED_BUFFER>& display);

    void init();
    
    // update() - только математика и стейт-машины (НИКАКОЙ отрисовки)
    void update(); 
    
    // draw() - только отправка байтов в буфер экрана (НИКАКОЙ логики)
    void draw();   

    void setMood(Mood newMood);
    void blink();
};