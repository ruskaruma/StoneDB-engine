#include "PageManager.h"
#include <cassert>
#include <cstring>
#include <vector>
#include <iostream>

using namespace sdb;

int main(){
    const char* db="test_db.sdb";

    //created fresh
    {
        PageManager pm;
        pm.open(db,true,4096);
        assert(pm.pagesz()==4096);
        assert(pm.pagecount()==1); // header only
        pm.sync();
    }

    uint64_t a,b,c;
    //alloc & write patterns
    {
        PageManager pm;
        pm.open(db,false);
        a=pm.alloc(); b=pm.alloc(); c=pm.alloc();
        assert(a==1 && b==2 && c==3);

        std::vector<char> buf(pm.pagesz());
        std::memset(buf.data(),0xAA,buf.size()); pm.writep(a,buf.data(),buf.size());
        std::memset(buf.data(),0xBB,buf.size()); pm.writep(b,buf.data(),buf.size());
        std::memset(buf.data(),0xCC,buf.size()); pm.writep(c,buf.data(),buf.size());
        pm.sync();
    }

    //read back verify
    {
        PageManager pm;
        pm.open(db,false);
        std::vector<char> got(pm.pagesz()), exp(pm.pagesz());

        pm.readp(a,got.data(),got.size());
        std::memset(exp.data(),0xAA,exp.size());
        assert(std::memcmp(got.data(),exp.data(),got.size())==0);

        pm.readp(b,got.data(),got.size());
        std::memset(exp.data(),0xBB,exp.size());
        assert(std::memcmp(got.data(),exp.data(),got.size())==0);

        pm.readp(c,got.data(),got.size());
        std::memset(exp.data(),0xCC,exp.size());
        assert(std::memcmp(got.data(),exp.data(),got.size())==0);
        pm.sync();
    }

    //free & reuse
    {
        PageManager pm;
        pm.open(db,false);
        pm.freep(b);
        auto d=pm.alloc();
        assert(d==b);
        pm.sync();
    }
    std::cout<<"OK\n";
    return 0;
}
