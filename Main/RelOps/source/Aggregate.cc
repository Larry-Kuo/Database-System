#ifndef AGG_CC
#define AGG_CC

#include "MyDB_Record.h"
#include "MyDB_PageReaderWriter.h"
#include "MyDB_TableReaderWriter.h"
#include "Aggregate.h"
#include <unordered_map>
#include "RegularSelection.h"
#include "MyDB_PageListIteratorAlt.h"

using namespace std;

Aggregate :: Aggregate (MyDB_TableReaderWriterPtr inputIn, MyDB_TableReaderWriterPtr outputIn,
    	vector <pair <MyDB_AggType, string>> aggsToComputeIn,
		vector <string> groupingsIn, string selectionPredicateIn) {
    input = inputIn;
    output = outputIn;
    aggsToCompute = aggsToComputeIn;
    groupings = groupingsIn;
    selectionPredicate = selectionPredicateIn;
}

void Aggregate :: run () {
    // intialProjection
    vector <string> attrToProject;
    for (auto grouping : groupings)
        attrToProject.push_back(grouping);
    for (auto aggs : aggsToCompute)
        attrToProject.push_back(aggs.second);
    RegularSelection intialProjection (input, output, selectionPredicate, attrToProject);
	intialProjection.run ();

    size_t aggAttNum = aggsToCompute.size();

    // 
    unordered_map<size_t, vector<MyDB_PageReaderWriter>> pagesByGroups;
    MyDB_RecordIteratorAltPtr tableItr = output->getIteratorAlt();
    MyDB_RecordPtr outRecord = output->getEmptyRecord();

    do {
        tableItr->getCurrent(outRecord);
        size_t hashVal = 0;
		for (int i=0;i<groupings.size();i++) {
			hashVal ^= outRecord->getAtt(i)->hash ();
		}
        if (pagesByGroups[hashVal].size() == 0) {           
            MyDB_PageReaderWriterPtr groupBuffer = make_shared<MyDB_PageReaderWriter> (*output->getBufferMgr());
            pagesByGroups[hashVal].push_back(*groupBuffer);
        }
        if (!pagesByGroups[hashVal].back().append(outRecord)) {
            MyDB_PageReaderWriterPtr groupBuffer = make_shared<MyDB_PageReaderWriter> (*output->getBufferMgr());
            pagesByGroups[hashVal].push_back(*groupBuffer);
            pagesByGroups[hashVal].back().append(outRecord);
        }
    } while (tableItr->advance());

    // clear output table
    for(int i=0;i<output->getNumPages();i++)
        (*output)[i].clear();
    // output->getTable()->setLastPage(0);

    // traverse buckets
    MyDB_SchemaPtr mySchemaAgg = make_shared <MyDB_Schema> ();
    int aggCount = 0;
    for (int i=groupings.size(); i<output->getTable()->getSchema ()->getAtts().size();i++)
		mySchemaAgg->appendAtt ({"agg" + to_string(aggCount++), output->getTable()->getSchema ()->getAtts()[i].second});



    MyDB_RecordPtr aggRec = make_shared <MyDB_Record> (mySchemaAgg);

    for (auto &p : output->getTable()->getSchema()->getAtts())
		mySchemaAgg->appendAtt (p);
        
    MyDB_RecordPtr outputRec = output->getEmptyRecord();
    MyDB_RecordPtr combinedRec = make_shared <MyDB_Record> (mySchemaAgg);
	combinedRec->buildFrom (aggRec, outputRec);
    
    for (auto pages: pagesByGroups) {
        // initialize aggRec
        for (int i=0;i<aggAttNum;i++)
            aggRec->getAtt(i)->fromInt(0);

        MyDB_PageListIteratorAlt groupItr = MyDB_PageListIteratorAlt(pages.second);
        do {
            groupItr.getCurrent(outputRec);
            for (int i=0;i<aggAttNum;i++) {
                if (aggsToCompute[i].first == MyDB_AggType::sum || aggsToCompute[i].first == MyDB_AggType::avg) {
                    func aggSum = combinedRec->compileComputation( "+ ([agg" + to_string(i) + "],[" + output->getTable ()->getSchema ()->getAtts()[i+groupings.size()].first + "])");
                    aggRec->getAtt (i)->set (aggSum());
                } else {
                    aggRec->getAtt (i)->fromInt(aggRec->getAtt (i)->toInt() + 1);
                }
            }
            aggRec->recordContentHasChanged ();
        } while (groupItr.advance());
        // aggregrate process
        for (int i=0;i<aggAttNum;i++) {
            if (aggsToCompute[i].first == MyDB_AggType::sum ) {
                outputRec->getAtt(i+groupings.size())->set(aggRec->getAtt(i));
            } else if (aggsToCompute[i].first == MyDB_AggType::avg ) {
                func aggAvg = aggRec->compileComputation( "/ ([agg" + to_string(i) + "],[" + mySchemaAgg->getAtts()[aggAttNum-1].first + "])");
                outputRec->getAtt(i+groupings.size())->set(aggAvg());
            } else {
                outputRec->getAtt(i+groupings.size())->set(aggRec->getAtt(i));
            }
        }
        outputRec->recordContentHasChanged ();
		output->append (outputRec);	
    }
    
}

#endif