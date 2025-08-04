#ifndef PAGEMANAGER_H
#define PAGEMANAGER_H

class PageManager {
public:
    PageManager();
    ~PageManager();

    int page_count() const;
    void allocate_page();
private:
    int count;
};

#endif // PAGEMANAGER_H
