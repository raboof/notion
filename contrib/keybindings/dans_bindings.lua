-- Authors: Unknown
-- License: Unknown
-- Last Changed: Unknown
--
-- Dan's bindings: these allow you to navigate ion in a sane way.
--
--      META + L/R cycles between clients in a frame.
--      META + U/D cycles between workspaces.
--
--      META + shift + L/R/U/D navigates frames intuitively on a WIonWS.
--
--      META + control + L/R moves clients on a frame.
--      META + control + U/D moves clients between workspaces. (TODO)

-- Caution: these may break the default bindings.
UP="Up" ; DOWN="Down" ; LEFT="Left" ; RIGHT="Right"
-- UP="K" ; DOWN="J" ; LEFT="H" ; RIGHT="L"
-- UP="W" ; DOWN="S" ; LEFT="A" ; RIGHT="D"

defbindings("WScreen", {
    kpress(META..UP, "WScreen.switch_prev(_)"),
    kpress(META..DOWN, "WScreen.switch_next(_)"), })
defbindings("WFrame", {
    kpress(META..LEFT.."+Control", "WFrame.dec_index(_, _sub)", "_sub:non-nil"),
    kpress(META..RIGHT.."+Control", "WFrame.inc_index(_, _sub)", "_sub:non-nil"),
    kpress(META..LEFT, "WFrame.switch_prev(_)"),
    kpress(META..RIGHT, "WFrame.switch_next(_)"), })
defbindings("WTiling", {
    kpress(META..UP.."+Shift", "WTiling.goto_dir(_, 'above')"),
    kpress(META..DOWN.."+Shift", "WTiling.goto_dir(_, 'below')"),
    kpress(META..LEFT.."+Shift", "WTiling.goto_dir(_, 'left')"),
    kpress(META..RIGHT.."+Shift", "WTiling.goto_dir(_, 'right')"), })
