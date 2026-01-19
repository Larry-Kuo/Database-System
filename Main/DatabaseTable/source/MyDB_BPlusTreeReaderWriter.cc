#ifndef BPLUS_C
#define BPLUS_C

#include "MyDB_BPlusTreeReaderWriter.h"
#include "MyDB_INRecord.h"
#include "MyDB_PageListIteratorAlt.h"
#include "MyDB_PageListIteratorSelfSortingAlt.h"
#include "MyDB_PageReaderWriter.h"
#include "RecordComparator.h"
#include <iostream>
#include <queue>

MyDB_BPlusTreeReaderWriter ::MyDB_BPlusTreeReaderWriter(
    string orderOnAttName, MyDB_TablePtr forMe, MyDB_BufferManagerPtr myBuffer)
    : MyDB_TableReaderWriter(forMe, myBuffer) {

  // find the ordering attribute
  auto res = forMe->getSchema()->getAttByName(orderOnAttName);

  // remember information about the ordering attribute
  orderingAttType = res.second;
  whichAttIsOrdering = res.first;

  // set root location (page)
  getTable()->setRootLocation(getTable()->lastPage());

  // and the root location
  rootLocation = getTable()->getRootLocation();

  // new B+tree
  MyDB_INRecordPtr record = getINRecord();
  getTable()->setLastPage(getTable()->lastPage() + 1);
  record->setPtr(getTable()->lastPage());
  (*this)[record->getPtr()].clear();
  (*this)[record->getPtr()].setType(RegularPage);
  (*this)[rootLocation].clear();
  (*this)[rootLocation].setType(DirectoryPage);
  (*this)[rootLocation].append(record);
}

MyDB_RecordIteratorAltPtr
MyDB_BPlusTreeReaderWriter ::getSortedRangeIteratorAlt(MyDB_AttValPtr low,
                                                       MyDB_AttValPtr high) {
  vector<MyDB_PageReaderWriter> targetPageList;
  MyDB_PageReaderWriterPtr emptyPage =
      make_shared<MyDB_PageReaderWriter>(*getBufferMgr());
  discoverPages(rootLocation, targetPageList, low, high);

  MyDB_PageReaderWriterPtr firstPage =
      make_shared<MyDB_PageReaderWriter>(*getBufferMgr());
  MyDB_PageReaderWriterPtr lastPage =
      make_shared<MyDB_PageReaderWriter>(*getBufferMgr());
  firstPage->clear();
  lastPage->clear();
  MyDB_INRecordPtr lowRecord = getINRecord();
  MyDB_INRecordPtr highRecord = getINRecord();
  MyDB_RecordPtr record = getEmptyRecord();
  lowRecord->setKey(low);
  highRecord->setKey(high);

  if (targetPageList.size() >= 1) {
    MyDB_RecordIteratorAltPtr itr = targetPageList[0].getIteratorAlt();

    do {
      itr->getCurrent(record);

      auto comparatorLow = buildComparator(record, lowRecord);
      auto comparatorHigh = buildComparator(highRecord, record);
      if (!comparatorLow() && !comparatorHigh()) {
        firstPage->append(record);
      }
    } while (itr->advance());

    targetPageList[0] = *firstPage;
  }
  if (targetPageList.size() > 1) {
    MyDB_RecordIteratorAltPtr itr =
        targetPageList[targetPageList.size() - 1].getIteratorAlt();

    do {
      itr->getCurrent(record);
      auto comparatorLow = buildComparator(record, lowRecord);
      auto comparatorHigh = buildComparator(highRecord, record);

      if (!comparatorLow() && !comparatorHigh()) {
        lastPage->append(record);
      }
    } while (itr->advance());
    targetPageList[targetPageList.size() - 1] = *lastPage;
  }

  MyDB_INRecordPtr lhsIn = getINRecord();
  MyDB_INRecordPtr rhsIn = getINRecord();
  MyDB_INRecordPtr myRecIn = getINRecord();
  MyDB_INRecordPtr lowRecIn = getINRecord();
  MyDB_INRecordPtr highRecIn = getINRecord();
  lowRecIn->setKey(low);
  highRecIn->setKey(high);
  function<bool()> comparatorIn = buildComparator(lhsIn, rhsIn);
  function<bool()> lowComparatorIn = buildComparator(myRecIn, lowRecIn);
  function<bool()> highComparatorIn = buildComparator(highRecIn, myRecIn);
  targetPageList.push_back(*emptyPage);

  MyDB_RecordIteratorAltPtr itr =
      make_shared<MyDB_PageListIteratorSelfSortingAlt>(
          targetPageList, lhsIn, rhsIn, comparatorIn, myRecIn, lowComparatorIn,
          highComparatorIn, true);
  return itr;
}

