//
//  SteamworksStore.cpp
//  Plugin
//
//  Created by Stiven on 8/5/15.
//  Copyright (c) 2015 Corona Labs. All rights reserved.
//

#include "SteamworksStore.h"

#include "CoronaLibrary.h"
#include "CoronaEvent.h"
#include "steam_api.h"

#include "SteamworksUtils.h"
#include "SteamworksDelegate.h"
#include <string>
#include <sstream>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#include <Wingdi.h>
#pragma comment(lib, "Opengl32.lib")

#else
#include <dlfcn.h>

#endif


#define SSTR( x ) dynamic_cast< std::ostringstream & >( \
( std::ostringstream() << std::dec << x ) ).str()


// ----------------------------------------------------------------------------

class SteamworksStore
{
public:
    typedef SteamworksStore Self;
    
public:
    static const char kName[];
    static const char kEventName[];
    
protected:
    SteamworksStore( lua_State *L );
    ~SteamworksStore();
    
public:
    CoronaLuaRef GetListener() const { return fListener; }
    
public:
    static int Open( lua_State *L );
    
protected:
    static int Finalizer( lua_State *L );
    void Dispatch( bool isError, CoronaLuaRef listener);
    
public:
    static Self *ToLibrary( lua_State *L );
    
protected:
    int Init( lua_State *L );
    int GetUserInfo(lua_State *L);
    int Purchase(lua_State *L);
    int FinishTransaction(lua_State *L);
    int QueryTransaction(lua_State *L);
    int RefundTransaction(lua_State *L);
    int GetReport(lua_State *L);
    
public:
    static int init(lua_State *L);
    static int getUserInfo(lua_State *L);
    static int purchase(lua_State *L);
    static int finishTransaction(lua_State *L);
    static int queryTransaction(lua_State *L);
    static int refundTransaction(lua_State *L);
    static int getReport(lua_State *L);
    
private:
    static int runCallbacks(lua_State *L);
    SteamworksDelegate* registerDelegate(lua_State *L);
    void registerListener(lua_State *L, CoronaLuaRef &listener);
    
private:
    CoronaLuaRef fListener;
    CoronaLuaRef fOnMicroTxnAuthorizationResponseListener;
    lua_State *fL;
    const char* webAPIKey;
    std::vector<SteamworksDelegate*> fDelegates;
    
public:
    
    STEAM_CALLBACK( SteamworksStore, OnMicroTxnAuthorizationResponse, MicroTxnAuthorizationResponse_t, fCallbackMicroTxnAuthorizationResponse );
    
};

// ----------------------------------------------------------------------------

const char SteamworksStore::kName[] = "plugin.steamworks.store";

const char SteamworksStore::kEventName[] = "steamworksStoreEvent";

struct storeItem {
    std::string itemid;
    std::string qty;
    std::string amount;
    std::string description;
    std::string category;
};

SteamworksStore::SteamworksStore( lua_State *L ):
fCallbackMicroTxnAuthorizationResponse(this, &SteamworksStore::OnMicroTxnAuthorizationResponse),
fListener( NULL ),
fOnMicroTxnAuthorizationResponseListener( NULL ),
webAPIKey( NULL ),
fL( L )
{
}

SteamworksStore::~SteamworksStore()
{
    SteamAPI_Shutdown();
}


int
SteamworksStore::Open( lua_State *L )
{
    CoronaLuaInitializeGCMetatable( L, kName, Finalizer );
    

    const luaL_Reg kFunctions[] =
    {
        { "init", init },
        { "getUserInfo", getUserInfo },
        { "purchase", purchase },
        { "finishTransaction", finishTransaction },
        { "queryTransaction", queryTransaction },
        { "refundTransaction", refundTransaction },
        { "getReport", getReport },
            
        { NULL, NULL }
    };
    
    // Use 'provider' in closure for kFunctions
    Self *library = new Self( L );
    CoronaLuaPushUserdata( L, library, kName );
    luaL_openlib( L, kName, kFunctions, 1 );
    
    return 1;
}

