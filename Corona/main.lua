local centerX = display.contentCenterX
local centerY = display.contentCenterY
local _W = display.contentWidth
local _H = display.contentHeight
local widget = require( "widget" )
local gameNetwork = require("gameNetwork")
local json = require("json")

-- create app background
local bg = display.newImageRect( "assets/bg.png", _W, _H )
bg.x, bg.y = display.contentWidth*0.5, display.contentHeight*0.5

local callbackText = display.newText({text = "", x = centerX, y = _H-75, fontSize = 20})

-- variables (and forward declarations)
local statTabButtons, statTabs, menuTabButtons, menuTabs
local updateCallbackText
local maxFeetTraveledText, averageSpeedText
local setMaxFeetTraveledText, setAverageSpeedDataText, setAverageSpeedTimeText
local requestStatsBtn, storeStatsBtn, resetStatsBtn, getStatBtn, setStatBtn, setMaxFeetTraveledStepper, getAverageStatBtn, setAverageStatBtn, setAverageSpeedDataStepper, setAverageSpeedTimeStepper
local gamesPlayedText, getGamesPlayedStatBtn
local statScreen = 1
local numberOfCurrentPlayers
local achievementTabButtons, achievementTabs
local achievementsScreen = 1
local achievementsTable
local setWinnerAchievementBtn, clearWinnerAchievementBtn, setChampionAchievementSlider, setChampionAchievementBtn, clearChampionAchievementBtn
local storeAchievementsBtn, resetAchievementsBtn, updateTableBtn
local updateLeaderboardInfo, updateLeaderboardEntries
local leaderboardNameText, leaderboardEntryCountText, leaderboardSortMethodText, leaderboardDisplayTypeText, leaderboardTable
local setLeaderboardStatStepperText, setLeaderboardStatStepper, setLeaderboardStatBtn
local currentLeaderboard


local function steamworksListener(event)
	--print(event.type)
	print(event.type, json.encode(event))
	updateCallbackText(event.type)
	if event.type == "onNumberOfCurrentPlayers" then
		numberOfCurrentPlayers.text = "Current Players: "..event.players
	elseif event.type == "onLeaderboardFindResult" then
		currentLeaderboard = event.steamLeaderboard
		updateLeaderboardInfo(event.steamLeaderboard)
	elseif event.type == "onLeaderboardScoresDownloaded" then
		updateLeaderboardEntries(event.leaderboardEntries, event.count)

	elseif event.type == "onGlobalAchievementPercentagesReady" and not event.isError then

	elseif event.type == "onGlobalStatsReceived" and not event.isError then

	end
end

local steamworkInitialized = gameNetwork.init("steamworks", steamworksListener)
local steamIsRunning = gameNetwork.request("isSteamRunning")
gameNetwork.request("getNumberOfCurrentPlayers")

local steamworkInitializedText = display.newText({text = "Steamworks Initialized: "..tostring(steamworkInitialized) , x = 100, y = _H-110, fontSize = 15})
local steamIsRunningText = display.newText({text = "Steam is Running: "..tostring(steamIsRunning) , x = 100, y = _H-90, fontSize = 15})
numberOfCurrentPlayers = display.newText({text = "Current Players: " , x = 100, y = _H-70, fontSize = 15})

local restartAppIfNecessaryBtn = widget.newButton(
{
	x = _W-150,
	y = _H-75,
	width = 298,
	height = 56,
	label = "Restart App If Necessary",
	onRelease = function() gameNetwork.request("restartAppIfNecessary", {appID = 480}); end
})

local composer = require "composer"

-- setup composer scenes (non-external module scenes)
local statsScene = composer.newScene( "statsScene" )
local achievementsScene = composer.newScene( "achievementsScene" )
local leaderboardsScene = composer.newScene( "leaderboardsScene" )


--functions
local function requestStats()
	if statScreen == 1 then
		gameNetwork.request("requestStats")
	elseif statScreen == 2 then
		gameNetwork.request("requestStats", {user = 76561198090314038})
	elseif statScreen == 3 then
		gameNetwork.request("requestGlobalStats", {historyDays = 60})
	end
