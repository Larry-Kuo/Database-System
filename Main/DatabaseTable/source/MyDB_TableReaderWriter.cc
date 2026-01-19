#ifndef TABLE_RW_C
#define TABLE_RW_C

#include "MyDB_TableReaderWriter.h"
#include "MyDB_PageReaderWriter.h"
#include "MyDB_TableRecIterator.h"
#include "MyDB_TableRecIteratorAlt.h"
#include <fstream>

using namespace std;

MyDB_TableReaderWriter ::MyDB_TableReaderWriter(
    MyDB_TablePtr forMe, MyDB_BufferManagerPtr myBuffer) {
  this->table = forMe;
  this->buffer = myBuffer;
  if (this->table->lastPage() == -1) {
    this->table->setLastPage(0);
    this->pageReaderWriter =
        make_shared<MyDB_PageReaderWriter>(*this, this->table->lastPage());
    this->pageReaderWriter->clear();
  } else {
    this->pageReaderWriter =
        make_shared<MyDB_PageReaderWriter>(*this, this->table->lastPage());
  }
}

MyDB_PageReaderWriter MyDB_TableReaderWriter ::operator[](size_t i) {
  while (i > (size_t)table->lastPage()) {
    table->setLastPage(table->lastPage() + 1);
    pageReaderWriter =
        make_shared<MyDB_PageReaderWriter>(*this, table->lastPage());
    pageReaderWriter->clear();
  }
  return MyDB_PageReaderWriter(*this, i);
}

MyDB_RecordPtr MyDB_TableReaderWriter ::getEmptyRecord() {
  return make_shared<MyDB_Record>(this->table->getSchema());
}

MyDB_PageReaderWriter MyDB_TableReaderWriter ::last() {
  return MyDB_PageReaderWriter(*this, this->table->lastPage());
}

int MyDB_TableReaderWriter ::getNumPages() { return table->lastPage() + 1; }

void MyDB_TableReaderWriter ::append(MyDB_RecordPtr newRecord) {
  if (!this->pageReaderWriter->append(newRecord)) {
    this->table->setLastPage(this->table->lastPage() + 1);
    this->pageReaderWriter =
        make_shared<MyDB_PageReaderWriter>(*this, this->table->lastPage());
    pageReaderWriter->clear();
    pageReaderWriter->append(newRecord);
  }
}

void MyDB_TableReaderWriter ::loadFromTextFile(string fileName) {
  this->table->setLastPage(0);
  this->pageReaderWriter =
      make_shared<MyDB_PageReaderWriter>(*this, this->table->lastPage());
  this->pageReaderWriter->clear();

  string line;
  ifstream file(fileName);
  if (file.is_open()) {
    MyDB_RecordPtr tempRec = getEmptyRecord();
    while (getline(file, line)) {
      tempRec->fromString(line);
      this->append(tempRec);
    }
    file.close();
  }
}

MyDB_RecordIteratorPtr
MyDB_TableReaderWriter ::getIterator(MyDB_RecordPtr iterateIntoMe) {
  return make_shared<MyDB_TableRecIterator>(*this, iterateIntoMe);
}

MyDB_RecordIteratorAltPtr MyDB_TableReaderWriter ::getIteratorAlt() {
  return make_shared<MyDB_TableRecIteratorAlt>(*this, table);
}

MyDB_RecordIteratorAltPtr
MyDB_TableReaderWriter ::getIteratorAlt(int lowPage, int highPage) {
  return make_shared<MyDB_TableRecIteratorAlt>(*this, table, lowPage, highPage);
}

MyDB_PageReaderWriter MyDB_TableReaderWriter ::getPinned(size_t i) {
  return MyDB_PageReaderWriter(true, *this, i);
}

void MyDB_TableReaderWriter ::writeIntoTextFile(string fileName) {
  ofstream output;
  output.open(fileName);
  MyDB_RecordPtr tempRec = getEmptyRecord();
  MyDB_RecordIteratorPtr myIter = getIterator(tempRec);
  while (myIter->hasNext()) {
    myIter->getNext();
    output << tempRec << "\n";
  }
  output.close();
}

MyDB_TablePtr MyDB_TableReaderWriter ::getTable() { return this->table; }

MyDB_BufferManagerPtr MyDB_TableReaderWriter ::getBufferMgr() {
  return this->buffer;
}

#endif
