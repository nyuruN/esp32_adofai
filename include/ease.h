
#include "math.h"

float easeOutBack(float x) {
    if (x >= 1.0) return 1.0;

    const float c1 = 1.70158;
    const float c3 = c1 + 1;

    return 1 + c3 * pow(x - 1, 3) + c1 * pow(x - 1, 2);
}