end

local function storeStats()
	gameNetwork.request("storeStats")
end

local function resetStats(achievementsToo)
	gameNetwork.request("resetAllStats", {resetAchievements = true})
end

local function getStat( stat )
	if statScreen == 1 then
		return gameNetwork.request("getStat", {name = stat})
	elseif statScreen == 2 then
		return gameNetwork.request("getStat", {name = stat, user = 76561198090314038})
	elseif statScreen == 3 then
		return gameNetwork.request("getGlobalStat", {name = "NumGames"})
	end
end

local function setStat( data )
	gameNetwork.request("setStat", {name = "MaxFeetTraveled", data = data})
end

local function setAverageStat( data, time )
	gameNetwork.request("setStat", {name = "AverageSpeed", type = "average", data = data, time = time})
end

local function setMaxFeetTraveledStepperPress(event)
	if ( "increment" == event.phase ) then
        setMaxFeetTraveledText.text = tonumber(setMaxFeetTraveledText.text) + 5
    elseif ( "decrement" == event.phase ) then
        setMaxFeetTraveledText.text = tonumber(setMaxFeetTraveledText.text) - 5
    end
end

local function setAverageSpeedDataStepperPress(event)
	if ( "increment" == event.phase ) then
        setAverageSpeedDataText.text = tonumber(setAverageSpeedDataText.text) + 5
    elseif ( "decrement" == event.phase ) then
        setAverageSpeedDataText.text = tonumber(setAverageSpeedDataText.text) - 5
    end
end

local function setAverageSpeedTimeStepperPress(event)
	if ( "increment" == event.phase ) then
        setAverageSpeedTimeText.text = tonumber(setAverageSpeedTimeText.text) + 1
    elseif ( "decrement" == event.phase ) then
        setAverageSpeedTimeText.text = tonumber(setAverageSpeedTimeText.text) - 1
    end
end


local function onMyStatsTabPress(event)
	statScreen = 1
	setMaxFeetTraveledText.isVisible = true
	setAverageSpeedDataText.isVisible = true
	setAverageSpeedTimeText.isVisible = true
	requestStatsBtn.isVisible = true
	storeStatsBtn.isVisible = true
	resetStatsBtn.isVisible = true
	getStatBtn.isVisible = true
	setStatBtn.isVisible = true
	setMaxFeetTraveledStepper.isVisible = true
	getAverageStatBtn.isVisible = true
	setAverageStatBtn.isVisible = true
	setAverageSpeedDataStepper.isVisible = true
	setAverageSpeedTimeStepper.isVisible = true
	maxFeetTraveledText.isVisible = true
	averageSpeedText.isVisible = true
	gamesPlayedText.isVisible = false
	getGamesPlayedStatBtn.isVisible = false

	maxFeetTraveledText.text = "Max Feet Traveled:"
	averageSpeedText.text = "Average Speed:"

end

local function onUserStatsTabPress(event)
	statScreen = 2
	setMaxFeetTraveledText.isVisible = false
	setAverageSpeedDataText.isVisible = false
	setAverageSpeedTimeText.isVisible = false
	requestStatsBtn.isVisible = true
	storeStatsBtn.isVisible = false
	resetStatsBtn.isVisible = false
	getStatBtn.isVisible = true
	setStatBtn.isVisible = false
	setMaxFeetTraveledStepper.isVisible = false
	getAverageStatBtn.isVisible = true
	setAverageStatBtn.isVisible = false
	setAverageSpeedDataStepper.isVisible = false
	setAverageSpeedTimeStepper.isVisible = false
	maxFeetTraveledText.isVisible = true
	averageSpeedText.isVisible = true
	gamesPlayedText.isVisible = false
	getGamesPlayedStatBtn.isVisible = false

	maxFeetTraveledText.text = "Max Feet Traveled:"
	averageSpeedText.text = "Average Speed:"