int
SteamworksStore::Finalizer( lua_State *L )
{
    Self *library = (Self *)CoronaLuaToUserdata( L, 1 );
    
    CoronaLuaPushRuntime( L ); // push 'Runtime'
    
    if ( lua_type( L, -1 ) == LUA_TTABLE )
    {
        lua_getfield( L, -1, "removeEventListener" ); // push 'f', i.e. Runtime.addEventListener
        lua_insert( L, -2 ); // swap so 'f' is below 'Runtime'
        lua_pushstring( L, "enterFrame" );
        lua_pushlightuserdata(L, library);
        lua_pushcclosure( L, &runCallbacks, 1 );
        
        CoronaLuaDoCall( L, 3, 0 );
    }
    else
    {
        lua_pop( L, 1 ); // pop nil
    }
    
    CoronaLuaDeleteRef( L, library->fListener );
    
    for (std::vector<SteamworksDelegate*>::iterator iter = library->fDelegates.begin(); iter != library->fDelegates.end(); iter++){
        delete *iter;
    }
    library->fDelegates.clear();
    
    delete library;
    
    return 0;
}

SteamworksStore *
SteamworksStore::ToLibrary( lua_State *L )
{
    // library is pushed as part of the closure
    Self *library = (Self *)CoronaLuaToUserdata( L, lua_upvalueindex( 1 ) );

    return library;
}

int
SteamworksStore::runCallbacks(lua_State *L)
{
    if (SteamUtils()->BOverlayNeedsPresent())
    {
#ifdef _WIN32
        HDC hdc = wglGetCurrentDC();
        HWND window = WindowFromDC(hdc);
        if (window)
        {
            ::InvalidateRect(window, nullptr, FALSE);
        }
#endif
        
    }
    SteamAPI_RunCallbacks();
    
    
    //Self *library = ToLibrary(L);
    Self *library = (Self *)lua_touserdata(L, lua_upvalueindex(1));
    
    for ( int i = (int)library->fDelegates.size()-1; i>=0; i--){
        SteamworksDelegate *delegate = library->fDelegates[i];
        if (delegate->called){
            delete delegate;
            library->fDelegates.erase(library->fDelegates.begin()+i);
        }
    }
    
    return 0;
}


SteamworksDelegate*
SteamworksStore::registerDelegate(lua_State *L)
{
    CoronaLuaRef listener = NULL;
    bool globalListener = false;
    if( lua_type( L, -1 ) == LUA_TTABLE ){
        lua_getfield(L, -1, "listener");
        if ( CoronaLuaIsListener( L, -1, kEventName ) )
        {
            listener = CoronaLuaNewRef( L, -1 );
        }
        else if (!lua_isnil(L, -1))
        {
            CoronaLuaWarning(L, "store: listener argument must be a listener for %s events.\n", kName);
        }
        lua_pop(L, 1);
    }else if( CoronaLuaIsListener( L, 2, kEventName )){
        listener = CoronaLuaNewRef( L, 2 );
    }
    if (listener == NULL){
        listener = fListener;
        globalListener = true;
    }
    SteamworksDelegate *delegate = new SteamworksDelegate(listener, L, globalListener);
    fDelegates.push_back(delegate);
    return delegate;
}

void
SteamworksStore::registerListener(lua_State *L, CoronaLuaRef &listener)
{
    CoronaLuaDeleteRef(L, listener);
    listener = NULL;
    if( lua_type( L, -1 ) == LUA_TTABLE ){
        lua_getfield(L, -1, "listener");
        if ( CoronaLuaIsListener( L, -1, kEventName ) )
        {
            listener = CoronaLuaNewRef( L, -1 );
        }
        else if (!lua_isnil(L, -1))
        {
            CoronaLuaWarning(L, "store: listener argument must be a listener for %s events.\n", kEventName);
        }
        lua_pop(L, 1);
    }else if( CoronaLuaIsListener( L, -1, kEventName )){
        listener = CoronaLuaNewRef( L, -1 );
    }
}

