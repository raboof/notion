-- Ion xinerama module - lua setup
-- 
-- by Tomas Ebenlendr <ebik@ucw.cz>
--
-- This library is free software; you can redistribute it and/or
-- modify it under the terms of the GNU Lesser General Public
-- License as published by the Free Software Foundation; either
-- version 2.1 of the License,or (at your option) any later version.
--
-- This library is distributed in the hope that it will be useful,
-- but WITHOUT ANY WARRANTY; without even the implied warranty of
-- MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
-- Lesser General Public License for more details.
--
-- You should have received a copy of the GNU Lesser General Public
-- License along with this library; if not,write to the Free Software
-- Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
--

-- This is a slight abuse of the package.loaded variable perhaps, but
-- library-like packages should handle checking if they're loaded instead of
-- confusing the user with require/include differences.
if package.loaded["mod_xinerama"] then return end

if not ioncore.load_module("mod_xinerama") then
    return
end

local mod_xinerama=_G["mod_xinerama"]

assert(mod_xinerama)


-- Helper functions {{{

local function max(one, other) 
    if one == nil then return other end
    if other == nil then return one end

    return (one > other) and one or other
end

-- creates new table, converts {x,y,w,h} representation to {x,y,xmax,ymax}
local function to_max_representation(screen)
    return {
	x = screen.x,
	y = screen.y,
	xmax = screen.x + screen.w,
	ymax = screen.y + screen.h
    }
end

-- edits passed table, converts representation {x,y,xmax,ymax} to {x,y,w,h},
-- and sorts table of indices (entry screen.ids)
local function fix_representation(screen)
    screen.w = screen.xmax - screen.x
    screen.h = screen.ymax - screen.y
    screen.xmax = nil
    screen.ymax = nil
    table.sort(screen.ids)
end

local function fix_representations(screens)
    for _k, screen in pairs(screens) do
	fix_representation(screen)
    end
end

-- }}}

