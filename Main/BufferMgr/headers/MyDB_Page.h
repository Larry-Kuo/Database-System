#ifndef MYDB_PAGE_H
#define MYDB_PAGE_H

#include <memory> // Required for shared_ptr
#include <string>

class MyDB_BufferManager; // Forward declaration

using namespace std;

class PageBase; // Forward declaration for typedef
typedef shared_ptr<PageBase> Page;

class PageBase {
public:
  char *memory;
  std::string table;
  int pageID;
  bool pin;
  bool dirty;
  MyDB_BufferManager &bufferManager;

  ~PageBase();

  PageBase(char *address, std::string tableName, long ID, bool pinValid,
           MyDB_BufferManager &bm)
      : memory(address), table(tableName), pageID(ID), pin(pinValid),
        dirty(false), bufferManager(bm) {}
};

#endif
