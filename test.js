var launchctl = require('./lib/index')
  , assert = require('assert')
  describe('#list()', function() {
    it('should return an array of job objects', function(done) {
      launchctl.list(function(err, data) {
        assert.ifError(err);
        assert.equal(Array.isArray(data), true);
        done();
      });
    }); 
  });
  
  describe('#listSync()', function() {
    it('should return an array of job objects', function(done) {
      var jobs = launchctl.listSync();
      assert.equal(Array.isArray(jobs), true);
      done();
    });
  });
  
  describe('#list(\'com.apple.Dock.agent\')', function() {
    if (process.getuid() == 0) {
      it('should throw error as we are root', function(done) {
        launchctl.list('com.apple.Dock.agent', function(err, data) {
          assert.equal(err instanceof Error, true);
          console.log(launchctl.strerror(err.errno));
          done();
        });
      });
    } else {
      it('should return a single object', function(done) {
        launchctl.list('com.apple.Dock.agent', function(err, data) {
          assert.ifError(err);
          assert.equal(typeof data, 'object');
          done();
        });
      });      
    }
  });
  
  describe('#listSync(\'com.apple.Dock.agent\')', function() {
    if (process.getuid() === 0) {
      it('should through error', function(done) {
        try {
          var job = launchctl.listSync('com.apple.Dock.agent');  
        }
        catch (e) {
          if (e) {
            console.log(launchctl.strerror(e.errno));
            done();
          }
        }
      });
    } else {
      it('should return a single object', function(done) {
        var job = launchctl.listSync('com.apple.Dock.agent');
        assert.equal(typeof job, 'object');
        done();
      });
    }
  });
  
  describe('#list(/^com.apple.([\w]+)/)', function() {
    it('should return an array of job objects', function(done) {
      launchctl.list(/^com.apple.([\w]+)/, function(err, data) {
        assert.ifError(err);
        assert.equal(Array.isArray(data), true);
        done();
      });
    }); 
  });
  
  describe('#list(\'com.apple.thisisafakejob.test\')', function() {
    it('should throw an error', function(done) {
      launchctl.list('com.apple.thisisafakejob.test', function(err, data) {
        assert.equal(err instanceof Error, true);
        console.log(launchctl.strerror(err.errno));
        done();
      });
    });
  });

  describe('#start(\'com.thisisafakejob.test\')', function() {
    it('should throw error [No such process]', function(done) {
      launchctl.start('com.thisisafakejob.test', function(err) {
        assert.equal(err.msg, "No such process");
        console.log(launchctl.strerror(err.errno));
        return done();
      });
    });
  });
  
  describe('#startSync(\'com.thisisafakejob.test\')',function() {
    it('should throw error [No such process]', function(done) {
      try {
        launchctl.startSync('com.thisisafakejob.test');
      }
      catch (e) {
        assert.equal(e.msg, launchctl.strerror(e.errno));
      }
      done();
    });
  });
  describe('#stop(\'com.thisisafakejob.test\')', function() {
    it('should throw error [No such process]', function(done) {
      launchctl.stop('com.thisisafakejob.test', function(err) {
        assert.equal(err.msg, launchctl.strerror(err.errno));
        return done();
      });
    });
  });
  
  describe('#stopSync(\'com.thisisafakejob.test\')', function() {
    it('should throw error [No such process]', function(done) {
      try {
        launchctl.stopSync('com.thisisafakejob.test');
      }
      catch (e) {
        assert.equal(e.msg, launchctl.strerror(e.errno));
      }
      done();
    });
  });

  describe('#load(\'/System/Library/LaunchDaemons/com.thisisafakejob.test.plist\')', function() {
    it('should throw error [No such file or directory]', function(done) {
      launchctl.load('/System/Library/LaunchDaemons/com.thisisafakejob.test.plist', function(err, res) {
        assert.equal(err.msg, launchctl.strerror(err.errno));
        return done();
      });
    });
  });
  
  describe('#getManagerName()', function() {
    it('should return Aqua or System', function(done) {
      var name = launchctl.getManagerName();
      if (name != "Aqua" && name != "System") {
        fail();
      }
      done();
    });
  });
  
  describe('#getManagerUID()', function() {
    it('Should return a number', function(done) {
      var uid = launchctl.getManagerUID();
      assert.equal(typeof uid, "number");
      if (process.getuid() === 0) {
        assert.equal(uid, 0);
      }
      done();
    });
  });
  
  describe('#getManagerPID()', function() {
    it('Should return a number', function(done) {
      var pid = launchctl.getManagerPID();
      assert.equal(typeof pid, "number");
      if (process.getuid() === 0) {
        assert.equal(pid, 1);
      }
      done();
    });
  });