end

local function onGlobalTabPress(event)
	statScreen = 3
	setMaxFeetTraveledText.isVisible = false
	setAverageSpeedDataText.isVisible = false
	setAverageSpeedTimeText.isVisible = false
	requestStatsBtn.isVisible = true
	storeStatsBtn.isVisible = false
	resetStatsBtn.isVisible = false
	getStatBtn.isVisible = false
	setStatBtn.isVisible = false
	setMaxFeetTraveledStepper.isVisible = false
	getAverageStatBtn.isVisible = false
	setAverageStatBtn.isVisible = false
	setAverageSpeedDataStepper.isVisible = false
	setAverageSpeedTimeStepper.isVisible = false

	maxFeetTraveledText.isVisible = false
	averageSpeedText.isVisible = false
	gamesPlayedText.isVisible = true
	getGamesPlayedStatBtn.isVisible = true
end

-- statsScene (first tab) ---------------------------------------------------------------

function statsScene:create( event )
	local sceneGroup = self.view
	statTabButtons = 
	{
		{
			width = 32, 
			height = 32,
			label = "My Stats",
			size = 15,
			selected = true,
			onPress = onMyStatsTabPress
		},
		{
			width = 32,
			height = 32,
			size = 15,
			label = "User Stats",
			onPress = onUserStatsTabPress
		},
		{
			width = 32,
			height = 32,
			size = 15,
			label = "Global Stats",
			onPress = onGlobalTabPress
		},
	}
		
	-- create a tab-bar and place it at the bottom of the screen
	statTabs = widget.newTabBar
	{
		top = 0,
		width = display.contentWidth,
		tabSelectedFrameWidth = 20,
		tabSelectedFrameHeight = 52,
		buttons = statTabButtons
	}
	sceneGroup:insert( statTabs )

	requestStatsBtn = widget.newButton
	{ 
		x = centerX/3,
		y = 100,
		width = 298,
		height = 25,
		label = "Request Stats",
		onRelease = requestStats
	}
	sceneGroup:insert( requestStatsBtn )

	storeStatsBtn = widget.newButton
	{ 
		x = centerX/3,
		y = 140,
		width = 298,
		height = 25,
		label = "Store Stats",
		onRelease = storeStats
	}
	sceneGroup:insert( storeStatsBtn )

	resetStatsBtn = widget.newButton
	{ 
		x = centerX/3,
		y = 180,
		width = 298,
		height = 25,
		label = "Reset All Stats",
		onRelease = function() resetStats(false); end
	}
	sceneGroup:insert( resetStatsBtn )

	gamesPlayedText = display.newText({text = "Global Games Played: " , x = centerX*3/2, y = 150, fontSize = 15})
	sceneGroup:insert( gamesPlayedText )

	getGamesPlayedStatBtn = widget.newButton
	{ 
		x = centerX,
		y = 150,
		width = 298,
		height = 25,
		label = "Get Stat",
		onRelease = function() gamesPlayedText.text = "Global Games Played: "..tostring(getStat()); end
	}
	sceneGroup:insert( getGamesPlayedStatBtn )
	gamesPlayedText.isVisible = false
	getGamesPlayedStatBtn.isVisible = false

	maxFeetTraveledText = display.newText({text = "Max Feet Traveled: " , x = centerX*3/2, y = 150, fontSize = 15})
	sceneGroup:insert( maxFeetTraveledText )

	getStatBtn = widget.newButton
	{ 
		x = centerX,
		y = 150,
		width = 298,
		height = 25,
		label = "Get Stat",
		onRelease = function() maxFeetTraveledText.text = "Max Feet Traveled: "..tostring(getStat("MaxFeetTraveled")); end
	}
	sceneGroup:insert( getStatBtn )

	setMaxFeetTraveledText = display.newText({text = "0" , x = centerX*3/2, y = 200, fontSize = 15})
	sceneGroup:insert( setMaxFeetTraveledText )

	setMaxFeetTraveledStepper = widget.newStepper(
	{
		x = centerX*3/2,
		y = 225,
		onPress = setMaxFeetTraveledStepperPress
	})
	sceneGroup:insert( setMaxFeetTraveledStepper )

	setStatBtn = widget.newButton
	{ 
		x = centerX,
		y = 220,
		width = 298,
		height = 25,
		label = "Set Stat",
		onRelease = function() setStat(tonumber(setMaxFeetTraveledText.text)); end
	}
	sceneGroup:insert( setStatBtn )

	averageSpeedText = display.newText({text = "Average Speed: " , x = centerX*3/2, y = 300, fontSize = 15})
	sceneGroup:insert( averageSpeedText )

	getAverageStatBtn = widget.newButton
	{ 
		x = centerX,
		y = 300,
		width = 298,
		height = 25,
		label = "Get Average Stat",
		onRelease = function() averageSpeedText.text = "Average Speed: "..tostring(getStat("AverageSpeed")); end
	}
	sceneGroup:insert( getAverageStatBtn )

	setAverageSpeedDataText = display.newText({text = "0" , x = centerX*4/3, y = 350, fontSize = 15})
	sceneGroup:insert( setAverageSpeedDataText )

	setAverageSpeedDataStepper = widget.newStepper(
	{
		x = centerX*4/3,
		y = 375,
		onPress = setAverageSpeedDataStepperPress
	})
	sceneGroup:insert( setAverageSpeedDataStepper )

	setAverageSpeedTimeText = display.newText({text = "0" , x = centerX*5/3, y = 350, fontSize = 15})
	sceneGroup:insert( setAverageSpeedTimeText )

	setAverageSpeedTimeStepper = widget.newStepper(
	{
		x = centerX*5/3,
		y = 375,
		onPress = setAverageSpeedTimeStepperPress
	})
	sceneGroup:insert( setAverageSpeedTimeStepper )

	setAverageStatBtn = widget.newButton
	{ 
		x = centerX,
		y = 370,
		width = 298,
		height = 25,
		label = "Set Average Stat",
		onRelease = function() setAverageStat(tonumber(setAverageSpeedDataText.text), tonumber(setAverageSpeedTimeText.text)); end
	}
	sceneGroup:insert( setAverageStatBtn )



