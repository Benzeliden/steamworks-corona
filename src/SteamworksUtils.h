//
//  SteamworksUtils.h
//  Plugin
//
//  Created by Stiven on 8/3/15.
//  Copyright (c) 2015 Corona Labs. All rights reserved.
//

#ifndef __Plugin__SteamworksUtils__
#define __Plugin__SteamworksUtils__

#include <stdio.h>
#include "steam_api.h"
#include "CoronaLua.h"
#include <sstream>

class SteamworksUtils
{
public:
    static const char* steamIDToString(CSteamID id);
    static CSteamID stringToSteamID(const char* stringID);
    static bool makeAppIdEnvVar(lua_State *L);
    static void stackdump_g(lua_State* L);
    static const char* getWebAPIKey(lua_State *L);
};

#endif /* defined(__Plugin__SteamworksUtils__) */
