var test = require('tap').test
  , ctl = require('../lib')

test('managername', function(t) {
  var name = ctl.managername()
  var keys = ['Aqua', 'LoginWindow', 'Background', 'StandardIO', 'System']
  t.notEqual(-1, keys.indexOf(name), 'should be valid')
  t.end()
})

test('manageruid', function(t) {
  var uid = ctl.manageruid()
  t.type(uid, 'number', 'should be a number')
  t.end()
})

test('managerpid', function(t) {
  var pid = ctl.managerpid()
  t.type(pid, 'number', 'should be a number')
  t.end()
})