end
statsScene:addEventListener( "create", statsScene )

local function onAchievementsRowRender(event)
	local row = event.row
	local params = row.params
    -- Cache the row "contentWidth" and "contentHeight" because the row bounds can change as children objects are added
    local rowHeight = row.contentHeight
    local rowWidth = row.contentWidth
    local idText = display.newText({text = tostring(params.name), y = rowHeight/3, x = 130, fontSize = 12})
    local unlockedText = display.newText({text = tostring(params.unlocked), y = rowHeight/3, x = 260, fontSize = 12})
    if type(params.unlockTime) == "number" then
    	params.unlockTime = os.date("%c", params.unlockTime)
    end
    local unlockTimeText = display.newText({text = tostring(params.unlockTime), y = rowHeight/3, x = 390, fontSize = 12})
    local nameText = display.newText({text = tostring(params.nameText), y = rowHeight/3, x = 520, fontSize = 12})
    local descText = display.newText({text = tostring(params.desc), y = rowHeight*2/3, x = rowWidth/2, fontSize = 12})

    idText:setFillColor( 0, 0, 0 )
    unlockedText:setFillColor( 0, 0, 0 )
    unlockTimeText:setFillColor( 0, 0, 0 )
    nameText:setFillColor( 0, 0, 0 )
    descText:setFillColor( 0, 0, 0 )

    row:insert(idText)
    row:insert(unlockedText)
    row:insert(unlockTimeText)
    row:insert(nameText)
    row:insert(descText)
end

