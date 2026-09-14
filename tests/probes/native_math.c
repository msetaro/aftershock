#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

float Q_rsqrt(float number);

int main(void) {
    /* Measured from the unmodified GPL routine in a 32-bit SSE C executable. */
    static const float inputs[] = {0.125f, 0.5f, 1.0f, 2.0f, 4.0f, 16.0f, 100.0f, 4096.0f};
    static const uint32_t expected[] = {
        0x4034f95e, 0x3fb4f95e, 0x3f7f910f, 0x3f34f95e,
        0x3eff910f, 0x3e7f910f, 0x3dcc7b79, 0x3c7f910f
    };
    for (unsigned i = 0; i < sizeof(inputs) / sizeof(inputs[0]); ++i) {
        float actual = Q_rsqrt(inputs[i]);
        uint32_t bits;
        memcpy(&bits, &actual, sizeof(bits));
        assert(bits == expected[i]);
    }
    puts("PASS: native Q_rsqrt matches all eight original 32-bit words");
    return 0;
}
