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
    assert(bitlen < 64);
    if (bitlen >= 32) {
        dst[0] = (src[0] << src_offset) | (src[1] >> (8 - src_offset));
        dst[1] = (src[1] << src_offset) | (src[2] >> (8 - src_offset));
        dst[2] = (src[2] << src_offset) | (src[3] >> (8 - src_offset));
        dst[3] = (src[3] << src_offset) | (src[4] >> (8 - src_offset));
        bitlen -= 32;
        dst += 4;
        src += 4;
    }
    assert(bitlen < 32);
    if (bitlen >= 16) {
        dst[0] = (src[0] << src_offset) | (src[1] >> (8 - src_offset));
        dst[1] = (src[1] << src_offset) | (src[2] >> (8 - src_offset));
        bitlen -= 16;
        dst += 2;
        src += 2;
    }
    assert(bitlen < 16);
    if (bitlen >= 8) {
        dst[0] = (src[0] << src_offset) | (src[1] >> (8 - src_offset));
        bitlen -= 8;
        dst += 1;
        src += 1;
    }
    assert(bitlen < 8);
    if (bitlen) {
        let src_mask = _get_uint8_mask(0, bitlen);
        dst[0] = (dst[0] & ~src_mask) |
                 (((src[0] << src_offset) | (src[1] >> (8 - src_offset))) &
                  src_mask);
    }
}

void bitcpy(void* dst,
            size_t dst_offset,
            void const* src,
            size_t src_offset,
            size_t bitlen) {
    dst += dst_offset >> 3;
    src += src_offset >> 3;
    dst_offset &= 7U;
    src_offset &= 7U;

    if (dst_offset == src_offset) {
        return _bitcpy_same_offset(dst, src, src_offset, bitlen);
    }
    _bitcpy_not_same_offset(dst, dst_offset, src, src_offset, bitlen);
}
