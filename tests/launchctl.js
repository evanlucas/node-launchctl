var launchctl = require('../lib/index')
  , should = require('should')

describe('launchctl', function() {
  describe('#list()', function() {
    it('should return an array of job objects', function(done) {
      launchctl.list(function(err, data) {
        should.ifError(err)
        data.should.be.instanceOf(Array)
        data.forEach(function(job) {
          job.should.have.property('LastExitStatus')
          job.should.have.property('Label')
        })
        done()
      })
    })
  })

  describe('#listSync()', function() {
    it('should return an array of job objects', function() {
      var jobs = launchctl.listSync()
      jobs.should.be.instanceOf(Array)
      jobs.forEach(function(job) {
        job.should.have.property('LastExitStatus')
        job.should.have.property('Label')
      })
    })
  })

  describe('#list(\'com.apple.Dock.agent\')', function() {
    if (process.getuid() == 0) {
      it('should throw error as we are root', function(done) {
        launchctl.list('com.apple.Dock.agent', function(err, data) {
          should.exist(err)
          should.not.exist(data)
          done()
        })
      })
    } else {
      it('should return a single object', function(done) {
        launchctl.list('com.apple.Dock.agent', function(err, data) {
          should.ifError(err)
          data.should.have.property('Label')
          data.should.have.property('LimitLoadToSessionType')
          data.should.have.property('OnDemand')
          data.should.have.property('LastExitStatus')
          data.should.have.property('PID')
          data.should.have.property('TimeOut')
          data.should.have.property('Program')
          data.should.have.property('MachServices')
          data.should.have.property('PerJobMachServices')
          done()
        })
      })
    }
  })

  describe('#listSync(\'com.apple.Dock.agent\')', function() {
    if (process.getuid() === 0) {
      it('should throw error', function() {
        try {
          var job = launchctl.listSync('com.apple.Dock.agent')
        }
        catch (e) {
          should.exist(e)
          e.should.be.instanceof(Error)
        }
      })
    } else {
      it('should return a single object', function() {
        var job = launchctl.listSync('com.apple.Dock.agent');
        job.should.have.property('Label')
        job.should.have.property('LimitLoadToSessionType')
        job.should.have.property('OnDemand')
        job.should.have.property('LastExitStatus')
        job.should.have.property('PID')
        job.should.have.property('TimeOut')
        job.should.have.property('Program')
        job.should.have.property('MachServices')
        job.should.have.property('PerJobMachServices')
      })
    }
  })

  describe('#list(/^com.apple.([\w]+)/)', function() {
    it('should return an array of job objects', function(done) {
      launchctl.list(/^com.apple.([\w]+)/, function(err, data) {
        should.ifError(err)
        data.should.be.instanceOf(Array)
        data.forEach(function(job) {
          job.should.have.property('LastExitStatus')
          job.should.have.property('Label')
        })
        done()
      })
    })
  })

  describe('#list(\'com.apple.thisisafakejob.test\')', function() {
    it('should throw an error', function(done) {
      launchctl.list('com.apple.thisisafakejob.test', function(err, data) {
        should.exist(err)
        should.not.exist(data)
        done()
      })
    })
  })

  describe('#start(\'com.thisisafakejob.test\')', function() {
    it('should throw error [No such process]', function(done) {
      launchctl.start('com.thisisafakejob.test', function(err) {
        should.exist(err)
        err.should.be.instanceof(Error)
        err.should.have.property('msg', 'No such process')
        done()
      })
    })
  })

  describe('#startSync(\'com.thisisafakejob.test\')',function() {
    it('should throw error [No such process]', function() {
      try {
        launchctl.startSync('com.thisisafakejob.test')
      }
      catch (e) {
        should.exist(e)
        e.should.be.instanceof(Error)
        e.should.have.property('msg', launchctl.strerror(e.errno))
      }
    })
  })

  describe('#stop(\'com.thisisafakejob.test\')', function() {
    it('should throw error [No such process]', function(done) {
      launchctl.stop('com.thisisafakejob.test', function(err) {
        should.exist(err)
        err.should.be.instanceof(Error)
        err.should.have.property('msg', launchctl.strerror(err.errno))
        done()
      })
    })
  })

  describe('#stopSync(\'com.thisisafakejob.test\')', function() {
    it('should throw error [No such process]', function() {
      try {
        launchctl.stopSync('com.thisisafakejob.test')
      }
      catch (e) {
        should.exist(e)
        e.should.be.instanceof(Error)
        e.should.have.property('msg', launchctl.strerror(e.errno))
      }
    })
  })

  describe('#load(\'/System/Library/LaunchDaemons/com.thisisafakejob.test.plist\')', function() {
    it('should throw error [No such file or directory]', function(done) {
      launchctl.load('/System/Library/LaunchDaemons/com.thisisafakejob.test.plist', function(err, res) {
        should.exist(err)
        err.should.be.instanceof(Error)
        err.should.have.property('msg', launchctl.strerror(err.errno))
        done()
      })
    })
  })

  describe('#managername()', function() {
    it('should return a valid manager name', function() {
      var name = launchctl.managername();
      var keys = ['Aqua', 'LoginWindow', 'Background', 'StandardIO', 'System']
      keys.should.include(name)
    })
  })

  describe('#manageruid()', function() {
    it('Should return a number', function() {
      var uid = launchctl.manageruid();
      uid.should.be.a('number')
      if (process.getuid() === 0) {
        uid.should.equal(0)
      }
    })
  })

  describe('#managerpid()', function() {
    it('Should return a number', function() {
      var pid = launchctl.managerpid();
      pid.should.be.a('number')
      if (process.getuid() === 0) {
        pid.should.equal(1)
      }
    })
  })

  describe('#submitSync()', function() {
    describe('Submitting a job that is not already loaded', function() {
      // no err
      it('Should not throw an error', function(done) {
        this.timeout(10000)
        var path = require('path')
        var prog = path.join(__dirname, 'test.sh')
        var res = launchctl.submitSync({
            label: 'com.node.ctl.test'
          , program: prog
          , stderr: path.join(__dirname, 'test.err.log')
          , stdout: path.join(__dirname, 'test.out.log')
          , args: []
        })
        res.should.eql(0)

        launchctl.startSync('com.node.ctl.test')
        setTimeout(function() {
          launchctl.removeSync('com.node.ctl.test')
          done()
        }, 5000)
      })
    })
  })

  describe('#limit()', function() {
    describe('Get all limits', function() {
      it('Should return an object', function() {
        var lims = launchctl.limit()
        lims.should.be.a('object')
        lims.should.have.property('cpu')
        lims.should.have.property('filesize')
        lims.should.have.property('data')
        lims.should.have.property('stack')
        lims.should.have.property('core')
        lims.should.have.property('rss')
        lims.should.have.property('memlock')
        lims.should.have.property('maxproc')
        lims.should.have.property('maxfiles')
      })
    })

    describe('Get a specific limit', function() {
      var keys = ['cpu', 'filesize', 'data', 'stack', 'core', 'rss', 'memlock', 'maxproc', 'maxfiles']
      keys.forEach(function(key) {
        it('Should return an object', function() {
          var lims = launchctl.limit(key)
          lims.should.have.property('soft')
          lims.should.have.property('hard')
        })
      })
    })
  })

  describe('Set limit of maxfiles to unlimited', function() {
    it('Should throw an error', function() {
      // This will purposefully throw an error
      // I did not want to make any system changes during unit tests :]
      (function() {
        launchctl.limit('maxfiles', 'unlimited')
      }).should.throw()
    })
  })

  describe('#getRUsage()', function() {
    it('Should return an object', function() {
      var res = launchctl.getRUsage('self')
      var keys = ['user_time_used', 'system_time_used', 'max_resident_set_size', 'shared_text_memory_size', 'unshared_data_size', 'unshared_stack_size', 'page_reclaims', 'page_faults', 'swaps', 'block_input_operations', 'block_output_operations', 'messages_sent', 'messages_received', 'signals_received', 'voluntary_context_switches', 'involuntary_context_switches']
      res.should.be.a('object')
      res.should.have.keys(keys)
    })
  })
})