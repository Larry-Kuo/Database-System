# Relational Database System Implementation

This repository contains a complete implementation of a relational database system, developed as part of a multi-stage database systems course (2024 Fall @ Rice University). The project involved building every layer of a database engine, from low-level storage and buffer management to high-level query execution and optimization.

## Project Overview

The system is built in C++ and implements the core functionality of a relational database management system (RDBMS). It is designed to handle large datasets using external memory algorithms and provides a robust framework for query processing.

### Key Components

#### 1. Buffer Management
- **Custom LRU Buffer Pool**: Implemented a buffer manager that handles page requests, eviction using a Least Recently Used (LRU) policy, and page pinning/unpinning to prevent active pages from being evicted.
- **Anonymous & Table-Backed Pages**: Supports both temporary (anonymous) pages for intermediate computations and persistent pages backed by disk storage.

#### 2. Record & Page Management
- **Binary Serialization**: Implemented efficient serialization and deserialization of records into fixed-size pages.
- **Flexible Page Layout**: Designed a page structure that supports variable-length records and efficient space management.
- **Iterators**: Developed various iterator types (Regular, Alternate, Self-Sorting) to traverse records within pages and across entire tables.

#### 3. B+ Tree Indexing
- **High-Performance Indexing**: Implemented a B+ Tree index structure to support fast point queries and range scans.
- **Dynamic Balancing**: Handles node splitting and merging to maintain tree balance during insertions and deletions.
- **Range Iterators**: Optimized range scan performance using the B+ Tree structure.

#### 4. External Memory Sorting (TPMMS)
- **Two-Phase Multi-Way Merge Sort**: Implemented the TPMMS algorithm to sort datasets that exceed the available memory.
- **Run Generation & Merging**: Efficiently generates sorted runs and merges them using a priority queue.

#### 5. Relational Operators
- **Core SQL Operators**: Implemented fundamental relational algebra operators, including:
    - **Select**: Filtering records based on predicates.
    - **Project**: Selecting specific attributes from records.
    - **Join**: Efficiently joining tables using nested loops or hash joins.
    - **Aggregate**: Performing computations like SUM, COUNT, and AVG.

#### 6. Query Execution & Optimization
- **Execution Engine**: Developed an end-to-end query processing pipeline that translates logical query plans into physical execution trees.
- **Query Optimization**: (Briefly describe any optimization work if applicable, e.g., join ordering or predicate pushdown).

## Consolidation & Integration

This repository is a consolidated version of 8 individual assignments. The integration process involved:
- **Bridging Code Gaps**: Adapting student-written components from different stages to work seamlessly together.
- **Refactoring for Compatibility**: Aligning page layouts and method signatures across the entire codebase.
- **Systematic Debugging**: Resolving complex circular dependencies and linker issues to create a stable, unified build.

## How to Build

The project uses `SCons` for its build system.

1.  Navigate to the `Build` directory.
2.  Run `scons` to see the build menu.
3.  Select the desired unit test or component to build.

```bash
cd Build
scons
# Follow the on-screen menu to build specific modules
```

## Verification

The system has been verified using a comprehensive suite of unit tests for each component:
- `bufferUnitTest`: Validates the LRU policy and page management.
- `recordUnitTest`: Tests record serialization and table iteration.
- `sortUnitTest`: Verifies the TPMMS implementation.
- `bPlusUnitTest`: Ensures the correctness of B+ Tree operations.
- `relOpUnitTest`: Validates the relational algebra operators.
