#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define var __auto_type
#define let var const
#define min(a, b)                                                              \
    ({                                                                         \
        let _a = (a);                                                          \
        let _b = (b);                                                          \
        _a < _b ? _a : _b;                                                     \
    })

static void bit_stream_print(void const* _stream,
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

static uint32_t _get_mask(size_t offset, size_t bitlen) {
    assert(offset + bitlen <= 32);

    static const uint32_t _mask_s[] = {
        [0x00] = 0x00000000, [0x01] = 0x00000080, [0x02] = 0x000000C0,
        [0x03] = 0x000000E0, [0x04] = 0x000000F0, [0x05] = 0x000000F8,
        [0x06] = 0x000000FC, [0x07] = 0x000000FE, [0x08] = 0x000000FF,

        [0x09] = 0x000080FF, [0x0A] = 0x0000C0FF, [0x0B] = 0x0000E0FF,
        [0x0C] = 0x0000F0FF, [0x0D] = 0x0000F8FF, [0x0E] = 0x0000FCFF,
        [0x0F] = 0x0000FEFF, [0x10] = 0x0000FFFF,

        [0x11] = 0x0080FFFF, [0x12] = 0x00C0FFFF, [0x13] = 0x00E0FFFF,
        [0x14] = 0x00F0FFFF, [0x15] = 0x00F8FFFF, [0x16] = 0x00FCFFFF,
        [0x17] = 0x00FEFFFF, [0x18] = 0x00FFFFFF,

        [0x19] = 0x80FFFFFF, [0x1A] = 0xC0FFFFFF, [0x1B] = 0xE0FFFFFF,
        [0x1C] = 0xF0FFFFFF, [0x1D] = 0xF8FFFFFF, [0x1E] = 0xFCFFFFFF,
        [0x1F] = 0xFEFFFFFF, [0x20] = 0xFFFFFFFF,
    };

    uint32_t const mask = (~_mask_s[offset]) ^ (~_mask_s[offset + bitlen]);
#ifndef NDEBUG
    printf("_get_mask(%zu, %zu) = 0x%08x\n", offset, bitlen, mask);
    bit_stream_print(&mask, sizeof(mask), offset, bitlen, true);
#endif
    return mask;
}

static inline uint8_t _get_uint8_mask(size_t offset, size_t bitlen) {
    static const uint8_t _mask_s[] = {
        0x80,
        0xc0,
        0xe0,
        0xf0,
        0xf8,
        0xfc,
        0xfe,
        0xff,
    };
    return _mask_s[bitlen - 1] >> offset;
}

static inline void _bitcpy_same_offset(uint8_t* dst,
                                       uint8_t const* src,
                                       size_t offset,
                                       size_t bitlen) {
    if (offset) {
        let _bitlen_left = 8 - offset;
        if (bitlen <= _bitlen_left) {
            let src_mask = _get_uint8_mask(offset, bitlen);
            dst[0] = (dst[0] & ~src_mask) | (src[0] & src_mask);
            return;
        } else {
            let src_mask = _get_uint8_mask(offset, _bitlen_left);
            dst[0] = (dst[0] & ~src_mask) | (src[0] & src_mask);
            bitlen -= _bitlen_left;
            if (!bitlen) {
                return;
            }
            dst++;
            src++;
        }
    }

    let bytelen = bitlen >> 3;
    if (bytelen) {
        memcpy(dst, src, bytelen);
        dst += bytelen;
        src += bytelen;
    }
    bitlen &= 7U;
    if (bitlen) {
        let src_mask = _get_uint8_mask(0, bitlen);
        dst[0] = (dst[0] & ~src_mask) | (src[0] & src_mask);
    }
}

static inline void _bitcpy_not_same_offset(uint8_t* dst,
                                           size_t dst_offset,
                                           uint8_t const* src,
                                           size_t src_offset,
                                           size_t bitlen) {
    assert(dst_offset != src_offset);
    if (dst_offset) {
        uint8_t _sdata;
        let _bitlen = min(bitlen, 8 - dst_offset);
        let src_mask = _get_uint8_mask(dst_offset, _bitlen);

        if (dst_offset > src_offset) {
            _sdata = (src[0] >> (dst_offset - src_offset));
        } else {
            _sdata = ((src[0] << src_offset) | (src[1] >> (8 - src_offset))) >>
                     dst_offset;
        }
        dst[0] = (dst[0] & ~src_mask) | (_sdata & src_mask);

        bitlen -= _bitlen;
        if (!bitlen) {
            return;
        }

        src_offset += _bitlen;
        src += src_offset >> 3;
        src_offset &= 7U;

        dst_offset += _bitlen;
        dst += dst_offset >> 3;
        dst_offset &= 7U;
    }

    assert(dst_offset == 0);
    while (bitlen >= 64) {
        dst[0] = (src[0] << src_offset) | (src[1] >> (8 - src_offset));
        dst[1] = (src[1] << src_offset) | (src[2] >> (8 - src_offset));
        dst[2] = (src[2] << src_offset) | (src[3] >> (8 - src_offset));
        dst[3] = (src[3] << src_offset) | (src[4] >> (8 - src_offset));
        dst[4] = (src[4] << src_offset) | (src[5] >> (8 - src_offset));
        dst[5] = (src[5] << src_offset) | (src[6] >> (8 - src_offset));
        dst[6] = (src[6] << src_offset) | (src[7] >> (8 - src_offset));
        dst[7] = (src[7] << src_offset) | (src[8] >> (8 - src_offset));
        bitlen -= 64;
        dst += 8;
        src += 8;
    }
    if (bitlen >= 32) {
        dst[0] = (src[0] << src_offset) | (src[1] >> (8 - src_offset));
        dst[1] = (src[1] << src_offset) | (src[2] >> (8 - src_offset));
        dst[2] = (src[2] << src_offset) | (src[3] >> (8 - src_offset));
        dst[3] = (src[3] << src_offset) | (src[4] >> (8 - src_offset));
        bitlen -= 32;
        dst += 4;
        src += 4;
    }
    if (bitlen >= 16) {
        dst[0] = (src[0] << src_offset) | (src[1] >> (8 - src_offset));
        dst[1] = (src[1] << src_offset) | (src[2] >> (8 - src_offset));
        bitlen -= 16;
        dst += 2;
        src += 2;
    }
    while (bitlen >= 8) {
        dst[0] = (src[0] << src_offset) | (src[1] >> (8 - src_offset));
        bitlen -= 8;
        dst += 1;
        src += 1;
    }
    if (bitlen) {
        let src_mask = _get_uint8_mask(0, bitlen);
        dst[0] = (dst[0] & ~src_mask) |
                 (((src[0] << src_offset) | (src[1] >> (8 - src_offset))) &
                  src_mask);
        assert(bitlen < 8);
    }
}

void bitcpy(void* _dst,
            size_t dst_offset,
            void const* _src,
            size_t src_offset,
            size_t bitlen) {
    uint8_t* dst = (uint8_t*)_dst;
    uint8_t const* src = (uint8_t const*)_src;

    dst += dst_offset >> 3;
    src += src_offset >> 3;
    dst_offset &= 7;
    src_offset &= 7;

    if (dst_offset == src_offset) {
        return _bitcpy_same_offset(dst, src, dst_offset, bitlen);
    } else {
        return _bitcpy_not_same_offset(dst,
                                       dst_offset,
                                       src,
                                       src_offset,
                                       bitlen);
    }
}
