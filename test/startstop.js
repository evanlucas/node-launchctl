var test = require('tap').test
  , ctl = require('../lib')

test('start - non existent job', function(t) {
  ctl.start('com.thisisafakejob.test', function(err) {
    t.type(err, Error, 'Error does exist')
    t.equal(err.message, 'No such process')
    t.end()
  })
})

test('startSync - non existent job', function(t) {
  try {
    var res = ctl.startSync('com.thisisafakejob.test')
    var err = ctl.errorFromErrno(res)
    throw err
    t.ok(false, 'should not be reached')
    t.end()
  }
  catch (err) {
    t.type(err, Error, 'Error does exist')
    t.equal(err.message, 'No such process')
    t.end()
  }
})

test('stop - non existent job', function(t) {
  ctl.stop('com.thisisafakejob.test', function(err) {
    t.type(err, Error, 'Error does exist')
    t.equal(err.message, 'No such process')
    t.end()
  })
})

test('stopSync - non existent job', function(t) {
  try {
    var res = ctl.stopSync('com.thisisafakejob.test')
    var err = ctl.errorFromErrno(res)
    throw err
    t.ok(false, 'should not be reached')
    t.end()
  }
  catch (err) {
    t.type(err, Error, 'Error does exist')
    t.equal(err.message, 'No such process')
    t.end()
  }
})
