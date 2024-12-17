#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

static unsigned char const ByteMask[] =
    {0xff, 0x7f, 0x3f, 0x1f, 0x0f, 0x07, 0x03, 0x01, 0x00};

static unsigned int const Mask[] = {
    0xffffffff, 0xffffff7f, 0xffffff3f, 0xffffff1f, 0xffffff0f, 0xffffff07,
    0xffffff03, 0xffffff01, 0xffffff00, 0xffff7f00, 0xffff3f00, 0xffff1f00,
    0xffff0f00, 0xffff0700, 0xffff0300, 0xffff0100, 0xffff0000, 0xff7f0000,
    0xff3f0000, 0xff1f0000, 0xff0f0000, 0xff070000, 0xff030000, 0xff010000,
    0xff000000, 0x7f000000, 0x3f000000, 0x1f000000, 0x0f000000, 0x07000000,
    0x03000000, 0x01000000, 0x00000000};

static void memcpy_of_same_offset_in8bit(uint8_t const* src,
                                         unsigned int src_begin_bit,
                                         uint8_t* dest,
                                         unsigned int dest_begin_byte,
                                         unsigned int bitslen) {
    unsigned char* d = dest + dest_begin_byte;
    unsigned char const* s = src + src_begin_bit / 8;
    unsigned char BeginMask, EndMask;
    if (bitslen + src_begin_bit % 8 <= 8) {
        BeginMask = ByteMask[src_begin_bit % 8];
        EndMask = (unsigned char)~(ByteMask[src_begin_bit % 8 + bitslen]);
        unsigned char CurMask = (unsigned char)(EndMask & BeginMask);
        *d = (unsigned char)(((*d) & (~CurMask)) | ((*s) & CurMask));
        return;
    }
    if (src_begin_bit % 8 != 0) {
        BeginMask = ByteMask[src_begin_bit % 8];
        *d = (unsigned char)(((*d) & (~BeginMask)) | ((*s) & BeginMask));
        d++;
        s++;
        bitslen -= 8 - src_begin_bit % 8;
    }
    unsigned char* dt = d + bitslen / 8;
    while (d < dt) {
        *d = *s;
        d++;
        s++;
    }
    if (bitslen % 8 != 0) {
        EndMask = (unsigned char)(~(ByteMask[bitslen % 8]));
        *d = (unsigned char)(((*d) & (~EndMask)) | ((*s) & EndMask));
    }
}

static void memcpy_of_same_offset_in32bit(uint8_t const* src,
                                          unsigned int src_begin_bit,
                                          uint8_t* dest,
                                          unsigned int dest_begin_int,
                                          unsigned int bitslen) {
    if (bitslen + src_begin_bit % 32 <= 32) {
        unsigned int BeginMask = Mask[src_begin_bit % 32];
        unsigned int EndMask = ~(Mask[src_begin_bit % 32 + bitslen]);
        unsigned int CurMask = EndMask & BeginMask;
        unsigned char const* sh = src;
        unsigned char* dh = dest;
        {
            unsigned int* d = (unsigned int*)dh + dest_begin_int;
            unsigned int* s = (unsigned int*)sh + src_begin_bit / 32;
            *d = ((*d) & (~CurMask)) | ((*s) & CurMask);
        }
        return;
    }
    unsigned char const* sh = src;
    unsigned char* dh = dest;
    {
        unsigned int* d = (unsigned int*)dh + dest_begin_int;
        unsigned int* s = (unsigned int*)sh + src_begin_bit / 32;
        if (src_begin_bit % 32 != 0) {
            unsigned int BeginMask = Mask[src_begin_bit % 32];
            *d = ((*d) & (~BeginMask)) | ((*s) & BeginMask);
            d++;
            s++;
            bitslen -= 32 - src_begin_bit % 32;
        }
        unsigned int* dt = d + bitslen / 32;
        while (d < dt) {
            *d = *s;
            d++;
            s++;
        }
        if (bitslen % 32 != 0) {
            unsigned int EndMask = ~(Mask[bitslen % 32]);
            *d = ((*d) & (~EndMask)) | ((*s) & EndMask);
        }
    }
}