int
SteamworksStore::Init( lua_State *L )
{
    bool result = false;
    if ( !fListener )
    {
        int index = 1;
        if ( CoronaLuaIsListener( L, index, kEventName ) )
        {
            if(!SteamworksUtils::makeAppIdEnvVar(L)){
                CoronaLuaError(L, "store.init(): Specify AppId in config.lua.\n");
                return 0;
            }
            webAPIKey = SteamworksUtils::getWebAPIKey(L);
            if (strcmp(webAPIKey, "") == 0){
                CoronaLuaError(L, "store.init(): Specify webAPIKey in config.lua.\n");
                return 0;
            }
            result = SteamAPI_Init();
            if (result){
                CoronaLuaRef listener = CoronaLuaNewRef( L, index );
                fListener = listener;
                
                //Runtime:addEventListener( "enterFrame", runCallbacks )
                CoronaLuaPushRuntime( L );
                lua_getfield( L, -1, "addEventListener" );
                lua_insert( L, -2 );
                lua_pushstring( L, "enterFrame" );
                
                lua_pushlightuserdata(L, this);
                lua_pushcclosure( L, &runCallbacks, 1 );
                CoronaLuaDoCall( L, 3, 0 );
                
                ToLibrary(L);
            }else{
                CoronaLuaWarning(L, "steamAPI failed to initialize");
            }
        }
        else
        {
            // Error: Listener is required
            CoronaLuaError(L, "store.init(): Expected argument %d to be a listener for %s events.\n", index, kEventName);
        }
    }
    else
    {
        CoronaLuaWarning(L, "store.init(): This function has already been called. It should only be called once.\n");
    }
    lua_pushboolean(L, result);

    return 1;
}

int
SteamworksStore::GetUserInfo( lua_State *L )
{
    if (!fListener){
        CoronaLuaWarning(L, "Steam API is not initialized. Call store.init()\n");
        return 0;
    }
    bool sandbox = false;
    if( lua_type( L, 1 ) == LUA_TTABLE ){
        lua_getfield(L, 1, "sandbox");
        if ( lua_type(L, -1) == LUA_TBOOLEAN )
        {
            sandbox = lua_toboolean(L, -1)? true: false;
        }
        lua_pop(L, 1);
    }
    HTTPRequestHandle handle;
    if (sandbox){
        handle = SteamHTTP()->CreateHTTPRequest(k_EHTTPMethodGET, "https://api.steampowered.com/ISteamMicroTxnSandbox/GetUserInfo/V0001/");
    }else{
        handle = SteamHTTP()->CreateHTTPRequest(k_EHTTPMethodGET, "https://api.steampowered.com/ISteamMicroTxn/GetUserInfo/V0001/");
    }
    const char *steamUserID = SteamworksUtils::steamIDToString(SteamUser()->GetSteamID());
    SteamHTTP()->SetHTTPRequestGetOrPostParameter(handle, "steamid", steamUserID);
    SteamHTTP()->SetHTTPRequestGetOrPostParameter(handle, "key", webAPIKey);
    
    SteamAPICall_t hSteamAPICall;
    SteamHTTP()->SendHTTPRequest(handle, &hSteamAPICall);
    SteamworksDelegate *delegate = registerDelegate(L);
    delegate->fcallResultHTTPRequestCompleted.Set( hSteamAPICall, delegate, &SteamworksDelegate::OnHTTPRequestCompleted );
    return 0;
}

