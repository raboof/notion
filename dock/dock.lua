--
-- Ion-devel dock module configuration
--

-- create a new dock on screen 0
dock = dockmod.create_dock(0, {
    name="dock",	-- name for use in target="..." winprops
    hpos="right",	-- horizontal position left|center|right
    vpos="bottom",	-- vertical position top|middle|bottom
    grow="left",	-- growth direction up|down|left|right
    is_auto=true,	-- whether new dockapps should be added automatically
})

defcmd("global", "toggle_dock",
        function()
            WDock.toggle(dock)	-- toggle map/unmapped state
        end)

defbindings("WScreen", {
    kpress(DEFAULT_MOD.."space", "toggle_dock")
})

-- dockapp ordering

-- Use the dockposition winprop to enforce an ordering on dockapps. The value is
-- arbitrary and relative to other dockapps only. The default is 0.
-- Unfortunately, many dockapps do not set their class/instance/role so they
-- cannot be used with the winprop system.

-- dockapp borders

-- Use dockappborder=false to disable the drawing of a border around a dockapp.
-- This is only effective if outline_style="each" (see dock-draw.lua).

defwinprop{
	instance="gkrellm2",
	dockposition=-100,	-- place first
	dockborder=false,	-- do not draw border if outline_style="each"
}

-- kludges

defwinprop{
	instance="wmxmms",
	target="dock",
}
