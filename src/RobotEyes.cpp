#include "RobotEyes.h"

RobotEyes::RobotEyes(GyverOLED<SSD1306_128x64, OLED_BUFFER>& display) 
    : oled(display), currentMood(Mood::NORMAL), blinkState(BlinkState::IDLE) {}

void RobotEyes::init() {
    rng = PCG32(analogRead(A0) * 123456789ULL, 987654321ULL);
    
    // Изначальные размеры (круглые глаза)
    normalHeight = 24.0;
    eyeWidth = 24.0;
    eyeHeight = 24.0;
    eyeDist = 48.0;
    
    targetWidth = 24.0;
    targetHeight = normalHeight;
    targetEyeDist = 48.0;

    blinkSpeed = 0.5; // Высокая скорость для резкого моргания
    moodSpeed = 0.15; // Низкая скорость для плавного изменения эмоций (зум)

    lastBlinkTime = millis();
    nextBlinkDelay = rng.range(3000) + 1000; // От 1 до 4 секунд
}

// Простой, но крайне эффективный Easing (фильтр низких частот)
void RobotEyes::approach(float &current, float target, float speed) {
    current += (target - current) * speed;
}

void RobotEyes::setMood(Mood newMood) {
    if (currentMood == newMood) return;
    currentMood = newMood;

    switch (currentMood) {
        case Mood::NORMAL:
            targetWidth = 24.0;
            normalHeight = 24.0;
            targetEyeDist = 48.0;
            break;
        case Mood::LISTENING:
            // Глаза становятся больше (зум) и сдвигаются к переносице
            targetWidth = 34.0;
            normalHeight = 36.0;
            targetEyeDist = 36.0; 
            break;
		case Mood::SPEAKING:
            // Глаза пульсируют в такт разговору (размер меняется)
            targetWidth = 24.0;
            normalHeight = 20.0;
            targetEyeDist = 48.0;
            break;
        case Mood::DIZZY:
            // Глаза разъезжаются в стороны и становятся маленькими - эффект растерянности
            targetWidth = 14.0;
            normalHeight = 14.0;
            targetEyeDist = 60.0; 
            blinkSpeed = 0.8; // Быстрое, нервное моргание
            break;
        case Mood::BITMAP_MODE:
            break;
    }
    
    // Если мы сейчас не моргаем, обновляем целевую высоту
    if (blinkState == BlinkState::IDLE) {
        targetHeight = normalHeight;
    }
}

void RobotEyes::blink() {
    // Начинаем моргать, только если глаза открыты
    if (blinkState == BlinkState::IDLE) {
        blinkState = BlinkState::CLOSING;
        targetHeight = 0.0; // Цель: сплющить глаз до 0
    }
}

// ---------------------------------------------------------
// UPDATE: ТОЛЬКО МАТЕМАТИКА. Вызывается максимально часто.
// ---------------------------------------------------------
void RobotEyes::update() {
    uint32_t now = millis();

    // 1. PCG-Генератор случайных морганий
    if (blinkState == BlinkState::IDLE && (now - lastBlinkTime > nextBlinkDelay)) {
        blink();
        lastBlinkTime = now;
        nextBlinkDelay = rng.range(4000) + 500; 
    }

    // 2. Стейт-машина векторного моргания
    if (blinkState == BlinkState::CLOSING) {
        targetHeight = 0.0;
        // Если высота сплющилась почти до конца (1.5 пикселя)
        if (eyeHeight <= 1.5) { 
            blinkState = BlinkState::OPENING;
            targetHeight = normalHeight; // Цель: вернуть в открытое состояние
        }
    } 
    else if (blinkState == BlinkState::OPENING) {
        targetHeight = normalHeight;
        // Если глаз почти открылся
        if (normalHeight - eyeHeight <= 1.0) { 
            eyeHeight = normalHeight; // Привязка к сетке (Snap)
            blinkState = BlinkState::IDLE;
        }
    }

    // 3. Расчет новой геометрии кадра (Easing)
    approach(eyeWidth, targetWidth, moodSpeed);
    approach(eyeHeight, targetHeight, blinkSpeed); // Для высоты скорость моргания
    approach(eyeDist, targetEyeDist, moodSpeed);
}