local function updateAchievementsTable()
	achievementsTable:deleteAllRows()
	local numOfAchievements = gameNetwork.request("getNumAchievements")
	local params = {}
	params.name = "ID"
	params.unlocked = "Unlocked"
	params.unlockTime = "Unlock Time"
	params.nameText = "Name"
	params.desc = "Description"
	achievementsTable:insertRow( {params = params} )

	for i = 1, numOfAchievements do
		local params = {}
		params.name = gameNetwork.request("getAchievementName", { i=i } )
		if achievementsScreen == 1 then
			params.unlocked = gameNetwork.request("getAchievement", {name = params.name} )
			params.unlockTime = gameNetwork.request("getAchievementProperty", {name = params.name, type = "unlockTime"})
		else
			params.unlocked = gameNetwork.request("getAchievement", {name = params.name, user = 76561198090314038} )
			params.unlockTime = gameNetwork.request("getAchievementProperty", {name = params.name, type = "unlockTime", user = 76561198090314038})
		end
		params.nameText = gameNetwork.request("getAchievementProperty", {name = params.name, type = "name"})
		params.desc = gameNetwork.request("getAchievementProperty", {name = params.name, type = "desc"})
		achievementsTable:insertRow( {params = params} )
	end
end

local function onMyAchievementsTabPress(event)
	achievementsScreen = 1
	setWinnerAchievementBtn.isVisible = true
	clearWinnerAchievementBtn.isVisible = true
	setChampionAchievementSlider.isVisible = true
	setChampionAchievementBtn.isVisible = true
	clearChampionAchievementBtn.isVisible = true

	storeAchievementsBtn.isVisible = true
	resetAchievementsBtn.isVisible = true

	updateAchievementsTable()
end

local function onUserAchievementsTabPress(event)
	achievementsScreen = 2
	setWinnerAchievementBtn.isVisible = false
	clearWinnerAchievementBtn.isVisible = false
	setChampionAchievementSlider.isVisible = false
	setChampionAchievementBtn.isVisible = false
	clearChampionAchievementBtn.isVisible = false

	storeAchievementsBtn.isVisible = false
	resetAchievementsBtn.isVisible = false

	updateAchievementsTable()
end

function achievementsScene:create( event )
	local sceneGroup = self.view

	achievementTabButtons = 
	{
		{
			width = 32, 
			height = 32,
			label = "My Achievements",
			size = 15,
			selected = true,
			onPress = onMyAchievementsTabPress
		},
		{
			width = 32,
			height = 32,
			size = 15,
			label = "User Achievements",
			onPress = onUserAchievementsTabPress
		},
	}
		
	-- create a tab-bar and place it at the bottom of the screen
	achievementTabs = widget.newTabBar
	{
		top = 0,
		width = display.contentWidth,
		tabSelectedFrameWidth = 20,
		tabSelectedFrameHeight = 52,
		buttons = achievementTabButtons
	}
	sceneGroup:insert( achievementTabs )


	achievementsTable = widget.newTableView(
	{
		x = centerX+100,
		y = centerY,
		height = _H-200,
		width = _W-300,
		onRowRender = onAchievementsRowRender
	})
	sceneGroup:insert(achievementsTable)

	updateAchievementsTable()

	setWinnerAchievementBtn = widget.newButton
	{ 
		x = centerX/4,
		y = 100,
		width = 298,
		height = 25,
		label = "Set 'Winner' Achievement",
		onRelease = function() gameNetwork.request("setAchievement", {name = "ACH_WIN_ONE_GAME"}); end
	}
	sceneGroup:insert( setWinnerAchievementBtn )

	clearWinnerAchievementBtn = widget.newButton
	{ 
		x = centerX/4,
		y = 150,
		width = 298,
		height = 25,
		label = "Clear 'Winner' Achievement",
		onRelease = function() gameNetwork.request("clearAchievement", {name = "ACH_WIN_ONE_GAME"}); end
	}
	sceneGroup:insert( clearWinnerAchievementBtn )

	setChampionAchievementSlider = widget.newSlider
	{
	    x = centerX/4,
		y = 250,
	    orientation = "horizontal",
	    height = 200,
	    value = 0,
	    listener = function(event) setChampionAchievementSlider.value = event.value; end
	}
	sceneGroup:insert( setChampionAchievementSlider )

	setChampionAchievementBtn = widget.newButton
	{ 
		x = centerX/4,
		y = 300,
		width = 298,
		height = 25,
		label = "Set 'Champion' Achievement",
		onRelease = function() gameNetwork.request("setAchievement", {name = "ACH_WIN_100_GAMES", type = "progress", currProgress = setChampionAchievementSlider.value, maxProgress = 100}); end
	}
	sceneGroup:insert( setChampionAchievementBtn )

	clearChampionAchievementBtn = widget.newButton
	{ 
		x = centerX/4,
		y = 350,
		width = 298,
		height = 25,
		label = "Clear 'Champion' Achievement",
		onRelease = function() gameNetwork.request("clearAchievement", {name = "ACH_WIN_100_GAMES"}); end
	}
	sceneGroup:insert( clearChampionAchievementBtn )

	storeAchievementsBtn = widget.newButton
	{ 
		x = centerX/4,
		y = 420,
		width = 298,
		height = 25,
		label = "Store Achievements",
		onRelease = function() gameNetwork.request("storeStats"); end
	}
	sceneGroup:insert( storeAchievementsBtn )

	resetAchievementsBtn = widget.newButton
	{ 
		x = centerX/4,
		y = 450,
		width = 298,
		height = 25,
		label = "Reset All Stats and Achievements",
		onRelease = function() resetStats(true); end
	}
	sceneGroup:insert( resetAchievementsBtn )

	updateTableBtn = widget.newButton
	{ 
		x = centerX/4,
		y = 480,
		width = 298,
		height = 25,
		label = "Update Table",
		onRelease = function() updateAchievementsTable(); end
	}
	sceneGroup:insert( updateTableBtn )



