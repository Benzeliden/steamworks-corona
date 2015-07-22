//
//  SteamworksProvider.cpp
//  Plugin
//
//  Created by Stiven on 7/14/15.
//  Copyright (c) 2015 Corona Labs. All rights reserved.
//

#include "SteamworksProvider.h"
#include "CoronaLibrary.h"
#include "CoronaEvent.h"
#include "steam_api.h"

#include <inttypes.h>
// ----------------------------------------------------------------------------

class SteamworksProvider
{
public:
    typedef SteamworksProvider Self;
    
public:
    static const char kName[];
    static const char kProviderName[];
    static const char kEventName[];
    
protected:
    SteamworksProvider( lua_State *L );
    ~SteamworksProvider();
    
public:
    CoronaLuaRef GetListener() const { return fListener; }
    
public:
    static int Open( lua_State *L );
    
protected:
    static int Finalizer( lua_State *L );
    void Dispatch( bool isError );
    
public:
    static Self *ToLibrary( lua_State *L );
    
protected:
    int Init( lua_State *L );
    int Show( lua_State *L );
    int Request( lua_State *L );
    
public:
    static int init(lua_State *L);
    static int show(lua_State *L);
    static int request(lua_State *L);
    
private:
    CoronaLuaRef fListener;
    lua_State *_L;
    
public:
    STEAM_CALLBACK( SteamworksProvider, OnUserStatsReceived, UserStatsReceived_t, m_CallbackUserStatsReceived );
    STEAM_CALLBACK( SteamworksProvider, OnUserStatsStored, UserStatsStored_t, m_CallbackUserStatsStored );
    STEAM_CALLBACK( SteamworksProvider, OnUserAchievementStored, UserAchievementStored_t, m_CallbackUserAchievementStored );
    STEAM_CALLBACK( SteamworksProvider, OnUserStatsUnloaded, UserStatsUnloaded_t, m_CallbackUserStatsUnloaded );
    STEAM_CALLBACK( SteamworksProvider, OnUserAchievementIconFetched, UserAchievementIconFetched_t, m_CallbackUserAchievementIconFetched );
    STEAM_CALLBACK( SteamworksProvider, OnLeaderboardUGCSet, LeaderboardUGCSet_t, m_CallbackLeaderboardUGCSet );
    
    void OnLeaderboardFindResult( LeaderboardFindResult_t *pResult, bool bIOFailure );
    CCallResult<SteamworksProvider, LeaderboardFindResult_t> m_callResultFindLeaderboard;
    
    void OnLeaderboardScoresDownloaded( LeaderboardScoresDownloaded_t *pResult, bool bIOFailure );
    CCallResult<SteamworksProvider, LeaderboardScoresDownloaded_t> m_callResultDownloadedLeaderboardScores;
    
    void OnLeaderboardScoreUploaded( LeaderboardScoreUploaded_t *pResult, bool bIOFailure );
    CCallResult<SteamworksProvider, LeaderboardScoreUploaded_t> m_callResultUploadedLeaderboardScore;
    
    void OnNumberOfCurrentPlayers( NumberOfCurrentPlayers_t *pResult, bool bIOFailure );
    CCallResult<SteamworksProvider, NumberOfCurrentPlayers_t> m_callResultNumberOfCurrentPlayers;
    
    void OnGlobalStatsReceived( GlobalStatsReceived_t *pResult, bool bIOFailure );
    CCallResult<SteamworksProvider, GlobalStatsReceived_t> m_callResultReceivedGlobalStats;

    void OnGlobalAchievementPercentagesReady( GlobalAchievementPercentagesReady_t *pResult, bool bIOFailure );
    CCallResult<SteamworksProvider, GlobalAchievementPercentagesReady_t> m_callResultGlobalAchievementPercentagesReady;
};

// ----------------------------------------------------------------------------

const char SteamworksProvider::kName[] = "CoronaProvider.gameNetwork.steamworks";

const char SteamworksProvider::kProviderName[] = "steamworks";

const char SteamworksProvider::kEventName[] = "steamworksEvent";

static const char kPublisherId[] = "com.coronalabs";

SteamworksProvider::SteamworksProvider( lua_State *L )
:
fListener( NULL ),
_L( L ),
m_CallbackUserStatsReceived(this, &SteamworksProvider::OnUserStatsReceived),
m_CallbackUserStatsStored(this, &SteamworksProvider::OnUserStatsStored),
m_CallbackUserAchievementStored(this, &SteamworksProvider::OnUserAchievementStored),
m_CallbackUserStatsUnloaded(this, &SteamworksProvider::OnUserStatsUnloaded),
m_CallbackUserAchievementIconFetched(this, &SteamworksProvider::OnUserAchievementIconFetched),
m_CallbackLeaderboardUGCSet(this, &SteamworksProvider::OnLeaderboardUGCSet)
{
}

SteamworksProvider::~SteamworksProvider()
{
    SteamAPI_Shutdown();
}


int
SteamworksProvider::Open( lua_State *L )
{
    const char *name = lua_tostring( L, 1 ); CORONA_ASSERT( 0 == strcmp( name, kName ) );
    int result = CoronaLibraryProviderNew( L, "gameNetwork", name, kPublisherId );
    
    if ( result )
    {
        const luaL_Reg kFunctions[] =
        {
            { "init", init },
            { "show", show },
            { "request", request },
            
            { NULL, NULL }
        };
        
        CoronaLuaInitializeGCMetatable( L, kName, Finalizer );
        
        // Use 'provider' in closure for kFunctions
        Self *provider = new Self( L );
        CoronaLuaPushUserdata( L, provider, kName );
        luaL_openlib( L, NULL, kFunctions, 1 );
    }
    
    return result;
}

int
SteamworksProvider::Finalizer( lua_State *L )
{
    Self *library = (Self *)CoronaLuaToUserdata( L, 1 );
    
    CoronaLuaDeleteRef( L, library->GetListener() );
    
    delete library;
    
    return 0;
}

SteamworksProvider *
SteamworksProvider::ToLibrary( lua_State *L )
{
    // library is pushed as part of the closure
    Self *library = (Self *)CoronaLuaToUserdata( L, lua_upvalueindex( 1 ) );
    return library;
}

void
SteamworksProvider::Dispatch( bool isError )
{
    if ( CORONA_VERIFY( fListener ) && CORONA_VERIFY( _L ))
    {
        lua_pushstring( _L, kProviderName );
        lua_setfield( _L, -2, "provider" );
        
        lua_pushboolean( _L, isError );
        lua_setfield( _L, -2, "isError" );
        
        CoronaLuaDispatchEvent( _L, fListener, 0 );
    }
}

