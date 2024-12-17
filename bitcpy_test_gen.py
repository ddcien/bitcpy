import os
import sys
import random


def fake_bitcpy(
    dst: str, dst_offset: int, src: str, src_offset: int, bitcnt: int
) -> str:
    return (
        dst[:dst_offset]
        + src[src_offset : src_offset + bitcnt]
        + dst[dst_offset + bitcnt :]
    )


def bar(name: str, data: str, const: bool = False) -> str:
    ret = "uint8_t {}{}[] = {{".format("const " if const else "", name)
    for i in range(0, len(data), 8):
        ret += "\n    0b" + data[i : i + 8] + ","
    ret += "\n};\n"

    return ret


def foo():
    data_len = random.randint(8, 256)
    src_data = os.urandom(data_len)
    dst_data = os.urandom(data_len)

    src = "".join("{:08b}".format(i) for i in src_data)
    dst = "".join("{:08b}".format(i) for i in dst_data)

    bits_len = data_len * 8
    src_offset = random.randint(0, min(bits_len - 1, 31))
    dst_offset = random.randint(0, min(bits_len - 1, 31))

    bitcnt_max = bits_len - max(src_offset, dst_offset)
    bitcnt = random.randint(1, bitcnt_max)

    ret = fake_bitcpy(dst, dst_offset, src, src_offset, bitcnt)

    print("\n/*")
    print(f"Copy from src {src_offset} to dest {dst_offset} cnt = {bitcnt}")
    print("src :    ", src)
    print("dest:    ", dst)
    print("{:2}:{:2}:{:2} ".format(bitcnt, dst_offset, src_offset), ret)
    print("*/")

    print("{")
    print(
        f"size_t dst_offset={dst_offset}, src_offset={src_offset}, bitcnt={bitcnt}, data_len={data_len};"
    )
    print(bar("src", src, True))
    print(bar("dst", dst, False))
    print(bar("ret", ret, True))

    print("bitcpy(dst, dst_offset, src, src_offset, bitcnt);")
    print("if(memcmp(dst, ret, data_len) != 0) {")
    print('printf("FAIL: dst_offset=%zu src_offset=%zu bitcnt=%zu data_len=%zu\\n", dst_offset, src_offset, bitcnt, data_len);')
    # print("bit_stream_print(src, data_len, src_offset, bitcnt, 1);")
    # print("bit_stream_print(dst, data_len, dst_offset, bitcnt, 0);")
    # print("bit_stream_print(ret, data_len, dst_offset, bitcnt, 0);")
    print("assert(0);")
    print("}")
    print("}")

    return src, dst, src_offset, dst_offset, bitcnt


if __name__ == "__main__":
    print("#include <assert.h>")
    print("#include <stdint.h>")
    print("#include <string.h>")
    print("#include <stdio.h>")
    print("#include <stdbool.h>")
    print(
        "extern void bitcpy(void* dst, size_t dst_offset, void const* src, size_t src_offset, size_t bitlen);"
    )
    print(
        "void bit_stream_print(void const* _stream, size_t len, size_t offset, size_t bitlen, bool idx);"
    )
    print("void bitcpy_test(void) {")

    for _ in range(int(sys.argv[1])):
        foo()

    print("}")
