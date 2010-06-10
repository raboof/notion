-- Emacs-like keyboard configuration for Ion, version 3.
-- Written by Matthieu MOY (Matthieu.Moy@imag.fr) on February 15, 2005.
-- No copyright.

-- Please note: You will almost certainly want to change Ion3's default
-- META binding to Alt (as this will interfere with Emacs).
-- See the FAQ or the wiki entries on this subject.
-- Use cfg_debian.lua on Debian installations to make the change global
-- across ion3 upgrades.

--[[
  Copy this file in your ~/.ion3/ directory
  Add the line
   dopath("emacs-bindings")
  At the *end* of your ~/.ion3/cfg_ion.lua file, or in ~/.ion3/cfg_user.lua

  These bindings are not for every ion3 function.  By copying the file, you
  will add bindings for ion3.  You will still have to use the default ion3
  bindings for any functions not bound here.

  Comments/Feedback welcome.  See the changelog at the end.
--]]

--[[
List of the bindings.  These are executed after the main ion3 bindings.

What you should do is familiarize yourself with the main ion3 bindings and
then review these additions.  The Emacs bindings should provide additional
bindings *not* replace the default ion3 bindings.

Note that ion bindings use "META+<something>" but since we're all Emacs
users here, I left it in Emacs form "META-<something>".

  Ion binding     Emacs binding  Description

WTiling:
  META-x 0        C-x 0          Destroy the current frame
  META-x 1        C-x 1          Move all windows to single frame; destroy rest
  META-x 2        C-x 2          Split current frame vertically
  META-x 3        C-x 3          Split current frame horizontally
  META-x META-x 2                Split current floating frame vertically
  META-x META-x 3                Split current floating frame horizontally
  META-x o        C-x o          Cycle to the next workspace

 WMPlex:
  META-x b        C-x b          Query client win to attach to active frame
  META-x k        C-x k          Close current object
  META-u META-x k                Kill client owning current client window
  META-q          C-q            Enter next character literally
  META-Ctrl-Space                Attach tagged objects to this frame
  META-Shift-:    C-:            Query for Lua code to execute

 WScreen
  META-Shift-:    C-:            Query for Lua code to execute (XXX DUP?)
  META-x b        C-x b          Query for a client window to go to
  META-x META-b   C-x C-b        Display the window list menu
  META-x w                       Query for workspace to go to (or create)
  META-x META-w                  Display list of workspaces

 WEdln:
  M-f             M-f            Skip one word forward
  M-<right>       M-<right>      Skip one word forward
  M-b             M-b            Skip one word backward
  M-<left>        M-<left>       Skip one word backward
  M-d             M-d            Delete one word forward
  M-<backspace>   M-<backspace>  Delete one word backward
  C-k             C-k            Delete from current position to end of line
  C-w             C-w            Cut selection
  C-y             C-y            Paste from the clipboard
  C-<space>       C-SPC          Set mark/begin selection
  C-g             C-g            Clear mark/cancel selection or abort
  M-p             M-p            Select next (matching) history entry
  M-n             M-n            Select previous (matching) history entry

TODO:
  Global M-h k		  C-h k		 Describe key
  WEdlin M-t		  M-t		 Transpose word
--]]

Emacs = {};


-- Download it from http://modeemi.cs.tut.fi/~tuomov/repos/ion-scripts-3/scripts/collapse.lua
-- Debian includes it in the ion3-scripts package.
dopath("collapse")

--
-- WTiling
--

function WTiling.other_client(ws, sub)
    if ws:current() == ioncore.goto_next(sub, 'right') then
        ioncore.goto_next(sub, 'bottom')
    end
end

Emacs.WTiling = {
    bdoc("Unbind META-TAB"),
    kpress(META.."Tab", nil),

    submap(META.."x", {
        bdoc("Destroy current frame."),
        kpress ("AnyModifier+0", "WTiling.unsplit_at(_, _sub)"),

        bdoc("Move all windows on WTiling to single frame and destroy rest"),
        kpress ("AnyModifier+1", "collapse.collapse(_)"),

        bdoc("Split current frame vertically"),
        kpress ("AnyModifier+2", "WTiling.split_at(_, _sub, 'top', true)"),

        bdoc("Split current frame horizontally"),
        kpress ("AnyModifier+3", "WTiling.split_at(_, _sub, 'left', true)"),

        submap(META.."x", {
            bdoc("Split current floating frame vertically"),
            kpress("AnyModifier+2",
 	          "WTiling.split_at(_, _sub, 'floating:bottom', true)"),

	    bdoc("Split current floating frame horizontally"),
            kpress("AnyModifier+3",
                  "WTiling.split_at(_, _sub, 'floating:right', true)"),
         }),

        bdoc("Cycle to the next workspace"),
        kpress ("AnyModifier+o", "WTiling.other_client(_, _sub)")
    }),
}

defbindings("WTiling", Emacs.WTiling)

--
-- WMPlex
--
-- odds:
--   * C-TAB, C-S-TAB, C-SPACE, C-C-SPACE, C-u C-x k are not emacs binding
--   * C-:, C-! should maybe M-, M-!: (we would need to set MOD2, probably
--     not a good idea)
--

