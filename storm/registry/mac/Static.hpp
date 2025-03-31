#ifndef STORM_REGISTRY_MAC_STATIC_HPP
#define STORM_REGISTRY_MAC_STATIC_HPP

#import <Foundation/Foundation.h>

class SRegStatic {
public:
    NSMutableDictionary<NSString*, NSUserDefaults*>* hives;

    static SRegStatic Get();

    SRegStatic() {
        this->hives = [NSMutableDictionary init];
    }

    ~SRegStatic() {
        this->hives = nil;
    }
};

#endif
