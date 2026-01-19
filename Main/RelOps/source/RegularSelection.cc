
#ifndef REG_SELECTION_C                                        
#define REG_SELECTION_C

#include "RegularSelection.h"

RegularSelection :: RegularSelection (MyDB_TableReaderWriterPtr inputIn, MyDB_TableReaderWriterPtr outputIn,
                string finalSelectionPredicateIn, vector <string> projectionsIn) {
    inTable = inputIn;
    output = outputIn;
    finalSelectionPredicate = finalSelectionPredicateIn;
    projections = projectionsIn;
}

void RegularSelection :: run () {
	MyDB_RecordPtr inputRec = inTable->getEmptyRecord ();
    MyDB_RecordPtr outputRec = output->getEmptyRecord();

        
    func selectionFunc = inputRec->compileComputation(finalSelectionPredicate);

	vector <func> projectionFuncs;
	for (string p : projections) {
		projectionFuncs.push_back (inputRec->compileComputation (p));
	}

    MyDB_RecordIteratorPtr recordIter = inTable->getIterator(inputRec);
    while (recordIter->hasNext()) {
        recordIter->getNext();

        if (selectionFunc()->toBool()) {
            for (int i=0;i<projectionFuncs.size();i++) {
                outputRec->getAtt(i)->set(projectionFuncs[i]());
            }
            outputRec->recordContentHasChanged();
            output->append(outputRec);
        }
    }

}

#endif
