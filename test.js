var launchctl = require('./lib/index')
	, assert = require('assert');

describe('node-launchctl', function() {

	describe('Get All Jobs', function() {
		it('should return an array of job objects', function() {
			launchctl.list(function(err, data) {
				assert(err == null);
				console.log('Found '+data.length+' jobs');
			});
		});	
	});
	
	describe('Get job named com.apple.Dock.agent', function() {
		it('should return a single object', function() {
			launchctl.list('com.apple.Dock.agent', function(err, data) {
				assert(err == null);
			});
		});	
	});
	
	describe('Get job matching regex /^com.apple.([\w]+)/', function() {
		it('should return an array of job objects', function() {
			launchctl.list(/^com.apple.([\w]+)/, function(err, data) {
				assert.equal(err, null);
				console.log('Found '+data.length+' jobs');
			});
		});	
	});

});

