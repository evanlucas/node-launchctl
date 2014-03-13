var test = require('tap').test
  , ctl = require('../lib')

test('getEnvVar', function(t) {
  var res = ctl.getEnvVar()
  t.type(res, 'object', 'should be an object')
  t.ok(res.PATH, 'should have PATH')
  t.ok(res.USER, 'should have USER')
  t.ok(res.HOME, 'should have HOME')
  t.end()
})
