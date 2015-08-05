//
//  SteamworksUtils.cpp
//  Plugin
//
//  Created by Stiven on 8/3/15.
//  Copyright (c) 2015 Corona Labs. All rights reserved.
//

#include "SteamworksUtils.h"


const char*
SteamworksUtils::steamIDToString(CSteamID id)
{
    static char stringID[32];
    uint64 intID = id.ConvertToUint64();
    _snprintf(stringID, 32, "%llu", intID);
    return stringID;
}

CSteamID
SteamworksUtils::stringToSteamID(const char* stringID)
{
    std::stringstream strValue;
    strValue << stringID;
    uint64 intID;
    strValue >> intID;
    return CSteamID(intID);
}