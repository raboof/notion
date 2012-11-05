-- Authors: Sadrul Habib Chowdhury <imadil@gmail.com>
-- License: Public domain
-- Last Changed: Unknown
--
-- rss_feed.lua
--
-- Use:
-- You have to dopath() this script for it to work. I have set a binding
-- MOD1+F12 for a popup-menu. You can also add %rss to the statusbar, which
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

if not rss_feed then
  rss_feed = {
    interval=250,       -- how often does the scroller update?
    width = 40,         -- the width of the scroller.. duh
    refresh = 30 * (60*1000),    -- how often to pull from the server?
    url = {             -- guess what
        {"Slashdot", "http://slashdot.org/index.rss"},
        {"CNN", "http://rss.cnn.com/rss/cnn_latest.rss"},
        {"BBC", "http://newsrss.bbc.co.uk/rss/newsonline_world_edition/front_page/rss.xml"},
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
        rss_string = rss_string .. " --- "..src.."# "..clean(info[1])
    end
end

--
-- If you have been looking for the worst possible rss-parser ever,
-- it's your lucky day.
--
local function get_next(str)
    local t, d, r = nil,"",""

    if str == nil then return t,d,r end
    __, _ = string.find(str, "<item[^s]")
    if not __ then return t,d,r end
    
    r = string.sub(str, _)
    __, _ = string.find(r, "<title>")
    if not __ then return t,d,r end
    e, s = string.find(r, "</title>")
    if not e then return t,d,r end
    t = string.sub(r, _+1, e-1)
    r = string.sub(r, s+1)
    
    __, _ = string.find(r, "<description>")
    if not __ then return t,d,r end
    e, s = string.find(r, "</description>")
    if not e then return t,d,r end
    d = string.sub(r, _+1, e-1)
    r = string.sub(r, s+1)
    
    return t,d,r
end

local function parse_rss(str)
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
    title, desc, data = get_next(data)
    while title do
        title = clean(title)
        desc = clean(desc)
        rss_desc = rss_desc .. "\nTitle: "..title.."\n"..desc.."\n"
        table.insert(new, {title, desc})
        title,desc,data = get_next(data)
    end
    feeds[src] = new
    rss_string = ""
    table.foreach(feeds, reconstruct_rss)
    rss_length = string.len(rss_string)
    rss_string = rss_string .. string.sub(rss_string, 1, rss_feed.width)
    count = 1
    rss_timer:set(rss_feed.refresh, retrieve)
end

local function retrieve_rss(src, str)
    ioncore.popen_bgread("echo "..src.."&& curl " .. str .. " 2>/dev/null",
                    coroutine.wrap(parse_rss))
end

--
-- pull feed from each server
-- 
function retrieve()
    local str = ""
    for _, entry in pairs(rss_feed.url) do
        retrieve_rss(entry[1], entry[2])
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
    table.insert(ret, menuentry("Re-read from server", "retrieve()"))
    for title, tbl in pairs(feeds) do
        count = 1
        table.insert(ret, submenu("RSS Feeds from "..title, sub_rss_menu(tbl)))
    end
    return ret
end

init_rss()

ioncore.defbindings("WScreen", {
    kpress(MOD1.."Shift+D", "mod_query.message(_, rss_desc)"),
    kpress(MOD1.."F12", 'mod_menu.bigmenu(_, _sub, show_menu)'),
})