int
SteamworksStore::Purchase( lua_State *L )
{
    if (!fListener){
        CoronaLuaWarning(L, "Steam API is not initialized. Call store.init()\n");
        return 0;
    }
    registerListener(L, fOnMicroTxnAuthorizationResponseListener);
    bool sandbox = false;
    std::string orderid;
    std::string appid;
    std::string itemcount;
    std::string language;
    std::string currency;
    std::vector<storeItem> items;
    
    if( lua_type( L, 1 ) == LUA_TTABLE ){
        lua_getfield(L, 1, "sandbox");
        if ( lua_type(L, -1) == LUA_TBOOLEAN )
        {
            sandbox = lua_toboolean(L, -1)? true: false;
        }
        lua_pop(L, 1);
        
        lua_getfield(L, 1, "orderid");
        if ( lua_type(L, -1) == LUA_TSTRING )
        {
            orderid = lua_tostring(L, -1);
        }else{
            CoronaLuaError(L, "store.purchase(): specify orderid");
            lua_pop(L, 1);
            return 0;
        }
        lua_pop(L, 1);
        
        lua_getfield(L, 1, "appid");
        if ( lua_type(L, -1) == LUA_TNUMBER )
        {
            appid = SSTR((int)lua_tonumber(L, -1));
        }else{
            CoronaLuaError(L, "store.purchase(): specify appid");
            lua_pop(L, 1);
            return 0;
        }
        lua_pop(L, 1);
        
        lua_getfield(L, 1, "itemcount");
        if ( lua_type(L, -1) == LUA_TNUMBER )
        {
            itemcount = SSTR((int)lua_tonumber(L, -1));
        }else{
            CoronaLuaError(L, "store.purchase(): specify itemcount");
            lua_pop(L, 1);
            return 0;
        }
        lua_pop(L, 1);
        
        lua_getfield(L, 1, "language");
        if ( lua_type(L, -1) == LUA_TSTRING )
        {
            language = lua_tostring(L, -1);
        }else{
            CoronaLuaError(L, "store.purchase(): specify language");
            lua_pop(L, 1);
            return 0;
        }
        lua_pop(L, 1);
        
        lua_getfield(L, 1, "currency");
        if ( lua_type(L, -1) == LUA_TSTRING )
        {
            currency = lua_tostring(L, -1);
        }else{
            CoronaLuaError(L, "store.purchase(): specify currency");
            lua_pop(L, 1);
            return 0;
        }
        lua_pop(L, 1);
        
        lua_getfield(L, -1, "items");
        if (lua_istable(L, -1)){
            int t = CoronaLuaNormalize(L, -1);
            lua_pushnil(L);  /* first key */
            int i = 0;
            while (lua_next(L, t) != 0) {
                storeItem item;
                if (lua_istable(L, -1)){
                    lua_getfield(L, -1, "itemid");
                    if ( lua_type(L, -1) == LUA_TNUMBER )
                    {
                        item.itemid = SSTR((int)lua_tonumber(L, -1));
                    }else{
                        CoronaLuaError(L, "store.purchase(): invalid itemid in item %i", i+1);
                        lua_pop(L, 2);
                        return 0;
                    }
                    lua_pop(L, 1);
                    
                    lua_getfield(L, -1, "qty");
                    if ( lua_type(L, -1) == LUA_TNUMBER )
                    {
                        item.qty = SSTR((int)lua_tonumber(L, -1));
                    }else{
                        CoronaLuaError(L, "store.purchase(): invalid qty in item %i", i+1);
                        lua_pop(L, 2);
                        return 0;
                    }
                    lua_pop(L, 1);
                    
                    lua_getfield(L, -1, "amount");
                    if ( lua_type(L, -1) == LUA_TNUMBER )
                    {
                        item.amount = SSTR((int)lua_tonumber(L, -1));
                    }else{
                        CoronaLuaError(L, "store.purchase(): invalid amount in item %i", i+1);
                        lua_pop(L, 2);
                        return 0;
                    }
                    lua_pop(L, 1);
                    
                    lua_getfield(L, -1, "description");
                    if ( lua_type(L, -1) == LUA_TSTRING )
                    {
                        item.description = lua_tostring(L, -1);
                    }else{
                        CoronaLuaError(L, "store.purchase(): invalid description in item %i", i+1);
                        lua_pop(L, 2);
                        return 0;
                    }
                    lua_pop(L, 1);
                    
                    lua_getfield(L, -1, "category");
                    if ( lua_type(L, -1) == LUA_TSTRING )
                    {
                        item.category = lua_tostring(L, -1);
                    }else{
                        CoronaLuaError(L, "store.purchase(): invalid category in item %i", i+1);
                        lua_pop(L, 2);
                        return 0;
                    }
                    lua_pop(L, 1);
                    items.push_back(item);
                }else{
                    CoronaLuaError(L, "store.purchase(): specify items");
                    lua_pop(L, 1);
                    return 0;
                }
                i++;
                lua_pop(L, 1);
            }
        }
        lua_pop(L, 1);
    }else{
        CoronaLuaError(L, "store.purchase(): provide params table");
        return 0;
    }
    
    HTTPRequestHandle handle;
    if (sandbox){
        handle = SteamHTTP()->CreateHTTPRequest(k_EHTTPMethodPOST, "https://api.steampowered.com/ISteamMicroTxnSandbox/InitTxn/V0002/");
    }else{
        handle = SteamHTTP()->CreateHTTPRequest(k_EHTTPMethodPOST, "https://api.steampowered.com/ISteamMicroTxn/InitTxn/V0002/");
    }
    
    const char *steamUserID = SteamworksUtils::steamIDToString(SteamUser()->GetSteamID());
    SteamHTTP()->SetHTTPRequestGetOrPostParameter(handle, "orderid", orderid.c_str());
    SteamHTTP()->SetHTTPRequestGetOrPostParameter(handle, "steamid", steamUserID);
    SteamHTTP()->SetHTTPRequestGetOrPostParameter(handle, "appid", appid.c_str());
    SteamHTTP()->SetHTTPRequestGetOrPostParameter(handle, "itemcount", itemcount.c_str());
    SteamHTTP()->SetHTTPRequestGetOrPostParameter(handle, "language", language.c_str());
    SteamHTTP()->SetHTTPRequestGetOrPostParameter(handle, "currency", currency.c_str());
    SteamHTTP()->SetHTTPRequestGetOrPostParameter(handle, "key", webAPIKey);
    for (int i = 0; i<(int)items.size(); i++){
        std::stringstream itemidStringstream;
        itemidStringstream << "itemid[" << i << "]";
        std::string itemidString = itemidStringstream.str();
        
        std::stringstream qtyStringstream;
        qtyStringstream << "qty[" << i << "]";
        std::string qtyString = qtyStringstream.str();
        
        std::stringstream amountStringstream;
        amountStringstream << "amount[" << i << "]";
        std::string amountString = amountStringstream.str();
        
        std::stringstream descriptionStringstream;
        descriptionStringstream << "description[" << i << "]";
        std::string descriptionString = descriptionStringstream.str();
        
        std::stringstream categoryStringstream;
        categoryStringstream << "category[" << i << "]";
        std::string categoryString = categoryStringstream.str();
        
        SteamHTTP()->SetHTTPRequestGetOrPostParameter(handle, itemidString.c_str(), items[i].itemid.c_str());
        SteamHTTP()->SetHTTPRequestGetOrPostParameter(handle, qtyString.c_str(), items[i].qty.c_str());
        SteamHTTP()->SetHTTPRequestGetOrPostParameter(handle, amountString.c_str(), items[i].amount.c_str());
        SteamHTTP()->SetHTTPRequestGetOrPostParameter(handle, descriptionString.c_str(), items[i].description.c_str());
        SteamHTTP()->SetHTTPRequestGetOrPostParameter(handle, categoryString.c_str(), items[i].category.c_str());
        
    }
    SteamAPICall_t hSteamAPICall;
    SteamHTTP()->SendHTTPRequest(handle, &hSteamAPICall);
    SteamworksDelegate *delegate = registerDelegate(L);
    delegate->fcallResultHTTPRequestCompleted.Set( hSteamAPICall, delegate, &SteamworksDelegate::OnHTTPRequestCompleted );
    
    return 0;
}

