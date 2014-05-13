local lut = require 'lut'
lut.Doc.make {
  sources = {
    'lens',
    {'doc', prepend = 'examples/lens'},
  },
  copy = { 'doc', prepend = 'examples/lens', filter = '%.lua' },
  target = 'html',
  format = 'html',
  header = [[<h1><a href='http://lubyk.org'>Lubyk</a> documentation</h1> ]],
  index  = [=[
--[[--
  # Lubyk documentation

  ## List of available modules
--]]--
]=]
}
