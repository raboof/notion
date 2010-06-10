-- document_menus.lua
-- 
-- Canaan Hadley-Voth
--
-- Navigate the filesystem by way of ion menu.
--
-- One way to implement this is to define a menu containing useful folders,
-- and use that as a starting point for binding to (see below).
--
-- 2005-12-20:
--   * Added a second possible action.  Define docmenus.exec and exec2 below.
--	Selecting an item normally (Return or right arrow) will use exec,
--	Control+Return will use exec2.
--   * Script no longer puts a temp file in the session dir.
--   * Better handling of filenames with single and double quotes.

defmenu("useful folders", {
    -- Define commonly used locations here.
    -- Or skip this defmenu and put something like the following under mainmenu...
    
    submenu("/", function() return docmenus.getfiletable("/") end, {noautoexpand=true}),
    submenu("~", function() return docmenus.getfiletable(os.getenv("HOME")) end, {noautoexpand=true}),
    submenu(".ion3/", function() return docmenus.getfiletable(ioncore.get_paths().userdir) end, {noautoexpand=true}),

    -- Noautoexpand makes mod_query.query_menu skip this menu's expansion, which
    -- would probably have crashed ion.  (Though using document_menus to
    -- expand filenames in a query is kind of like using wine to run firefox.
    -- mod_query.query_editfile is sufficient for that.)
})

defbindings("WMPlex", { 
    kpress(MOD1.."minus", "mod_menu.menu(_, _sub, 'useful folders')"),
})
    
defbindings("WScreen", {
    submap(MOD1.."K", {
        kpress("minus", "docmenus.toggle_dotfiles()"),
    }),
})

defbindings("WMenu", {
    kpress("Control+Return", "docmenus.finish2(_)"),
})

-- This script creates menus far larger than Ion would
-- normally handle.  The default scroll behavior is going to seem
-- inadequate, and this is how you would speed it up:

--mod_menu.set({ scroll_delay=5, scroll_amount=6 })


if not docmenus then
  -- Specify something other than run-mailcap if necessary.
  docmenus={
      show_dotfiles = true,
      exec = "run-mailcap --action=edit",
      exec2 = "xterm -e vi",
  }
end

function docmenus.getfiletable(dirpath)
    local filetable = {}
    local menufile
    local safedirpath
    -- os shell is used for the menu's directory listing.
    --
    -- lscommand can be customized, to a point.  
    -- Scrap the -L to keep from following symbolic links.
    -- Just don't remove -p.
    local lscommand = "ls -Lp"
    if docmenus.show_dotfiles then lscommand = lscommand.." -a" end

    if string.sub(dirpath, -1) ~= "/" then
	dirpath = dirpath.."/"
    end
    
    safedirpath = string.gsub(dirpath, "\\", "\\\\")
    safedirpath = string.gsub(safedirpath, "\"", "\\\"")

    local ls = assert(io.popen(lscommand.." \""..safedirpath.."\"", "r"))
    
    for menufile in pairs(ls:lines()) do
	if menufile == "./" then menufile = "." end

	if string.sub(menufile, 3) == "ls:" then
	    -- ls error message on broken link. skip it
	elseif menufile == "../" then
	    -- I'd prefer not to show parent directory as a folder.
	elseif string.sub(menufile, -1) == "/" then
	    table.insert(filetable, docmenus.subdir(menufile, dirpath..menufile))
    	else
	    table.insert(filetable, docmenus.file(menufile, dirpath..menufile))
    	end
    end
    ls:close()
    
    return filetable
end

function docmenus.file(menustr,path)
    path = string.shell_safe(path)
    path = string.gsub(path, "\\", "\\\\")
    path = string.gsub(path, "\"", "\\\"")
    return menuentry(menustr, 
	"docmenus.doexec(_, \""..path.."\")"
    )
end

function docmenus.doexec(mplex, path)
    local cmd
    if docmenus.use_exec2 then
	cmd = docmenus.exec2.." "..path
    else
	cmd = docmenus.exec.." "..path
    end
    ioncore.exec_on(mplex, cmd)
    docmenus.use_exec2 = false
end

function docmenus.subdir(menustr,path)
    return submenu(menustr, function() return docmenus.getfiletable(path) end, {noautoexpand=true})
end

function docmenus.toggle_dotfiles()
    docmenus.show_dotfiles=not docmenus.show_dotfiles
end

function docmenus.finish2(wmenu)
    docmenus.use_exec2 = true
    wmenu:finish()
end
