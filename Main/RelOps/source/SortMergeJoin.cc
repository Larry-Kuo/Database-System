
#ifndef SORTMERGE_CC
#define SORTMERGE_CC

#include "Aggregate.h"
#include "MyDB_Record.h"
#include "MyDB_PageReaderWriter.h"
#include "MyDB_TableReaderWriter.h"
#include "SortMergeJoin.h"
#include "Sorting.h"
#include "MyDB_PageListIteratorAlt.h"

SortMergeJoin :: SortMergeJoin (MyDB_TableReaderWriterPtr leftInputIn, MyDB_TableReaderWriterPtr rightInputIn,
		MyDB_TableReaderWriterPtr outputIn, string finalSelectionPredicateIn, 
		vector <string> projectionsIn,
		pair <string, string> equalityChecksIn, string leftSelectionPredicateIn,
		string rightSelectionPredicateIn) {
    output = outputIn;
	finalSelectionPredicate = finalSelectionPredicateIn;
	projections = projectionsIn;
    equalityChecks = equalityChecksIn;
	leftTable = leftInputIn;
	rightTable = rightInputIn;
	leftSelectionPredicate = leftSelectionPredicateIn;
	rightSelectionPredicate = rightSelectionPredicateIn;
}

void SortMergeJoin :: run () {
    // left table sort phase
    MyDB_RecordPtr lhsLeft = leftTable->getEmptyRecord();
    MyDB_RecordPtr rhsLeft = leftTable->getEmptyRecord();
    MyDB_RecordIteratorAltPtr ptrLeft = buildItertorOverSortedRuns (leftTable->getBufferMgr()->numPages/2, *leftTable, 
	buildRecordComparator(lhsLeft, rhsLeft, equalityChecks.first), lhsLeft, rhsLeft, leftSelectionPredicate);
    // right table sort phase
    MyDB_RecordPtr lhsRight = rightTable->getEmptyRecord();
    MyDB_RecordPtr rhsRight = rightTable->getEmptyRecord();
    MyDB_RecordIteratorAltPtr ptrRight = buildItertorOverSortedRuns (rightTable->getBufferMgr()->numPages/2, *rightTable, 
	buildRecordComparator(lhsRight, rhsRight, equalityChecks.second), lhsRight, rhsRight, rightSelectionPredicate);
    
    // and get the schema that results from combining the left and right records
	MyDB_SchemaPtr mySchemaOut = make_shared <MyDB_Schema> ();
	for (auto &p : leftTable->getTable ()->getSchema ()->getAtts ())
		mySchemaOut->appendAtt (p);
	for (auto &p : rightTable->getTable ()->getSchema ()->getAtts ())
		mySchemaOut->appendAtt (p);

    MyDB_RecordPtr recordLeft = leftTable->getEmptyRecord();
    MyDB_RecordPtr recordRight = rightTable->getEmptyRecord();
    // get the combined record
	MyDB_RecordPtr combinedRec = make_shared <MyDB_Record> (mySchemaOut);
	combinedRec->buildFrom (recordLeft, recordRight);
    func finalPredicate = combinedRec->compileComputation (finalSelectionPredicate);

    vector <func> finalComputations;
	for (string s : projections) {
		finalComputations.push_back (combinedRec->compileComputation (s));
	}

	MyDB_RecordPtr outputRec = output->getEmptyRecord ();
    
    ptrLeft->getCurrent(recordLeft);
    ptrRight->getCurrent(recordRight);
    ptrLeft->advance();
    ptrRight->advance();

    while (true) {
        ptrLeft->getCurrent(recordLeft);
        ptrRight->getCurrent(recordRight);
        // 
        func equal = combinedRec->compileComputation( "== (" + equalityChecks.first + "," + equalityChecks.second + ")");
        func greater = combinedRec->compileComputation( "> (" + equalityChecks.first + "," + equalityChecks.second + ")");
        
        if (equal()->toBool ()) {
            bool reachBottomLeft = true;
            bool reachBottomRight = true;
            string keyValue = recordLeft->compileComputation(equalityChecks.first)()->toString();
            // create anonymous pages for left and right
            MyDB_PageReaderWriterPtr leftBuffer = make_shared<MyDB_PageReaderWriter> (*leftTable->getBufferMgr());
            MyDB_PageReaderWriterPtr rightBuffer = make_shared<MyDB_PageReaderWriter> (*leftTable->getBufferMgr());
            vector<MyDB_PageReaderWriter> leftBuffers = {*leftBuffer};
            vector<MyDB_PageReaderWriter> rightBuffers = {*rightBuffer};
            // add first element
            leftBuffers.back().append(recordLeft);
            rightBuffers.back().append(recordRight);
            // left
            int leftTime = 0;
            while (ptrLeft->advance())
            {
                ptrLeft->getCurrent(recordLeft);
                if (recordLeft->compileComputation(equalityChecks.first)()->toString() != keyValue) {
                    reachBottomLeft = false;
                    break;
                } else {
                    leftTime++;
                    if (!leftBuffers.back().append(recordLeft)){
                        leftBuffer = make_shared<MyDB_PageReaderWriter> (*leftTable->getBufferMgr());
                        leftBuffers.push_back(*leftBuffer);
                        leftBuffers.back().append(recordLeft);
                    }
                }
            }
            // right

            int rightTime = 0;
            while (ptrRight->advance())
            {
                ptrRight->getCurrent(recordRight);
                if (recordRight->compileComputation(equalityChecks.second)()->toString() != keyValue){
                    reachBottomRight = false;
                    break;
                } else {
                    rightTime++;
                    if (!rightBuffers.back().append(recordRight)){
                        rightBuffer = make_shared<MyDB_PageReaderWriter> (*leftTable->getBufferMgr());
                        rightBuffers.push_back(*rightBuffer);
                        rightBuffers.back().append(recordRight);
                    }
                }
            }
            // explore all combinations
            MyDB_PageListIteratorAlt leftItr = MyDB_PageListIteratorAlt(leftBuffers);
            do {
                MyDB_PageListIteratorAlt rightItr = MyDB_PageListIteratorAlt(rightBuffers);
                do {
                    leftItr.getCurrent(recordLeft);
                    rightItr.getCurrent(recordRight);
                    //
                    if (finalPredicate ()->toBool ()) {
                        // run all of the computations
                        int i = 0;
			        	for (auto &f : finalComputations) {
			        		outputRec->getAtt (i++)->set (f());
			        	}
                        // cout << endl;
                        outputRec->recordContentHasChanged ();
			        	output->append (outputRec);	
                    }
                } while (rightItr.advance());
            }
            while (leftItr.advance());
            if (reachBottomLeft || reachBottomRight)
                break;
        } else if (greater()->toBool()) {
            if (!ptrRight->advance())
                break;
        } else {
            if (!ptrLeft->advance())
                break;
        }
    }
}

#endif