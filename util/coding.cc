// Copyright (c) 2011 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#include "util/coding.h"

namespace leveldb {
// uint32_t 转换为string*
void PutFixed32(std::string* dst, uint32_t value) {
    char buf[sizeof(value)];
    EncodeFixed32(buf, value);
    dst->append(buf, sizeof(buf));
}
// uint64_t 转换为string*
void PutFixed64(std::string* dst, uint64_t value) {
    char buf[sizeof(value)];
    EncodeFixed32(buf, value);
    dst->append(buf, sizeof(buf));
}
// 把uint32_t编码，压缩存储
/*一个字节是8位，大神将一个字节分为两个部分，最高位和低7位，低7位用来存数据，最高位用来表示有没有结束，举个栗子
   值为300的int变量，二进制为100101100，在内存中实际只用两个字节，浪费两个字节
   在内存中是这么存的           |0|0|0|0|0|0|0|1| 0|0|1|0|1|1|0|0|
   而编码后在内存中是这么存的    |0|0|0|0|0|0|1|0| 1|0|1|0|1|1|0|0|
   区别在哪儿呢，就是在第8位中插入一位，设置为1，为什么要怎么做
   我们上面有讲，一个字节中7位用来存数据，1位表示是否结束，当我们读到一个字节时，判断最高位是否为1
   如果是1，说明编码后的int还没有结束，需要继续读下一个字节，直到读到字节的最高位为0为止
*/
char* EncodeVarint32(char* dst, uint32_t v) {
    uint8_t* ptr = reinterpret_cast<uint8_t*>(dst);
    static const int B = 128;
    // v小于128，说明7bit能存储，那么一个字节就能搞定，将ptr赋值为v，返回ptr的下一个地址
    if (v < (1 << 7)) {
        *(ptr++) = v;
    } else if (v < (1 << 14)) {        // 如果v小于(1<<14)，说明要两个字节来存储
        *(ptr++) = v | B;              // v|B是将v的第8位设置为1，然后赋值给ptr，这里只会赋值一个字节
        *(ptr++) = v >> 7;             // 将v右移7位，实际上是为了得到第二个字节，下面的以此类推
    } else if (v < (1 << 21)) {
        *(ptr++) = v | B;
        *(ptr++) = (v >> 7) | B;
        *(ptr++) = v >> 14;
    } else if (v < (1 << 28)) {
        *(ptr++) = v | B;
        *(ptr++) = (v >> 7) | B;
        *(ptr++) = (v >> 14) | B;
        *(ptr++) = v >> 21;
    } else {
        *(ptr++) = v | B;
        *(ptr++) = (v >> 7) | B;
        *(ptr++) = (v >> 14) | B;
        *(ptr++) = (v >> 21) | B;
        *(ptr++) = v >> 28;
    }
    return reinterpret_cast<char*>(ptr);
}

void PutVarint32(std::string* dst, uint32_t v) {
    char buf[5];
    char* ptr = EncodeVarint32(buf, v);
    dst->append(buf, ptr - buf);    // 压缩的过程中，会把ptr指针不断向右移动，所以压缩完成后ptr指针指向存储完所有内容的位置
                                    // 减去buf的起始位置，就是存储的内容的长度
}

}