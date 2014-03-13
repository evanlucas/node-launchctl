var test = require('tap').test
  , ctl = require('../lib')

test('load - non existent', function(t) {
  var fp = '/fasdfasdf/asdfasdfasdf'
  ctl.load(fp, function(err, res) {
    t.type(err, Error, 'Error should exist')
    t.equal(err.errno, 149)
    t.end()
  })
})