int
SteamworksProvider::Init(lua_State *L)
{    
    CORONA_ASSERT( 0 == strcmp( kProviderName == lua_tostring( L, 1 ) ) );
    
    bool result = false;
    if ( !fListener )
    {
        int index = 2;
        if ( CoronaLuaIsListener( L, index, kProviderName ) )
        {
            result = SteamAPI_Init();
            if (result){
                CoronaLuaRef listener = CoronaLuaNewRef( L, index );
                fListener = listener;
            }else{
                CoronaLuaWarning(L, "steamAPI failed to initialize");
            }
        }
        else
        {
            // Error: Listener is required
            CoronaLuaError(L, "gameNetwork.init(): Expected argument %d to be a listener for %s events.\n", index, kProviderName);
		}
    }
    else
    {
        CoronaLuaWarning(L, "gameNetwork.init(): This function has already been called. It should only be called once.\n");
    }
    lua_pushboolean(L, result);
    
    return 1;
}

int
SteamworksProvider::Show(lua_State *L)
{
	return 0;
}

void stackdump_g(lua_State* l)
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


int
SteamworksProvider::Request(lua_State *L)
{
    int result = 0;
    if (lua_type(L, 1) != LUA_TSTRING){
        CoronaLuaError(L, "gameNetwork.request() argument 1 must be a string");
        return 0;
    }
    const char *command = lua_tostring(L, 1);
    
    if ( !fListener && 0 != strcmp(command, "isSteamRunning") && 0 != strcmp(command, "restartAppIfNecessary") && 0 != strcmp(command, "runCallbacks") )
    {
        CoronaLuaWarning(L, "Steam API is not initialized. Call gameNetwork.init()\n");
        return 0;
    }
    bool options = false;
    
    if( ( lua_type( L, -1 ) == LUA_TTABLE ) ) // look for the options inside the table
    {
        options = true;
    }
    //gameNetwork.request("isSteamRunning")
    if (0 == strcmp(command, "isSteamRunning")){
        lua_pushboolean(L, SteamAPI_IsSteamRunning()? 1 : 0);
        result = 1;
    }
    //gameNetwork.request("restartAppIfNecessary", {appID = 12345890})
    else if (0 == strcmp(command, "restartAppIfNecessary")){
        int appID;
        if (options){
            lua_getfield(L, -1, "appID");
            if (!lua_isnil(L, -1)){
                appID = lua_tonumber(L, -1);
                lua_pop(L, 1);
            }else{
                lua_pop(L, 1);
                CoronaLuaError(L, "gameNetwork.request(\"restartAppIfNecessary\", params): Specify appID in params table.");
                return 0;
            }
        }else if (lua_isnumber(L, 2)){
            appID = lua_tonumber(L, 2);
        }else{
            CoronaLuaError(L, "gameNetwork.request(\"restartAppIfNecessary\", params): Specify appID in params table.");
            return 0;
        }
        lua_pushboolean(L, SteamAPI_RestartAppIfNecessary(appID));
        result = 1;
    }
    //gameNetwork.request("runCallbacks")
    else if (0 == strcmp(command, "runCallbacks")){
        SteamAPI_RunCallbacks();
    }
    //gameNetwork.request("requestStats")
    //gameNetwork.request("requestStats", {user = 123456789})
    else if (0 == strcmp(command, "requestStats")){
        uint64 user = NULL;
        if (options){
            lua_getfield(L, -1, "user");
            if (!lua_isnil(L, -1)){
                user = lua_tonumber(L, -1);
            }
            lua_pop(L, 1);
        }else if (lua_isnumber(L, 2)){
            user = lua_tonumber(L, 2);
        }
        if (user){
            lua_pushboolean(L, SteamUserStats()->RequestUserStats(CSteamID(user))? 1 : 0);
        }else{
            lua_pushboolean(L, SteamUserStats()->RequestCurrentStats()? 1 : 0);
        }
        result = 1;
    }
    //gameNetwork.request("storeStats")
    else if (0 == strcmp(command, "storeStats")){
        lua_pushboolean(L, SteamUserStats()->StoreStats());
        result = 1;
    }
    //gameNetwork.request("getStat", {name = "name"})
    //gameNetwork.request("getStat", {name = "name", user = 123456789})
    else if (0 == strcmp(command, "getStat")){
        const char *name = NULL;
        uint64 user = NULL;
        float data = 0.0;
        if (options){
            lua_getfield(L, -1, "name");
            if (!lua_isnil(L, -1)){
                name = lua_tostring(L, -1);
            }
            lua_pop(L, 1);
            lua_getfield(L, -1, "user");
            if (!lua_isnil(L, -1)){
                user = lua_tonumber(L, -1);
            }
            lua_pop(L, 1);
        }else if (lua_isstring(L, 2)){
            name = lua_tostring(L, 2);
        }
        if (!name){
            CoronaLuaError(L, "gameNetwork.request(\"getStat\", params): Specify name in params table.");
            return 0;
        }
        bool success;
        if (user){
            success = SteamUserStats()->GetUserStat(CSteamID(user), name, &data);
        }else{
            success = SteamUserStats()->GetStat(name, &data);
        }
        if (success){
            lua_pushnumber(L, data);
        }else{
            lua_pushnil( L );
        }
        result = 1;
    }
    //gameNetwork.request("setStat", {name = "name", data = 123456})
    //gameNetwork.request("setStat", {name = "name", type = "average", data = 12345, time = 60})
    else if (0 == strcmp(command, "setStat")){
        if (!options){
            CoronaLuaError(L, "gameNetwork.request(\"setStat\", params): Specify name and data in params table.");
            return 0;
        }
        const char *name = NULL;
        float data = NULL;
        const char *type = "";
        double time = NULL;
        lua_getfield(L, -1, "name");
        if (!lua_isnil(L, -1)){
            name = lua_tostring(L, -1);
        }
        lua_pop(L, 1);
        lua_getfield(L, -1, "data");
        if (!lua_isnil(L, -1)){
            data = lua_tonumber(L, -1);
        }
        lua_pop(L, 1);
        lua_getfield(L, -1, "type");
        if (!lua_isnil(L, -1)){
            type = lua_tostring(L, -1);
        }
        lua_pop(L, 1);
        lua_getfield(L, -1, "time");
        if (!lua_isnil(L, -1)){
            time = lua_tonumber(L, -1);
        }
        lua_pop(L, 1);
        if (!name || !data){
            CoronaLuaError(L, "gameNetwork.request(\"setStat\", params): Specify name and data in params table.");
            return 0;
        }
        
        if (0 == strcmp(type, "average") && time){
            lua_pushboolean(L, SteamUserStats()->UpdateAvgRateStat(name, data, time)? 1 : 0);
        }else{
            lua_pushboolean(L, SteamUserStats()->SetStat(name, data)? 1 : 0);
        }
        result = 1;
        
    }
    //gameNetwork.request("getAchievement", {name = "name"})
    //gameNetwork.request("getAchievement", {name = "name", user = 1234567890})
    else if (0 == strcmp(command, "getAchievement")){
        const char *name = NULL;
        uint64 user = NULL;
        bool achieved = false;
        if (options){
            lua_getfield(L, -1, "name");
            if (!lua_isnil(L, -1)){
                name = lua_tostring(L, -1);
            }
            lua_pop(L, 1);
            lua_getfield(L, -1, "user");
            if (!lua_isnil(L, -1)){
                user = lua_tonumber(L, -1);
            }
            lua_pop(L, 1);
        }else if (lua_isstring(L, 2)){
            name = lua_tostring(L, 2);
        }
        if (!name){
            CoronaLuaError(L, "gameNetwork.request(\"getAchievement\", params): Specify name in params table.");
            return 0;
        }
        bool success;
        if (user){
            success = SteamUserStats()->GetUserAchievement(CSteamID(user), name, &achieved);
        }else{
            success = SteamUserStats()->GetAchievement(name, &achieved);
        }
        if (success){
            lua_pushboolean(L, achieved? 1 : 0);
        }else{
            lua_pushnil(L);
        }
        
        result = 1;
        
    }
    //gameNetwork.request("getAchievementProperty", {name = "name", type = "unlockTime"})
    //gameNetwork.request("getAchievementProperty", {name = "name", type = "unlockTime", user = 1234567890})
    //gameNetwork.request("getAchievementProperty", {name = "name", type = "icon"})
    //gameNetwork.request("getAchievementProperty", {name = "name", type = "name"})
    //gameNetwork.request("getAchievementProperty", {name = "name", type = "desc"})
    //gameNetwork.request("getAchievementProperty", {name = "name", type = "hidden"})
    else if (0 == strcmp(command, "getAchievementProperty")){
        if (!options){
            CoronaLuaError(L, "gameNetwork.request(\"getAchievementProperty\", params): Specify name and type in params table.");
            return 0;
        }
        const char *name = NULL;
        const char *type = "";
        uint64 user = NULL;
        lua_getfield(L, -1, "name");
        if (!lua_isnil(L, -1)){
            name = lua_tostring(L, -1);
        }
        lua_pop(L, 1);
        lua_getfield(L, -1, "type");
        if (!lua_isnil(L, -1)){
            type = lua_tostring(L, -1);
        }
        lua_pop(L, 1);
        lua_getfield(L, -1, "user");
        if (!lua_isnil(L, -1)){
            user = lua_tonumber(L, -1);
        }
        lua_pop(L, 1);
        
        if (!name || !type){
            CoronaLuaError(L, "gameNetwork.request(\"getAchievementProperty\", params): Specify name and type in params table.");
            return 0;
        }

        // TODO: Unlock time table
        if (0 == strcmp(type, "unlockTime") && user){
            bool achieved = false;
            uint32 unlockTime = 0;
            bool success = SteamUserStats()->GetUserAchievementAndUnlockTime(CSteamID(user), name, &achieved, &unlockTime);
            if (achieved){
                lua_pushnumber(L, unlockTime);
                result = 1;
            }else if(success){
                lua_pushboolean(L, achieved? 1 : 0);
                result = 1;
            }else{
                result = 0;
            }
        }else if (0 == strcmp(type, "unlockTime")){
            bool achieved = false;
            uint32 unlockTime = 0;
            bool success = SteamUserStats()->GetAchievementAndUnlockTime(name, &achieved, &unlockTime);
            if (achieved){
                lua_pushnumber(L, unlockTime);
                result = 1;
            }else if(success){
                lua_pushboolean(L, achieved? 1 : 0);
                result = 1;
            }else{
                result = 0;
            }
        }else if (0 == strcmp(type, "icon")){
            //TODO
            int icon = SteamUserStats()->GetAchievementIcon(name);
            lua_pushnumber(L, icon);
            result = 1;
        }else if (0 == strcmp(type, "name") || 0 == strcmp(type, "desc") || 0 == strcmp(type, "hidden")){
            //Hidden doesn't return anything
            const char *attribute = SteamUserStats()->GetAchievementDisplayAttribute(name, type);
            lua_pushstring(L, attribute);
            result = 1;
        }else{
            CoronaLuaError(L, "gameNetwork.request(\"getAchievementProperty\", params): Specify name and type in params table.");
            return 0;
        }
    }
    //gameNetwork.request("setAchievement", {name = "name"})
    //gameNetwork.request("setAchievement", {name = "name", type = "progress", currProgress = 75, maxProgress = 100})
    else if (0 == strcmp(command, "setAchievement")){
        if (!options && (lua_isstring(L, 2))){
            lua_pushboolean(L, SteamUserStats()->SetAchievement(lua_tostring(L, 2))? 1 : 0);
        }else if(!options){
            CoronaLuaError(L, "gameNetwork.request(\"setAchievement\", params): Specify name in params table.");
            return 0;
        }else{
            const char *name = NULL;
            const char *type = "";
            uint32 currProgress = NULL;
            uint32 maxProgress = NULL;
            
            lua_getfield(L, -1, "name");
            if (!lua_isnil(L, -1)){
                name = lua_tostring(L, -1);
            }
            lua_pop(L, 1);
            lua_getfield(L, -1, "type");
            if (!lua_isnil(L, -1)){
                type = lua_tostring(L, -1);
            }
            lua_pop(L, 1);
            lua_getfield(L, -1, "currProgress");
            if (!lua_isnil(L, -1)){
                currProgress = lua_tonumber(L, -1);
            }
            lua_pop(L, 1);
            lua_getfield(L, -1, "maxProgress");
            if (!lua_isnil(L, -1)){
                maxProgress = lua_tonumber(L, -1);
            }
            lua_pop(L, 1);
            
            if (!name){
                CoronaLuaError(L, "gameNetwork.request(\"setAchievement\", params): Specify name in params table.");
                return 0;
            }
            if (0 == strcmp(type, "progress") && currProgress && maxProgress){
                lua_pushboolean(L, SteamUserStats()->IndicateAchievementProgress(name, currProgress, maxProgress)? 1 : 0);
            }else{
                lua_pushboolean(L, SteamUserStats()->SetAchievement(name)? 1 : 0);
            }
        }
        result = 1;
    }
    //gameNetwork.request("clearAchievement", {name = "name"})
    else if (0 == strcmp(command, "clearAchievement")){
        const char *name;
        if (options){
            lua_getfield(L, -1, "name");
            if (!lua_isnil(L, -1)){
                name = lua_tostring(L, -1);
                lua_pop(L, 1);
            }else{
                lua_pop(L, 1);
                CoronaLuaError(L, "gameNetwork.request(\"clearAchievement\", params): Specify name in params table.");
                return 0;
            }
        }else if (lua_isstring(L, 2)){
            name = lua_tostring(L, 2);
        }else{
            CoronaLuaError(L, "gameNetwork.request(\"clearAchievement\", params): Specify name in params table.");
            return 0;
        }
        lua_pushboolean(L, SteamUserStats()->ClearAchievement(name)? 1 : 0);
        result = 1;
    }
    //gameNetwork.request("getNumAchievements")
    else if (0 == strcmp(command, "getNumAchievements")){
        
        lua_pushnumber(L, SteamUserStats()->GetNumAchievements());
        result = 1;
    }
    //gameNetwork.request("getAchievementName", {i=1})
    else if (0 == strcmp(command, "getAchievementName")){
        int i;
        if (options){
            lua_getfield(L, -1, "i");
            if (!lua_isnil(L, -1)){
                i = lua_tonumber(L, -1);
                lua_pop(L, 1);
            }else{
                lua_pop(L, 1);
                CoronaLuaError(L, "gameNetwork.request(\"getAchievementName\", params): Specify i in params table.");
                return 0;
            }
        }else if (lua_isnumber(L, 2)){
            i = lua_tonumber(L, 2);
        }else{
            CoronaLuaError(L, "gameNetwork.request(\"getAchievementName\", params): Specify i in params table.");
            return 0;
        }
        lua_pushstring(L, SteamUserStats()->GetAchievementName(--i)); //adjust to Corona index counting (starts with 1 not 0)
        result = 1;
    }
    //gameNetwork.request("resetAllStats", {resetAchievements = true})
    else if (0 == strcmp(command, "resetAllStats")){
        bool resetAchievements = false;
        if (options){
            lua_getfield(L, -1, "resetAchievements");
            if (!lua_isnil(L, -1)){
                resetAchievements = lua_toboolean(L, -1)? true : false;
            }
            lua_pop(L, 1);
        }else if (lua_isboolean(L, 2)){
            resetAchievements = lua_toboolean(L, 2)? true : false;
        }
        lua_pushboolean(L, SteamUserStats()->ResetAllStats(resetAchievements)? 1 : 0);
        result = 1;
    }
    //gameNetwork.request("findOrCreateLeaderboard", {name = "Quickest Win", sortMethod = "ascending", displayType = "numeric"})
    else if (0 == strcmp(command, "findOrCreateLeaderboard")){
        if (!options && (lua_isstring(L, 2))){
            SteamAPICall_t hSteamAPICall = SteamUserStats()->FindOrCreateLeaderboard( lua_tostring(L, 2), k_ELeaderboardSortMethodNone, k_ELeaderboardDisplayTypeNone );
            m_callResultFindLeaderboard.Set( hSteamAPICall, this, &SteamworksProvider::OnLeaderboardFindResult );
            lua_pushboolean(L, hSteamAPICall ? 1 : 0);
        }else if(!options){
            CoronaLuaError(L, "gameNetwork.request(\"findOrCreateLeaderboard\", params): Specify name in params table.");
            return 0;
        }else{
            const char *name = NULL;
            const char *sortMethod = "";
            const char *displayType = "";
            ELeaderboardSortMethod leaderboardSortMethod = k_ELeaderboardSortMethodNone;
            ELeaderboardDisplayType leaderboardDisplayType = k_ELeaderboardDisplayTypeNone;
            
            lua_getfield(L, -1, "name");
            if (!lua_isnil(L, -1)){
                name = lua_tostring(L, -1);
            }
            lua_pop(L, 1);
            
            lua_getfield(L, -1, "sortMethod");
            if (!lua_isnil(L, -1)){
                sortMethod = lua_tostring(L, -1);
            }
            lua_pop(L, 1);
            
            lua_getfield(L, -1, "displayType");
            if (!lua_isnil(L, -1)){
                displayType = lua_tostring(L, -1);
            }
            lua_pop(L, 1);
            
            if (!name){
                CoronaLuaError(L, "gameNetwork.request(\"findOrCreateLeaderboard\", params): Specify name in params table.");
                return 0;
            }
            if (0 == strcmp(sortMethod, "ascending")){
                leaderboardSortMethod = k_ELeaderboardSortMethodAscending;
            }else if (0 == strcmp(sortMethod, "descending")){
                leaderboardSortMethod = k_ELeaderboardSortMethodDescending;
            }
            
            if (0 == strcmp(displayType, "numeric")){
                leaderboardDisplayType = k_ELeaderboardDisplayTypeNumeric;
            }else if (0 == strcmp(displayType, "timeSeconds")){
                leaderboardDisplayType = k_ELeaderboardDisplayTypeTimeSeconds;
            }else if (0 == strcmp(displayType, "timeMilliSeconds")){
                leaderboardDisplayType = k_ELeaderboardDisplayTypeTimeMilliSeconds;
            }
            SteamAPICall_t hSteamAPICall = SteamUserStats()->FindOrCreateLeaderboard( name, leaderboardSortMethod, leaderboardDisplayType );
            m_callResultFindLeaderboard.Set( hSteamAPICall, this, &SteamworksProvider::OnLeaderboardFindResult );
            
            lua_pushboolean(L, hSteamAPICall? 1 : 0);
            result = 1;
        }

    }
    //gameNetwork.request("findLeaderboard", {name = "Quickest Win"})
    else if (0 == strcmp(command, "findLeaderboard")){
        const char *name;
        if (options){
            lua_getfield(L, -1, "name");
            if (!lua_isnil(L, -1)){
                name = lua_tostring(L, -1);
                lua_pop(L, 1);
            }else{
                lua_pop(L, 1);
                CoronaLuaError(L, "gameNetwork.request(\"findLeaderboard\", params): Specify name in params table.");
                return 0;
            }
        }else if (lua_isstring(L, 2)){
            name = lua_tostring(L, 2);
        }else{
            CoronaLuaError(L, "gameNetwork.request(\"findLeaderboard\", params): Specify name in params table.");
            return 0;
        }
        SteamAPICall_t hSteamAPICall = SteamUserStats()->FindLeaderboard( name );
        m_callResultFindLeaderboard.Set( hSteamAPICall, this, &SteamworksProvider::OnLeaderboardFindResult );
        
        lua_pushboolean(L, hSteamAPICall? 1 : 0);
        result = 1;
    }
    //gameNetwork.request("getLeaderboardName", {leaderboard = 123456})
    else if (0 == strcmp(command, "getLeaderboardName")){
        SteamLeaderboard_t leaderboard;
        if (options){
            lua_getfield(L, -1, "leaderboard");
            if (!lua_isnil(L, -1)){
                leaderboard = lua_tonumber(L, -1);
                lua_pop(L, 1);
            }else{
                lua_pop(L, 1);
                CoronaLuaError(L, "gameNetwork.request(\"getLeaderboardName\", params): Specify leaderboard in params table.");
                return 0;
            }
        }else if (lua_isnumber(L, 2)){
            leaderboard = lua_tonumber(L, 2);
        }else{
            CoronaLuaError(L, "gameNetwork.request(\"getLeaderboardName\", params): Specify leaderboard in params table.");
            return 0;
        }
        lua_pushstring(L, SteamUserStats()->GetLeaderboardName( leaderboard ));
        
        result = 1;
    }
    //gameNetwork.request("getLeaderboardEntryCount", {leaderboard = 123456})
    else if (0 == strcmp(command, "getLeaderboardEntryCount")){
        SteamLeaderboard_t leaderboard;
        if (options){
            lua_getfield(L, -1, "leaderboard");
            if (!lua_isnil(L, -1)){
                leaderboard = lua_tonumber(L, -1);
                lua_pop(L, 1);
            }else{
                lua_pop(L, 1);
                CoronaLuaError(L, "gameNetwork.request(\"getLeaderboardEntryCount\", params): Specify leaderboard in params table.");
                return 0;
            }
        }else if (lua_isnumber(L, 2)){
            leaderboard = lua_tonumber(L, 2);
        }else{
            CoronaLuaError(L, "gameNetwork.request(\"getLeaderboardEntryCount\", params): Specify leaderboard in params table.");
            return 0;
        }
        lua_pushnumber(L, SteamUserStats()->GetLeaderboardEntryCount( leaderboard ));
        
        result = 1;
    }
    //gameNetwork.request("getLeaderboardSortMethod", {leaderboard = 123456})
    else if (0 == strcmp(command, "getLeaderboardSortMethod")){
        SteamLeaderboard_t leaderboard;
        if (options){
            lua_getfield(L, -1, "leaderboard");
            if (!lua_isnil(L, -1)){
                leaderboard = lua_tonumber(L, -1);
                lua_pop(L, 1);
            }else{
                lua_pop(L, 1);
                CoronaLuaError(L, "gameNetwork.request(\"getLeaderboardSortMethod\", params): Specify leaderboard in params table.");
                return 0;
            }
        }else if (lua_isnumber(L, 2)){
            leaderboard = lua_tonumber(L, 2);
        }else{
            CoronaLuaError(L, "gameNetwork.request(\"getLeaderboardSortMethod\", params): Specify leaderboard in params table.");
            return 0;
        }
        ELeaderboardSortMethod sortMethod = SteamUserStats()->GetLeaderboardSortMethod( leaderboard );
        if (sortMethod == k_ELeaderboardSortMethodAscending){
            lua_pushstring(L, "ascending");
        }else if (sortMethod == k_ELeaderboardSortMethodDescending){
            lua_pushstring(L, "descending");
        }else if (sortMethod == k_ELeaderboardSortMethodNone){
            lua_pushstring(L, "none");
        }else{
            lua_pushstring(L, "unknown");
        }
        
        result = 1;
    }
    //gameNetwork.request("getLeaderboardDisplayType", {leaderboard = 123456})
    else if (0 == strcmp(command, "getLeaderboardDisplayType")){
        SteamLeaderboard_t leaderboard;
        if (options){
            lua_getfield(L, -1, "leaderboard");
            if (!lua_isnil(L, -1)){
                leaderboard = lua_tonumber(L, -1);
                lua_pop(L, 1);
            }else{
                lua_pop(L, 1);
                CoronaLuaError(L, "gameNetwork.request(\"getLeaderboardDisplayType\", params): Specify leaderboard in params table.");
                return 0;
            }
        }else if (lua_isnumber(L, 2)){
            leaderboard = lua_tonumber(L, 2);
        }else{
            CoronaLuaError(L, "gameNetwork.request(\"getLeaderboardDisplayType\", params): Specify leaderboard in params table.");
            return 0;
        }
        ELeaderboardDisplayType displayType = SteamUserStats()->GetLeaderboardDisplayType( leaderboard );
        if (displayType == k_ELeaderboardDisplayTypeNumeric){
            lua_pushstring(L, "numeric");
        }else if (displayType == k_ELeaderboardDisplayTypeTimeSeconds){
            lua_pushstring(L, "timeSeconds");
        }else if (displayType == k_ELeaderboardDisplayTypeTimeMilliSeconds){
            lua_pushstring(L, "timeMilliSeconds");
        }else if (displayType == k_ELeaderboardDisplayTypeNone){
            lua_pushstring(L, "none");
        }else{
            lua_pushstring(L, "unknown");
        }
        
        result = 1;
    }
    //gameNetwork.request("downloadLeaderboardEntries", {leaderboard = 12345, dataRequest = "global", rangeStart = 1, rangeEnd = 12})
    else if (0 == strcmp(command, "downloadLeaderboardEntries")){
        if(!options){
            CoronaLuaError(L, "gameNetwork.request(\"downloadLeaderboardEntries\", params): Specify leaderboard, dataRequest, rangeStart, rangeEnd in params table.");
            return 0;
        }else{
            SteamLeaderboard_t leaderboard;
            ELeaderboardDataRequest dataRequest = k_ELeaderboardDataRequestGlobal;
            const char *dataRequestString = "";
            int rangeStart = 0, rangeEnd = 0;
            
            lua_getfield(L, -1, "leaderboard");
            if (!lua_isnil(L, -1)){
                leaderboard = lua_tonumber(L, -1);
            }
            lua_pop(L, 1);
            
            lua_getfield(L, -1, "dataRequest");
            if (!lua_isnil(L, -1)){
                dataRequestString = lua_tostring(L, -1);
            }
            lua_pop(L, 1);
            
            lua_getfield(L, -1, "rangeStart");
            if (!lua_isnil(L, -1)){
                rangeStart = lua_tonumber(L, -1);
            }
            lua_pop(L, 1);
            
            lua_getfield(L, -1, "rangeEnd");
            if (!lua_isnil(L, -1)){
                rangeEnd = lua_tonumber(L, -1);
            }
            lua_pop(L, 1);
            
            if (0 == strcmp(dataRequestString, "global")){
                dataRequest = k_ELeaderboardDataRequestGlobal;
            }else if (0 == strcmp(dataRequestString, "globalAroundUser")){
                dataRequest = k_ELeaderboardDataRequestGlobalAroundUser;
            }else if (0 == strcmp(dataRequestString, "friends")){
                dataRequest = k_ELeaderboardDataRequestFriends;
            }
            
            if (!leaderboard || !rangeStart || !rangeEnd){
                CoronaLuaError(L, "gameNetwork.request(\"downloadLeaderboardEntries\", params): Specify leaderboard, dataRequest, rangeStart, rangeEnd in params table.");
                return 0;
            }

            SteamAPICall_t hSteamAPICall = SteamUserStats()->DownloadLeaderboardEntries(leaderboard, dataRequest, rangeStart, rangeEnd);
            m_callResultDownloadedLeaderboardScores.Set( hSteamAPICall, this, &SteamworksProvider::OnLeaderboardScoresDownloaded );
            
            lua_pushboolean(L, hSteamAPICall ? 1 : 0 );
            result = 1;
        }
        
    }
    //gameNetwork.request("downloadLeaderboardEntriesForUsers", {leaderboard = 12345, users = {123, 124, 125, 126})
    else if (0 == strcmp(command, "downloadLeaderboardEntriesForUsers")){
        if(!options){
            CoronaLuaError(L, "gameNetwork.request(\"downloadLeaderboardEntriesForUsers\", params): Specify leaderboard, users in params table.");
            return 0;
        }else{
            SteamLeaderboard_t leaderboard = 0;
            CSteamID *prgUsers = NULL;
            int count = 0;
            
            lua_getfield(L, -1, "leaderboard");
            if (!lua_isnil(L, -1)){
                leaderboard = lua_tonumber(L, -1);
            }
            lua_pop(L, 1);
            
            lua_getfield(L, -1, "users");
            if (lua_istable(L, -1)){
                count = (int)lua_objlen(L, -1);
                prgUsers = new CSteamID[count];
                int t = CoronaLuaNormalize(L, -1);
                lua_pushnil(L);  /* first key */
                int i = 0;
                while (lua_next(L, t) != 0) {
                    uint64 userID = lua_tonumber(L, -1);
                    prgUsers[i] = CSteamID(userID);
                    i++;
                    lua_pop(L, 1);
                }
            }
            lua_pop(L, 1);
            
            if (!leaderboard || !prgUsers || !count){
                CoronaLuaError(L, "gameNetwork.request(\"downloadLeaderboardEntriesForUsers\", params): Specify leaderboard, users in params table.");
                return 0;
            }
            
            SteamAPICall_t hSteamAPICall = SteamUserStats()->DownloadLeaderboardEntriesForUsers(leaderboard, prgUsers, count);
            m_callResultDownloadedLeaderboardScores.Set( hSteamAPICall, this, &SteamworksProvider::OnLeaderboardScoresDownloaded );
            
            lua_pushboolean(L, hSteamAPICall ? 1 : 0);
            result = 1;
        }
        
    }
    //gameNetwork.request("getDownloadedLeaderboardEntry", {entries = 12345, index = 1})
    else if (0 == strcmp(command, "getDownloadedLeaderboardEntry")){
        if(!options){
            CoronaLuaError(L, "gameNetwork.request(\"getDownloadedLeaderboardEntry\", params): Specify entries, index in params table.");
            return 0;
        }else{
            SteamLeaderboardEntries_t entries = 0;
            int index = -1;
            LeaderboardEntry_t leaderboardEntry;
            
            lua_getfield(L, -1, "entries");
            if (!lua_isnil(L, -1)){
                entries = lua_tonumber(L, -1);
            }
            lua_pop(L, 1);
            
            lua_getfield(L, -1, "index");
            if (!lua_isnil(L, -1)){
                index = lua_tonumber(L, -1);
                index--;
            }
            lua_pop(L, 1);
            
            if (!entries || index == -1 ){
                CoronaLuaError(L, "gameNetwork.request(\"downloadLeaderboardEntriesForUsers\", params): Specify entries, index in params table.");
                return 0;
            }
            
            bool success = SteamUserStats()->GetDownloadedLeaderboardEntry(entries, index, &leaderboardEntry, NULL, 0);
            
            if (success){
                lua_newtable(L);
                
                lua_pushnumber( L, leaderboardEntry.m_steamIDUser.ConvertToUint64() );
                lua_setfield( L, -2, "user" );
                
                lua_pushnumber( L, leaderboardEntry.m_nGlobalRank );
                lua_setfield( L, -2, "rank" );
                
                lua_pushnumber( L, leaderboardEntry.m_nScore );
                lua_setfield( L, -2, "score" );
                
                lua_pushnumber( L, leaderboardEntry.m_cDetails );
                lua_setfield( L, -2, "details" );
                
                //lua_pushnumber( L, leaderboardEntry.m_hUGC );
                //lua_setfield( L, -2, "UGC" );
                
            }else{
                lua_pushnil(L);
            }
            result = 1;
        }
        
    }
    //gameNetwork.request("uploadLeaderboardScore", {leaderboard = 12345, uploadScoreMethod = "keepBest", score = 1})
    else if (0 == strcmp(command, "uploadLeaderboardScore")){
        if(!options){
            CoronaLuaError(L, "gameNetwork.request(\"uploadLeaderboardScore\", params): Specify leaderboard, uploadScoreMethod, score in params table.");
            return 0;
        }else{
            SteamLeaderboard_t leaderboard = 0;
            ELeaderboardUploadScoreMethod uploadScoreMethod = k_ELeaderboardUploadScoreMethodNone;
            const char *uploadScoreMethodString = "";
            int32 score;
            
            lua_getfield(L, -1, "leaderboard");
            if (!lua_isnil(L, -1)){
                leaderboard = lua_tonumber(L, -1);
            }
            lua_pop(L, 1);
            
            lua_getfield(L, -1, "uploadScoreMethod");
            if (!lua_isnil(L, -1)){
                uploadScoreMethodString = lua_tostring(L, -1);
            }
            lua_pop(L, 1);
            
            lua_getfield(L, -1, "score");
            if (!lua_isnil(L, -1)){
                score = (int32)lua_tointeger(L, -1);
            }else{
                CoronaLuaError(L, "gameNetwork.request(\"uploadLeaderboardScore\", params): Specify leaderboard, uploadScoreMethod, score in params table.");
                return 0;
            }
            lua_pop(L, 1);

            
            if (0 == strcmp(uploadScoreMethodString, "keepBest")){
                uploadScoreMethod = k_ELeaderboardUploadScoreMethodKeepBest;
            }else if (0 == strcmp(uploadScoreMethodString, "forceUpdate")){
                uploadScoreMethod = k_ELeaderboardUploadScoreMethodForceUpdate;
            }
            
            if (!leaderboard){
                CoronaLuaError(L, "gameNetwork.request(\"uploadLeaderboardScore\", params): Specify leaderboard, uploadScoreMethod, score in params table.");
                return 0;
            }
            
            SteamAPICall_t hSteamAPICall = SteamUserStats()->UploadLeaderboardScore(leaderboard, uploadScoreMethod, score, NULL, 0);
            m_callResultUploadedLeaderboardScore.Set( hSteamAPICall, this, &SteamworksProvider::OnLeaderboardScoreUploaded );
            
            lua_pushboolean(L, hSteamAPICall ? 1 : 0);
            result = 1;
        }
        
    }
    //gameNetwork.request("getNumberOfCurrentPlayers")
    else if (0 == strcmp(command, "getNumberOfCurrentPlayers")){
        SteamAPICall_t hSteamAPICall = SteamUserStats()->GetNumberOfCurrentPlayers();
        m_callResultNumberOfCurrentPlayers.Set( hSteamAPICall, this, &SteamworksProvider::OnNumberOfCurrentPlayers );
        lua_pushboolean(L, hSteamAPICall ? 1 : 0);
        result = 1;
    }
    //TODO
    //gameNetwork.request("requestGlobalAchievementPercentages")
    else if (0 == strcmp(command, "requestGlobalAchievementPercentages")){
        SteamAPICall_t hSteamAPICall = SteamUserStats()->RequestGlobalAchievementPercentages();
        m_callResultGlobalAchievementPercentagesReady.Set( hSteamAPICall, this, &SteamworksProvider::OnGlobalAchievementPercentagesReady );
        lua_pushboolean(L, hSteamAPICall? 1 : 0);
        result = 1;
    }
    //TODO
    //gameNetwork.request("getMostAchievedAchievementInfo")
    else if (0 == strcmp(command, "getMostAchievedAchievementInfo")){
         char *name = new char[10];
         float percent;
         bool achieved;
         int achievementIndex = SteamUserStats()->GetMostAchievedAchievementInfo(name, sizeof(name), &percent, &achieved);
         if (achievementIndex == -1){
             lua_pushboolean(L, false);
         }else{
             lua_pushnumber(L, achievementIndex);
         }
         result = 1;
    }
    //gameNetwork.request("requestGlobalStats", {historyDays = 60})
    else if (0 == strcmp(command, "requestGlobalStats")){
        int historyDays = 0;
        if (options){
            lua_getfield(L, -1, "historyDays");
            if (!lua_isnil(L, -1)){
                historyDays = lua_tonumber(L, -1);
            }
            lua_pop(L, 1);
        }else if (lua_isnumber(L, 2)){
            historyDays = lua_tonumber(L, 2);
        }
        if (historyDays > 60) historyDays = 60;
        SteamAPICall_t hSteamAPICall = SteamUserStats()->RequestGlobalStats(historyDays);
        m_callResultReceivedGlobalStats.Set( hSteamAPICall, this, &SteamworksProvider::OnGlobalStatsReceived );
        
        lua_pushboolean(L, hSteamAPICall ? 1 : 0);
        result = 1;
    }
    //gameNetwork.request("getGlobalStat", {name = "name"})
    else if (0 == strcmp(command, "getGlobalStat")){
        const char *name;
        if (options){
            lua_getfield(L, -1, "name");
            if (!lua_isnil(L, -1)){
                name = lua_tostring(L, -1);
                lua_pop(L, 1);
            }else{
                lua_pop(L, 1);
                CoronaLuaError(L, "gameNetwork.request(\"getGlobalStat\", params): Specify name in params table.");
                return 0;
            }
        }else if (lua_isstring(L, 2)){
            name = lua_tostring(L, 2);
        }else{
            CoronaLuaError(L, "gameNetwork.request(\"getGlobalStat\", params): Specify name in params table.");
            return 0;
        }
        double dData;
        int64 iData;
        bool iSuccess = SteamUserStats()->GetGlobalStat( name, &iData );
        bool dSuccess = SteamUserStats()->GetGlobalStat( name, &dData );
        
        if (dSuccess){
            lua_pushnumber(L, dData);
        }else if (iSuccess){
            lua_pushnumber(L, iData);
        }else{
            lua_pushnil( L );
        }
        
        result = 1;
    }
    //gameNetwork.request("getGlobalStatHistory", {name = "name", days = 15})
    else if (0 == strcmp(command, "getGlobalStatHistory")){
        if(!options){
            CoronaLuaError(L, "gameNetwork.request(\"getGlobalStatHistory\", params): Specify name, days in params table.");
            return 0;
        }else{
            const char *name = "";
            int days = 0;
            
            lua_getfield(L, -1, "name");
            if (!lua_isnil(L, -1)){
                name = lua_tostring(L, -1);
            }
            lua_pop(L, 1);
            
            lua_getfield(L, -1, "days");
            if (!lua_isnil(L, -1)){
                days = lua_tonumber(L, -1);
            }
            lua_pop(L, 1);
            
            if (!name || !days ){
                CoronaLuaError(L, "gameNetwork.request(\"getGlobalStatHistory\", params): Specify name, days in params table.");
                return 0;
            }
            
            int64 *iData = new int64[days];
            double *dData = new double[days];
            
            int32 iCount = SteamUserStats()->GetGlobalStatHistory( name, iData, days*sizeof(int64) );
            int32 dCount = SteamUserStats()->GetGlobalStatHistory( name, dData, days*sizeof(int64) );
            
            if (iCount){
                lua_newtable(L);
                for (int i = 0; i<iCount; i++) {
                    lua_pushnumber( L, iData[i] );
                    char field[32];
                    _snprintf(field, 32, "%i", i);
                    lua_setfield( L, -2, field );
                }
            }else if (dCount){
                lua_newtable(L);
                for (int i = 0; i<dCount; i++) {
                    lua_pushnumber( L, dData[i] );
					char field[32];
                    _snprintf(field, 32, "%i", i);
                    lua_setfield( L, -2, field );
                }
            }else{
                lua_pushnil(L);
            }
            result = 1;
        }
        
    }
	return result;
}




