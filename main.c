#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
extern void bitcpy_test(void);

void bit_stream_print(void const* _stream,
                      size_t len,
                      size_t offset,
                      size_t bitlen,
                      bool idx) {
    uint8_t const* stream = _stream;
    if (idx) {
        for (int i = 0; i < len; ++i) {
            printf("       %x", i & 0xF);
        }
        printf("\n");
        for (int i = 0; i < len; ++i) {
            printf("76543210");
        }
        printf("\n");
    }

    for (int i = 0; i < len << 3; ++i) {
        if (i == offset) {
            putchar('[');
        } else if (i == offset + bitlen - 1) {
            putchar(']');
        } else {
            putchar(' ');
        }
    }
    printf("\n");

    for (size_t i = 0; i < len; ++i) {
        uint8_t d = stream[i];
        for (int j = 8; j--;) {
            putchar(d & (1 << j) ? '1' : '0');
        }
    }
    printf("\n");
    for (size_t i = 0; i < len; ++i) {
        uint8_t d = stream[i];
        printf("%x   ", (d >> 4) & 0xFU);
        printf("%x   ", (d >> 0) & 0xFU);
    }
    printf("\n");
}

int main(int argc, char* argv[]) {
    for (int i = 0; i < 400; ++i)
        bitcpy_test();
}
