---
layout: default
---

# Migration

The next major release of Notion will be Notion 4. This release will include a
number of changes that are either not backwards compatible, or change the
(default) behavior.

## Keybindings

Notion 4 comes with a complete new set of keybindings. You can see an overview
[here](notion4keys.html). If you are already running Notion4, you can also use
`META+/` to get a generated overview of your current bindings.

### Reverting to the old bindings

While we encourage you to give the new bindings a try and provide feedback,
of course many existing Notion users might want to stay on the old keybindings.
This can be done by taking `etc/cfg_bindings_notion3.lua` and
`etc/cfg_tiling_notion3.lua` and copying them to your
`~/.notion/cfg_bindings.lua` and `~/.notion/cfg_tiling.lua`

## Dependencies

### Lua

Notion is no longer compatible with Lua 5.0. This change was required in order to
support Lua 5.3, which removed "old-style" Lua 5.0 variadic function arguments,
the "new-style" variadic arguments were introduced with Lua 5.1.

If your custom scripts use the old `function(arg)` method of putting variadic
arguments into an array named `arg`, you will need to use the new `function(...)`
syntax instead.

```lua
function foo(a, b, arg)
    print(unpack(arg))
end
```
becomes
```lua
function foo(a, b, ...)
    print(...)
end
```
and capturing `...` into a table can be achieved with `local arg={...}`.

### C compiler

Notion 4 also requires a C compiler capable of compiling C99.

## Custom styles

Custom styles used to automatically inherit defaults from the "*" style. This
is no longer the case. To get the old behavior back, adding `based_on = "*"`
at the top of any custom styles is likely sufficient:

```
de.defstyle("tab", {
    based_on = "*",
    font = "xft:Source Code Sans:size=16",
})
```

## submap_wait keybindings

When defining a submap in your keybindings, it used to be possible to introduce
a `submap_wait` inside the submap that got triggered when the submap was
entered. This has been refactored, and now you can use `kpress` 'next to' the
submap.

Before:

```
defbindings("WFrame.toplevel", {
    submap(META.."K", {
        submap_wait("WFrame.set_numbers(_, 'during_grab')"),
```

After:

```
defbindings("WFrame.toplevel", {
    kpress(META.."K", "WFrame.set_numbers(_, 'during_grab')"),
    submap(META.."K", {
```

## Tab numbers

In Notion 3 there was a `ioncore.tabnum` module that took care of showing
numbers on `META.."K"`. This has now been replaced with `WFrame.set_numbers`
again.

Before:

```
defbindings("WFrame.toplevel", {
    kpress(META.."K", "ioncore.tabnum.show(_)"),
```

After:

```
defbindings("WFrame.toplevel", {
    kpress(META.."K", "WFrame.set_numbers(_, 'during_grab')"),
```

## Minimum window sizes

Notion used to respect the 'minimum size' as specified by some windows. This
is often inconvenient, since it can prevent resizing tiles. However, this
feature has been broken for some time, so it did not interfere.

In Notion 4, the feature works again, but is disabled by default. If you want
to respect the minimum size for some windows, you can enable it using the
winprops mechanism:

```
defwinprop{
    -- maybe more selectors
    ignore_min_size = false,
}
```
