#ifndef STORAGE_LEVELDB_INCLUDE_OPTIONS_H_
#define STORAGE_LEVELDB_INCLUDE_OPTIONS_H_

#include <stddef.h>

#include "leveldb/export.h"
namespace leveldb {
    // 此处不用include引入而直接声明，为前向声明，可以减少大量的.h文件的include，减小编译器的工作量。
    // 因为编译器会扫描符号表，也会找到该类的定义。
    class Cache;
    class Comparator;
    class Env;
    class Logger;
    class Snapshot;
    // 此处默认为Snappy压缩，这种压缩方式追求的是速度和稳定性，而不是最大的压缩率。
    enum CompressionType {
        kNoCompression = 0x0,
        kSnappyCompression = 0x1
    };

    struct Options {
        const Comparator* comparator;
        bool create_if_missing;
        bool error_if_exists;
        // 是否做严格的检查，如果发现错误则退出。如果开启，有可能因为一个入口崩溃而导致所有入口不可见。
        bool paranoid_check;

        Env *env;
        Logger* info_log;
        // default : 4M, memtable 的最大size
        size_t write_buffer_size;

        // db 中需要打开的最大文件数，包括sstable
        int max_open_files;

        // Control over blocks (user data is stored in a set of blocks, and
        // a block is the unit of reading from disk).
        //
        // If non-NULL, use the specified cache for blocks.
        // If NULL, leveldb will automatically create and use an 8MB internal cache.
        // Default: NULL
        Cache* block_cache;

         // Approximate size of user data packed per block.  Note that the
        // block size specified here corresponds to uncompressed data.  The
        // actual size of the unit read from disk may be smaller if
        // compression is enabled.  This parameter can be changed dynamically.
        //
        // Default: 4K
        // 即sstable 中的block的size
        size_t block_size;

        // Number of keys between restart points for delta encoding of keys.
        // This parameter can be changed dynamically.  Most clients should
        // leave this parameter alone.
        //
        // Default: 16
        // 即前缀压缩的两个重启点之间的距离
        int block_restart_interval;

        // Compress blocks using the specified compressino algorithm. This
        // parameter can be changed dynamically.
        //
        // Default: kSnappyCompression, which give lightweight but fast 
        // compression.
        //
        // Typical speeds of kSnappyCompression on an Inter(R) Core(TM)2 2.4GHz:
        //  ~200-500MB/s compression
        //  ~400-800MB/s decompression
        // Note that these speeds are significantly faster than most persistent
        // storage speeds,  and therefore it is typically never worth switching
        // to kNoCompression. Even if the input data is imcompressible,the
        // kSnappyCompression implementation will efficiently detect that and 
        // will switch to uncompressed mode.
        CompressionType compression;
        // if non-NULL, use the specified filter to reduce disk reads.
        // Many applications will benifit from passing the result of
        // NewBloomFilterPolicy() here.
        //
        // Default: NULL
        const FilterPolicy* filter_policy;
        Options();
    };

    struct ReadOptions {
        // 是否对读到的block 做校验
        bool verify_checksums;
        // should the data read for this iteration be cached in memory?
        // Callers may whith to set this field to false for bulk scans.
        // Default: true
        // 即是否对读到的block 加cache
        bool fill_cache;

        // If "snapshot" is non-NULL, read as of the supplied snapshot 
        // (which must belong to the DB that is being read and which must
        // not have been released). If "snapshot" is NULL, use an implicit
        // snapshot of the state at the beginning of this read operation.
        // 
        // Default: NULL
        // 指定读取的snapshot
        const Snapshot* snapshot;
         ReadOptions()
                : verify_checksums(false),
                fill_cache(true),
                snapshot(NULL){ }
    };

    struct WriteOptions {
        // If true, the write will be flushed from the operating system
        // buffer cache (by calling WritableFile::Sync()) before the write
        // is considered complete. If this flag is true, writes will be 
        // slower.
        //
        // If this is false, and the machine crashed, some recent 
        // writes may be lost. Note that if it is just the process that
        // crashes(i.e., the machine does not reboot), no writes will be
        // lost even if sync == false.
        // 
        // Default: false
        // 是否及时将内存中数据写入磁盘，即DIO
        bool sync;
        WriteOptions()
                : sync(false) { }
    };
}
#endif