// -------------------------
// Callbacks
// -------------------------

void
SteamworksProvider::OnUserStatsReceived( UserStatsReceived_t *pCallback )
{
    CoronaLuaNewEvent(_L, kEventName);
    
    lua_pushstring(_L, "onUserStatsReceived");
    lua_setfield(_L, -2, "type");
    
    lua_pushnumber(_L, pCallback->m_nGameID);
    lua_setfield(_L, -2, "gameID");
    
    lua_pushnumber(_L, pCallback->m_eResult);
    lua_setfield(_L, -2, "result");
    
    lua_pushnumber(_L, pCallback->m_steamIDUser.ConvertToUint64());
    lua_setfield(_L, -2, "user");
    
    Dispatch(pCallback->m_eResult!=k_EResultOK);
}

void
SteamworksProvider::OnUserStatsStored( UserStatsStored_t *pCallback)
{
    CoronaLuaNewEvent(_L, kEventName);
    
    lua_pushstring(_L, "onUserStatsStored");
    lua_setfield(_L, -2, "type");
    
    lua_pushnumber(_L, pCallback->m_nGameID);
    lua_setfield(_L, -2, "gameID");
    
    lua_pushnumber(_L, pCallback->m_eResult);
    lua_setfield(_L, -2, "result");
    
    Dispatch(pCallback->m_eResult!=k_EResultOK);
}
void
SteamworksProvider::OnUserAchievementStored(UserAchievementStored_t *pCallback)
{
    CoronaLuaNewEvent(_L, kEventName);
    
    lua_pushstring(_L, "onUserAchievementStored");
    lua_setfield(_L, -2, "type");
    
    lua_pushnumber(_L, pCallback->m_nGameID);
    lua_setfield(_L, -2, "gameID");
    
    lua_pushboolean(_L, pCallback->m_bGroupAchievement);
    lua_setfield(_L, -2, "groupAchievement");
    
    lua_pushstring(_L, pCallback->m_rgchAchievementName);
    lua_setfield(_L, -2, "name");
    
    lua_pushnumber(_L, pCallback->m_nCurProgress);
    lua_setfield(_L, -2, "currProgress");
    
    lua_pushnumber(_L, pCallback->m_nMaxProgress);
    lua_setfield(_L, -2, "maxProgress");
    
    Dispatch(false);
}

