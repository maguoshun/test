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
// 把uint32_t编码压缩存储到string* dst中
void PutVarint32(std::string* dst, uint32_t v) {
    char buf[5];
    char* ptr = EncodeVarint32(buf, v);
    dst->append(buf, ptr - buf);    // 压缩的过程中，会把ptr指针不断向右移动，所以压缩完成后ptr指针指向存储完所有内容的位置
                                    // 减去buf的起始位置，就是存储的内容的长度，这样才是只存储了压缩后的内容，才节省了空间。
}

char* EncodeVarint64(char* dst, uint64_t v) {
    static const int B = 128;
    uint8_t* ptr = reinterpret_cast<uint8_t*>(dst);
    while (v >= B) {
        *(ptr++) = v | B;
        v >>= 7;
    }
    *(ptr++) = static_cast<uint8_t>(v);
    return reinterpret_cast<char*>(ptr);
}

void PutVarint64(std::string* dst, uint64_t v) {
    char buf[10];
    char* ptr = EncodeVarint64(buf, v);
    dst->append(buf, ptr - buf);
}
// 先存size,再存data，即 len(str) + str的形式
void PutLengthPrefixedSlice(std::string* dst, const Slice& value) {
    PutVarint32(dst, value.size());
    dst->append(value.data(), value.size());
}
// 编码后的uint64_t的真实长度
int VarintLength(uint64_t v) {
    int len = 1;
    while (v >= 128) {
        v >>= 7;
        len++;
    }
    return len;
}

const char* GetVarint32PtrFallback(const char* p, const char* limit,
                                   uint32_t* value) {
    uint32_t result = 0;
    for (uint32_t shift = 0; shift <= 28 && p < limit; shift += 7) {
        uint32_t byte = *(reinterpret_cast<const uint8_t*>(p));
        p++;
        if (byte & 128) {
            // More bytes are present
            result |= ((byte & 127) << shift);
        } else {
            result |= (byte << shift);
            *value = result;
            return reinterpret_cast<const char*>(p);
        }
    }
    return nullptr;
}
// 把input的前面的空字符去掉
bool GetVarint32(Slice* input, uint32_t* value) {
    const char* p = input->data();
    const char* limit = p + input->size();
    const char* q = GetVarint32Ptr(p, limit, value);
    if (q == nullptr) {
        return false;
    } else {
        *input = Slice(q, limit - q);
        return true;
    }
}

const char* GetVarint64Ptr(const char* p, const char* limit, uint64_t* value) {
    uint64_t result = 0;
    for (uint32_t shift = 0; shift <= 63 && p < limit; shift += 7) {
        uint64_t byte = *(reinterpret_cast<const uint8_t*>(p));
        p++;
        if (byte & 128) {
            // More bytes are present
            result |= ((byte & 127) << shift);
        } else {
            result |= (byte << shift);
            *value = result;
            return reinterpret_cast<const char*>(p);
        }
  }
  return nullptr;
}

bool GetVarint64(Slice* input, uint64_t* value) {
    const char* p = input->data();
    const char* limit = p + input->size();
    const char* q = GetVarint64Ptr(p, limit, value);
    if (q == nullptr) {
        return false;
    } else {
        *input = Slice(q, limit - q);
        return true;
    }
}

const char* GetLengthPrefixedSlice(const char* p, const char* limit,
                                   Slice* result) {
    uint32_t len;
    p = GetVarint32Ptr(p, limit, &len);
    if (p == nullptr) return nullptr;
    if (p + len > limit) return nullptr;
    *result = Slice(p, len);
    return p + len;
}

bool GetLengthPrefixedSlice(Slice* input, Slice* result) {
    uint32_t len;
    if (GetVarint32(input, &len) && input->size() >= len) {
        *result = Slice(input->data(), len);
        input->remove_prefix(len);
        return true;
    } else {
        return false;
    }
}
} // namespace leveldb