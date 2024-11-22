#include "test/big/BigDataTest.hpp"

BigDataTest::BigDataTest() {
    SBigNew(&this->num);
}

BigDataTest::~BigDataTest() {
    SBigDel(this->num);
}
