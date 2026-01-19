
#ifndef SFW_QUERY_CC
#define SFW_QUERY_CC

#include "ParserTypes.h"
#include "MyDB_Schema.h"
#include <algorithm>
#include <vector>
#include <random>
#include <iostream>
#include <string>
	
// builds and optimizes a logical query plan for a SFW query, returning the logical query plan
pair <LogicalOpPtr, double> SFWQuery :: optimizeQueryPlan (map <string, MyDB_TablePtr> &allTables) {

	// here we call the recursive, exhaustive enum. algorithm
	// return optimizeQueryPlan (...);
	// filter tables
	// map <string, MyDB_TablePtr> usedTables;
	// for (const auto& table : tablesToProcess) {
	// 	usedTables[table.second] = allTables[table.first];
	// }
	// for (auto table : tablesToProcess) {
	// 	allTables[table.first] = allTables[table.first]->alias(table.second);
	// }

	MyDB_SchemaPtr schema = make_shared<MyDB_Schema>();
	for (auto table : tablesToProcess) {
		for (auto att : allTables[table.first]->getSchema()->getAtts()) {
			for (auto proj : valuesToSelect) {
				if (proj->referencesAtt(table.second, att.first)) {
					schema->getAtts().push_back(make_pair(table.second + "_" + att.first, att.second));
					break;
				}
			}
		}
	}

	return optimizeQueryPlan(allTables, schema, tablesToProcess, allDisjunctions);;
}

void SFWQuery :: generateCombinations(vector<pair<string, string>>& items, vector<pair<string, string>>& group1, vector<pair<string, string>>& group2, int index, vector<Result>& results) {
    if (index == items.size()) {
		if (!group1.empty() && !group2.empty()) {
            results.push_back({group1, group2});
        }
        return;
    }

    group1.push_back(items[index]);
    generateCombinations(items, group1, group2, index + 1, results);
    group1.pop_back(); 

    group2.push_back(items[index]);
    generateCombinations(items, group1, group2, index + 1, results);
    group2.pop_back(); 
}

string SFWQuery :: generateRandomString(int length) {
    std::string charset = "abcdefghijklmnopqrstuvwxyz0123456789";
    if (length > (int)charset.size()) {
        throw std::runtime_error("Requested length exceeds the character set size.");
    }

    std::vector<char> chars(charset.begin(), charset.end());

    std::random_device rd;
    std::mt19937 gen(rd());

    std::shuffle(chars.begin(), chars.end(), gen);

    std::string result(chars.begin(), chars.begin() + length);

    return result;
}