void bitcpy(uint8_t* dst,
            size_t dst_begin_bit,
            uint8_t const* src,
            size_t src_begin_bit,
            size_t bitslen) {

    if (dst_begin_bit % 32 == src_begin_bit % 32) {
        if (src_begin_bit % 32 == 0) {
        } else {
            memcpy_of_same_offset_in32bit(src,
                                          src_begin_bit,
                                          dst,
                                          dst_begin_bit / 32,
                                          bitslen);
        }
        return;
    } else  // Richard
    {
        assert(dst_begin_bit != src_begin_bit);

        unsigned char const* sh = src;
        unsigned char* dh = dst;
        {
            unsigned char const* s = sh + src_begin_bit / 8;
            unsigned char* d = dh + dst_begin_bit / 8;
            unsigned int uSrcBeginBit = src_begin_bit % 8;
            unsigned int uDstBeginBit = dst_begin_bit % 8;
            if (uSrcBeginBit == uDstBeginBit) {
                if (uSrcBeginBit == 0) {
                    int byteLen = (int)bitslen / 8;
                    memcpy(d, s, byteLen);
                    d += byteLen;
                    s += byteLen;

                    if (bitslen % 8 != 0) {
                        unsigned char EndMask = ByteMask[bitslen % 8];
                        *d = (unsigned char)((*d & EndMask) | (*s & ~EndMask));
                    }
                    return;
                } else {
                    memcpy_of_same_offset_in8bit(s,
                                                 uSrcBeginBit,
                                                 d,
                                                 0,
                                                 bitslen);
                    return;
                }
            } else if (uSrcBeginBit > uDstBeginBit) {
                int uShiftLBits = (int)(uSrcBeginBit - uDstBeginBit);

                if (bitslen + uDstBeginBit <= 8) {
                    unsigned char BeginMask = ByteMask[uDstBeginBit];
                    unsigned char EndMask =
                        (unsigned char)~(ByteMask[uDstBeginBit + bitslen]);
                    unsigned char CurMask =
                        (unsigned char)(EndMask & BeginMask);
                    *d = (unsigned char)(((*d) & (~CurMask)) |
                                         ((((*s) << uShiftLBits) |
                                           ((*(s + 1)) >> (8 - uShiftLBits))) &
                                          CurMask));
                } else {
                    int leftBitsLen = (int)bitslen;
                    // if(uDstBeginBit!=0)
                    {
                        unsigned char BeginMask = ByteMask[uDstBeginBit];
                        *d = (unsigned char)((*d & ~BeginMask) |
                                             ((((*s) << uShiftLBits) |
                                               ((*(s + 1)) >>
                                                (8 - uShiftLBits))) &
                                              BeginMask));
                        d++;
                        s++;
                        leftBitsLen -= (int)(8 - uDstBeginBit);
                    }
                    for (int i = 0; i < leftBitsLen / 8; i++) {
                        *d = (unsigned char)(((*s) << uShiftLBits) |
                                             ((*(s + 1)) >> (8 - uShiftLBits)));
                        d++;
                        s++;
                    }
                    if (leftBitsLen % 8 != 0) {
                        unsigned char EndMask = ByteMask[leftBitsLen % 8];
                        *d = (unsigned char)((*d & EndMask) |
                                             ((((*s) << uShiftLBits) |
                                               ((*(s + 1)) >>
                                                (8 - uShiftLBits))) &
                                              ~EndMask));
                    }
                }
            } else  // uSrcBeginBit < uDstBeginBit
            {
                int uShiftRBits = (int)(uDstBeginBit - uSrcBeginBit);

                if (bitslen + uDstBeginBit <= 8) {
                    unsigned char BeginMask = ByteMask[uDstBeginBit];
                    unsigned char EndMask =
                        (unsigned char)~(ByteMask[uDstBeginBit + bitslen]);
                    unsigned char CurMask =
                        (unsigned char)(EndMask & BeginMask);
                    *d = (unsigned char)(((*d) & (~CurMask)) |
                                         (((*s) >> uShiftRBits) & CurMask));
                } else {
                    int leftBitsLen = (int)(bitslen);
                    unsigned char bReMand = 0;
                    // if(uDstBeginBit !=0)
                    {
                        unsigned char BeginMask = ByteMask[uDstBeginBit];
                        *d = (unsigned char)((*d & ~BeginMask) |
                                             (((*s) >> uShiftRBits) &
                                              BeginMask));
                        bReMand = (unsigned char)((*s) << (8 - uShiftRBits));
                        d++;
                        s++;
                        leftBitsLen -= (int)(8 - uDstBeginBit);
                    }
                    for (int i = 0; i < leftBitsLen / 8; i++) {
                        *d++ = (unsigned char)(((*s) >> uShiftRBits) | bReMand);
                        bReMand = (unsigned char)((*s++) << (8 - uShiftRBits));
                    }
                    if (leftBitsLen % 8 != 0) {
                        unsigned char EndMask = ByteMask[leftBitsLen % 8];
                        *d = (unsigned char)((*d & EndMask) |
                                             (((*s >> uShiftRBits) | bReMand) &
                                              ~EndMask));
                    }
                }
            }
        }
    }
}