// ---------------------------------------------------------
// DRAW: ТОЛЬКО РЕНДЕР. Никаких вычислений логики.
// ---------------------------------------------------------
/*void RobotEyes::draw() {
    oled.clear();

    if (currentMood == Mood::BITMAP_MODE) {
        // Здесь можно вызвать функцию oled.drawBitmap()
        oled.setCursor(30, 30);
        oled.print("BITMAP MODE");
    } else {
        int centerX = 64;
        int centerY = 32;

        // Половинные значения для отрисовки от центра
        int halfDist = (int)(eyeDist / 2.0);
        int halfW = (int)(eyeWidth / 2.0);
        int halfH = (int)(eyeHeight / 2.0);

        int leftEyeX = centerX - halfDist;
        int rightEyeX = centerX + halfDist;

        // Если высота меньше 1 пикселя, рисуем линию (закрытое веко)
        if (halfH < 1) {
            oled.fastLineH(leftEyeX - halfW, centerY, leftEyeX + halfW, centerY);
            oled.fastLineH(rightEyeX - halfW, centerY, rightEyeX + halfW, centerY);
        } else {
            // Вычисляем радиус скругления. Он равен наименьшей стороне,
            // поэтому при открытом глазе это круг, а при моргании - овал/прищур
            int radius = min(halfW, halfH); 

            // Рисуем динамические векторные глаза (Скругленные прямоугольники)
            oled.roundRect(
                leftEyeX - halfW, centerY - halfH, 
                leftEyeX + halfW, centerY + halfH, 
                OLED_FILL
            );

            oled.roundRect(rightEyeX - halfW, centerY - halfH, rightEyeX + halfW, centerY + halfH, OLED_FILL);
            
            // Динамический блик для живости взгляда (двигается вместе с веком)
            oled.circle(
                leftEyeX + halfW/2, 
                centerY - halfH/2, 
                max(1, radius/3), 
                OLED_CLEAR
            );
            oled.circle(
                rightEyeX + halfW/2, 
                centerY - halfH/2, 
                max(1, radius/3), 
                OLED_CLEAR
            );
        }
    }
    
    // Выталкиваем просчитанный буфер на экран по I2C
    oled.update();
}
*/
// ---------------------------------------------------------
// DRAW: Рендер через "Веки" (Маскирование черным цветом)
// ---------------------------------------------------------
void RobotEyes::draw() {
    oled.clear();

    if (currentMood == Mood::BITMAP_MODE) {
        oled.setCursor(30, 30);
        oled.print("BITMAP MODE");
    } else {
        int centerX = 64;
        int centerY = 32;

        int halfDist = (int)(eyeDist / 2.0);
        int halfW = (int)(eyeWidth / 2.0); // halfW выступает как базовый радиус глаза
        int halfH = (int)(eyeHeight / 2.0);

        int leftEyeX = centerX - halfDist;
        int rightEyeX = centerX + halfDist;

        // Если высота меньше 1 пикселя, глаз закрыт - рисуем линию стыка век
        if (halfH < 1) {
            oled.line(leftEyeX - halfW, centerY, leftEyeX + halfW, centerY);
            oled.line(rightEyeX - halfW, centerY, rightEyeX + halfW, centerY);
        } else {
            // 1. Рисуем полностью открытые круглые глаза
            oled.circle(leftEyeX, centerY, halfW, OLED_FILL);
            oled.circle(rightEyeX, centerY, halfW, OLED_FILL);
            
            // 2. Рисуем блики ДО закрытия век (чтобы веко реалистично перекрывало блик)
            int glareRadius = max(1, halfW / 4);
            int glareOffset = halfW / 3;
            oled.circle(leftEyeX + glareOffset, centerY - glareOffset, glareRadius, OLED_CLEAR);
            oled.circle(rightEyeX + glareOffset, centerY - glareOffset, glareRadius, OLED_CLEAR);

            // 3. МАСКИРОВКА ВЕКАМИ (Эффект моргания / прищура)
            // Если текущая высота меньше ширины, значит веки прикрыты
            if (halfH < halfW) {
                // Верхнее веко (стираем всё, что выше разрешенной высоты halfH)
                oled.rect(
                    leftEyeX - halfW, centerY - halfW - 1, 
                    leftEyeX + halfW, centerY - halfH - 1, 
                    OLED_CLEAR
                );
                oled.rect(
                    rightEyeX - halfW, centerY - halfW - 1, 
                    rightEyeX + halfW, centerY - halfH - 1, 
                    OLED_CLEAR
                );
                
                // Нижнее веко (стираем всё, что ниже разрешенной высоты halfH)
                oled.rect(
                    leftEyeX - halfW, centerY + halfH + 1, 
                    leftEyeX + halfW, centerY + halfW + 1, 
                    OLED_CLEAR
                );
                oled.rect(
                    rightEyeX - halfW, centerY + halfH + 1, 
                    rightEyeX + halfW, centerY + halfW + 1, 
                    OLED_CLEAR
                );
            }
        }
    }
    
    // Выталкиваем буфер на экран
    oled.update();
}