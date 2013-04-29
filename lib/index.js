/*!
 * Module dependencies
 */
var ctl = require('../build/Release/launchctl.node');

/*!
 * Expose LaunchCTL
 */
var LaunchCTL = exports;

/**
 * Equivalent to launchctl list [<job_label>]
 * 
 * Get a job by label
 * @param {String} name The job label
 * ========== OR =============
 * Get jobs by regex
 * @param {String} regex Job to search for in regex form ex. /^com.apple.(.*)/ (techinally an object)
 * ========== OR =============
 * Get all jobs (no args) 
 *
 * @api public
 */
exports.listSync = function() {
  var args = Array.prototype.slice.call(arguments)
    , regex = (typeof args[args.length-1] === 'object') && args.pop()
    , name = (typeof args[args.length-1] === 'string') && args.pop();
  
  if (name) {
    return ctl.getJobSync(name);
  } else if (regex) {
    var results = [];
    var jobs = ctl.getAllJobsSync();
    jobs.forEach(function(job, index) {
      var label = job.label;
      if (regex.test(label)) {
        results.push(job);
      }
    });
    return results;
  } else {
    return ctl.getAllJobsSync();
  }
}

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
 *
 * @api public
 */
exports.list = function() {
  var args = Array.prototype.slice.call(arguments)
    , cb = (typeof args[args.length-1] === 'function') && args.pop()
    , regex = (typeof args[args.length-1] === 'object') && args.pop()
    , name = (typeof args[args.length-1] === 'string') && args.pop();

  if (name) {
    ctl.getJob(name, function(err, data) {
      if (err) {
        return cb(err);
      } else {
        return cb(null, data);
      }
    });
  } else if (regex) {
    // regex
    var results = [];
    ctl.getAllJobs(function(err, jobs) {
      jobs.forEach(function(job, index) {
        var label = job.label;
        if (regex.test(label)) {
          results.push(job);
        }
      });
      return cb(null, results);
    });
  } else {
    //var jobs = ctl.getAllJobsSync();
    return ctl.getAllJobs(function(err, data) {
      if (err) return cb(err);
      return cb(null, data);
    });
  }
}

/**
 * Start job with the given label
 *
 * @param {String} label The job label
 * @api public
 */
exports.startSync = function(label) {
  return ctl.startJobSync(label);
}

/**
 * Start job with the given label
 *
 * @param {String} label The job label
 * @param {Function} cb function(err, res)
 * @api public
 */
exports.start = function(label, cb) {
  ctl.startJob(label, function(err) {
    if (err) return cb(err);
    return cb(null);
  });
}

/**
 * Stop job with the given label
 *
 * @param {String} label The job label
 * @api public
 */
exports.stopSync = function(label) {
  return ctl.stopJobSync(label);
}

/**
 * Stop job with the given label
 *
 * @param {String} label The job label
 * @param {Function} function(err)
 * @api public
 */
exports.stop = function(label, cb) {
  ctl.stopJob(label, function(err) {
    if (err) return cb(err);
    return cb(null);
  });
}

/**
 * Restart a job with the given label
 * 
 * @param {String} label The job label
 * @api public
 */
exports.restartSync = function(label) {
  var o = ctl.stopJobSync(label);
  return ctl.startJobSync(label);
}

/**
 * Restart a job with the given label
 * 
 * @param {String} label The job label
 * @param {Function{ function(err)
 * @api public
 */
exports.restart = function(label, cb) {
  ctl.stopJob(label, function(err) {
    if (err) return cb(err);
    ctl.startJob(label, function(err) {
      if (err) return cb(err);
      return cb(null);
    });
  })
}