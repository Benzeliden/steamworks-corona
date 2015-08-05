//
//  SteamworksDelegate.h
//  Plugin
//
//  Created by Stiven on 8/3/15.
//  Copyright (c) 2015 Corona Labs. All rights reserved.
//

#ifndef __Plugin__SteamworksDelegate__
#define __Plugin__SteamworksDelegate__

#include <stdio.h>
#include "steam_api.h"
#include "CoronaLua.h"
#include "CoronaMacros.h"
#include "CoronaAssert.h"
#include "SteamworksUtils.h"

class SteamworksDelegate
{
public:
    typedef SteamworksDelegate Self;
    
public:
    static const char kProviderName[];
    static const char kEventName[];
    
public:
    SteamworksDelegate( CoronaLuaRef listener, lua_State *L, bool isGlobalListener );
    ~SteamworksDelegate();
    
protected:
    void Dispatch( bool isError );
    
private:
    CoronaLuaRef fListener;
    lua_State *fL;
    bool globalListener;
    
public:
    bool called = false;
    
    
public:
    void OnLeaderboardFindResult( LeaderboardFindResult_t *pResult, bool bIOFailure );
    CCallResult<SteamworksDelegate, LeaderboardFindResult_t> fcallResultFindLeaderboard;
    
    void OnLeaderboardScoresDownloaded( LeaderboardScoresDownloaded_t *pResult, bool bIOFailure );
    CCallResult<SteamworksDelegate, LeaderboardScoresDownloaded_t> fcallResultDownloadedLeaderboardScores;
    
    void OnLeaderboardScoreUploaded( LeaderboardScoreUploaded_t *pResult, bool bIOFailure );
    CCallResult<SteamworksDelegate, LeaderboardScoreUploaded_t> fcallResultUploadedLeaderboardScore;
    
    void OnNumberOfCurrentPlayers( NumberOfCurrentPlayers_t *pResult, bool bIOFailure );
    CCallResult<SteamworksDelegate, NumberOfCurrentPlayers_t> fcallResultNumberOfCurrentPlayers;
    
    void OnGlobalStatsReceived( GlobalStatsReceived_t *pResult, bool bIOFailure );
    CCallResult<SteamworksDelegate, GlobalStatsReceived_t> fcallResultReceivedGlobalStats;
    
    void OnGlobalAchievementPercentagesReady( GlobalAchievementPercentagesReady_t *pResult, bool bIOFailure );
    CCallResult<SteamworksDelegate, GlobalAchievementPercentagesReady_t> fcallResultGlobalAchievementPercentagesReady;
};

#endif /* defined(__Plugin__SteamworksDelegate__) */
