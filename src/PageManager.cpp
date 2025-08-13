#include "PageManager.h"

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>
#include <stdexcept>
#include <vector> // zero page buffer

namespace sdb
{
    static inline void oops(const char* w)
    {
        throw std::runtime_error(std::string(w)+": "+std::strerror(errno));
    }

    PageManager::~PageManager()
    {
        close();
    }

    void PageManager::_ensureOpen() const
    {
        if(fd_==-1) throw std::runtime_error("not open");
    }

    void PageManager::_checkPid(uint64_t pid) const
    {
        if(pid==0 || pid>=hdr_.pgcnt) throw std::runtime_error("bad page id");
    }

    ::off_t PageManager::_off(uint64_t pid) const
    {
        return (::off_t)(pid) * (::off_t)(hdr_.pgsz);
    }

    void PageManager::open(const std::string& p,bool mk,uint32_t pg)
    {
        if(fd_!=-1) close();
        path_=p;

        int fl=O_RDWR;
#ifdef O_CLOEXEC
        fl|=O_CLOEXEC;
#endif
        if(mk) fl|=O_CREAT;
        fd_=::open(path_.c_str(),fl,0644);
        if(fd_==-1) oops("open");

        try
        {
            _loadOrInit(mk,pg);
        }
        catch(...)
        {
            ::close(fd_);
            fd_=-1;
            throw;
        }
    }

    void PageManager::close()
    {
        if(fd_!=-1)
        {
            try
            {
                _persistHdr();
                ::fsync(fd_);
            }
            catch(...) {}
            ::close(fd_);
            fd_=-1;
        }
    }

    void PageManager::_loadOrInit(bool mk,uint32_t pg)
    {
        FileHeader h{};
        ssize_t r=::pread(fd_,&h,sizeof(h),0);
        if(r==(ssize_t)sizeof(h) && h.magic==SDB_MAGIC && h.ver==SDB_VER && h.pgsz>=512)
        {
            hdr_=h;
            return;
        }
        if(!mk) throw std::runtime_error("not a StonedDB file or header broken");

        uint32_t ps=pg?pg:SDB_DEF_PG;
        if(ps<4096) ps=4096;
        hdr_.magic=SDB_MAGIC;
        hdr_.ver=SDB_VER;
        hdr_.pgsz=ps;
        hdr_._pad=0;
        hdr_.pgcnt=1;
        hdr_.freelist=-1;

        std::vector<char> zero(ps,0);
        if(::pwrite(fd_,zero.data(),zero.size(),0)!=(ssize_t)zero.size()) oops("write header page");
        _persistHdr();
        if(::fsync(fd_)==-1) oops("fsync init");
    }

    void PageManager::_persistHdr()
    {
        _ensureOpen();
        if(::pwrite(fd_,&hdr_,sizeof(hdr_),0)!=(ssize_t)sizeof(hdr_)) oops("write header");
    }

    uint64_t PageManager::alloc()
    {
        _ensureOpen();
        if(hdr_.freelist!=-1)
        {
            uint64_t reused=(uint64_t)hdr_.freelist;
            int64_t nxt=-1;
            if(::pread(fd_,&nxt,sizeof(nxt),_off(reused))!=(ssize_t)sizeof(nxt)) oops("read freelist");
            hdr_.freelist=nxt;
            _persistHdr();
            return reused;
        }
        uint64_t newpid=hdr_.pgcnt;
        std::vector<char> z(hdr_.pgsz,0);
        if(::pwrite(fd_,z.data(),z.size(),_off(newpid))!=(ssize_t)z.size()) oops("append page");
        hdr_.pgcnt++;
        _persistHdr();
        return newpid;
    }

    void PageManager::freep(uint64_t pid)
    {
        _ensureOpen();
        _checkPid(pid);
        int64_t nxt=hdr_.freelist;
        if(::pwrite(fd_,&nxt,sizeof(nxt),_off(pid))!=(ssize_t)sizeof(nxt)) oops("link freelist");
        hdr_.freelist=(int64_t)pid;
        _persistHdr();
    }

    void PageManager::readp(uint64_t pid,void* dst,size_t n)
    {
        _ensureOpen();
        _checkPid(pid);
        if(n!=hdr_.pgsz) throw std::runtime_error("readp: size mismatch");
        if(::pread(fd_,dst,n,_off(pid))!=(ssize_t)n) oops("read page");
    }

    void PageManager::writep(uint64_t pid,const void* src,size_t n)
    {
        _ensureOpen();
        _checkPid(pid);
        if(n!=hdr_.pgsz) throw std::runtime_error("writep: size mismatch");
        if(::pwrite(fd_,src,n,_off(pid))!=(ssize_t)n) oops("write page");
    }

    void PageManager::sync()
    {
        _ensureOpen();
        _persistHdr();
        if(::fsync(fd_)==-1) oops("fsync");
    }

} // namespace sdb
