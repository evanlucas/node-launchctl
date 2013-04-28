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
