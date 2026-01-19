#ifndef SORT_C
#define SORT_C

#include "MyDB_PageReaderWriter.h"
#include "MyDB_TableRecIterator.h"
#include "MyDB_TableRecIteratorAlt.h"
#include "MyDB_TableReaderWriter.h"
#include "Sorting.h"
#include "MyDB_PageListIteratorAlt.h"
#include "MyDB_PageRecIteratorAlt.h"
#include <vector>
#include <memory>

typedef shared_ptr <MyDB_PageListIteratorAlt > MyDB_PageListIteratorAltPtr;

using namespace std;

struct MyDB_RecordIteratorComparator {
    function<bool()> comparator;
    MyDB_RecordPtr lhs;
    MyDB_RecordPtr rhs;

    MyDB_RecordIteratorComparator(function<bool()> comp, MyDB_RecordPtr lhs, MyDB_RecordPtr rhs)
        : comparator(comp), lhs(lhs), rhs(rhs) {}

    bool operator()(MyDB_RecordIteratorAltPtr leftIter, MyDB_RecordIteratorAltPtr rightIter) {
        leftIter->getCurrent(lhs);
        rightIter->getCurrent(rhs);

        return !comparator();
    }
};

void mergeIntoFile (MyDB_TableReaderWriter &sortIntoMe, vector <MyDB_RecordIteratorAltPtr> &mergeUs,
 function <bool ()>comparator, MyDB_RecordPtr lhs, MyDB_RecordPtr rhs) {
	// 
	priority_queue<MyDB_RecordIteratorAltPtr, vector<MyDB_RecordIteratorAltPtr>, MyDB_RecordIteratorComparator> pq(
    MyDB_RecordIteratorComparator(comparator, lhs, rhs));
	MyDB_RecordPtr temp = sortIntoMe.getEmptyRecord();
	// Initialize pq
	for (int i = 0; i < mergeUs.size(); i++) {
		pq.push(mergeUs[i]);
	}

	while(!pq.empty()) {
		MyDB_RecordIteratorAltPtr top = pq.top();
		pq.pop();
		top->getCurrent(temp);
		sortIntoMe.append(temp);
		if(top->advance()) {
			pq.push(top);
		}
	}
}
vector<MyDB_PageReaderWriter> mergeIntoList(MyDB_BufferManagerPtr parent, MyDB_RecordIteratorAltPtr leftIter, MyDB_RecordIteratorAltPtr rightIter, function<bool()> comparator, MyDB_RecordPtr lhs, MyDB_RecordPtr rhs) {
    vector<MyDB_PageReaderWriter> mergedList;
    MyDB_PageReaderWriterPtr firstMergedPage = make_shared<MyDB_PageReaderWriter>(*parent);
    bool leftEmpty = false, rightEmpty = false;
    bool leftRight; // 0: lhs smaller, 1: rhs smaller

    mergedList.push_back(*firstMergedPage);

    while (!leftEmpty || !rightEmpty) {
        if (!leftEmpty) leftIter->getCurrent(lhs);
        if (!rightEmpty) rightIter->getCurrent(rhs);

        if (leftEmpty) {
            leftRight = 1;  
        } else if (rightEmpty) {
            leftRight = 0; 
        } else {
            leftRight = !comparator();  
        }

        if (!leftRight) {
            // lhs is smaller
            if (!mergedList.back().append(lhs)) {
                // Create a new page if the current page is full
                MyDB_PageReaderWriterPtr newPage = make_shared<MyDB_PageReaderWriter>(*parent);
                newPage->append(lhs);
                mergedList.push_back(*newPage);
            }

            if (!leftIter->advance()) {
                leftEmpty = true;
            }
        } else {
            // rhs is smaller
            if (!mergedList.back().append(rhs)) {
                // Create a new page if the current page is full
                MyDB_PageReaderWriterPtr newPage = make_shared<MyDB_PageReaderWriter>(*parent);
                newPage->append(rhs);
                mergedList.push_back(*newPage);
            }

            if (!rightIter->advance()) {
                rightEmpty = true;
            }
        }
    }

    return mergedList;
}
	
void sort (int runSize, MyDB_TableReaderWriter &sortMe, MyDB_TableReaderWriter &sortIntoMe,
 function <bool ()> comparator, MyDB_RecordPtr lhs, MyDB_RecordPtr rhs) {
	// Sort pages in each run
	int start = 0;
	MyDB_BufferManagerPtr bufferManager = sortMe.getBufferMgr();
	vector<MyDB_RecordIteratorAltPtr> runIterList;
	queue<MyDB_PageListIteratorAltPtr> pageQueue;
	while (start < sortMe.getNumPages()) {
		for (int i = start; i < min(start + runSize, (int)sortMe.getNumPages()); i++) {
			sortMe[i].sortInPlace(comparator, lhs, rhs);
			vector<MyDB_PageReaderWriter> vec {sortMe[i]};
    		MyDB_PageListIteratorAltPtr pageIter = make_shared<MyDB_PageListIteratorAlt>(vec);
			vec.clear();
			pageQueue.push(pageIter);
		}
		// MergeSort
		while(pageQueue.size() > 1) {

			MyDB_PageListIteratorAltPtr leftIter = pageQueue.front();
			pageQueue.pop();
			MyDB_PageListIteratorAltPtr rightIter = pageQueue.front();
			pageQueue.pop();
			vector<MyDB_PageReaderWriter> mergedVector = mergeIntoList(bufferManager, leftIter, rightIter, comparator, lhs, rhs);
			leftIter.reset();
    		rightIter.reset();
			MyDB_PageListIteratorAltPtr mergedIter = make_shared<MyDB_PageListIteratorAlt>(mergedVector);
			mergedVector.clear(); 
			pageQueue.push(mergedIter);
		}
		MyDB_PageListIteratorAltPtr runIter = pageQueue.front();
		pageQueue.pop();
		runIterList.push_back(runIter);
		start += runSize;
	}
	// Hopefully
	mergeIntoFile(sortIntoMe, runIterList, comparator, lhs, rhs);
} 

#endif