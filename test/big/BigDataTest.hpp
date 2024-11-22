#ifndef TEST_BIG_BIG_DATA_TEST_HPP
#define TEST_BIG_BIG_DATA_TEST_HPP

#include "storm/Big.hpp"

class BigData;

// Fixture for repetitive handling of BigData objects.
struct BigDataTest {
    using BigDataPtr = BigData*;

    BigData* num;

    BigDataTest();
    ~BigDataTest();

    BigData** operator &() { return &num; }
    operator BigDataPtr() const { return num; }
    BigData* operator->() const { return num; }
};


#endif