end
achievementsScene:addEventListener( "create", achievementsScene )

function updateLeaderboardInfo(leaderboard)

	leaderboardNameText.text = "Leaderboard Name: "..gameNetwork.request("getLeaderboardName", {leaderboard = leaderboard})
	leaderboardEntryCountText.text = "Entry Count: "..gameNetwork.request("getLeaderboardEntryCount", {leaderboard = leaderboard})
	leaderboardSortMethodText.text = "Sort Method: "..gameNetwork.request("getLeaderboardSortMethod", {leaderboard = leaderboard})
	leaderboardDisplayTypeText.text = "Display Type: "..gameNetwork.request("getLeaderboardDisplayType", {leaderboard = leaderboard})

	gameNetwork.request("downloadLeaderboardEntries", {leaderboard = leaderboard, dataRequest = "globalAroundUser", rangeStart = -3, rangeEnd = 10})

end

function updateLeaderboardEntries(entries, count)
	leaderboardTable:deleteAllRows()
	local params = {}
	params.user = "User"
	params.rank = "Rank"
	params.score = "Score"
	leaderboardTable:insertRow({params = params})
	for i = 1, count do
		local params = {}
		local entry = gameNetwork.request("getDownloadedLeaderboardEntry", {entries = entries, index = i})
		params.user = entry.user
		params.rank = entry.rank
		params.score = entry.score
		leaderboardTable:insertRow({params = params})
	end
end

local function onLeaderboardRowRender(event)
	local row = event.row
	local params = row.params
    -- Cache the row "contentWidth" and "contentHeight" because the row bounds can change as children objects are added
    local rowHeight = row.contentHeight
    local rowWidth = row.contentWidth
    local userText = display.newText({text = tostring(params.user), y = rowHeight/2, x = 130, fontSize = 15})
    local rankText = display.newText({text = tostring(params.rank), y = rowHeight/2, x = 260, fontSize = 15})
    local scoreText = display.newText({text = tostring(params.score), y = rowHeight/2, x = 390, fontSize = 15})

    userText:setFillColor( 0, 0, 0 )
    rankText:setFillColor( 0, 0, 0 )
    scoreText:setFillColor( 0, 0, 0 )

    row:insert(userText)
    row:insert(rankText)
    row:insert(scoreText)
