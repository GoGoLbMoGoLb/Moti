#pragma once
#include <Arduino.h>

class PCG32 {
private:
    uint64_t state;
    uint64_t inc;

public:
    // Инициализация генератора
    PCG32(uint64_t init_state = 0x853c49e6748fea9bULL, uint64_t init_seq = 0xda3e39cb94b95bdbULL) {
        state = 0U;
        inc = (init_seq << 1u) | 1u;
        next();
        state += init_state;
        next();
    }

    // Генерация следующего 32-битного числа
    uint32_t next() {
        uint64_t oldstate = state;
        // Линейный конгруэнтный шаг
        state = oldstate * 6364136223846793005ULL + inc;
        // Permutation шаг (XOR-shift и вращение)
        uint32_t xorshifted = ((oldstate >> 18u) ^ oldstate) >> 27u;
        uint32_t rot = oldstate >> 59u;
        return (xorshifted >> rot) | (xorshifted << ((-rot) & 31));
    }

    // Получение числа в заданном диапазоне [0, bound-1]
    uint32_t range(uint32_t bound) {
        uint32_t threshold = -bound % bound;
        for (;;) {
            uint32_t r = next();
            if (r >= threshold) return r % bound;
        }
    }
};