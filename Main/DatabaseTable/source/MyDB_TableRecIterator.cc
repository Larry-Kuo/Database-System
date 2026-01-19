#ifndef TABLE_REC_ITER_C
#define TABLE_REC_ITER_C

#include "MyDB_TableRecIterator.h"

using namespace std;
void MyDB_TableRecIterator ::getNext() { this->currentPageItrPtr->getNext(); };

bool MyDB_TableRecIterator ::hasNext() {
  while (this->current <= (int)this->tableReaderWriter.getNumPages() - 1) {
    if (this->currentPageItrPtr->hasNext()) {
      return true;
    } else {
      ++this->current;
      if (this->current > (int)this->tableReaderWriter.getNumPages() - 1)
        break;
      this->currentPageItrPtr =
          this->tableReaderWriter[this->current].getIterator(this->recordPtr);
    }
  }
  return false;
};

MyDB_RecordPtr MyDB_TableRecIterator ::getRecord() { return this->recordPtr; };

MyDB_TableRecIterator ::~MyDB_TableRecIterator(){};

#endif