MyDB_RecordIteratorAltPtr
MyDB_BPlusTreeReaderWriter ::getRangeIteratorAlt(MyDB_AttValPtr low,
                                                 MyDB_AttValPtr high) {
  MyDB_PageReaderWriterPtr emptyPage =
      make_shared<MyDB_PageReaderWriter>(*getBufferMgr());
  vector<MyDB_PageReaderWriter> targetPageList;
  discoverPages(rootLocation, targetPageList, low, high);
  MyDB_PageReaderWriterPtr firstPage =
      make_shared<MyDB_PageReaderWriter>(*getBufferMgr());
  MyDB_PageReaderWriterPtr lastPage =
      make_shared<MyDB_PageReaderWriter>(*getBufferMgr());
  firstPage->clear();
  lastPage->clear();
  MyDB_INRecordPtr lowRecord = getINRecord();
  MyDB_INRecordPtr highRecord = getINRecord();
  MyDB_RecordPtr record = getEmptyRecord();
  lowRecord->setKey(low);
  highRecord->setKey(high);

  if (targetPageList.size() >= 1) {

    MyDB_RecordIteratorAltPtr itr = targetPageList[0].getIteratorAlt();

    do {
      itr->getCurrent(record);
      auto comparatorLow = buildComparator(record, lowRecord);
      auto comparatorHigh = buildComparator(highRecord, record);

      if (!comparatorLow() && !comparatorHigh()) {
        firstPage->append(record);
      }
    } while (itr->advance());

    targetPageList[0] = *firstPage;
  }
  if (targetPageList.size() > 1) {
    // MyDB_PageReaderWriter tail = targetPageList[targetPageList.size() - 1];
    MyDB_RecordIteratorAltPtr itr =
        targetPageList[targetPageList.size() - 1].getIteratorAlt();

    do {
      itr->getCurrent(record);
      auto comparatorLow = buildComparator(record, lowRecord);
      auto comparatorHigh = buildComparator(highRecord, record);

      if (!comparatorLow() && !comparatorHigh()) {
        lastPage->append(record);
      }
    } while (itr->advance());
    targetPageList[targetPageList.size() - 1] = *lastPage;
  }
  targetPageList.push_back(*emptyPage);

  MyDB_RecordIteratorAltPtr itr =
      make_shared<MyDB_PageListIteratorAlt>(targetPageList);
  return itr;
}

bool MyDB_BPlusTreeReaderWriter ::discoverPages(
    int pageNum, vector<MyDB_PageReaderWriter> &List, MyDB_AttValPtr low,
    MyDB_AttValPtr high) {
  MyDB_INRecordPtr lowRecord = getINRecord();
  MyDB_INRecordPtr highRecord = getINRecord();
  MyDB_PageReaderWriterPtr currPage =
      make_shared<MyDB_PageReaderWriter>(*this, pageNum);
  MyDB_RecordIteratorAltPtr itr = currPage->getIteratorAlt();
  MyDB_INRecordPtr inRecord = getINRecord();
  MyDB_RecordPtr record = getEmptyRecord();

  lowRecord->setKey(low);
  highRecord->setKey(high);

  // regular
  if (currPage->getType() == RegularPage) {
    do {
      itr->getCurrent(record);
      auto comparatorLow = buildComparator(record, lowRecord);
      auto comparatorHigh = buildComparator(highRecord, record);

      if (!comparatorLow() && !comparatorHigh()) {
        List.push_back(*currPage);
        return true;
      }
    } while (itr->advance());
    return false;
  }

  // internal
  bool valid = false;
  bool oneTime = false;

  do {
    itr->getCurrent(inRecord);
    auto comparatorLow = buildComparator(inRecord, lowRecord);
    auto comparatorHigh = buildComparator(inRecord, highRecord);

    if (!comparatorLow())
      valid = true;
    if (!comparatorHigh()) {
      valid = false;
      oneTime = true;
    }
    if (valid || oneTime) {
      discoverPages(inRecord->getPtr(), List, low, high);
      oneTime = false;
    }

  } while (itr->advance());
  return false;
}

