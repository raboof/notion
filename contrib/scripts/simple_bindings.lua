-- Authors: Etan Reisner <deryni@gmail.com>
-- License: MIT, see http://opensource.org/licenses/mit-license.php
-- Last Changed: 2007-08-17
--
--[[
Author: Etan Reisner
Email: deryni@gmail.com
Summary: Read a simplified binding file to let people set up some custom
         bindings without needing to mess with lua directly. To use create a
         'bindings' file in ~/.ion3 .
Last Updated: 2007-08-17

Copyright (c) Etan Reisner 2007

This software is released under the terms of the MIT license. For more
information, see http://opensource.org/licenses/mit-license.php .
--]]

--[[
Examples:

go_to_workspace1 = "M-S-1"
go_to_workspace4 = "M-S-4"
xterm = "M-x"
urxvt = "M-u"
--]]

--[[
Available bindings

attach
detach
go_to_workspace#
next_frame_up
next_frame_down
next_frame_left
next_frame_right
move_tab_left
move_tab_right
split_horiz
split_vert
split_horiz_top
split_vert_top

any other single word will be interpreted as the name of a program to run.
--]]

--[[
cycle windows
--]]

--{{{ Standalone hacks

if not ioncore then
    ioncore = {get_paths = function() return {userdir="/home/deryni/.ion3"} end}
    warn = print
    loadstring = function(str) return function() print(str) end end
end

--}}} Standalone hacks

local bindings = {}

bindings.go_to_workspace = {section = "WScreen", funcstr = "WScreen.switch_nth(_, NUM)"}

bindings.split_at = {section = "WTiling", funcstr = "WTiling.split_at(_, _sub, 'DIR', true)"}
bindings.split_top = {section = "WTiling", funcstr = "WTiling.split_top(_, 'DIR')"}

bindings.next_tab = {section = "WFrame.toplevel", funcstr = "WFrame.switch_next(_)"}
bindings.prev_tab = {section = "WFrame.toplevel", funcstr = "WFrame.switch_prev(_)"}

bindings.next_frame = {section = "WTiling", funcstr = "ioncore.goto_next(_sub, 'DIR', {no_ascend=_})"}

bindings.move_tab = {section = "WFrame.toplevel", funcstr = "WFrame.DIR_index(_, _sub)"}

bindings.tag = {section = "WFrame.toplevel", funcstr = "WRegion.set_tagged(_sub, 'toggle')"}
bindings.attach_tagged = {section = "WFrame.toplevel", funcstr = "ioncore.tagged_attach(_)"}

bindings.close_window = {section = "WMPlex", funcstr = "WRegion.rqclose_propagate(_, _sub)"}
bindings.kill_window = {section = "WClientWin", "WClientWin.kill(_)"}

bindings.detach = {section = "WMPlex", funcstr = "ioncore.detach(_chld, 'toggle')"}

bindings.exec = {section = "WMPlex.toplevel", funcstr = "ioncore.exec_on(_, 'PROG')"}

local paths, chunk, ok, err, fenv

paths = ioncore.get_paths()
chunk, err = loadfile(paths.userdir.."/bindings")

if not chunk then
    warn("Failed to load bindings file")
    return
end

fenv = {}
chunk = setfenv(chunk, fenv)
ok, err = pcall(chunk)

if not ok then
    warn("Failed to run the bindings file: "..err)
    return
end

local sections = {}

for k,v in pairs(fenv) do
    local section, bindstr, keystr

    keystr = v:gsub("([MCS])%-", {M = "Mod1+", C = "Control+", S = "Shift+"})

    if bindings[k] then
        bindstr = bindings[k].funcstr
        section = bindings[k].section
    end

    if (not bindstr) and k:match("^..tach$") then
        bindstr = bindings.detach.funcstr
        section = bindings.detach.section
    end

    local action, num = k:match("^(go_to_workspace)(%d+)$")
    if (not bindstr) and action and num then
        bindstr = bindings[action].funcstr:gsub("NUM", num - 1)
        section = bindings[action].section
    end

    local dir = k:match("^next_frame_(%w+)$")
    if (not bindstr) and dir then
        bindstr = bindings.next_frame.funcstr:gsub("DIR", dir)
        section = bindings.next_frame.section
    end

    local dir = k:match("^move_tab_(%w+)$")
    if (not bindstr) and dir then
        local dir = (dir == "right") and "inc" or "dec"
        bindstr = bindings.move_tab.funcstr:gsub("DIR", dir)
        section = bindings.move_tab.section
    end

    local dir, top = k:match("^split_([^_]+)_?(%w*)")
    if (not bindstr) and dir then
        local action = (top == "top") and "split_top" or "split_at"
        local dir = (dir == "horiz") and "right" or "bottom"
        bindstr = bindings[action].funcstr:gsub("DIR", dir)
        section = bindings[action].section
    end

    if not bindstr then
        bindstr = bindings.exec.funcstr:gsub("PROG", k)
        section = bindings.exec.section
    end

    if not sections[section] then
        sections[section] = {}
    end
    section = sections[section]
    section[#section + 1] = {bindstr = bindstr, keystr = keystr}
end

for section, tab in pairs(sections) do
    local str = [[defbindings("]]..section.."\", {\n"
    for i,bind in pairs(tab) do
        str = str.."\tkpress(\""..bind.keystr..[[", "]]..bind.bindstr.."\"),\n"
    end
    str = str.."})"
    loadstring(str)()
end

-- vim: set expandtab sw=4:
