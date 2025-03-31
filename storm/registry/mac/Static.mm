#include "storm/registry/mac/Static.hpp"

static SRegStatic* SRegStatic::Get() {
    static SRegStatic sregstatic;
    return &sregstatic;
}

SRegStatic::SRegStatic() {
    this->hives = [[NSMutableDictionary alloc] init];
}

SRegStatic::~SRegStatic() {
    this->hives = nil;
}
