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

-- Contained screens {{{

-- true if [from1, to1] contains [from2, to2] 
local function contains(from1, to1, from2, to2)
    return (from1 <= from2) and (to1 >= to2)
end

-- true if scr1 contains scr2
local function screen_contains(scr1, scr2)
    local x_in = contains(scr1.x, scr1.x+scr1.w, scr2.x, scr2.x+scr2.w)
    local y_in = contains(scr1.y, scr1.y+scr1.h, scr2.y, scr2.y+scr2.h)
    return x_in and y_in
end

-- Filters screens for fully contained screens.
-- The output screens also contain field ids containing the
-- numbers of merged screens.
function mod_xinerama.merge_contained_screens(screens)
    local ret = {}
    table.foreach(screens,function (newnum,newscreen)
	local merged = nil
	table.foreach(ret, function (prevnum,prevscreen)
	    if screen_contains(prevscreen, newscreen) then
		table.insert(prevscreen.ids,newnum)
		merged = true;
	    elseif screen_contains(newscreen, prevscreen) then
		prevscreen.x = newscreen.x
		prevscreen.y = newscreen.y
		prevscreen.w = newscreen.w
		prevscreen.h = newscreen.h
		table.insert(prevscreen.ids,newnum)
	    end
	    return merged
	end)
	if not merged then
	    newscreen.ids = { newnum }
	    table.insert(ret, newscreen)
	end
	return nil
    end)
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
    local x_in = overlaps(scr1.x, scr1.x+scr1.w, scr2.x, scr2.x+scr2.w)
    local y_in = overlaps(scr1.y, scr1.y+scr1.h, scr2.y, scr2.y+scr2.h)
    return x_in and y_in
end

-- Filters screens for overlapping screens.
-- The output screens also contain field ids containing the
-- numbers of merged screens.
function mod_xinerama.merge_overlapping_screens(screens)
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
    table.foreach(screens,function (newnum,newscreen)
	newscreen.id = newnum

	--Find all screensets to merge with:
	--if there is a screen in a screenset that overlaps
	--with current screen, then we mark the set in 'mergekeys'
	local mergekeys = {}
	table.foreach(screensets, function (setkey, screenset)
	    table.foreach(screenset, function(_k, prevscreen)
		if screen_overlaps(newscreen, prevscreen) then
		    -- we need the indices backwards
		    table.insert(mergekeys, 1, setkey)
		    return true
		end
		return nil
	    end)
	    return nil
	end)

	-- Here we merge all marked screensets to one new screenset.
	-- We also delete the merged screensets from the 'screensets' table.
	local mergedset = {newscreen}
	local pos
	table.foreach(mergekeys, function(_k, setkey)
	    table.foreach(screensets[setkey], function(_k2, prevscreen)
		table.insert(mergedset, prevscreen)
	    end)
	    table.remove(screensets, setkey)
	    pos = setkey
	end)

	table.insert(screensets, pos, mergedset)
    end)

    -- Now we have the screenset that contain the screens to be merged
    local ret = {}
    table.foreach(screensets, function(_k, screenset)
	local x1 = screenset[1].x
	local x2 = x1
	local y1 = screenset[1].y
	local y2 = y1
	local idset = {}
	table.foreach(screenset, function(_k2, screen)
	    if (x1 > screen.x) then x1 = screen.x end
	    if (x2 < screen.x + screen.w) then x2 = screen.x + screen.w end
	    if (y1 > screen.y) then y1 = screen.y end
	    if (y2 < screen.y + screen.h) then y2 = screen.y + screen.h end
	    table.insert(idset, screen.id)
	end)
	table.sort(idset)
	table.insert(ret, {
	    x = x1, y = y1, w = x2 - x1, h = y2 - y1, ids = idset
	})
    end)
    
    return ret
end

-- }}}


-- Mark ourselves loaded.
package.loaded["mod_xinerama"]=true


-- Load configuration file
dopath('cfg_xinerama', true)

local screens = mod_xinerama.query_screens();
if screens then
    mod_xinerama.setup_screens_once(mod_xinerama.merge_overlapping_screens(screens));
end 
