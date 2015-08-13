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

bool
SteamworksUtils::makeAppIdEnvVar(lua_State *L)
{
    bool success = false;
    lua_getglobal(L, "require");
    lua_pushstring(L, "config");
    lua_call(L, 1, 0);
    
    //gets the application table
    lua_getglobal(L, "application");
    if (lua_type(L, -1) == LUA_TTABLE) {
        //push the steam table to the top of the stack
        lua_getfield(L, -1, "steam");
        if(lua_type(L, -1) == LUA_TTABLE) {
            //gets the appID field from the google table
            lua_getfield(L, -1, "appId");
            if(lua_type(L, -1) == LUA_TSTRING) {
                const char* appID = lua_tostring(L, -1);
                int error;
#ifdef _WIN32
                error = _putenv_s("SteamAppId", appID);
#else
                error = setenv("SteamAppId", appID, 1);
#endif
                if (!error){
                    success = true;
                }
                else{
                    CoronaLuaLog(L, "Steam App Id Failed. Error %i", error);
                }
            }
            lua_pop(L, 1);
        }
        lua_pop(L, 1);
    }
    lua_pop(L, 1);
    return success;
}

const char*
SteamworksUtils::getWebAPIKey(lua_State *L)
{
    const char* key = "";
    
    lua_getglobal(L, "require");
    lua_pushstring(L, "config");
    lua_call(L, 1, 0);
    
    //gets the application table
    lua_getglobal(L, "application");
    if (lua_type(L, -1) == LUA_TTABLE) {
        //push the steam table to the top of the stack
        lua_getfield(L, -1, "steam");
        if(lua_type(L, -1) == LUA_TTABLE) {
            //gets the appID field from the google table
            lua_getfield(L, -1, "webAPIKey");
            if(lua_type(L, -1) == LUA_TSTRING) {
                key = lua_tostring(L, -1);
            }
            lua_pop(L, 1);
        }
        lua_pop(L, 1);
    }
    lua_pop(L, 1);
    
    return key;
}

void
SteamworksUtils::stackdump_g(lua_State* l)
{
    int i;
    int top = lua_gettop(l);
    
    CoronaLog("total in stack %d\n",top);
    
    for (i = 1; i <= top; i++)
    {  /* repeat for each level */
        int t = lua_type(l, i);
        switch (t) {
            case LUA_TSTRING:  /* strings */
                CoronaLog("string: '%s'\n", lua_tostring(l, i));
                break;
            case LUA_TBOOLEAN:  /* booleans */
                CoronaLog("boolean %s\n",lua_toboolean(l, i) ? "true" : "false");
                break;
            case LUA_TNUMBER:  /* numbers */
                CoronaLog("number: %g\n", lua_tonumber(l, i));
                break;
            default:  /* other values */
                CoronaLog("%s\n", lua_typename(l, t));
                break;
        }
        CoronaLog("  ");  /* put a separator */
    }
    CoronaLog("\n");  /* end the listing */
}