int
SteamworksStore::FinishTransaction( lua_State *L )
{
    if (!fListener){
        CoronaLuaWarning(L, "Steam API is not initialized. Call store.init()\n");
        return 0;
    }
    bool sandbox = false;
    std::string orderid;
    std::string appid;
    
    if( lua_type( L, 1 ) == LUA_TTABLE ){
        lua_getfield(L, 1, "sandbox");
        if ( lua_type(L, -1) == LUA_TBOOLEAN )
        {
            sandbox = lua_toboolean(L, -1)? true: false;
        }
        lua_pop(L, 1);
        
        lua_getfield(L, 1, "orderid");
        if ( lua_type(L, -1) == LUA_TSTRING )
        {
            orderid = lua_tostring(L, -1);
        }else{
            CoronaLuaError(L, "store.finishTransaction(): specify orderid");
            lua_pop(L, 1);
            return 0;
        }
        lua_pop(L, 1);
        
        lua_getfield(L, 1, "appid");
        if ( lua_type(L, -1) == LUA_TNUMBER )
        {
            appid = SSTR((int)lua_tonumber(L, -1));
        }else{
            CoronaLuaError(L, "store.finishTransaction(): specify appid");
            lua_pop(L, 1);
            return 0;
        }
        lua_pop(L, 1);
    }else{
        CoronaLuaError(L, "store.finishTransaction(): provide params table");
        return 0;
    }
    
    HTTPRequestHandle handle;
    if (sandbox){
        handle = SteamHTTP()->CreateHTTPRequest(k_EHTTPMethodPOST, "https://api.steampowered.com/ISteamMicroTxnSandbox/FinalizeTxn/V0001/");
    }else{
        handle = SteamHTTP()->CreateHTTPRequest(k_EHTTPMethodPOST, "https://api.steampowered.com/ISteamMicroTxn/FinalizeTxn/V0001/");
    }
    
    SteamHTTP()->SetHTTPRequestGetOrPostParameter(handle, "orderid", orderid.c_str());
    SteamHTTP()->SetHTTPRequestGetOrPostParameter(handle, "appid", appid.c_str());
    SteamHTTP()->SetHTTPRequestGetOrPostParameter(handle, "key", webAPIKey);
    SteamAPICall_t hSteamAPICall;
    SteamHTTP()->SendHTTPRequest(handle, &hSteamAPICall);
    SteamworksDelegate *delegate = registerDelegate(L);
    delegate->fcallResultHTTPRequestCompleted.Set( hSteamAPICall, delegate, &SteamworksDelegate::OnHTTPRequestCompleted );
    
    return 0;
}

