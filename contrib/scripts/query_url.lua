-- Authors: Reuben Thomas <rrt@sc3d.org>
-- License: GPL, version 2
-- Last Changed: 2005
--
-- Prompt for a URL and open it
-- Use Opera bookmarks for completion
-- (c) Reuben Thomas 2005 (rrt@sc3d.org); released under the GPL
--
-- Completes on full URL, name of bookmark, or URL with initial http[s]://[www.] stripped
--
-- Use something like:
--
--    kpress(MOD2.."F3", 'query_url(_)'),
--
-- The browser defaults to "sensible-browser"; configure below:

local browser = "sensible-browser"


-- Read Opera bookmarks
function readOperaBookmarks ()
  local bookmarks, names = {}, {}

  -- Parse bookmarks file
  -- Make a list of bookmarks and their names for use as completions.
  -- Also make a map of names to URLs.
  local f = io.lines (os.getenv ("HOME") .. "/.opera/opera6.adr")
  local nameTag = "\tNAME="
  local nameOff = string.len (nameTag) + 1
  local namePat = "^" .. nameTag
  local urlTag = "\tURL="
  local urlOff = string.len (urlTag) + 1
  local urlPat = "^" .. urlTag
  local l = f ()
  while l do
    if string.find (l, namePat) then
      local name = string.sub (l, nameOff)
      l = f ()
      if string.find (l, urlPat) then
        local url = string.sub (l, urlOff)
        table.insert (bookmarks, url)
        -- TODO: Make the name and stripped URL processing below
        -- independent of browsing Opera's bookmarks, and add other
        -- bookmark formats
        local urlIndex = table.getn (bookmarks)
        names[name] = urlIndex
        table.insert (bookmarks, name)
        local strippedUrl = string.gsub (url, "^https?://", "")
        strippedUrl = string.gsub (strippedUrl, "^www\.", "")
        if url ~= strippedUrl then
          names[strippedUrl] = urlIndex
          table.insert (bookmarks, strippedUrl)
        end
      end
    else
      l = f ()
    end
  end

  return bookmarks, names
end

-- Prompt for a URL, matching by name or URL
function query_url (mplex)
  local bookmarks, names = readOperaBookmarks ()
  local function handler (mplex, str)
    -- If str is the name of a URL, get the url instead
    local index = names[str]
    if index then
      str = bookmarks[index]
    end
    -- FIXME: Ion needs a standard way of invoking a browser
    ioncore.exec_on (mplex, browser .. ' "' .. str .. '"')
  end
  local function completor (wedln, what)
    wedln:set_completions (mod_query.complete_from_list (bookmarks, what))
  end
  mod_query.query (mplex, "URL: ", nil, handler, completor)
end
