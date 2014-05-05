var test = require('tap').test
  , ctl = require('../lib')

test('list', function(t) {
  ctl.list(function(err, data) {
    t.equal(err, null, 'Error does not exist')
    t.type(data, Array, 'data should be an array')
    t.end()
  })
})

/*
test('list - com.apple.ReportCrash', function(t) {
  ctl.list('com.apple.ReportCrash', function(err, data) {
    t.equal(err, null, 'Error does not exist')
    t.type(data, 'object', 'data should be an object')
    t.end()
  })
})
*/

test('listSync', function(t) {
  var jobs = ctl.listSync()
  t.type(jobs, Array, 'jobs should be an array')
  t.end()
})

test('listSync - com.apple.Finder', function(t) {
  var job = ctl.listSync('com.apple.Finder')
  t.type(job, 'object', 'job should be an object')
  t.end()
})

test('listSync - regex', function(t) {
  var jobs = ctl.listSync(/com.apple.([\w]+)/)
  t.type(jobs, Array, 'jobs should be an array')
  jobs.forEach(function(job) {
    t.ok(job.Label, 'Job should have Label')
    t.ok(/com.apple/.test(job.Label), 'Job Label should match com.apple')
  })
  t.end()
})
