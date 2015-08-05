//
//  SteamworksDelegate.cpp
//  Plugin
//
//  Created by Stiven on 8/3/15.
//  Copyright (c) 2015 Corona Labs. All rights reserved.
//

#include "SteamworksDelegate.h"

const char SteamworksDelegate::kProviderName[] = "steamworks";

const char SteamworksDelegate::kEventName[] = "steamworksEvent";


SteamworksDelegate::SteamworksDelegate( CoronaLuaRef listener, lua_State *L, bool isGlobalListener ):
fListener(listener),
fL(L),
globalListener(isGlobalListener)
{
}

SteamworksDelegate::~SteamworksDelegate()
{
    if (!globalListener){
        CoronaLuaDeleteRef(fL, fListener);
    }
}

void
SteamworksDelegate::Dispatch( bool isError )
{
    if ( CORONA_VERIFY( fListener ) && CORONA_VERIFY( fL ))
    {
        lua_pushstring( fL, kProviderName );
        lua_setfield( fL, -2, "provider" );
        
        lua_pushboolean( fL, isError );
        lua_setfield( fL, -2, "isError" );
        
        CoronaLuaDispatchEvent( fL, fListener, 0 );
        called = true;
    }
}

void
SteamworksDelegate::OnLeaderboardFindResult(LeaderboardFindResult_t *pResult, bool bIOFailure)
{
    CoronaLuaNewEvent(fL, kEventName);
    
    lua_pushstring(fL, "onLeaderboardFindResult");
    lua_setfield(fL, -2, "type");
    
    lua_pushnumber(fL, pResult->m_hSteamLeaderboard);
    lua_setfield(fL, -2, "steamLeaderboard");
    
    lua_pushboolean(fL, pResult->m_bLeaderboardFound);
    lua_setfield(fL, -2, "leaderboardFound");
    
    Dispatch(bIOFailure || !pResult->m_hSteamLeaderboard );
}


void
SteamworksDelegate::OnLeaderboardScoresDownloaded(LeaderboardScoresDownloaded_t *pResult, bool bIOFailure)
{
    CoronaLuaNewEvent(fL, kEventName);
    
    lua_pushstring(fL, "onLeaderboardScoresDownloaded");
    lua_setfield(fL, -2, "type");
    
    lua_pushnumber(fL, pResult->m_hSteamLeaderboard);
    lua_setfield(fL, -2, "steamLeaderboard");
    
    lua_pushnumber(fL, pResult->m_hSteamLeaderboardEntries);
    lua_setfield(fL, -2, "leaderboardEntries");
    
    lua_pushnumber(fL, pResult->m_cEntryCount);
    lua_setfield(fL, -2, "count");
    
    Dispatch(bIOFailure );
}


void
SteamworksDelegate::OnLeaderboardScoreUploaded(LeaderboardScoreUploaded_t *pResult, bool bIOFailure)
{
    CoronaLuaNewEvent(fL, kEventName);
    
    lua_pushstring(fL, "onLeaderboardScoreUploaded");
    lua_setfield(fL, -2, "type");
    
    lua_pushboolean(fL, pResult->m_bSuccess);
    lua_setfield(fL, -2, "success");
    
    lua_pushnumber(fL, pResult->m_hSteamLeaderboard);
    lua_setfield(fL, -2, "steamLeaderboard");
    
    lua_pushnumber(fL, pResult->m_nScore);
    lua_setfield(fL, -2, "score");
    
    lua_pushboolean(fL, pResult->m_bScoreChanged);
    lua_setfield(fL, -2, "scoreChanged");
    
    lua_pushnumber(fL, pResult->m_nGlobalRankNew);
    lua_setfield(fL, -2, "globalRankNew");
    
    lua_pushnumber(fL, pResult->m_nGlobalRankPrevious);
    lua_setfield(fL, -2, "globalRankPrevious");
    
    Dispatch(!pResult->m_bSuccess || bIOFailure );
    
}


void
SteamworksDelegate::OnNumberOfCurrentPlayers(NumberOfCurrentPlayers_t *pResult, bool bIOFailure)
{
    CoronaLuaNewEvent(fL, kEventName);
    
    lua_pushstring(fL, "onNumberOfCurrentPlayers");
    lua_setfield(fL, -2, "type");
    
    lua_pushboolean(fL, pResult->m_bSuccess);
    lua_setfield(fL, -2, "success");
    
    lua_pushnumber(fL, pResult->m_cPlayers);
    lua_setfield(fL, -2, "players");
    
    Dispatch(bIOFailure || !pResult->m_bSuccess );
}


void
SteamworksDelegate::OnGlobalAchievementPercentagesReady(GlobalAchievementPercentagesReady_t *pResult, bool bIOFailure)
{
    CoronaLuaNewEvent(fL, kEventName);
    
    lua_pushstring(fL, "onGlobalAchievementPercentagesReady");
    lua_setfield(fL, -2, "type");
    
    lua_pushnumber(fL, pResult->m_nGameID);
    lua_setfield(fL, -2, "gameID");
    
    lua_pushnumber(fL, pResult->m_eResult);
    lua_setfield(fL, -2, "result");
    
    Dispatch(pResult->m_eResult != k_EResultOK || bIOFailure );
}


void
SteamworksDelegate::OnGlobalStatsReceived(GlobalStatsReceived_t *pResult, bool bIOFailure)
{
    CoronaLuaNewEvent(fL, kEventName);
    
    lua_pushstring(fL, "onGlobalStatsReceived");
    lua_setfield(fL, -2, "type");
    
    lua_pushnumber(fL, pResult->m_nGameID);
    lua_setfield(fL, -2, "gameID");
    
    lua_pushnumber(fL, pResult->m_eResult);
    lua_setfield(fL, -2, "result");
    
    Dispatch(pResult->m_eResult != k_EResultOK || bIOFailure );
}
