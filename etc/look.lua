if mod_xinerama and mod_xinerama.query_screens()[1]['w'] >= 3200 then
  dopath('look_newviolet_hidpi')
else
  dopath('look_newviolet')
end
