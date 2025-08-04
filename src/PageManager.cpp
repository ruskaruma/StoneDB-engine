#include "PageManager.h"

PageManager::PageManager() : count(0) {}

PageManager::~PageManager() {}

int PageManager::page_count() const {
    return count;
}

void PageManager::allocate_page() {
    ++count;
}
