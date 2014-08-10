-- Authors: Sadrul Habib Chowdhury <imadil@gmail.com>
-- License: Public domain
-- Last Changed: Unknown
--
-- schedule.lua: schedule notification script
--
-- You can schedule some messages to show up in the statusbar at
-- specified times.
-- 
-- Syntax:
-- y <year> <month> <date> <hour> <minute> <message>
--      or
-- m <month> <date> <hour> <minute> <message>
--      or
-- d <date> <hour> <minute> <message>
--      or
-- <hour> <minute> <message>
--
-- Current year/month/date will be used if you do not specify one. Messages
-- need not be within quotes. Some examples follow:
--
-- Example:
-- y 2005 4 1 2 0 "April fool!!"    --> 1st April, 2005, 2 am
-- m 4 5 2 0 "Some message"         --> 5th April of the current year, 2 am
-- d 5 2 0 "Other message"          --> 5th of the current month and year, 2 am
-- 20 0 Last message                --> 8 pm of today
--
-- Note:
-- The script now saves notification information in a file in the userdir
-- (usually ~/.ion3/schedule_save.lua). So the notifications stay after a
-- restart.
--
-- Author:
-- Sadrul Habib Chowdhury (Adil)
-- imadil at gmail dot com

if not mod_statusbar then return end

--
-- how often should the script check for scheduled messages?
--

if not schedule then
  schedule = {
    interval = 60 * 1000,   -- check every minute
    save_filename = "schedule_save.lua"
  }
end

local timer = nil       -- the timer
local list = nil        -- doubly linked list of messages

--
-- extract the first token
--
function schedule.get_token(str)
    local bg, fn
    bg, fn = string.find(str, "%w+%s+")
    return string.gsub(string.sub(str, bg, fn), "%s+", ""),
            string.sub(str, fn+1)
end

--
-- show the notification when scheduled
--
function schedule.notify()
    local l = list
    local time = os.time()
    if l and time > l.time then
        io.stderr:write("schedule: " .. l.message .. "\n")
        mod_statusbar.inform("schedule", l.message)
        mod_statusbar.inform("schedule_hint", "important")
        mod_statusbar.update()
        list = l.next
        if list then
            list.prev = nil
        end
        l = nil
        schedule.save_file()
    end
    timer:set(schedule.interval, schedule.notify)
end

--
-- very hackish
--
function schedule.insert_into_list(time, message)
    local l = list
    local t = nil

    if os.time() > time then return false end

    if l == nil then
        l = {}
        l.prev = nil
        l.next = nil
        l.time = time
        l.message = message
        list = l
        return true
    end

    while l do
        if l.time > time then
            l = l.prev
            break
        end

        if l.next then
            l = l.next
        else
            break
        end
    end

    t = {}
    t.time = time
    t.message = message

    if l == nil then
        list.prev = t
        t.next = list
        t.prev = nil
        list = t
    else
        t.next = l.next
        t.prev = l
        l.next = t
    end
    return true
end

--
-- add messages in the queue
--
function schedule.add_message(mplex, str)
    local token = nil
    local tm = os.date("*t")

    token, str = schedule.get_token(str)

    if token == "y" then
        tm.year, str = schedule.get_token(str)
        tm.month, str = schedule.get_token(str)
        tm.day, str = schedule.get_token(str)
        tm.hour, str = schedule.get_token(str)
    elseif token == "m" then
        tm.month, str = schedule.get_token(str)
        tm.day, str = schedule.get_token(str)
        tm.hour, str = schedule.get_token(str)
    elseif token == "d" then
        tm.day, str = schedule.get_token(str)
        tm.hour, str = schedule.get_token(str)
    else
        tm.hour = token
    end

    tm.min, str = schedule.get_token(str)

    if schedule.insert_into_list(os.time(tm), str) then
        mod_query.message(mplex, "Notification \"" .. str .. "\" scheduled at " ..
                        os.date("[%a %d %h %Y] %H:%M", os.time(tm)))
        schedule.save_file()
    else
        mod_query.message(mplex, "dude, can't schedule notification for past!")
    end
end

function schedule.ask_message(mplex)
    mod_query.query(mplex, TR("Schedule notification:"), nil, schedule.add_message, 
            nil, "schedule")
end

-- 
-- show the scheduled notifications
--
function schedule.show_all(mplex)
    local l = list
    local output = "List of scheduled notifications:"
    while l do
        output = output .. "\n" .. os.date("[%a %d %h %Y] %H:%M", l.time) ..
                        " => " .. l.message
        l = l.next
    end
    mod_query.message(mplex, output)
end

--
-- save in file
--
function schedule.save_file()
    if not list then return end
    local t = ioncore.get_paths()    -- saving the file in userdir
    local f = io.open(t.userdir .."/".. schedule.save_filename, "w")
    if not f then return end

    local l = list
    while l do
        f:write("schedule.insert_into_list("..l.time..", \""..l.message.."\")\n")
        l = l.next
    end
    f:close()
end

--
-- the key bindings
--
defbindings("WMPlex", {
    -- you can change the key bindings to your liking
    kpress(META.."F5", "schedule.ask_message(_)"),
    kpress(META.."Shift+F5", function ()  -- clear notification
                mod_statusbar.inform("schedule", "")
                mod_statusbar.inform("schedule_hint", "")
                mod_statusbar.update()
            end),
    kpress(META.."Control+F5", "schedule.show_all(_)"),
})

dopath(schedule.save_filename, true)    -- read any saved notifications
timer = ioncore.create_timer()
timer:set(schedule.interval, schedule.notify)
mod_statusbar.inform("schedule", "")

