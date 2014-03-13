var test = require('tap').test
  , ctl = require('../lib')

test('limit', function(t) {
  var lims = ctl.limit()
  t.type(lims, 'object', 'should be an object')
  t.ok(lims.cpu, 'should have cpu')
  t.ok(lims.filesize, 'should have filesize')
  t.ok(lims.data, 'should have data')
  t.ok(lims.stack, 'should have stack')
  t.ok(lims.core, 'should have core')
  t.ok(lims.rss, 'should have rss')
  t.ok(lims.memlock, 'should have memlock')
  t.ok(lims.maxproc, 'should have maxproc')
  t.ok(lims.maxfiles, 'should have maxfiles')
  t.end()
})

test('limit - specific', function(t) {
  var keys = ['cpu', 'filesize', 'data', 'stack', 'core', 'rss', 'memlock', 'maxproc', 'maxfiles']
  keys.forEach(function(key) {
    var lim = ctl.limit(key)
    t.type(lim, 'object')
    t.ok(lim.soft, 'should have soft')
    t.ok(lim.hard, 'should have hard')
  })
  t.end()
})

test('limit - set maxfiles to unlimited', function(t) {
  try {
    var res = ctl.limit('maxfiles', 'unlimited')
    t.ok(false, 'should not be reached')
    t.end()
  }
  catch (err) {
    t.type(err, Error, 'should have error')
    t.end()
  }
})
