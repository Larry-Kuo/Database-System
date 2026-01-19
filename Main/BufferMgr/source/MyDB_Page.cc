
#ifndef MYDB_PAGE_C
#define MYDB_PAGE_C

#include "MyDB_Page.h"
#include "MyDB_BufferManager.h"
#include <iostream>

using namespace std;

PageBase ::~PageBase() { memory = nullptr; }

#endif