local steamworks = require("plugin.steamworks")
local json = require('json')
local widget = require('widget')

local page = 1
local perPage = 15
local scope = "Global"
local imgSize = "small"
local rowHeight = 18

transition.to( display.newGroup( ), {x=100, iterations=-1} )


display.setStatusBar( display.HiddenStatusBar )



local function onRowRender( event )
	local row = event.row
	local w,h = row.contentWidth, row.contentHeight
	local params = row.params

	if params.caption then
		local t = display.newText( {
			parent = row,
			y = h*0.5,
			x = w*0.5,
			text = params.caption ,
			fontSize = 16,
			font = native.systemFontBold,
		} )
		t:setFillColor( {0,0,} )
		return
	end

	local t = display.newText( {
		parent = row,
		y = h*0.5,
		x = 40,
		text = tostring( params.rank ),
		align = "right"
	} )
	t:setFillColor( {0,0,} )
	t:translate( -t.width*0.5, 0 )

	local obj
	local imgDims = rowHeight-2
	if params.rank == 1 then
		obj = display.newCircle( row, 0, 0, imgDims*0.5 )
	else
		obj = display.newRect( row, 0, 0, imgDims, imgDims )
	end
	if params.img and params.img > 0 then
		local tex = steamworks.newTexture(params.img)
		obj.fill = { type="image", filename=tex.filename, baseDir=tex.baseDir }
		tex:releaseSelf()
	else
		obj.fill = {0.5}
	end
	obj:translate( 40 + imgDims*0.5 + 5, h*0.5 )
	params.obj = obj

	t = display.newText( {
		parent = row,
		y = h*0.5,
		x = 40 + imgDims + 10,
		text = params.name,
		align = "left"
	} )
	t:setFillColor( {0,0,} )
	t:translate( t.width*0.5, 0 )

	t = display.newText( {
		parent = row,
		y = h*0.5,
		x = w-10,
		text = tostring(params.score),
		align = "right",
		font = "Courier New"
	} )
	t:setFillColor( {0,0,} )
	t:translate( -t.width*0.5, 0 )
end

local leaderboardTable


-- steam will produce this event onlyf or "large" image size
local function avatarFetched(event)	
	-- print(require("json").prettify(event))
	for i=1,leaderboardTable:getNumRows() do
		local row = leaderboardTable:getRowAtIndex(i)
		if row and row.params and row.params.user == event.userSteamId then
			local params = row.params
			params.img = event.imageId
			local tex = steamworks.newTexture(params.img)
			params.obj.fill = {type="image", filename=tex.filename, baseDir=tex.baseDir}
			tex:releaseSelf()
			break
		end
	end
end
steamworks.addEventListener("avatarFetched", avatarFetched)


local function leaderboardLoaded(event)
	native.setActivityIndicator( false )

	leaderboardTable:deleteAllRows()

	if event.isError then
		leaderboardTable:insertRow( {
			rowHeight = 36,
			params = {
				caption = "Error while fetching leaderboard",
				isCategory = true,
			}
		} )	
	end

	leaderboardTable:insertRow( {
		rowHeight = 36,
		params = {
			caption = event.leaderboardName,
			isCategory = true,
		}
	} )

	if #event.entries == 0 then
		leaderboardTable:insertRow( {
			rowHeight = 36,
			params = {
				caption = "No results for '" .. scope .. "' scope",
				isCategory = true,
			}
		} )		
	end

	for i=1,#event.entries do
		local e = event.entries[i]
		local img = steamworks.getUserAvatarImageId({userSteamId=e.userSteamId, size=imgSize})
		local userInfo = steamworks.getUserInfo(e.userSteamId)
		local name = "[unknown]"
		if userInfo and userInfo.name then
			name = userInfo.name
			if userInfo.nickname ~= "" then
				name = name .. " (" .. userInfo.nickname ..")"
			end
		end
		leaderboardTable:insertRow( {
			rowHeight=rowHeight, 
			params={ 
				img = img or 0,
				rank = e.globalRank,
				score = e.score,
				steamId = e.userSteamId,
				name = name,
				user = e.userSteamId,
			}
		} )
	end
end


local function requestLeaderboard( )
	if steamworks.requestLeaderboardEntries({
		leaderboardName = "Feet Traveled",
		range = { 1+perPage*(page-1), perPage*page },
		listener = leaderboardLoaded,
		playerScope = scope,
	}) then
		native.setActivityIndicator( true )
	else
		leaderboardTable:deleteAllRows()
		leaderboardTable:insertRow( {
			rowHeight = 36,
			params = {
				caption = "Error: Steam is not running!",
				isCategory = true,
			}
		} )	
	end
end

local function onRowTouch( event )
	local phase = event.phase
	local row = event.target
	if ( "release" == phase ) then
		steamworks.showWebOverlay( "http://steamcommunity.com/profiles/" .. row.params.user )
	end
end


leaderboardTable = widget.newTableView({
	onRowRender = onRowRender,
	onRowTouch = onRowTouch,
})

-- image size control

local function imageSize( event )
	local size = event.target.segmentNumber
	if size == 1 then
		imgSize = "small"
		rowHeight = 18
	elseif size == 2 then
		imgSize = "medium"
		rowHeight = 36
	elseif size == 3 then
		imgSize = "large"
		rowHeight = 54
	end
	requestLeaderboard()
end

local sizeSegment = widget.newSegmentedControl {
    segments = { "Small", "Medium", "Large" },
    defaultSegment = 1,
	segmentWidth = 55,
    onPress = imageSize
}

-- scope controls

local function leaderboardScope( event )
	local segNum = event.target.segmentNumber
	if segNum == 1 then
		scope = "Global"
	elseif segNum == 2 then
		scope = "GlobalAroundUser"
	elseif segNum == 3 then
		scope = "FriendsOnly"
	end
	requestLeaderboard()
end

local scopeSegment = widget.newSegmentedControl {
    segments = { "Global", "Local", "Friends" },
    defaultSegment = 1,
	segmentWidth = 55,
    onPress = leaderboardScope
}


-- page controls

local function pageListener( event )
	page = event.value
	requestLeaderboard()
end

local pageStepper = widget.newStepper {
    initialValue = 1,
    minimumValue = 1,
    maximumValue = 200,
	timerIncrementSpeed = 2000,
	changeSpeedAtIncrement = 1,
    onPress = pageListener
}

local function reLayout( )
	leaderboardTable.width = display.contentWidth
	leaderboardTable.height = display.contentHeight - 40
	leaderboardTable.x = display.contentCenterX
	leaderboardTable.y = display.contentCenterY - 20
	sizeSegment.x = sizeSegment.width*0.5 + 10
	sizeSegment.y = display.contentHeight-20
	scopeSegment.x = display.contentWidth - scopeSegment.width*0.5 - 10
	scopeSegment.y = display.contentHeight-20
	pageStepper.x = display.contentCenterX
	pageStepper.y = display.contentHeight-20
end

reLayout()
Runtime:addEventListener( "resize", reLayout )


requestLeaderboard()





