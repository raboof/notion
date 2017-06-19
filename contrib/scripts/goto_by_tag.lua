-- Authors: Philipp Hartwig <ph@phhart.de>
-- License: MIT, see http://opensource.org/licenses/mit-license.php
-- Last Changed: 2012-09-29
--
--[[

Description: This function sets focus to the first client with the given
tag.

Usage:

1) Define a tag for a client, using a winprop.
    For example:
    defwinprop{
       class = "Firefox-bin",
       instance = "Navigator",
       tag = "b",
    }

2) Add suitable bindings to the "WMPlex.toplevel" section of cfg_notioncore.
    For example:
    submap(META.."U", {
        kpress("A", "goto_by_tag('a')"),
        kpress("B", "goto_by_tag('b')"),
        kpress("C", "goto_by_tag('c')"),
        kpress("D", "goto_by_tag('d')"),
        kpress("E", "goto_by_tag('e')"),
        kpress("F", "goto_by_tag('f')"),
        kpress("G", "goto_by_tag('g')"),
        kpress("H", "goto_by_tag('h')"),
        kpress("I", "goto_by_tag('i')"),
        kpress("J", "goto_by_tag('j')"),
        kpress("K", "goto_by_tag('k')"),
        kpress("L", "goto_by_tag('l')"),
        kpress("M", "goto_by_tag('m')"),
        kpress("N", "goto_by_tag('n')"),
        kpress("O", "goto_by_tag('o')"),
        kpress("P", "goto_by_tag('p')"),
        kpress("Q", "goto_by_tag('q')"),
        kpress("R", "goto_by_tag('r')"),
        kpress("S", "goto_by_tag('s')"),
        kpress("T", "goto_by_tag('t')"),
        kpress("U", "goto_by_tag('u')"),
        kpress("V", "goto_by_tag('v')"),
        kpress("W", "goto_by_tag('w')"),
        kpress("X", "goto_by_tag('x')"),
        kpress("Y", "goto_by_tag('y')"),
        kpress("Z", "goto_by_tag('z')"),
    }),

3) In the example above, you can directly jump to the first instance of Firefox
by hitting META+U B.

--]]

goto_by_tag = function(tag)
    local s
    ioncore.clientwin_i(function(cwin)
        local winprop = ioncore.getwinprop(cwin)
        if winprop and winprop.tag == tag then
            s=cwin
            return false
        else
            return true
        end
    end)
    if s then
        s:goto_focus()
    end
end