void
SteamworksProvider::OnLeaderboardFindResult(LeaderboardFindResult_t *pResult, bool bIOFailure)
{
    CoronaLuaNewEvent(_L, kEventName);
    
    lua_pushstring(_L, "onLeaderboardFindResult");
    lua_setfield(_L, -2, "type");
    
    lua_pushnumber(_L, pResult->m_hSteamLeaderboard);
    lua_setfield(_L, -2, "steamLeaderboard");
    
    lua_pushboolean(_L, pResult->m_bLeaderboardFound);
    lua_setfield(_L, -2, "leaderboardFound");
    
    Dispatch(bIOFailure || !pResult->m_hSteamLeaderboard);
}


void
SteamworksProvider::OnLeaderboardScoresDownloaded(LeaderboardScoresDownloaded_t *pResult, bool bIOFailure)
{
    CoronaLuaNewEvent(_L, kEventName);
    
    lua_pushstring(_L, "onLeaderboardScoresDownloaded");
    lua_setfield(_L, -2, "type");
    
    lua_pushnumber(_L, pResult->m_hSteamLeaderboard);
    lua_setfield(_L, -2, "steamLeaderboard");
    
    lua_pushnumber(_L, pResult->m_hSteamLeaderboardEntries);
    lua_setfield(_L, -2, "leaderboardEntries");
    
    lua_pushnumber(_L, pResult->m_cEntryCount);
    lua_setfield(_L, -2, "count");
    
    Dispatch(bIOFailure);
}


