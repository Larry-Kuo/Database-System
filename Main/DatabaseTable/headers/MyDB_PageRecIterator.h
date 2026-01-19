#ifndef PAGE_REC_ITER_H
#define PAGE_REC_ITER_H

#include "MyDB_PageHandle.h"
#include "MyDB_Record.h"
#include "MyDB_RecordIterator.h"

class MyDB_PageRecIterator : public MyDB_RecordIterator {
private:
  MyDB_RecordPtr recordPtr;
  MyDB_PageHandle pageHandle;
  int bytesConsumed;

public:
  MyDB_PageRecIterator(MyDB_PageHandle hd, MyDB_RecordPtr r);

  ~MyDB_PageRecIterator() override;

  void getNext() override;

  bool hasNext() override;
};

typedef shared_ptr<MyDB_PageRecIterator> MyDB_PageRecIteratorPtr;

#endif