--[[

Description: This function sets focus to the first client with the given 
tag.

Author: Philipp Hartwig

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
        kpress("A", "display_by_tag('a')"),
        kpress("B", "display_by_tag('b')"),
        kpress("C", "display_by_tag('c')"),
        kpress("D", "display_by_tag('d')"),
        kpress("E", "display_by_tag('e')"),
        kpress("F", "display_by_tag('f')"),
        kpress("G", "display_by_tag('g')"),
        kpress("H", "display_by_tag('h')"),
        kpress("I", "display_by_tag('i')"),
        kpress("J", "display_by_tag('j')"),
        kpress("K", "display_by_tag('k')"),
        kpress("L", "display_by_tag('l')"),
        kpress("M", "display_by_tag('m')"),
        kpress("N", "display_by_tag('n')"),
        kpress("O", "display_by_tag('o')"),
        kpress("P", "display_by_tag('p')"),
        kpress("Q", "display_by_tag('q')"),
        kpress("R", "display_by_tag('r')"),
        kpress("S", "display_by_tag('s')"),
        kpress("T", "display_by_tag('t')"),
        kpress("U", "display_by_tag('u')"),
        kpress("V", "display_by_tag('v')"),
        kpress("W", "display_by_tag('w')"),
        kpress("X", "display_by_tag('x')"),
        kpress("Y", "display_by_tag('y')"),
        kpress("Z", "display_by_tag('z')"),
    }),

3) In the example above, you can directly jump to the first instance of Firefox 
by hitting META+U B.

--]]

display_by_tag = function(tag)
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
        s:display()
    end
end
