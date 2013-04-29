var launchctl = require('./lib/index')
  , assert = require('assert')

  describe('Get All Jobs', function() {
    it('should return an array of job objects', function(done) {
      launchctl.list(function(err, data) {
        if (err) {
          console.log(err);
          return done(err);
        } else {
          console.log('Found '+data.length+' jobs');
          return done();
        }
        
      });
    }); 
  });
  
  describe('Get job named com.apple.Dock.agent', function() {
    it('should return a single object', function(done) {
      launchctl.list('com.apple.Dock.agent', function(err, data) {
        return done(err);
      });
    }); 
  });
  
  describe('Get job matching regex /^com.apple.([\w]+)/', function() {
    it('should return an array of job objects', function(done) {
      launchctl.list(/^com.apple.([\w]+)/, function(err, data) {
        return done(err);
      });
    }); 
  });

  describe('Start a non-existent job', function() {
    it('should throw error', function(done) {
      launchctl.start('com.test.test', function(err) {
        if (err) {
          console.log(err);
          return done();
        } else {
          return done(new Error('Unexpected response.  No error thrown'));
        }
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
