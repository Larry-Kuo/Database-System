
#ifndef LOG_OP_CC
#define LOG_OP_CC

#include "MyDB_LogicalOps.h"
#include "Aggregate.h"
#include "BPlusSelection.h"
#include "RegularSelection.h"
#include "ScanJoin.h"
#include "SortMergeJoin.h"

void LogicalOp ::  exploreSelections (string& selections, vector <ExprTreePtr>& selectionPred, int index) {
	if (selectionPred.size() == 0) {
		selections = "bool [true]";
		return;
	}
	if (index < selectionPred.size()-1) {
		selections += "&& (" + selectionPred[index]->toString() + ", ";
		exploreSelections(selections, selectionPred, index+1);
		selections += ")";
	} else {
		selections += selectionPred[index]->toString();
	}
}

MyDB_TableReaderWriterPtr LogicalTableScan :: execute (map <string, MyDB_TableReaderWriterPtr> &allTableReaderWriters,
	map <string, MyDB_BPlusTreeReaderWriterPtr> &allBPlusReaderWriters) {
	// your code here!
	// get target tableReaderWriter
	string targetTableName = inputSpec->getName();
	// if (allTableReaderWriters.find(targetTableName) != allTableReaderWriters.end()) {
	MyDB_TableReaderWriterPtr targetTable = make_shared<MyDB_TableReaderWriter>(inputSpec, allTableReaderWriters[targetTableName]->getBufferMgr());
	// get selection string
	string selections = "";
	exploreSelections (selections, selectionPred, 0);
	// get projections
	vector <string> projections;
	MyDB_SchemaPtr outSchema = outputSpec->getSchema();
	for (auto att : outSchema->getAtts()) {
		projections.push_back("[" + att.first + "]");
	}
	MyDB_TableReaderWriterPtr outTable = make_shared<MyDB_TableReaderWriter>(outputSpec, targetTable->getBufferMgr());
	RegularSelection temp (targetTable, outTable, selections, projections);
	temp.run();
	return outTable;
}

bool LogicalJoin :: attInLeft (ExprTreePtr lhs, MyDB_TableReaderWriterPtr leftTable) {
	for (auto att : leftTable->getTable()->getSchema()->getAtts()){
		size_t lhsPos = lhs->getId().find('_');
		string lhsAttName = lhs->getId().substr(lhsPos + 1);
		size_t attPos = att.first.find('_');
		string attName = att.first.substr(attPos + 1);
		if (lhsAttName == attName) {
			return true;
		}
	}
	return false;
}


MyDB_TableReaderWriterPtr LogicalJoin :: execute (map <string, MyDB_TableReaderWriterPtr> &allTableReaderWriters,
	map <string, MyDB_BPlusTreeReaderWriterPtr> &allBPlusReaderWriters) {
	// your code here!
	MyDB_TableReaderWriterPtr leftTable = leftInputOp->execute(allTableReaderWriters, allBPlusReaderWriters);
	MyDB_TableReaderWriterPtr rightTable = rightInputOp->execute(allTableReaderWriters, allBPlusReaderWriters);

	int minPageNum = leftTable->getNumPages() < rightTable->getNumPages() ? leftTable->getNumPages() : rightTable->getNumPages();
	MyDB_TableReaderWriterPtr outTable = make_shared<MyDB_TableReaderWriter>(getOutputTable(), leftTable->getBufferMgr());
	// get topCNF
	string topSelections = "";
	exploreSelections (topSelections, outputSelectionPredicate, 0);
	// get projections
	vector <string> projections;
	MyDB_SchemaPtr outSchema = outputSpec->getSchema();
	for (auto att : outSchema->getAtts()) {
		projections.push_back("[" + att.first + "]");
	}
	// get equality checks
	vector <pair <string, string>> equalityChecks;
	for (auto ec : outputSelectionPredicate) {
		if (ec->isEq()) {
			if (attInLeft(ec->getLHS(), leftTable)) {
				equalityChecks.push_back(make_pair(ec->getLHS()->toString(), ec->getRHS()->toString()));
			} else {
				equalityChecks.push_back(make_pair(ec->getRHS()->toString(), ec->getLHS()->toString()));
			}
		} 
	}

	if (minPageNum <= leftTable->getBufferMgr()->getNumPages()/2) {
		// scan join	
		ScanJoin sjOp (leftTable, rightTable, outTable, topSelections, projections, equalityChecks, "bool[true]", "bool[true]");
		sjOp.run();
	} else {
		// sort-merge join
		SortMergeJoin smjOp (leftTable, rightTable, outTable, topSelections, projections, equalityChecks[0], "bool[true]", "bool[true]");
		smjOp.run();
	}

	//kill temp tables
	MyDB_BufferManagerPtr bufferMgr = leftTable->getBufferMgr();
	bufferMgr->killTable(leftTable->getTable());
	bufferMgr->killTable(rightTable->getTable());

	return outTable;
}

#endif