int
SteamworksStore::QueryTransaction( lua_State *L )
{
    if (!fListener){
        CoronaLuaWarning(L, "Steam API is not initialized. Call store.init()\n");
        return 0;
    }
    bool sandbox = false;
    std::string orderid;
    std::string transid;
    std::string appid;
    
    if( lua_type( L, 1 ) == LUA_TTABLE ){
        lua_getfield(L, 1, "sandbox");
        if ( lua_type(L, -1) == LUA_TBOOLEAN )
        {
            sandbox = lua_toboolean(L, -1)? true: false;
        }
        lua_pop(L, 1);
        
        lua_getfield(L, 1, "orderid");
        if ( lua_type(L, -1) == LUA_TSTRING )
        {
            orderid = lua_tostring(L, -1);
        }
        lua_pop(L, 1);
        
        lua_getfield(L, 1, "transid");
        if ( lua_type(L, -1) == LUA_TSTRING )
        {
            transid = lua_tostring(L, -1);
        }
        lua_pop(L, 1);
        
        lua_getfield(L, 1, "appid");
        if ( lua_type(L, -1) == LUA_TNUMBER )
        {
            appid = SSTR((int)lua_tonumber(L, -1));
        }else{
            CoronaLuaError(L, "store.queryTransaction(): specify appid");
            lua_pop(L, 1);
            return 0;
        }
        lua_pop(L, 1);
    }else{
        CoronaLuaError(L, "store.queryTransaction(): provide params table");
        return 0;
    }
    
    HTTPRequestHandle handle;
    if (sandbox){
        handle = SteamHTTP()->CreateHTTPRequest(k_EHTTPMethodGET, "https://api.steampowered.com/ISteamMicroTxnSandbox/QueryTxn/V0001/");
    }else{
        handle = SteamHTTP()->CreateHTTPRequest(k_EHTTPMethodGET, "https://api.steampowered.com/ISteamMicroTxn/QueryTxn/V0001/");
    }
    
    if(!orderid.empty()){
        SteamHTTP()->SetHTTPRequestGetOrPostParameter(handle, "orderid", orderid.c_str());
    }else if(!transid.empty()){
        SteamHTTP()->SetHTTPRequestGetOrPostParameter(handle, "transid", transid.c_str());
    }else{
        CoronaLuaError(L, "store.queryTransaction(): specify orderid or transid");
        return 0;
    }
    SteamHTTP()->SetHTTPRequestGetOrPostParameter(handle, "appid", appid.c_str());
    SteamHTTP()->SetHTTPRequestGetOrPostParameter(handle, "key", webAPIKey);
    SteamAPICall_t hSteamAPICall;
    SteamHTTP()->SendHTTPRequest(handle, &hSteamAPICall);
    SteamworksDelegate *delegate = registerDelegate(L);
    delegate->fcallResultHTTPRequestCompleted.Set( hSteamAPICall, delegate, &SteamworksDelegate::OnHTTPRequestCompleted );
    
    return 0;
}

