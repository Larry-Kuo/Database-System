#ifndef TABLE_RW_H
#define TABLE_RW_H

#include "MyDB_BufferManager.h"
#include "MyDB_Record.h"
#include "MyDB_RecordIterator.h"
#include "MyDB_RecordIteratorAlt.h"
#include "MyDB_Table.h"
#include <memory>
#include <string>
#include <vector>

using namespace std;

class MyDB_PageReaderWriter;
class MyDB_TableReaderWriter;
typedef shared_ptr<MyDB_TableReaderWriter> MyDB_TableReaderWriterPtr;

class MyDB_TableReaderWriter {

public:
  // ANY OTHER METHODS YOU WANT HERE
  MyDB_TableReaderWriter(MyDB_TablePtr forMe, MyDB_BufferManagerPtr myBuffer);

  // access the i^th page in this file
  MyDB_PageReaderWriter operator[](size_t i);

  // gets an empty record that can be used to write into the table
  MyDB_RecordPtr getEmptyRecord();

  // access the last page in the file
  MyDB_PageReaderWriter last();

  // appends a record to the file
  void append(MyDB_RecordPtr appendMe);

  // load from a text file
  void loadFromTextFile(string fileName);

  // write to a text file
  void writeIntoTextFile(string fileName);

  // return an itrator over this table... each time returnVal->next () is
  // called, the resulting record will be placed into the record pointed to
  // by iterateIntoMe
  MyDB_RecordIteratorPtr getIterator(MyDB_RecordPtr iterateIntoMe);

  size_t pageNum();

  void clearAllPages();

  MyDB_TablePtr getTable();
  MyDB_BufferManagerPtr getBufferMgr();

  // gets the number of pages
  int getNumPages();

  // gets an instance of an alternate iterator over the table
  MyDB_RecordIteratorAltPtr getIteratorAlt();

  // gets an instance of an alternate iterator over the table for a range of
  // pages
  MyDB_RecordIteratorAltPtr getIteratorAlt(int lowPage, int highPage);

  // gets a pinned page
  MyDB_PageReaderWriter getPinned(size_t i);

private:
  MyDB_TablePtr table;
  MyDB_BufferManagerPtr buffer;
  //
  // vector<MyDB_PageReaderWriter*> pages;
  shared_ptr<MyDB_PageReaderWriter> pageReaderWriter;
  vector<shared_ptr<MyDB_PageReaderWriter>> pageCache;
  // ANYTHING YOU NEED HERE
};

#endif