void MyDB_BPlusTreeReaderWriter ::append(MyDB_RecordPtr appendMe) {
  // check if the table is cleared
  if (getTable()->lastPage() == 0) {
    getTable()->setRootLocation(getTable()->lastPage());
    rootLocation = getTable()->getRootLocation();
    // new B+tree
    MyDB_INRecordPtr record = getINRecord();
    getTable()->setLastPage(getTable()->lastPage() + 1);
    record->setPtr(getTable()->lastPage());
    (*this)[record->getPtr()].clear();
    (*this)[record->getPtr()].setType(RegularPage);
    (*this)[rootLocation].clear();
    (*this)[rootLocation].setType(DirectoryPage);
    (*this)[rootLocation].append(record);
  }
  // add dummy INrecord
  MyDB_INRecordPtr dummyRecordPtr = getINRecord();
  dummyRecordPtr->setPtr(rootLocation);
  internalStack.push(dummyRecordPtr->getPtr());
  // find regular page
  MyDB_PageReaderWriterPtr current =
      make_shared<MyDB_PageReaderWriter>((*this), rootLocation);
  MyDB_INRecordPtr recordPtr = getINRecord();
  while (current->getType() == DirectoryPage) {
    MyDB_RecordIteratorAltPtr itr = current->getIteratorAlt();
    itr->getCurrent(recordPtr);
    auto comparator = buildComparator(recordPtr, appendMe);
    while (comparator()) {
      itr->advance();
      itr->getCurrent(recordPtr);
      comparator = buildComparator(recordPtr, appendMe);
    }
    internalStack.push(recordPtr->getPtr());
    current = make_shared<MyDB_PageReaderWriter>((*this), recordPtr->getPtr());
  }
  // Insert data in to page
  MyDB_RecordPtr newKey = append(internalStack.top(), appendMe);
  if (newKey) {
    internalStack.pop();
    MyDB_PageReaderWriterPtr targetInternal =
        make_shared<MyDB_PageReaderWriter>((*this), internalStack.top());
    split(*targetInternal, newKey);
  }
  // clear internalStack after append operation finished
  while (!internalStack.empty())
    internalStack.pop();
}
// split address internal page split
void MyDB_BPlusTreeReaderWriter ::split(MyDB_PageReaderWriter pageReaderWriter,
                                        MyDB_RecordPtr recordPtr) {
  MyDB_INRecordPtr rec1 = getINRecord();
  MyDB_INRecordPtr rec2 = getINRecord();

  if (!pageReaderWriter.append(recordPtr)) {
    MyDB_INRecordPtr newKey = getINRecord();
    vector<MyDB_RecordPtr> recordList;
    bool appended = false;

    pageReaderWriter.sortInPlace(buildComparator(rec1, rec2), rec1, rec2);

    MyDB_RecordIteratorAltPtr itr = pageReaderWriter.getIteratorAlt();
    MyDB_INRecordPtr temp = getINRecord();
    itr->getCurrent(temp);
    // check appendMe
    auto comparator = buildComparator(recordPtr, temp);
    if (!appended && comparator()) {
      recordList.push_back(recordPtr);
      appended = true;
      comparator = buildComparator(recordPtr, temp);
    }
    recordList.push_back(temp);
    while (itr->advance()) {
      MyDB_INRecordPtr temp = getINRecord();
      itr->getCurrent(temp);
      // check appendMe
      auto comparator = buildComparator(recordPtr, temp);
      if (!appended && comparator()) {
        recordList.push_back(recordPtr);
        appended = true;
        comparator = buildComparator(recordPtr, temp);
      }
      recordList.push_back(temp);
    }
    if (!appended)
      recordList.push_back(recordPtr);

    pageReaderWriter.clear();
    pageReaderWriter.setType(DirectoryPage);

    // larger half store back to ori page
    for (int i = recordList.size() / 2; i < recordList.size(); i++) {
      pageReaderWriter.append(recordList[i]);
    }
    // lower half store into new page
    getTable()->setLastPage(getTable()->lastPage() + 1);
    MyDB_PageReaderWriterPtr newPage =
        make_shared<MyDB_PageReaderWriter>((*this), getTable()->lastPage());
    newPage->clear();
    newPage->setType(DirectoryPage);
    for (int i = 0; i < recordList.size() / 2; i++) {
      newPage->append(recordList[i]);
    }

    newKey->setKey(getKey(recordList[recordList.size() / 2 - 1]));
    newKey->setPtr(getTable()->lastPage());

    if (internalStack.size() > 1) {
      MyDB_PageReaderWriterPtr targetInternal =
          make_shared<MyDB_PageReaderWriter>((*this), internalStack.top());
      internalStack.pop();
      split(*targetInternal, newKey);
    } else {
      // new a root page
      getTable()->setLastPage(getTable()->lastPage() + 1);
      MyDB_PageReaderWriterPtr newRootPage =
          make_shared<MyDB_PageReaderWriter>((*this), getTable()->lastPage());
      newRootPage->clear();
      newRootPage->setType(DirectoryPage);
      MyDB_INRecordPtr rootKey = getINRecord();
      rootKey->setPtr(rootLocation);
      newRootPage->append(newKey);
      newRootPage->append(rootKey);
      // update rootLocation
      getTable()->setRootLocation(getTable()->lastPage());
      rootLocation = getTable()->getRootLocation();
    }

  } else {
    pageReaderWriter.sortInPlace(buildComparator(rec1, rec2), rec1, rec2);
  }
  return;
}

