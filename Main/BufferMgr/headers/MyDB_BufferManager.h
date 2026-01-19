
#ifndef BUFFER_MGR_H
#define BUFFER_MGR_H

#include <fcntl.h>
#include <functional>
#include <list>
#include <memory>
#include <string>
#include <unistd.h>
#include <unordered_map>
#include <utility>

using namespace std;

struct pair_hash {
  template <class T1, class T2> size_t operator()(const pair<T1, T2> &p) const {
    auto hash1 = std::hash<T1>{}(p.first);
    auto hash2 = std::hash<T2>{}(p.second);
    return hash1 ^ hash2;
  }
};

class MyDB_PageHandleBase;
class PageBase;
class MyDB_Table;
typedef shared_ptr<MyDB_PageHandleBase> MyDB_PageHandle;
typedef shared_ptr<PageBase> Page;
typedef shared_ptr<MyDB_Table> MyDB_TablePtr;

class MyDB_BufferManager;
typedef shared_ptr<MyDB_BufferManager> MyDB_BufferManagerPtr;

class MyDB_BufferManager {

public:
  // THESE METHODS MUST APPEAR AND THE PROTOTYPES CANNOT CHANGE!

  // gets the i^th page in the table whichTable... note that if the page
  // is currently being used (that is, the page is current buffered) a handle
  // to that already-buffered page should be returned
  MyDB_PageHandle getPage(MyDB_TablePtr whichTable, long i);

  // gets a temporary page that will no longer exist (1) after the buffer
  // manager has been destroyed, or (2) there are no more references to it
  // anywhere in the program.  Typically such a temporary page will be used as
  // buffer memory. since it is just a temp page, it is not associated with any
  // particular table
  MyDB_PageHandle getPage();

  // gets the i^th page in the table whichTable... the only difference
  // between this method and getPage (whicTable, i) is that the page will be
  // pinned in RAM; it cannot be written out to the file
  MyDB_PageHandle getPinnedPage(MyDB_TablePtr whichTable, long i);

  // gets a temporary page, like getPage (), except that this one is pinned
  MyDB_PageHandle getPinnedPage();

  // un-pins the specified page
  void unpin(MyDB_PageHandle unpinMe);

  char *retrievePage(Page page);

  void updateBufferMap(string table, int id);

  // creates an LRU buffer manager... params are as follows:
  // 1) the size of each page is pageSize
  // 2) the number of pages managed by the buffer manager is numPages;
  // 3) temporary pages are written to the file tempFile
  MyDB_BufferManager(size_t pageSize, size_t numPages, string tempFile);

  // when the buffer manager is destroyed, all of the dirty pages need to be
  // written back to disk, any necessary data needs to be written to the
  // catalog, and any temporary files need to be deleted
  ~MyDB_BufferManager();

  // FEEL FREE TO ADD ADDITIONAL PUBLIC METHODS

  // returns the page size
  size_t getPageSize();

  // returns the number of pages in the pool
  size_t getNumPages();

  // kills the indicated table, so that no pages will ever be written back to it
  // also removes the physical file from disk, and gets rid of the FD
  void killTable(MyDB_TablePtr killMe);

private:
  // YOUR STUFF HERE

  unordered_map<pair<string, int>, pair<list<Page>::iterator, Page>, pair_hash>
      bufferMap;
  char *bufferPool;
  list<Page> pageContainer;
  unordered_map<string, int> tableMap;
  size_t pageSize;
  size_t numPages;
  size_t currPage = 0;
  size_t anonymousID = 0;
  string tempFile;
};

#endif
