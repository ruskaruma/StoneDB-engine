#pragma once
#include <cstdint>
#include <string>
//minimal but still kinda messy
namespace sdb
{
    static const uint32_t SDB_MAGIC=0x53444231; // "SDB1"
    static const uint32_t SDB_VER=1;
    static const uint32_t SDB_DEF_PG=4096;

    struct FileHeader
    {
        uint32_t magic;
        uint32_t ver;
        uint32_t pgsz;
        uint32_t _pad;
        uint64_t pgcnt;
        int64_t  freelist;
    };

    class PageManager
    {
    public:
        PageManager():fd_(-1),path_(),hdr_{}{}
        ~PageManager();

        // if create_if_missing==false and file not proper => throw
        void open(const std::string& p,bool mk=true,uint32_t pg=SDB_DEF_PG);
        void close();

        uint64_t alloc();      // returns page id >=1
        void freep(uint64_t pid);

        void readp(uint64_t pid,void* dst,size_t nbytes);
        void writep(uint64_t pid,const void* src,size_t nbytes);

        void sync(); // fsync + header

        inline uint32_t pagesz() const { return hdr_.pgsz; }
        inline uint64_t pagecount() const { return hdr_.pgcnt; }

    private:
        int fd_;
        std::string path_;
        FileHeader hdr_;

        void _loadOrInit(bool mk,uint32_t pg);
        void _persistHdr();
        ::off_t _off(uint64_t pid) const;

        void _ensureOpen() const;
        void _checkPid(uint64_t pid) const;
    };
}
