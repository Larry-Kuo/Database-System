#ifndef PAGE_REC_ITER_C
#define PAGE_REC_ITER_C

#include "MyDB_PageRecIterator.h"

#define NUM_BYTES_USED                                                         \
  *((size_t *)(((char *)pageHandle->getBytes()) + sizeof(size_t)))

void MyDB_PageRecIterator ::getNext() {
  void *pos = bytesConsumed + (char *)pageHandle->getBytes();
  void *nextPos = recordPtr->fromBinary(pos);
  bytesConsumed = ((char *)nextPos) - ((char *)pageHandle->getBytes());
}

bool MyDB_PageRecIterator ::hasNext() {
  return bytesConsumed != NUM_BYTES_USED;
}

MyDB_PageRecIterator ::~MyDB_PageRecIterator() {}

MyDB_PageRecIterator ::MyDB_PageRecIterator(MyDB_PageHandle hd,
                                            MyDB_RecordPtr r) {
  pageHandle = hd;
  recordPtr = r;
  bytesConsumed = sizeof(size_t) * 2;
}
#endif