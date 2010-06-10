-- rss_feed.lua
--
-- Use:
-- You have to dopath() this script for it to work. I have set a binding
-- MOD1+F11 for a popup-menu. You can also add %rss to the statusbar, which
-- will show a scrolling list of titles. You might want to give it a try
-- before disabling. :)
--
-- NOTES:
-- This is not a useful script. This script was written just to demonstrate
-- what idle time can do to a man.
--
-- Author:
-- Sadrul Habib Chowdhury (Adil)
-- imadil at gmail dot com
--
-- Modified by Henning Hasemann
-- hhasemann at web dot de
--
-- Changes
-- * Added Flag which allows to toggle if a feed will be displayed in
--   the statusbar
-- * A feed in the menu now opens the coresponding url in firefox
-- * Now uses MOD1+F11 by default, so it doesnt overlap with the
--   menu anymore
-- * I found the source-information in the statusbar distrubing so I removed
--   it

if not rss_feed then
  rss_feed = {
    interval=250,       -- how often does the scroller update?
    width = 40,         -- the width of the scroller.. duh
    refresh = 30 * (60*1000),    -- how often to pull from the server?
    url = {             -- guess what
        {"Slashdot", "http://rss.slashdot.org/Slashdot/slashdot", 1},
        {"GLSA", "http://www.gentoo.org/rdf/en/glsa-index.rdf", 0},
        {"TV 20:15", "http://www.tvmovie.de/tv-programm/2015rss.xml", 1},
        -- add other sites
    }
  }
end

local scroller_timer = nil  -- the timer for the scroller
local rss_timer = nil       -- timer for the rss refresh
rss_desc = ""
local rss_string = ""
local current = 1
local rss_length = 0
local feeds = {}

--
-- remove html-entities
-- 
local function clean(s)
    s = string.gsub(s, "\r", "")
    s = string.gsub(s, "\n", "")
    s = string.gsub(s, "&amp;", "&")
    s = string.gsub(s, "&quot;", "\"")
    s = string.gsub(s, "&nbsp;", " ")
    s = string.gsub(s, "&gt;", ">")
    s = string.gsub(s, "&lt;", "<")
    -- add any other conversions necessary here
    return s
end

local function reconstruct_rss(src, tbl)
    for _, info in pairs(tbl) do
      if info[4] == 1 then
            rss_string = rss_string .. " -- "..clean(info[1])
      end
    end
end

--
-- If you have been looking for the worst possible rss-parser ever,
-- it's your lucky day.
--
local function get_next(str)
    local t, d, r, l = nil,"","",""

    if str == nil then return t,d,r,l end
    __, _ = string.find(str, "<item[^s]")
    if not __ then return t,d,r,l end
    
    r = string.sub(str, _)
    __, _ = string.find(r, "<title>")
    if not __ then return t,d,r,l end
    e, s = string.find(r, "</title>")
    if not e then return t,d,r,l end
    t = string.sub(r, _+1, e-1)
    r = string.sub(r, s+1)

    r = string.sub(str, _)
    __, _ = string.find(r, "<link>")
    if not __ then return t,d,r,l end
    e, s = string.find(r, "</link>")
    if not e then return t,d,r,l end
    l = string.sub(r, _+1, e-1)
    r = string.sub(r, s+1)
    
    __, _ = string.find(r, "<description>")
    if not __ then return t,d,r,l end
    e, s = string.find(r, "</description>")
    if not e then return t,d,r,l end
    d = string.sub(r, _+1, e-1)
    r = string.sub(r, s+1)
    
    return t,d,r,l
end

local function parse_rss(str, show_in_sb, show_name)
    local data = ""
    while str do
        data = data .. str
        str = coroutine.yield()
    end

    local src = ""
    local ret = ""
    local new = {}

    __, _ = string.find(data, "[^\n]+")
    if __ then src = string.sub(data, __, _) end

    -- for title, desc in string.gfind(data,".-<item[^s].-<title>(.-)</title>.-<description>(.-)</description>.-</item>") do
    title, desc, data, link = get_next(data)
    while title do
        title = clean(title)
        desc = clean(desc)
        link = clean(link)
        rss_desc = rss_desc .. "\n"..title.."----------------------------\n"..desc.."\n"
        table.insert(new, {title, desc, link, show_in_sb, show_name})
        title,desc,data,link = get_next(data)
    end
    feeds[src] = new
    rss_string = ""
    table.foreach(feeds, reconstruct_rss)
    rss_length = string.len(rss_string)
    rss_string = rss_string .. string.sub(rss_string, 1, rss_feed.width)
    count = 1
    rss_timer:set(rss_feed.refresh, retrieve)
end

local function retrieve_rss(src, str, show_in_sb, show_name)
    local parse = coroutine.wrap(parse_rss)
    local interpret_line = function(str)
      parse(str, show_in_sb, show_name)
    end
    ioncore.popen_bgread("echo "..src.."&& curl " .. str .. " 2>/dev/null",
                    interpret_line)
end

--
-- pull feed from each server
-- 
function retrieve()
    local str = ""
    for _, entry in pairs(rss_feed.url) do
        retrieve_rss(entry[1], entry[2], entry[3], entry[4])
    end
end

local function get_string()
    return rss_string
end

local function scroll()
    local st = get_string()
    local show = string.sub(st, current, current+rss_feed.width)

    mod_statusbar.inform("rss", "- "..show.." -")
    mod_statusbar.update()
    scroller_timer:set(rss_feed.interval, scroll)
    current = current + 1
    if current > rss_length then
        current = 1
    end
end

local function init_rss()
    rss_timer = ioncore.create_timer()
    rss_length = rss_feed.width
    rss_string = string.rep('- ', rss_length)
    if mod_statusbar then
        -- do this iff mod_statusbar is loaded
        scroller_timer = ioncore.create_timer()
        mod_statusbar.inform("rss_template", string.rep("x", rss_feed.width))
        scroll()
    end
    retrieve()
end

function show_menu()
    local ret = {}
    local count = 1
    local function sub_rss_menu(tbl)
        local ret = {}
        for q, d in pairs(tbl) do
            local title = clean(d[1])
            local desc = clean(d[2])
            table.insert(ret, menuentry(tostring(count)..". "..title, 
                    "mod_query.message(_, '" .. string.gsub(desc, "'", "\\'") .. "')"))
            count = count + 1
        end
        if count == 1 then
            table.insert(ret, menuentry("No feed from server", "nil"))
        end
        return ret
    end
    local function view(tbl)
      local ret = {}
      for q, d in pairs(tbl) do
        local title = clean(d[1])
        local desc = clean(d[2])
        local link = clean(d[3])
        table.insert(ret, menuentry(tostring(count)..". "..title,
            "ioncore.popen_bgread('firefox " .. link .. "')"))
        count = count + 1
      end
      if count == 1 then
          table.insert(ret, menuentry("No feed from server", "nil"))
      end
      return ret
    end

    table.insert(ret, menuentry("Re-read from server", "retrieve()"))
    for title, tbl in pairs(feeds) do
        count = 1
        table.insert(ret, submenu("RSS Feeds from "..title, view(tbl)))
    end
    return ret
end

init_rss()

ioncore.defbindings("WScreen", {
    kpress(MOD1.."Shift+D", "mod_query.message(_, rss_desc)"),
    kpress(MOD1.."F11", 'mod_menu.bigmenu(_, _sub, show_menu)'),
})

