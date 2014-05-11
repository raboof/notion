-- Authors: Jeremy Hankins <nowan@nowan.org>, Johannes Segitz, Antti Kaihola <antti.kaihola@linux-aktivaattori.org>
-- License: Unknown
-- Last Changed: 2005-11-16
--
-- By Jeremy Hankins <nowan@nowan.org>
-- Multi-head support added by Johannes Segitz
-- Cycling contributed by
--   Antti Kaihola <antti.kaihola@linux-aktivaattori.org>
--
-- Time-stamp: <2005-11-16 12:02:46 Jeremy Hankins>
--             <2005-08-26 11:35:00 kaihola>
--
-- With these functions you can bind a key to start an application if
-- it's not already running, or cycle its windows if it is.  You can
-- use it by doing something like this in your bindings file:
--
-- kpress(META.."C", "app.byname('xterm -title shell', 'shell')"),
-- kpress(META.."T", "app.byclass('emacs', 'Emacs')"),
-- kpress(META.."M", "app.byinstance('xterm -name muttTerm -e mutt', 'XTerm', 'muttTerm')"),
--
-- The byname function expects an executable and a window title, the
-- byclass expects a class, the byinstance expects a class and an
-- instance.
--
-- NOTE: cycling windows requires ioncore.current(), so it will only
-- work with snapshot XXX or later.
--
-- If you use a multihead setup you can use something like this to start
-- applictions on your current screen
--
-- kpress(MOD1.."C", "app.byname('xterm -title shell', 'shell', _)"),
-- kpress(MOD1.."T", "app.byclass('emacs', 'Emacs', _)"),
--
-- For emacs users there's also app.emacs_eval, and app.query_editfile,
-- which interacts with any currently running emacs process (using
-- gnuclient), or starts emacs to run the given command.
-- app.query_editfile is a replacement for query_lib.query_editfile to
-- use the currently running emacs rather than ion-edit.
--

app={}

function app.match_class(class, instance)
   -- Return matching client windows as a table.
   -- If the current window is among them, it's placed last on the list
   -- and its successor at the beginning of the list. This facilitates
   -- cycling multiple windows of an application.
   local result = {}
   local offset = 0
   local currwin = ioncore.current()
   ioncore.clientwin_i(function (win)
      if class == win:get_ident().class then
        if instance then
            if instance == win:get_ident().instance then
                table.insert(result, #(result)-offset+1, win)
            end
        else
            table.insert(result, #(result)-offset+1, win)
        end
      end
      if win == currwin then
	 -- Current client window found, continue filling the table from
	 -- the beginning.
	 offset = #(result)
      end
      return true
   end)

   return result
end

function app.byname(prog, name, where)
   local win = ioncore.lookup_clientwin(name)
   if win then
      ioncore.defer(function () win:goto_focus() end)
   else
      if where then
	  ioncore.exec_on(where, prog)
      else
	  ioncore.exec(prog)
      end
   end
end

function app.byclass(prog, class, where)
   local win = app.match_class(class)[1]
   if win then
      ioncore.defer(function () win:goto_focus() end)
   else
      if where then
	  ioncore.exec_on(where, prog)
      else
	  ioncore.exec(prog)
      end
   end
end

function app.byinstance(prog, class, instance, where)
   local win = app.match_class(class, instance)[1]
   if win then
      ioncore.defer(function () win:goto_focus() end)
   else
      if where then
	  ioncore.exec_on(where, prog)
      else
	  ioncore.exec(prog)
      end
   end
end

function app.emacs_eval(expr)
   local emacswin = app.match_class("Emacs")[1]
   if emacswin then
      ioncore.exec("gnuclient -batch -eval '"..expr.."'")
      ioncore.defer(function () emacswin:goto_focus() end)
   else
      ioncore.exec("emacs -eval '"..expr.."'")
   end
end

function app.query_editfile(mplex, dir) 
   local function handler(file)
      app.emacs_eval("(find-file \""..file.."\")")
   end

   mod_query.do_query(mplex,
		     'Edit file:',
		     dir or mod_query.get_initdir(),
		     handler,
		     mod_query.file_completor)
end