// builds and optimizes a logical query plan for a SFW query, returning the logical query plan
pair <LogicalOpPtr, double> SFWQuery :: optimizeQueryPlan (map <string, MyDB_TablePtr> &allTables, 
	MyDB_SchemaPtr topSchema, vector <pair <string, string>> tablesToUse, vector <ExprTreePtr> &allDisjunctions) {
		
	vector<pair<string, string>> leftTables, rightTables;
	vector<Result> results;  	
	string randomStr = generateRandomString(20);	
	MyDB_TablePtr tempTable = make_shared<MyDB_Table>("tempTable"+randomStr, "tempTable"+randomStr+"Loc", topSchema);
	double cost = 0;
	double best = numeric_limits<double>::max();
	LogicalOpPtr bestLogicalOp = nullptr;

	// if only left one table
	if (tablesToUse.size () == 1) {
		MyDB_StatsPtr newStats = make_shared<MyDB_Stats>(allTables[tablesToUse[0].first]->alias(tablesToUse[0].second)); //->alias(tablesToUse[0].second)
		MyDB_StatsPtr outputStats = newStats->costSelection(allDisjunctions);
		cost += outputStats->getTupleCount();
		LogicalOpPtr topLogicalOP = make_shared<LogicalTableScan> (allTables[tablesToUse[0].first]->alias(tablesToUse[0].second), tempTable, outputStats, allDisjunctions);
		return make_pair (topLogicalOP, cost);
	}


	generateCombinations(tablesToUse, leftTables, rightTables, 0, results);
	
	for (auto result : results) {
		vector <ExprTreePtr> leftCNF, rightCNF, topCNF;
		MyDB_SchemaPtr leftSchema = make_shared<MyDB_Schema>();
		MyDB_SchemaPtr rightSchema = make_shared<MyDB_Schema>();
		for (auto disjunction : allDisjunctions) {
			bool inLeft = false;
			bool inRight = false;
			for (auto leftTable : result.leftTables) {
				if (disjunction->referencesTable(leftTable.second)) {
					inLeft = true;
					break;
				}
			}
			for (auto rightTable : result.rightTables) {
				if (disjunction->referencesTable(rightTable.second)) {
					inRight = true;
					break;
				}
			}
			if (inLeft && !inRight) {
				leftCNF.push_back(disjunction);
			} else if (!inLeft && inRight) {
				rightCNF.push_back(disjunction);
			} else {
				topCNF.push_back(disjunction);
			}
		}
		// attributes
		for (auto leftTable : result.leftTables) {
			for (auto leftAtt : allTables[leftTable.first]->getSchema()->getAtts()) {
				bool find = false;
				// topAttr
				for (auto topAtt : topSchema->getAtts()) {
					if (leftTable.second + "_" + leftAtt.first  == topAtt.first) {
						find = true;
						break;
					}
				}
				// topCNF
				if (!find) {
					for (auto a: topCNF) {
						if (a->referencesAtt (leftTable.second, leftAtt.first)) {
							find = true;
							break;
						}
					}
				}
				if (find) leftSchema->getAtts().push_back(make_pair(leftTable.second + "_" + leftAtt.first, leftAtt.second));
			}
		}
		for (auto rightTable : result.rightTables) {
			for (auto rightAtt : allTables[rightTable.first]->getSchema()->getAtts()) {
				bool find = false;
				// topAttr
				for (auto topAtt : topSchema->getAtts()) {
					if (rightTable.second + "_" + rightAtt.first  == topAtt.first) {
						find = true;
						break;
					}
				}
				// topCNF
				if (!find) {
					for (auto a: topCNF) {
						if (a->referencesAtt (rightTable.second, rightAtt.first)) {
							find = true;
							break;
						}
					}
				}
				if (find) rightSchema->getAtts().push_back(make_pair(rightTable.second + "_" + rightAtt.first, rightAtt.second));
			}
		}

		pair <LogicalOpPtr, double> lhs = optimizeQueryPlan(allTables, leftSchema, result.leftTables, leftCNF);
		pair <LogicalOpPtr, double> rhs = optimizeQueryPlan(allTables, rightSchema, result.rightTables, rightCNF);
		cost = lhs.second + rhs.second;
		MyDB_StatsPtr outputStats = lhs.first->getStats()->costJoin(topCNF, rhs.first->getStats());
		cost += outputStats->getTupleCount();
		LogicalOpPtr topLogicalOP = make_shared<LogicalJoin> (lhs.first, rhs.first, tempTable, topCNF, outputStats);
		// only run in root
		if (best > cost) {
			best = cost;
			bestLogicalOp = topLogicalOP;
		}
	}

	return make_pair (bestLogicalOp, best);
}

void SFWQuery :: print () {
	cout << "Selecting the following:\n";
	for (auto a : valuesToSelect) {
		cout << "\t" << a->toString () << "\n";
	}
	cout << "From the following:\n";
	for (auto a : tablesToProcess) {
		cout << "\t" << a.first << " AS " << a.second << "\n";
	}
	cout << "Where the following are true:\n";
	for (auto a : allDisjunctions) {
		cout << "\t" << a->toString () << "\n";
	}
	cout << "Group using:\n";
	for (auto a : groupingClauses) {
		cout << "\t" << a->toString () << "\n";
	}
}


SFWQuery :: SFWQuery (struct ValueList *selectClause, struct FromList *fromClause,
        struct CNF *cnf, struct ValueList *grouping) {
        valuesToSelect = selectClause->valuesToCompute;
        tablesToProcess = fromClause->aliases;
        allDisjunctions = cnf->disjunctions;
        groupingClauses = grouping->valuesToCompute;
}

SFWQuery :: SFWQuery (struct ValueList *selectClause, struct FromList *fromClause,
        struct CNF *cnf) {
        valuesToSelect = selectClause->valuesToCompute;
        tablesToProcess = fromClause->aliases;
	allDisjunctions = cnf->disjunctions;
}

SFWQuery :: SFWQuery (struct ValueList *selectClause, struct FromList *fromClause) {
        valuesToSelect = selectClause->valuesToCompute;
        tablesToProcess = fromClause->aliases;
        allDisjunctions.push_back (make_shared <BoolLiteral> (true));
}

#endif