-- Contained screens {{{

-- true if [from1, to1] contains [from2, to2] 
local function contains(from1, to1, from2, to2)
    return (from1 <= from2) and (to1 >= to2)
end

-- true if scr1 contains scr2
local function screen_contains(scr1, scr2)
    local x_in = contains(scr1.x, scr1.xmax, scr2.x, scr2.xmax)
    local y_in = contains(scr1.y, scr1.ymax, scr2.y, scr2.ymax)
    return x_in and y_in
end

--DOC
-- Filters out fully contained screens. I.e. it merges two screens
-- if one is fully contained in the other screen (this contains the
-- case that both screens are of the same geometry).
-- The output screens also contain field ids containing the numbers
-- of merged screens. The order of the screens is defined by the
-- first screen in the merged set. (I.e., having big B, and big C,
-- showing different parts of desktop and small A (primary) showing
-- part of C, then the order will be C,B and not B,C as someone
-- may expect.
--
-- Example input format: \{\{x=0,y=0,w=1024,h=768\},\{x=0,y=0,w=1280,h=1024\}\}
function mod_xinerama.merge_contained_screens(screens)
    local ret = {}
    for newnum, _newscreen in ipairs(screens) do
	newscreen = to_max_representation(_newscreen)
	local merged = false
	for prevnum, prevscreen in pairs(ret) do
	    if screen_contains(prevscreen, newscreen) then
		table.insert(prevscreen.ids,newnum)
		merged = true
	    elseif screen_contains(newscreen, prevscreen) then
		prevscreen.x = newscreen.x
		prevscreen.y = newscreen.y
		prevscreen.xmax = newscreen.xmax
		prevscreen.ymax = newscreen.ymax
		table.insert(prevscreen.ids,newnum)
		merged = true
	    end
	    if merged then break end
	end
	if not merged then
	    newscreen.ids = { newnum }
	    table.insert(ret, newscreen)
	end
    end
    fix_representations(ret)
    return ret
end

-- }}} 

--- {{{ Overlapping screens

-- true if [from1, to1] overlaps [from2, to2] 
local function overlaps (from1, to1, from2, to2)
    return (from1 < to2) and (from2 < to1)
end

-- true if scr1 overlaps scr2
local function screen_overlaps(scr1, scr2)
    local x_in = overlaps(scr1.x, scr1.xmax, scr2.x, scr2.xmax)
    local y_in = overlaps(scr1.y, scr1.ymax, scr2.y, scr2.ymax)
    return x_in and y_in
end

--DOC
-- Merges overlapping screens. I.e. it finds set of smallest rectangles,
-- such that these rectangles do not overlap and such that they contain
-- all screens.
--
-- Example input format: \{\{x=0,y=0,w=1024,h=768\},\{x=0,y=0,w=1280,h=1024\}\}
function mod_xinerama.merge_overlapping_screens(screens)
    local ret = {}
    for _newnum, _newscreen in ipairs(screens) do
	local newscreen = to_max_representation(_newscreen)
	newscreen.ids = { _newnum }
	local overlaps = true
	local pos
	while overlaps do
	    overlaps = false
	    for prevpos, prevscreen in pairs(ret) do
		if screen_overlaps(prevscreen, newscreen) then
		    -- stabilise ordering
		    if (not pos) or (prevpos < pos) then pos = prevpos end
		    -- merge with the previous screen
		    newscreen.x = math.min(newscreen.x, prevscreen.x)
		    newscreen.y = math.min(newscreen.y, prevscreen.y)
		    newscreen.xmax = math.max(newscreen.xmax, prevscreen.xmax)
		    newscreen.ymax = math.max(newscreen.ymax, prevscreen.ymax)
		    -- merge the indices
		    for _k, _v in ipairs(prevscreen.ids) do
			table.insert(newscreen.ids, _v)
		    end
		    
		    -- delete the merged previous screen
		    table.remove(ret, prevpos)

		    -- restart from beginning
		    overlaps = true
		    break
		end
	    end
	end
	if not pos then pos = table.maxn(ret)+1 end
	table.insert(ret, pos, newscreen)
    end
    fix_representations(ret)
    return ret
end

--DOC
-- Merges overlapping screens. I.e. it merges two screens
-- if they overlap. It merges two screens if and only if there
-- is a path between them using only overlapping screens.
-- one is fully contained in the other screen (this contains the
-- case that both screens are of the same geometry).
-- The output screens also contain field ids containing the numbers
-- of merged screens. The order of the screens is defined by the
-- first screen in the merged set. (I.e., having big B, and big C,
-- showing different parts of desktop and small A (primary) showing
-- part of C, then the order will be C,B and not B,C as someone
-- may expect.
--
-- This function may output overlapping regions, AB and C on the example:
--         *-------*
-- *-----* |   C   |
-- |     | *-------*
-- |  A  |
-- |   +-+-------*
-- *---+-*       |
--     |    B    |
--     |         |
--     +---------*
-- Notion's WScreen implementation will (partially) hide C when AB is focused.
-- Thus this algorithm is not what you want by default.
--
-- Example input format: \{\{x=0,y=0,w=1024,h=768\},\{x=0,y=0,w=1280,h=1024\}\}
-- See test_xinerama.lua for example input/output
function mod_xinerama.merge_overlapping_screens_alternative(screens)
    -- Group overlapping screens into sets for merging.
    --         *-------*
    -- *-----* |   C   |
    -- |     | *-------*
    -- |  A  |
    -- |   +-+-------*
    -- *---+-*       |
    --     |    B    |
    --     |         |
    --     +---------*
    --
    -- Our algorithm merges A with B, but it does not
    -- merge C to 'AB'. This is due to we only identify
    -- overlapping screen sets in first phase.
    --
    --
    -- *-----*     *-----*
    -- | A +-+-----+-+ B |
    -- *---+-+  C  +-+---+
    --     +---------+
    --
    -- Our algoritm merges all three screens even if
    -- it first decides that A and B does not overlap,
    -- and then takes screen C.
    --
    local screensets = {}
    for _newnum, _newscreen in ipairs(screens) do
	newscreen = to_max_representation(_newscreen)
	newscreen.id = _newnum

	--Find all screensets to merge with:
	--if there is a screen in a screenset that overlaps
	--with current screen, then we mark the set in 'mergekeys'
	local mergekeys = {}
	-- We use ipairs here, because we rely on the order.
	for setkey, screenset in ipairs(screensets) do
	    -- find any screen of 'screenset' that overlaps 'newscreen'
	    for _k, prevscreen in pairs(screenset) do
		if screen_overlaps(newscreen, prevscreen) then
		    -- Found. 'setkey' contains indices of overlapping
		    -- 'screenset's, sorted decreasingly
		    table.insert(mergekeys, 1, setkey)
		    break
		end
	    end
	end

	-- Here we merge all marked screensets to one new screenset.
	-- We also delete the merged screensets from the 'screensets' table.
	local mergedset = {newscreen}
	local pos
	-- we use ipairs here, because we rely on the order.
	for _k, setkey in ipairs(mergekeys) do
	    -- copy contents of 'screensets[setkey]' to 'mergedset'
	    for _k2, prevscreen in pairs(screensets[setkey]) do
		table.insert(mergedset, prevscreen)
	    end
	    -- remove 'screensets[setkey]'
	    table.remove(screensets, setkey)
	    pos = setkey
	end

	-- pos keeps index of first set that we merged in this loop,
	-- we want to insert the product of this merge to pos.
	if not pos then pos = table.maxn(screensets)+1 end
	table.insert(screensets, pos, mergedset)
    end

    -- Now we have the screenset that contains the screens to be merged
    local ret = {}
    for _k, screenset in ipairs(screensets) do
	local newscreen = {
	    x = screenset[1].x,
	    xmax = screenset[1].xmax,
	    y = screenset[1].y,
	    ymax = screenset[1].ymax,
	    ids = {}
	}
	for _k2, screen in pairs(screenset) do
	    newscreen.x = math.min(newscreen.x, screen.x)
	    newscreen.y = math.min(newscreen.y, screen.y)
	    newscreen.xmax = math.max(newscreen.xmax, screen.xmax)
	    newscreen.ymax = math.max(newscreen.ymax, screen.ymax)
	    table.insert(newscreen.ids, screen.id)
	end
	table.insert(ret, newscreen)
    end
    fix_representations(ret)
    
    return ret
end

-- }}}

--- {{{ Setup ion's screens */

--DOC
-- Move a WScreen off the visible virtual screen. 
function mod_xinerama.move_offscreen(screen, max_right)
    local dimensions = mod_xinerama.get_screen_dimensions(screen)
    dimensions.x = max_right + 1
    mod_xinerama.update_screen(screen, dimensions) 
end

--DOC
-- Perform the setup of ion screens.
--
-- The first call sets up the screens of ion, subsequent calls update the
-- current screens
--
-- Returns true on success, false on failure
--
-- Example input: {{x=0,y=0,w=1024,h=768},{x=1024,y=0,w=1280,h=1024}}
function mod_xinerama.setup_screens(screens)
    local max_screen_id = 0
    local max_right

    for screen_index, screen in ipairs(screens) do
        local screen_id = screen_index - 1
        max_screen_id = max(max_screen_id, screen_id)        
        max_right = max(max_right, screen.x + screen.w)

        local existing_screen = ioncore.find_screen_id(screen_id)
        if existing_screen ~= nil then
            mod_xinerama.update_screen(existing_screen, screen)
        else
            mod_xinerama.setup_new_screen(screen_id, screen)
        end
    end

    -- TODO what to do when the number of screens is lower than last time
    -- this function was called? Remove the screen and store its contents
    -- somewhere else?

    -- for now move the screen to a location outside the virtual screen, so
    -- it can't be accidentally focussed and obscure the proper screens
    local invisible_screen_id = max_screen_id + 1
    local invisible_screen = ioncore.find_screen_id(invisible_screen_id)
    while invisible_screen do
        mod_xinerama.move_offscreen(invisible_screen, max_right)

        invisible_screen_id = invisible_screen_id + 1
        invisible_screen = ioncore.find_screen_id(invisible_screen_id)
    end
end

-- }}}

-- Mark ourselves loaded.
package.loaded["mod_xinerama"]=true

-- Load configuration file
dopath('cfg_xinerama', true)

--DOC
-- Queries Xinerama for the screen dimensions and updates ion screens 
-- accordingly
function mod_xinerama.refresh()
    local screens = mod_xinerama.query_screens()
    if screens then
        local merged_screens = mod_xinerama.merge_overlapping_screens(screens)
        mod_xinerama.setup_screens(merged_screens)
    end 
    notioncore.screens_updated(notioncore.rootwin());
end

mod_xinerama.refresh()