Emacs.WMPlex = {
    submap(META.."x", {
      bdoc("Query for a client window to attach to active frame."),
      kpress("b", "mod_query.query_attachclient(_)"),

      bdoc("Close current object."),
      kpress("k", "WRegion.rqclose_propagate(_, _sub)"),
     }),

    submap(META.."u", {
        submap(META.."x", {
            bdoc("Kill client owning current client window."),
            kpress("k", "WClientWin.kill(_sub)", "_sub:WClientWin"),
	 })
     }),

    bdoc("Send next key press to current client window. "..
	 "Some programs may not allow this by default."),
    kpress(META.."q", "WClientWin.quote_next(_sub)", "_sub:WClientWin"),

    -- Interferes with mod_sp binding
    -- bdoc("Tag current object within the frame."),
    -- kpress(META.."space", "WRegion.set_tagged(_sub, 'toggle')"),

    bdoc("Attach tagged objects to this frame."),
    kpress(META.."Control+space", "WFrame.attach_tagged(_)"),

    bdoc("Query for Lua code to execute."),
    kpress(META.."Shift+colon", "mod_query.query_lua(_)"),
}

defbindings("WMPlex", Emacs.WMPlex)

--
-- WScreen
--
-- odds:
--   * C-x w, C-x C-w are not emacs bindings
--   * C-x b should really attach *and* focus the client
--

Emacs.WScreen = {
    bdoc("Query for Lua code to execute."),
    kpress(META.."Shift+colon", "mod_query.query_lua(_)"),

    -- This conflicts with going to the first display in multihead setup.
    -- bdoc("Query for command line to execute."),
    -- kpress(META.."Shift+exclam", "mod_query.query_exec(_)"),

    submap(META.."x", {
        bdoc("Query for a client window to go to."),
        kpress("b", "mod_query.query_gotoclient(_)"),

        bdoc("Display the window list menu."),
        kpress(META.."b", "mod_menu.menu(_, _sub, 'windowlist')"),

	bdoc("Query for workspace to go to or create a new one."),
        kpress("w", "mod_query.query_workspace(_)"),

        bdoc("Display a list of workspaces."),
        kpress(META.."w", "mod_menu.menu(_, _sub, 'workspacelist')"),
     }),
}

defbindings("WScreen", Emacs.WScreen);

--
-- WEdln
--
-- odds:
--   M-d, M-backspace don't copy to the clipboard
--   M-p, M-n do eshell like history, C-p, C-n model-line like history
--

function WEdln:cut_to_eol ()
    self:set_mark()
    self:eol()
    self:cut()
end

function WEdln:clear_mark_or_abort ()
    if -1 ~= self:mark() then
        self:clear_mark()
    else
        self:cancel()
    end
end

Emacs.WEdln = {
    bdoc("Skip one word forward."),
    kpress("Mod1+f", "WEdln.skip_word(_)"),

    bdoc("Skip one word backward."),
    kpress("Mod1+Right", "WEdln.skip_word(_)"),

    bdoc("Skip one word backward."),
    kpress("Mod1+b", "WEdln.bskip_word(_)"),

    bdoc("Skip one word backward."),
    kpress("Mod1+Left", "WEdln.bskip_word(_)"),

    bdoc("Delete one word forward."),
    kpress("Mod1+d", "WEdln.kill_word(_)"),

    bdoc("Delete one word backward."),
    kpress("Mod1+BackSpace", "WEdln.bkill_word(_)"),

    bdoc("Delete from current position to the end of the line."),
    kpress("Control+K", "WEdln.cut_to_eol(_)"),

    bdoc("Cut selection."),
    kpress("Control+W", "WEdln.cut(_)"),

    bdoc("Paste from the clipboard."),
    kpress("Control+Y", "WEdln.paste(_)"),

    bdoc("Set mark/begin selection."),
    kpress("Control+space", "WEdln.set_mark(_)"),

    bdoc("Clear mark/cancel selection or abort."),
    kpress("Control+G", "WEdln.clear_mark_or_abort(_)"),

    bdoc("Select next (matching) history entry."),
    kpress("Mod1+p", "WEdln.history_prev(_, true)"),

    bdoc("Select previous (matching) history entry."),
    kpress("Mod1+n", "WEdln.history_next(_, true)"),
}

defbindings("WEdln", Emacs.WEdln)


-- ChangeLog:
--
--   David Hansen <david.hansen@physik.fu-berlin.de> (all public domain)
--
-- * DEFAULT_MOD -> MOD1
-- * WEdln: M-f, M-b, M-backspace, M-d, M-p, M-n, M-right, M-left
-- * WEdln: C-g clear mark or abort if the mark is not set
-- * WEdln: C-k copy to clipboard
-- * WMPlex: removed C-! C-:, C-x b (they are also in WScreen)
-- * WMPlex: C-q, C-x C-k, C-u C-x C-k
-- * WScreen: C-x C-b, C-x w, C-x C-w
-- * WIonWS: C-x 2, C-x 3 a bit more emacs like
-- * WIonWS: fixed C-x o
-- * WIonWS: removed C-x C-x *
--
--   Tyranix <tyranix@gmail.com> (all public domain)
-- * Added comments about how emacs-bindings.lua relates to the defaults
-- * Added bdoc() comments to each binding
-- * Rearranged code so the bdoc comments are easier to read
-- * Added a main listing of every binding in this file along with what you
--   should think in Emacs terms
-- * Remove META-! because it conflicts with moving between multihead displays
-- * Changed MOD1 to META as this is the new name for it in newer ion3
-- * Added Shift+ to the META-: and META-exclam
-- * Removed META-<space> because it interferes with the scratch pad (mod_sp).
--

--   Adam Di Carlo <aph@debian.org> (all public domain, April 27 2007)
-- * For compatability with ion3 20070318, change WIonWS to WTiling.
-- * Disable non-working and overly obtrusive META-TAB and META-Shift-TAB 
--   remapping.