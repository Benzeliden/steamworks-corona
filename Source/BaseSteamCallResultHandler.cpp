// ----------------------------------------------------------------------------
// 
// BaseSteamCallResultHandler.cpp
// Copyright (c) 2016 Corona Labs Inc. All rights reserved.
//
// ----------------------------------------------------------------------------

#include "BaseSteamCallResultHandler.h"


BaseSteamCallResultHandler::BaseSteamCallResultHandler()
{
}

BaseSteamCallResultHandler::~BaseSteamCallResultHandler()
{
}

bool BaseSteamCallResultHandler::IsNotWaitingForResult() const
{
	return !IsWaitingForResult();
}
