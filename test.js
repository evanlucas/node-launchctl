var launchctl = require('./lib/index')
  , assert = require('assert')

  describe('#list()', function() {
    it('should return an array of job objects', function(done) {
      launchctl.list(function(err, data) {
        assert.ifError(err);
        assert.notEqual(data.length, 0, 'Should be an array of objects');
        done();
      });
    }); 
  });
  
  describe('#listSync()', function() {
    it('should return an array of job objects', function(done) {
      var jobs = launchctl.listSync();
      assert.notEqual(jobs.length, 0, 'Should be an array of objects');
      done();
    });
  });
  
  describe('#list(\'com.apple.Dock.agent\')', function() {
    it('should return a single object', function(done) {
      launchctl.list('com.apple.Dock.agent', function(err, data) {
        assert.ifError(err);
        assert.equal(typeof data, 'object');
        done();
      });
    }); 
  });
  
  describe('#listSync(\'com.apple.Dock.agent\')', function() {
    it('should return a single object', function(done) {
      var job = launchctl.listSync('com.apple.Dock.agent');
      assert.equal(typeof job, 'object');
      done();
    });
  });
  
  describe('#list(/^com.apple.([\w]+)/)', function() {
    it('should return an array of job objects', function(done) {
      launchctl.list(/^com.apple.([\w]+)/, function(err, data) {
        assert.notEqual(data.length, 0, 'Should be an array of objects');
        done();
      });
    }); 
  });

  describe('#start(\'com.thisisafakejob.test\')', function() {
    it('should throw error [No such process]', function(done) {
      launchctl.start('com.thisisafakejob.test', function(err) {
        assert.equal(err.code, "No such process");
        return done();
      });
    });
  });
  
  describe('#startSync(\'com.thisisafakejob.test\')',function() {
    it('should throw error [No such process]', function(done) {
      var result = launchctl.startSync('com.thisisafakejob.test');
      if (e = launchctl.error(result)) {
        assert.equal(e.msg, "No such process");
        return done();
      } else {
        fail();
      }
    });
  });
  describe('#stop(\'com.thisisafakejob.test\')', function() {
    it('should throw error [No such process]', function(done) {
      launchctl.stop('com.thisisafakejob.test', function(err) {
        assert.equal(err.code, "No such process");
        return done();
      });
    });
  });
  
  describe('#stopSync(\'com.thisisafakejob.test\')', function() {
    it('should throw error [No such process]', function(done) {
      var result = launchctl.stopSync('com.thisisafakejob.test');
      if (e = launchctl.error(result)) {
        assert.equal(e.msg, "No such process");
        return done();
      } else {
        fail();
      }
    });
  });

	describe('#load(\'/System/Library/LaunchDaemons/com.thisisafakejob.test.plist\')', function() {
		it('Not sure yet', function(done) {
			launchctl.load('/System/Library/LaunchDaemons/com.thisisafakejob.test.plist', function(err, res) {
		
			});
		});
	});
