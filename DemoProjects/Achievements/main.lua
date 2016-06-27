local steamworks = require("plugin.steamworks")
local json = require('json')
local widget = require('widget')


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

	local obj = nil
	if params.img and params.img > 0 then
		local tex = steamworks.newTexture(params.img)
		obj = display.newImageRect(row, tex.filename, tex.baseDir, 64, 64 )
		tex:releaseSelf()
		-- this is very similar to following:
		-- obj = steamworks.newImage(params.img)
		-- row:insert(obj)
	else
		obj = display.newRect( row, 0, 0, 64, 64 )
		obj.fill = {0.2}
	end
	obj:translate( 58, h*0.5 )
	obj.stroke =  { 0, 0, 0 }
	obj.strokeWidth = 0.6
	params.obj = obj

	t = display.newText( {
		parent = row,
		y = h*0.30,
		x = 58+32+10,
		text = params.name,
		font = native.systemFontBold,
		align = "left",
	} )
	t:setFillColor( {0,0,} )
	t:translate( t.width*0.5, 0 )

	t = display.newText( {
		parent = row,
		y = h*0.7,
		x = 58+32+10,
		text = params.desc,
		align = "left",
	} )
	t:setFillColor( {0,0,} )
	t:translate( t.width*0.5, 0 )

end

local achievementsTable = widget.newTableView({
	onRowRender = onRowRender
})


local function achivementIconFetched(event)	
	for i=1,achievementsTable:getNumRows() do
		local row = achievementsTable:getRowAtIndex(i)
		if row and row.params and row.params.aName == event.achievementName then
			local params = row.params
			params.img = event.imageId
			local tex = steamworks.newTexture(params.img)
			params.obj.fill = {type="image", filename=tex.filename, baseDir=tex.baseDir}
			tex:releaseSelf()
			break
		end
	end
end
steamworks.addEventListener("achivementIconFetched", achivementIconFetched)

local numAchievements = steamworks.getNumAchievements()
if numAchievements then

	achievementsTable:insertRow( {
		rowHeight = 35,
		params = {
			caption = "Achievements",
			isCategory = true,
		}
	} )

	for i=1,numAchievements do
		local aName = steamworks.getAchievementName(i)
		local achiv = steamworks.getAchievementInfo(aName)
		if not achiv.hidden then
			local img = steamworks.getAchievementImageId(aName)
			achievementsTable:insertRow( {
				rowHeight=67, 
				params={ 
					img = img or 0,
					name = achiv.localizedName,
					desc = achiv.localizedDescription,
					aName = aName,
				}
			} )

		end                    
	end
else
	achievementsTable:insertRow( {
		rowHeight = 35,
		params = {
			caption = "Error: Steam must be running!",
			isCategory = true,
		}
	} )
end
