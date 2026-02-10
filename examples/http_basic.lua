function spry.start()
--   font = spry.font_load 'roboto.ttf'
  font = spry.default_font()
end

function spry.frame(dt)
  font:draw('ABCDFGHIJKLMONPRQRSTUVWXYZ', 10, 10, 30, {1,0,0,1})
  font:draw('abcdfghijklmonprqrstuvwxyz', 10, 50, 30, {1,0,0,1})
end