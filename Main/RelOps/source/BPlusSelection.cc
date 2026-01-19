
#ifndef BPLUS_SELECTION_C                                        
#define BPLUS_SELECTION_C

#include "BPlusSelection.h"

BPlusSelection :: BPlusSelection (MyDB_BPlusTreeReaderWriterPtr input, MyDB_TableReaderWriterPtr outputIn,
		MyDB_AttValPtr lowValue, MyDB_AttValPtr highValue,
		string finalSelectionPredicateIn, vector <string> projectionsIn) {
    inTable = input;
    output = outputIn;
    finalSelectionPredicate = finalSelectionPredicateIn;
    projections = projectionsIn;
    low = lowValue;
    high = highValue;
}

void BPlusSelection :: run () {
    MyDB_RecordPtr inputRec = inTable->getEmptyRecord ();
    MyDB_RecordPtr outputRec = output->getEmptyRecord();

        
    func selectionFunc = inputRec->compileComputation(finalSelectionPredicate);

	vector <func> projectionFuncs;
	for (string p : projections) {
		projectionFuncs.push_back (inputRec->compileComputation (p));
	}

    MyDB_RecordIteratorAltPtr recordIter = inTable->getRangeIteratorAlt(low, high);
    while (recordIter->advance()) {
        recordIter->getCurrent(inputRec);

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