void
SteamworksProvider::OnLeaderboardScoreUploaded(LeaderboardScoreUploaded_t *pResult, bool bIOFailure)
{
    CoronaLog("OnLeaderboardScoreUploaded");
    CoronaLuaNewEvent(_L, kEventName);
    
    lua_pushstring(_L, "onLeaderboardScoreUploaded");
    lua_setfield(_L, -2, "type");
    
    lua_pushboolean(_L, pResult->m_bSuccess);
    lua_setfield(_L, -2, "success");
    
    lua_pushnumber(_L, pResult->m_hSteamLeaderboard);
    lua_setfield(_L, -2, "steamLeaderboard");
    
    lua_pushnumber(_L, pResult->m_nScore);
    lua_setfield(_L, -2, "score");
    
    lua_pushboolean(_L, pResult->m_bScoreChanged);
    lua_setfield(_L, -2, "scoreChanged");
    
    lua_pushnumber(_L, pResult->m_nGlobalRankNew);
    lua_setfield(_L, -2, "globalRankNew");
    
    lua_pushnumber(_L, pResult->m_nGlobalRankPrevious);
    lua_setfield(_L, -2, "globalRankPrevious");
    
    Dispatch(!pResult->m_bSuccess || bIOFailure);
    
}


void
SteamworksProvider::OnNumberOfCurrentPlayers(NumberOfCurrentPlayers_t *pResult, bool bIOFailure)
{
    CoronaLuaNewEvent(_L, kEventName);
    
    lua_pushstring(_L, "onNumberOfCurrentPlayers");
    lua_setfield(_L, -2, "type");
    
    lua_pushboolean(_L, pResult->m_bSuccess);
    lua_setfield(_L, -2, "success");
    
    lua_pushnumber(_L, pResult->m_cPlayers);
    lua_setfield(_L, -2, "players");

    Dispatch(bIOFailure || !pResult->m_bSuccess);
}