int
SteamworksStore::RefundTransaction( lua_State *L )
{
    if (!fListener){
        CoronaLuaWarning(L, "Steam API is not initialized. Call store.init()\n");
        return 0;
    }
    bool sandbox = false;
    std::string orderid;
    std::string appid;
    
    if( lua_type( L, 1 ) == LUA_TTABLE ){
        lua_getfield(L, 1, "sandbox");
        if ( lua_type(L, -1) == LUA_TBOOLEAN )
        {
            sandbox = lua_toboolean(L, -1)? true: false;
        }
        lua_pop(L, 1);
        
        lua_getfield(L, 1, "orderid");
        if ( lua_type(L, -1) == LUA_TSTRING )
        {
            orderid = lua_tostring(L, -1);
        }else{
            CoronaLuaError(L, "store.refundTransaction(): specify orderid");
            lua_pop(L, 1);
            return 0;
        }
        lua_pop(L, 1);
        
        lua_getfield(L, 1, "appid");
        if ( lua_type(L, -1) == LUA_TNUMBER )
        {
            appid = SSTR((int)lua_tonumber(L, -1));
        }else{
            CoronaLuaError(L, "store.refundTransaction(): specify appid");
            lua_pop(L, 1);
            return 0;
        }
        lua_pop(L, 1);
    }else{
        CoronaLuaError(L, "store.refundTransaction(): provide params table");
        return 0;
    }
    
    HTTPRequestHandle handle;
    if (sandbox){
        handle = SteamHTTP()->CreateHTTPRequest(k_EHTTPMethodPOST, "https://api.steampowered.com/ISteamMicroTxnSandbox/RefundTxn/V0001/");
    }else{
        handle = SteamHTTP()->CreateHTTPRequest(k_EHTTPMethodPOST, "https://api.steampowered.com/ISteamMicroTxn/RefundTxn/V0001/");
    }
    
    SteamHTTP()->SetHTTPRequestGetOrPostParameter(handle, "orderid", orderid.c_str());
    SteamHTTP()->SetHTTPRequestGetOrPostParameter(handle, "appid", appid.c_str());
    SteamHTTP()->SetHTTPRequestGetOrPostParameter(handle, "key", webAPIKey);
    SteamAPICall_t hSteamAPICall;
    SteamHTTP()->SendHTTPRequest(handle, &hSteamAPICall);
    SteamworksDelegate *delegate = registerDelegate(L);
    delegate->fcallResultHTTPRequestCompleted.Set( hSteamAPICall, delegate, &SteamworksDelegate::OnHTTPRequestCompleted );

    return 0;
}