// append address leaf page split, retrun INrec if the split happened
MyDB_RecordPtr MyDB_BPlusTreeReaderWriter ::append(int pageNum,
                                                   MyDB_RecordPtr appendMe) {
  MyDB_PageReaderWriterPtr current =
      make_shared<MyDB_PageReaderWriter>((*this), pageNum);
  if (!current->append(appendMe)) {
    MyDB_RecordPtr rec1 = getEmptyRecord();
    MyDB_RecordPtr rec2 = getEmptyRecord();
    MyDB_INRecordPtr newKey = getINRecord();
    vector<MyDB_RecordPtr> recordList;
    bool appended = false;

    current->sortInPlace(buildComparator(rec1, rec2), rec1, rec2);

    MyDB_RecordIteratorAltPtr itr = current->getIteratorAlt();
    MyDB_RecordPtr temp = getEmptyRecord();
    itr->getCurrent(temp);
    // check appendMe
    auto comparator = buildComparator(appendMe, temp);
    if (!appended && comparator()) {
      recordList.push_back(appendMe);
      appended = true;
      comparator = buildComparator(appendMe, temp);
    }
    recordList.push_back(temp);
    while (itr->advance()) {
      MyDB_RecordPtr temp = getEmptyRecord();
      itr->getCurrent(temp);
      // check appendMe
      auto comparator = buildComparator(appendMe, temp);
      if (!appended && comparator()) {
        recordList.push_back(appendMe);
        appended = true;
        comparator = buildComparator(appendMe, temp);
      }
      recordList.push_back(temp);
    }
    if (!appended)
      recordList.push_back(appendMe);
    current->clear();
    // larger half store back to ori page
    for (int i = recordList.size() / 2; i < recordList.size(); i++) {
      current->append(recordList[i]);
    }
    // lower half store into new page
    getTable()->setLastPage(getTable()->lastPage() + 1);
    MyDB_PageReaderWriterPtr newPage =
        make_shared<MyDB_PageReaderWriter>((*this), getTable()->lastPage());
    newPage->clear();

    for (int i = 0; i < recordList.size() / 2; i++) {
      newPage->append(recordList[i]);
    }

    newKey->setKey(getKey(recordList[recordList.size() / 2 - 1]));
    newKey->setPtr(getTable()->lastPage());

    return newKey;
  }

  return nullptr;
}

MyDB_INRecordPtr MyDB_BPlusTreeReaderWriter ::getINRecord() {
  return make_shared<MyDB_INRecord>(orderingAttType->createAttMax());
}

