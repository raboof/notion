-- Dan's bindings: these allow you to navigate ion in a sane way.
--
--      MOD1 + L/R cycles between clients in a frame.
--      MOD1 + U/D cycles between workspaces.
--
--      MOD1 + shift + L/R/U/D navigates frames intuitively on a WIonWS.
--
--      MOD1 + control + L/R moves clients on a frame.
--      MOD1 + control + U/D moves clients between workspaces. (TODO)

-- Caution: these may break the default bindings.
UP="Up" ; DOWN="Down" ; LEFT="Left" ; RIGHT="Right"
-- UP="K" ; DOWN="J" ; LEFT="H" ; RIGHT="L"
-- UP="W" ; DOWN="S" ; LEFT="A" ; RIGHT="D"

defbindings("WScreen", {
    kpress(MOD1..UP, "WScreen.switch_prev(_)"),
    kpress(MOD1..DOWN, "WScreen.switch_next(_)"), })
defbindings("WFrame", {
    kpress(MOD1..LEFT.."+Control", "WFrame.dec_index(_, _sub)", "_sub:non-nil"),
    kpress(MOD1..RIGHT.."+Control", "WFrame.inc_index(_, _sub)", "_sub:non-nil"),
    kpress(MOD1..LEFT, "WFrame.switch_prev(_)"),
    kpress(MOD1..RIGHT, "WFrame.switch_next(_)"), })
defbindings("WTiling", {
    kpress(MOD1..UP.."+Shift", "WTiling.goto_dir(_, 'above')"),
    kpress(MOD1..DOWN.."+Shift", "WTiling.goto_dir(_, 'below')"),
    kpress(MOD1..LEFT.."+Shift", "WTiling.goto_dir(_, 'left')"),
    kpress(MOD1..RIGHT.."+Shift", "WTiling.goto_dir(_, 'right')"), })
