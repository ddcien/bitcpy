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
        0x80U,
        0xc0U,
        0xe0U,
        0xf0U,
        0xf8U,
        0xfcU,
        0xfeU,
        0xffU,
    };
    return _mask_s[bitlen - 1] >> offset;
}

static __always_inline void _bitcpy_same_offset(uint8_t* const dst,
                                                uint8_t const* const src,
                                                size_t offset,
                                                size_t bitlen) {
    size_t idx = 0;
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
            idx = 1;
        }
    }

    let bytelen = bitlen >> 3;
    if (bytelen) {
        memcpy(dst + idx, src + idx, bytelen);
        idx += bytelen;
    }

    bitlen &= 7U;
    if (bitlen) {
        let src_mask = _get_uint8_mask(0, bitlen);
        dst[idx] = (dst[idx] & ~src_mask) | (src[idx] & src_mask);
    }
}

static __always_inline void _bitcpy_dst_offset_0(uint8_t* const dst,
                                                 uint8_t const* const src,
                                                 size_t const src_offset,
                                                 size_t const bitlen) {
    size_t idx = 0;
    let u01_len = bitlen & 7U;
    let u08_len = (bitlen >> 3) & 7U;
    let u64_len = bitlen >> 6;

    // clang-format off
    for (size_t i = u64_len; i--; idx += 8) {
        dst[idx + 0] = (src[idx + 0] << src_offset) | (src[idx + 1] >> (8 - src_offset));
        dst[idx + 1] = (src[idx + 1] << src_offset) | (src[idx + 2] >> (8 - src_offset));
        dst[idx + 2] = (src[idx + 2] << src_offset) | (src[idx + 3] >> (8 - src_offset));
        dst[idx + 3] = (src[idx + 3] << src_offset) | (src[idx + 4] >> (8 - src_offset));
        dst[idx + 4] = (src[idx + 4] << src_offset) | (src[idx + 5] >> (8 - src_offset));
        dst[idx + 5] = (src[idx + 5] << src_offset) | (src[idx + 6] >> (8 - src_offset));
        dst[idx + 6] = (src[idx + 6] << src_offset) | (src[idx + 7] >> (8 - src_offset));
        dst[idx + 7] = (src[idx + 7] << src_offset) | (src[idx + 8] >> (8 - src_offset));
    }

    switch (u08_len) {
        case 7: dst[idx + 6] = (src[idx + 6] << src_offset) | (src[idx + 7] >> (8 - src_offset)); __attribute__((fallthrough));
        case 6: dst[idx + 5] = (src[idx + 5] << src_offset) | (src[idx + 6] >> (8 - src_offset)); __attribute__((fallthrough));
        case 5: dst[idx + 4] = (src[idx + 4] << src_offset) | (src[idx + 5] >> (8 - src_offset)); __attribute__((fallthrough));
        case 4: dst[idx + 3] = (src[idx + 3] << src_offset) | (src[idx + 4] >> (8 - src_offset)); __attribute__((fallthrough));
        case 3: dst[idx + 2] = (src[idx + 2] << src_offset) | (src[idx + 3] >> (8 - src_offset)); __attribute__((fallthrough));
        case 2: dst[idx + 1] = (src[idx + 1] << src_offset) | (src[idx + 2] >> (8 - src_offset)); __attribute__((fallthrough));
        case 1: dst[idx + 0] = (src[idx + 0] << src_offset) | (src[idx + 1] >> (8 - src_offset)); __attribute__((fallthrough));
        case 0: break;
    }
    // clang-format on
    if (u01_len) {
        let src_mask = _get_uint8_mask(0, u01_len);
        idx += u08_len;
        dst[idx] =
            (dst[idx] & ~src_mask) |
            (((src[idx] << src_offset) | (src[idx + 1] >> (8 - src_offset))) &
             src_mask);
    }
}

static __always_inline void _bitcpy_not_same_offset(uint8_t* dst,
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

        dst += 1;
    }
    return _bitcpy_dst_offset_0(dst, src, src_offset, bitlen);
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
