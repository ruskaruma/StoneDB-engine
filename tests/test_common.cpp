#include"common.hpp"
#include<cassert>
int main()
{
    stonedb::Record rec("test", "value");
    assert(rec.key == "test");
    assert(rec.value == "value");
    assert(!rec.isDeleted);
    stonedb::Page page(1);
    assert(page.pageId == 1);
    assert(page.data.size() == stonedb::PAGE_SIZE);
    stonedb::log("common tests passed");
    return 0;
}
