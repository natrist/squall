#ifndef STORM_BIG_HPP
#define STORM_BIG_HPP

#include "storm/big/BigData.hpp"
#include <cstdint>

void SBigFromUnsigned(BigData* num, uint32_t val);

void SBigNew(BigData** num);

void SBigToBinaryBuffer(BigData* num, uint8_t* data, uint32_t maxBytes, uint32_t* bytes);

#endif
