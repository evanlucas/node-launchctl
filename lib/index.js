var ctl = require('../launchctl.node');

var LaunchCTL = exports;

/**
 * Equivalent to launchctl list [<job_label>]
 * 
 * @param {String} name The job label
 * @param {Function} cb function(err, data)
 * ========== OR =============
 * @param {String} regex Job to search for in regex form ex. /^com.apple.(.*)/ (techinally an object)
 * @param {Function} cb function(err, data)
 * ========== OR =============
 * @param {Function} cb function(err, data)
 *
 * Currently synchronous...need to look into making it asynchronous
 *
 * @api public
 */

exports.list = function() {
	var args = Array.prototype.slice.call(arguments)
		, cb = (typeof args[args.length-1] === 'function') && args.pop()
		, regex = (typeof args[args.length-1] === 'object') && args.pop()
		, name = (typeof args[args.length-1] === 'string') && args.pop();

	if (name) {
		var job = ctl.getJob(name);
		if (job == 0) {
			return cb(new Error('Unable to find job labeled '+name));
		}
		return cb(null, job);
	} else if (regex) {
		// regex
		var jobs = ctl.getAllJobs();
		var results = [];
		jobs.forEach(function(job, index) {
			var label = job.label;
			if (regex.test(label)) {
				results.push(job);
			}
		});
		return cb(null, results);
	} else {
		var jobs = ctl.getAllJobs();
		return cb(null, jobs);
	}
}

/* Currently synchronous */
exports.start = function(name) {
	return ctl.startJob(name);
}

exports.stop = function(name) {
	return ctl.stopJob(name);
}