int
SteamworksStore::GetReport( lua_State *L )
{
    if (!fListener){
        CoronaLuaWarning(L, "Steam API is not initialized. Call store.init()\n");
        return 0;
    }
    bool sandbox = false;
    std::string appid;
    std::string type;
    double timeInSecs = 86401;
    std::string time;
    std::string maxresults = "1000";
    
    if( lua_type( L, 1 ) == LUA_TTABLE ){
        lua_getfield(L, 1, "sandbox");
        if ( lua_type(L, -1) == LUA_TBOOLEAN )
        {
            sandbox = lua_toboolean(L, -1)? true: false;
        }
        lua_pop(L, 1);
        
        lua_getfield(L, 1, "appid");
        if ( lua_type(L, -1) == LUA_TNUMBER )
        {
            appid = SSTR((int)lua_tonumber(L, -1));
        }else{
            CoronaLuaError(L, "store.getReport(): specify appid");
            lua_pop(L, 1);
            return 0;
        }
        lua_pop(L, 1);
        
        lua_getfield(L, 1, "type");
        if ( lua_type(L, -1) == LUA_TSTRING )
        {
            type = lua_tostring(L, -1);
        }else{
            CoronaLuaError(L, "store.getReport(): specify type");
            lua_pop(L, 1);
            return 0;
        }
        lua_pop(L, 1);
        
        lua_getfield(L, 1, "time");
        if ( lua_type(L, -1) == LUA_TNUMBER )
        {
            timeInSecs = lua_tonumber(L, -1);
        }
        lua_pop(L, 1);
        
        lua_getfield(L, 1, "maxresults");
        if ( lua_type(L, -1) == LUA_TNUMBER )
        {
            maxresults = SSTR((int)lua_tonumber(L, -1));
        }
        lua_pop(L, 1);
    }else{
        CoronaLuaError(L, "store.getReport(): provide params table");
        return 0;
    }
    lua_getglobal(L, "os");
    lua_getfield(L, -1, "date");
    lua_pushstring(L, "%Y-%m-%dT%H:%M:%SZ");
    lua_pushnumber(L, timeInSecs);
    lua_call(L, 2, 1);
    
    time = lua_tostring(L, -1);
    lua_pop(L, 2);
    
    HTTPRequestHandle handle;
    if (sandbox){
        handle = SteamHTTP()->CreateHTTPRequest(k_EHTTPMethodGET, "https://api.steampowered.com/ISteamMicroTxnSandbox/GetReport/V0002/");
    }else{
        handle = SteamHTTP()->CreateHTTPRequest(k_EHTTPMethodGET, "https://api.steampowered.com/ISteamMicroTxn/GetReport/V0002/");
    }
    
    SteamHTTP()->SetHTTPRequestGetOrPostParameter(handle, "appid", appid.c_str());
    SteamHTTP()->SetHTTPRequestGetOrPostParameter(handle, "type", type.c_str());
    SteamHTTP()->SetHTTPRequestGetOrPostParameter(handle, "time", time.c_str());
    SteamHTTP()->SetHTTPRequestGetOrPostParameter(handle, "maxresults", maxresults.c_str());
    SteamHTTP()->SetHTTPRequestGetOrPostParameter(handle, "key", webAPIKey);
    SteamAPICall_t hSteamAPICall;
    SteamHTTP()->SendHTTPRequest(handle, &hSteamAPICall);
    SteamworksDelegate *delegate = registerDelegate(L);
    delegate->fcallResultHTTPRequestCompleted.Set( hSteamAPICall, delegate, &SteamworksDelegate::OnHTTPRequestCompleted );
    
    return 0;
}

void
SteamworksStore::Dispatch( bool isError, CoronaLuaRef listener  )
{
    if (!listener){
        listener = fListener;
    }
    if ( CORONA_VERIFY( listener ) && CORONA_VERIFY( fL ))
    {
        lua_pushboolean( fL, isError );
        lua_setfield( fL, -2, "isError" );
        
        CoronaLuaDispatchEvent( fL, listener, 0 );
    }
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// Callbacks
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

void
SteamworksStore::OnMicroTxnAuthorizationResponse(MicroTxnAuthorizationResponse_t *pResult)
{
    CoronaLuaNewEvent(fL, kEventName);
    
    lua_pushstring(fL, "onTransactionResponse");
    lua_setfield(fL, -2, "type");
    
    lua_pushnumber(fL, pResult->m_unAppID);
    lua_setfield(fL, -2, "appID");
    
    lua_pushnumber(fL, pResult->m_ulOrderID);
    lua_setfield(fL, -2, "orderID");
    
    lua_pushboolean(fL, pResult->m_bAuthorized);
    lua_setfield(fL, -2, "authorized");
    
    Dispatch(false, fOnMicroTxnAuthorizationResponseListener);
    
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// Lua Shims
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

int
SteamworksStore::init( lua_State *L )
{
    Self *library = ToLibrary( L );
    return library->Init( L );
}

int
SteamworksStore::getUserInfo( lua_State *L )
{
    Self *library = ToLibrary( L );
    return library->GetUserInfo( L );
}
int
SteamworksStore::purchase( lua_State *L )
{
    Self *library = ToLibrary( L );
    return library->Purchase( L );
}
int
SteamworksStore::finishTransaction( lua_State *L )
{
    Self *library = ToLibrary( L );
    return library->FinishTransaction( L );
}
int
SteamworksStore::queryTransaction( lua_State *L )
{
    Self *library = ToLibrary( L );
    return library->QueryTransaction( L );
}
int
SteamworksStore::refundTransaction( lua_State *L )
{
    Self *library = ToLibrary( L );
    return library->RefundTransaction( L );
}
int
SteamworksStore::getReport( lua_State *L )
{
    Self *library = ToLibrary( L );
    return library->GetReport( L );
}

// ----------------------------------------------------------------------------

CORONA_EXPORT int luaopen_plugin_steamworks_store( lua_State *L )
{
    return SteamworksStore::Open(L);
}
