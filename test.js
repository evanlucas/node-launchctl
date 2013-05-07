var launchctl = require('./lib/index')
  , assert = require('assert')

  describe('#list()', function() {
    it('should return an array of job objects', function(done) {
      launchctl.list(function(err, data) {
        if (err) {
          return done(err);
        } else {
          return done();
        }
        
      });
    }); 
  });
  
  describe('#listSync()', function() {
    it('should return an array of job objects', function(done) {
      var jobs = launchctl.listSync();
      if (jobs) {
        return done();
      }
    });
  });
  
  describe('#list(\'com.apple.Dock.agent\')', function() {
    it('should return a single object', function(done) {
      launchctl.list('com.apple.Dock.agent', function(err, data) {
        return done(err);
      });
    }); 
  });
  
  describe('#listSync(\'com.apple.Dock.agent\')', function() {
    it('should return a single object', function(done) {
      var job = launchctl.listSync('com.apple.Dock.agent');
      if (job) {
        return done();
      }
    });
  });
  
  describe('#list(/^com.apple.([\w]+)/)', function() {
    it('should return an array of job objects', function(done) {
      launchctl.list(/^com.apple.([\w]+)/, function(err, data) {
        return done(err);
      });
    }); 
  });

  describe('#start(\'com.test.test\')', function() {
    it('should throw error', function(done) {
      launchctl.start('com.test.test', function(err) {
        console.log(err);
        assert(err != null);
        return done();
      });
    });
  });
  
  describe('Stop a non-existent job', function() {
    it('should throw error', function(done) {
      launchctl.stop('com.test.test', function(err) {
        if (err) {
          console.log(err);
          return done();
        } else {
          return done(new Error('Unexpected response.  No error thrown'));
        }
      });
    });
  });
  
  describe('Start a process that has already been started', function() {
    it('should not throw an error, but does nothing', function(done) {
      launchctl.start('com.apple.Dock.agent', function(err) {
        return done(err);
      });
    });
  });
  
  //
  // Uncomment to test restarting dock
  // I figured people didn't want their dock being restarted in a test...
  //
  
  
	/*
describe('Restart a process that is known to be running', function() {
    it('should return 0', function(done) {
      launchctl.restart('com.apple.Dock.agent', function(err) {
				return done(err);
      });
    });
  });
*/

  
  describe('Restart a non-existent job', function() {
    it('should throw error', function(done) {
      launchctl.restart('com.test.test', function(err) {
        if (err) {
          console.log(err);
          return done();
        } else {
          return done(new Error('Unexpected response. No error thrown'));
        }
      });
    });
  });
  
  describe('Load job', function() {
    it('should not throw error', function() {
      var res = launchctl.loadSync('/System/Library/LaunchDaemons/com.hbc.CardioOnCall.plist');
      console.log(res);
    });
  });
