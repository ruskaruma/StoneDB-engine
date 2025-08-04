#include "../src/PageManager.h"
#include <cassert>
#include <iostream>

int main() {
    PageManager pm;
    assert(pm.page_count() == 0);

    pm.allocate_page();
    pm.allocate_page();
    assert(pm.page_count() == 2);

    std::cout << "All tests passed for PageManager!" << std::endl;
    return 0;
}