void
SteamworksProvider::OnUserStatsUnloaded(UserStatsUnloaded_t *pCallback)
{
    CoronaLuaNewEvent(_L, kEventName);
    
    lua_pushstring(_L, "onUserStatsUnloaded");
    lua_setfield(_L, -2, "type");
    
    lua_pushnumber(_L, pCallback->m_steamIDUser.ConvertToUint64());
    lua_setfield(_L, -2, "user");
    
    Dispatch(false);
}


void
SteamworksProvider::OnUserAchievementIconFetched(UserAchievementIconFetched_t *pCallback)
{
    CoronaLuaNewEvent(_L, kEventName);
    
    lua_pushstring(_L, "onUserAchievementIconFetched");
    lua_setfield(_L, -2, "type");
    
    lua_pushnumber(_L, pCallback->m_nGameID.ToUint64());
    lua_setfield(_L, -2, "gameID");
    
    lua_pushstring(_L, pCallback->m_rgchAchievementName);
    lua_setfield(_L, -2, "schievementName");
    
    lua_pushboolean(_L, pCallback->m_bAchieved);
    lua_setfield(_L, -2, "achieved");
    
    lua_pushnumber(_L, pCallback->m_nIconHandle);
    lua_setfield(_L, -2, "iconID");

    Dispatch(false);
}