end

local function setLeaderboardStatStepperPress(event)
	if ( "increment" == event.phase ) then
        setLeaderboardStatStepperText.text = tonumber(setLeaderboardStatStepperText.text) + 10
    elseif ( "decrement" == event.phase ) then
        setLeaderboardStatStepperText.text = tonumber(setLeaderboardStatStepperText.text) - 10
    end
end

function leaderboardsScene:create( event )
	local sceneGroup = self.view
	
	leaderboardNameText = display.newText({text = "Leaderboard Name: " , x = centerX/4, y = 150, fontSize = 15, width = 150})
	sceneGroup:insert( leaderboardNameText )

	leaderboardEntryCountText = display.newText({text = "Entry Count: " , x = centerX/4, y = 200, fontSize = 15, width = 150})
	sceneGroup:insert( leaderboardEntryCountText )

	leaderboardSortMethodText = display.newText({text = "Sort Method: " , x = centerX/4, y = 250, fontSize = 15, width = 150})
	sceneGroup:insert( leaderboardSortMethodText )

	leaderboardDisplayTypeText = display.newText({text = "Display Type: " , x = centerX/4, y = 300, fontSize = 15, width = 150})
	sceneGroup:insert( leaderboardDisplayTypeText )

	leaderboardTable = widget.newTableView(
	{
		x = centerX+200,
		y = centerY,
		height = _H-200,
		width = _W-400,
		onRowRender = onLeaderboardRowRender
	})
	sceneGroup:insert(leaderboardTable)

	gameNetwork.request("findLeaderboard", {name = "Quickest Win"})


	setLeaderboardStatStepperText = display.newText({text = "0" , x = centerX/4, y = 350, fontSize = 15})
	sceneGroup:insert( setLeaderboardStatStepperText )

	setLeaderboardStatStepper = widget.newStepper(
	{
		x = centerX/4,
		y = 375,
		onPress = setLeaderboardStatStepperPress
	})
	sceneGroup:insert( setLeaderboardStatStepper )

	setLeaderboardStatBtn = widget.newButton
	{ 
		x = centerX/4,
		y = 425,
		width = 298,
		height = 25,
		label = "Set Leaderboard Stat",
		onRelease = function() gameNetwork.request("uploadLeaderboardScore", {leaderboard = currentLeaderboard, uploadScoreMethod = "forceUpdate", score = tonumber(setLeaderboardStatStepperText.text)}); gameNetwork.request("findLeaderboard", {name = "Quickest Win"}); end
	}
	sceneGroup:insert( setLeaderboardStatBtn )

end
leaderboardsScene:addEventListener( "create", leaderboardsScene )

menuTabButtons = 
{
	{
		width = 32, 
		height = 32,
		label = "Stats",
		size = 15,
		onPress = function() composer.gotoScene( "statsScene" ); end,
		selected = true
	},
	{
		width = 32,
		height = 32,
		size = 15,
		label = "Achievements",
		onPress = function() composer.gotoScene( "achievementsScene" ); end,
	},
	{
		width = 32,
		height = 32,
		size = 15,
		label = "Leaderboards",
		onPress = function() composer.gotoScene( "leaderboardsScene" ); end,
	},
}
	
-- create a tab-bar and place it at the bottom of the screen
menuTabs = widget.newTabBar
{
	top = _H-50,
	width = display.contentWidth,
	tabSelectedFrameWidth = 20,
	tabSelectedFrameHeight = 52,
	buttons = menuTabButtons
}

composer.gotoScene( "statsScene" )

function updateCallbackText( text )
	transition.cancel(callbackText)
	callbackText.alpha = 1
	callbackText.text = text
	transition.fadeOut(callbackText, {delay = 5000})
end

local function runCallbacks()
	gameNetwork.request("runCallbacks")
end

Runtime:addEventListener("enterFrame", runCallbacks)



