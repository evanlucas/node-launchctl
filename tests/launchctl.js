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
  
  describe('#getManagerName()', function() {
    it('should return a valid manager name', function() {
      var name = launchctl.getManagerName();
      var keys = ['Aqua', 'LoginWindow', 'Background', 'StandardIO', 'System']
      keys.should.include(name)
    })
  })
  
  describe('#getManagerUID()', function() {
    it('Should return a number', function() {
      var uid = launchctl.getManagerUID();
      uid.should.be.a('number')
      if (process.getuid() === 0) {
        uid.should.equal(0)
      }
    })
  })
  
  describe('#getManagerPID()', function() {
    it('Should return a number', function() {
      var pid = launchctl.getManagerPID();
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
})