void
SteamworksProvider::OnGlobalAchievementPercentagesReady(GlobalAchievementPercentagesReady_t *pResult, bool bIOFailure)
{
    CoronaLuaNewEvent(_L, kEventName);
    
    lua_pushstring(_L, "onGlobalAchievementPercentagesReady");
    lua_setfield(_L, -2, "type");
    
    lua_pushnumber(_L, pResult->m_nGameID);
    lua_setfield(_L, -2, "gameID");
    
    lua_pushnumber(_L, pResult->m_eResult);
    lua_setfield(_L, -2, "result");

    Dispatch(pResult->m_eResult != k_EResultOK || bIOFailure);
}

void
SteamworksProvider::OnLeaderboardUGCSet(LeaderboardUGCSet_t *pCallback)
{
    CoronaLuaNewEvent(_L, kEventName);
    
    lua_pushstring(_L, "onLeaderboardUGCSet");
    lua_setfield(_L, -2, "type");
    
    Dispatch(false);
}


void
SteamworksProvider::OnGlobalStatsReceived(GlobalStatsReceived_t *pResult, bool bIOFailure)
{
    CoronaLuaNewEvent(_L, kEventName);
    
    lua_pushstring(_L, "onGlobalStatsReceived");
    lua_setfield(_L, -2, "type");
    
    lua_pushnumber(_L, pResult->m_nGameID);
    lua_setfield(_L, -2, "gameID");
    
    lua_pushnumber(_L, pResult->m_eResult);
    lua_setfield(_L, -2, "result");
    
    Dispatch(pResult->m_eResult != k_EResultOK || bIOFailure);
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// Lua Shims
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

int
SteamworksProvider::init( lua_State *L )
{
    Self *library = ToLibrary( L );
    return library->Init( L );
}

int
SteamworksProvider::show( lua_State *L )
{
    Self *library = ToLibrary( L );
    return library->Show( L );
}

int
SteamworksProvider::request( lua_State *L )
{
    Self *library = ToLibrary( L );
    return library->Request( L );
}


// ----------------------------------------------------------------------------

CORONA_EXPORT int luaopen_CoronaProvider_gameNetwork_steamworks( lua_State *L )
{
    return SteamworksProvider::Open(L);
}