void MyDB_BPlusTreeReaderWriter ::printTree() {
  queue<MyDB_INRecordPtr> recordBuffer;
  queue<string> leafRecordBuffer;
  MyDB_PageReaderWriterPtr current =
      make_shared<MyDB_PageReaderWriter>((*this), rootLocation);
  MyDB_RecordIteratorAltPtr itr = current->getIteratorAlt();
  MyDB_INRecordPtr temp = getINRecord();
  itr->getCurrent(temp);
  recordBuffer.push(temp);

  while (itr->advance()) {
    MyDB_INRecordPtr temp = getINRecord();
    itr->getCurrent(temp);
    recordBuffer.push(temp);
  }

  while (!recordBuffer.empty()) {
    int s = recordBuffer.size();
    for (int i = 0; i < s; i++) {
      MyDB_INRecordPtr r = recordBuffer.front();
      recordBuffer.pop();
      cout << r->getKey()->toString() << ',';
      // store next page into queue
      MyDB_PageReaderWriterPtr current =
          make_shared<MyDB_PageReaderWriter>((*this), r->getPtr());
      if (current->getType() == DirectoryPage) {
        MyDB_RecordIteratorAltPtr itr = current->getIteratorAlt();
        MyDB_INRecordPtr temp = getINRecord();
        itr->getCurrent(temp);
        recordBuffer.push(temp);

        while (itr->advance()) {
          MyDB_INRecordPtr temp = getINRecord();
          itr->getCurrent(temp);
          recordBuffer.push(temp);
        }
      } else {
        MyDB_RecordIteratorAltPtr itr = current->getIteratorAlt();
        MyDB_RecordPtr temp = getEmptyRecord();
        itr->getCurrent(temp);
        leafRecordBuffer.push(getKey(temp)->toString());
        while (itr->advance()) {
          MyDB_RecordPtr temp = getEmptyRecord();
          itr->getCurrent(temp);
          leafRecordBuffer.push(getKey(temp)->toString());
        }
        MyDB_RecordPtr empty = getEmptyRecord();
        leafRecordBuffer.push(" ");
      }
    }
    cout << endl;
  }
  // print leaf record
  while (!leafRecordBuffer.empty()) {
    string leaf = leafRecordBuffer.front();
    leafRecordBuffer.pop();
    cout << leaf << ',';
  }
  cout << endl;
}

MyDB_AttValPtr MyDB_BPlusTreeReaderWriter ::getKey(MyDB_RecordPtr fromMe) {

  // in this case, got an IN record
  if (fromMe->getSchema() == nullptr)
    return fromMe->getAtt(0)->getCopy();

  // in this case, got a data record
  else
    return fromMe->getAtt(whichAttIsOrdering)->getCopy();
}

function<bool()>
MyDB_BPlusTreeReaderWriter ::buildComparator(MyDB_RecordPtr lhs,
                                             MyDB_RecordPtr rhs) {

  MyDB_AttValPtr lhAtt, rhAtt;

  // in this case, the LHS is an IN record
  if (lhs->getSchema() == nullptr) {
    lhAtt = lhs->getAtt(0);

    // here, it is a regular data record
  } else {
    lhAtt = lhs->getAtt(whichAttIsOrdering);
  }

  // in this case, the LHS is an IN record
  if (rhs->getSchema() == nullptr) {
    rhAtt = rhs->getAtt(0);

    // here, it is a regular data record
  } else {
    rhAtt = rhs->getAtt(whichAttIsOrdering);
  }

  // now, build the comparison lambda and return
  if (orderingAttType->promotableToInt()) {
    return [lhAtt, rhAtt] { return lhAtt->toInt() < rhAtt->toInt(); };
  } else if (orderingAttType->promotableToDouble()) {
    return [lhAtt, rhAtt] { return lhAtt->toDouble() < rhAtt->toDouble(); };
  } else if (orderingAttType->promotableToString()) {
    return [lhAtt, rhAtt] { return lhAtt->toString() < rhAtt->toString(); };
  } else {
    cout << "This is bad... cannot do anything with the >.\n";
    exit(1);
  }
}

#endif