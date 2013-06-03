/*!
 * Module dependencies
 */
var ctl;
try {
  ctl = require('../build/Release/launchctl.node');
} 
catch (e) {
  if (e) {
    ctl = require('../build/Debug/launchctl.node');
  }
}
var debug = require('debug')('launchctl')
  , errno = require('syserrno');

/*!
 * Expose LaunchCTL
 */
var LaunchCTL = exports;


/*!
 * Expose errno
 */
exports.strerror = errno.strerror;
exports.errorFromCode = errno.errorForCode;
exports.errorFromErrno = errno.errorForErrno;

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
    debug('Listing job by label');
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
    debug('List job by label');
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
  var res;
  try {
    res = ctl.startStopRemoveSync(label, 1);
  }
  catch (e) {
    if (e.msg && e.errno) {
      var err = errno.errorForErrno(e.errno);
      if (err.errno > 0) {
        throw err;        
      }
    }
    throw e;
  }
  return res;
}

/**
 * Start job with the given label
 *
 * @param {String} label The job label
 * @param {Function} cb function(err, res)
 * @api public
 */
exports.start = function(label, cb) {
  ctl.startStopRemove(label, 1, function(err) {
    if (err) {
      if (err.msg && err.errno) {
        var e = errno.errorForErrno(err.errno);
        if (e.errno > 0) return cb(e);
      }
      
      return cb(err);
    }
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
  var res;
  try {
    res = ctl.startStopRemoveSync(label, 2);
  }
  catch (e) {
    if (e.msg && e.errno) {
      var err = errno.errorForErrno(e.errno);
      if (err.errno > 0) {
        throw err;        
      }
    }
    throw e;
  }
  return res;
}

/**
 * Stop job with the given label
 *
 * @param {String} label The job label
 * @param {Function} cb function(err)
 * @api public
 */
exports.stop = function(label, cb) {
  ctl.startStopRemove(label, 2, function(err) {
    if (err) {
      if (err.msg && err.errno) {
        var e = errno.errorForErrno(err.errno);
        if (e.errno > 0) return cb(e);
      }
      
      return cb(err);
    }
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
  var o = ctl.startStopRemoveSync(label, 2);
  if (o != 0) return o;
  return ctl.startStopRemove(label, 1);
}

/**
 * Restart a job with the given label
 * 
 * @param {String} label The job label
 * @param {Function} cb function(err)
 * @api public
 */
exports.restart = function(label, cb) {
  ctl.startStopRemove(label, 2, function(err) {
    if (err) {
      if (err.msg && err.errno) {
        var e = errno.errorForErrno(err.errno);
        if (e.errno > 0) return cb(e);
      }
      
      return cb(err);
    }
    ctl.startStopRemove(label, 1, function(err) {
      if (err) {
        if (err.msg && err.errno) {
          var e = errno.errorForErrno(err.errno);
          if (e.errno > 0) return cb(e);
        }
        
        return cb(err);
      }
      return cb(null);
    });
  });
}

/**
 * Removes a job with the given label from launchd
 * 
 * @param {String} label The job label
 * @api public
 */
exports.removeSync = function(label) {
  var res;
  try {
    res = ctl.startStopRemoveSync(label, 3);
  }
  catch (e) {
    if (e.msg && e.errno) {
      var err = errno.errorForErrno(e.errno);
      if (err.errno > 0) {
        throw err;        
      }
    }
    throw e;
  }
  return res;
}

/**
 * Removes a job with the given label from launchd
 * 
 * @param {String} label The job label
 * @param {Function} cb function(err)
 * @api public
 */
exports.remove = function(label, cb) {
	ctl.startStopRemove(label, 3, function(err) {
		if (err) {
      if (err.msg && err.errno) {
        var e = errno.errorForErrno(err.errno);
        if (e.errno > 0) return cb(e);
      }
      
      return cb(err);
    }
		return cb(null);
	});
}

/**
 * Loads a job
 *
 * @param {String} path The path to a plist specifying job info
 * @param {Object} opts editondisk, forceload, session_type, domain
 *
 * @api public
 */
exports.loadSync = function() {
  var args = Array.prototype.slice.call(arguments)
    , jobpath
    , editondisk
    , forceload
    , session_type
    , domain
    , as = [];
  
  if (args.length == 1) {
    jobpath = args[0];
    as.push(jobpath);
    as.push(false);
    as.push(false);
  } else if (args.length == 2) {
    if (typeof args[0] !== 'string') throw new Error('Job path must be a string');
    jobpath = args[0];
    as.push(jobpath);
    if (typeof args[1] !== 'object') throw new Error('Options must be an object');
    var opts = args[1];
    editondisk = opts.editondisk || false;
    as.push(editondisk);
    forceload = opts.forceload || false;
    as.push(forceload);
    
    if (opts.session_type) {
      as.push(opts.session_type);
    }
    if (opts.domain) {
      as.push(opts.domain);
    }
  } else {
    throw new Error('Invalid arguments');
  }
  if (as.length < 3) {
    throw new Error('Invalid arguments.');
  }
  return ctl.loadJobSync.apply(ctl, as);
}

/**
 * Loads a job
 *
 * @param {String} path The path to a plist specifying job info
 * @param {Object} opts editondisk, forceload, session_type, domain
 * @param {Function} cb function(err, result)
 *
 * @api public
 */
exports.load = function() {
  var args = Array.prototype.slice.call(arguments)
    , cb = (typeof args[args.length-1] === 'function') && args.pop()
    , jobpath
    , editondisk
    , forceload
    , session_type
    , domain
    , as = [];
  if (!cb) {
    throw new Error('Callback must be a function');
  }
  jobpath = args[0];
  as.push(jobpath);
  if (args.length == 1) {
    as.push(false);
    as.push(false);
  } else if (args.length == 2) {
    if (typeof args[0] !== 'string') throw new Error('Job path must be a string');
    if (typeof args[1] !== 'object') throw new Error('Options must be an object');
    var opts = args[1];
    editondisk = opts.editondisk || false;
    as.push(editondisk);
    forceload = opts.forceload || false;
    as.push(forceload);
    
    if (opts.session_type) {
      as.push(opts.session_type);
    }
    if (opts.domain) {
      as.push(opts.domain);
    }
  }
  as.push(cb);
  if (as.length < 4) {
    throw new Error('Invalid arguments');
  }
  ctl.loadJob.apply(ctl, as);
}

/**
 * Unload job at given path
 *
 * @param {String} path The path to a plist specifying job info
 * @param {Object} opts editondisk, forceload, session_type, domain
 *
 * @api public
 */
exports.unloadSync = function() {
 var args = Array.prototype.slice.call(arguments)
    , jobpath
    , editondisk
    , forceload
    , session_type
    , domain
    , as = [];
  
  if (args.length == 1) {
    jobpath = args[0];
    as.push(jobpath);
    as.push(false);
    as.push(false);
  } else if (args.length == 2) {
    if (typeof args[0] !== 'string') throw new Error('Job path must be a string');
    jobpath = args[0];
    as.push(jobpath);
    if (typeof args[1] !== 'object') throw new Error('Options must be an object');
    var opts = args[1];
    editondisk = opts.editondisk || false;
    as.push(editondisk);
    forceload = opts.forceload || false;
    as.push(forceload);
    
    if (opts.session_type) {
      as.push(opts.session_type);
    }
    if (opts.domain) {
      as.push(opts.domain);
    }
  } else {
    throw new Error('Invalid arguments');
  }
  if (as.length < 3) {
    throw new Error('Invalid arguments.');
  }
  return ctl.unloadJobSync.apply(ctl, as);
}

/**
 * Unloads a job
 *
 * @param {String} path The path to a plist specifying job info
 * @param {Object} opts editondisk, forceload, session_type, domain
 * @param {Function} cb function(err, result)
 *
 * @api public
 */
exports.unload = function() {
  var args = Array.prototype.slice.call(arguments)
    , cb = (typeof args[args.length-1] === 'function') && args.pop()
    , jobpath
    , editondisk
    , forceload
    , session_type
    , domain
    , as = [];
  if (!cb) {
    throw new Error('Callback must be a function');
  }
  jobpath = args[0];
  as.push(jobpath);
  if (args.length == 1) {
    as.push(false);
    as.push(false);
  } else if (args.length == 2) {
    if (typeof args[0] !== 'string') throw new Error('Job path must be a string');
    if (typeof args[1] !== 'object') throw new Error('Options must be an object');
    var opts = args[1];
    editondisk = opts.editondisk || false;
    as.push(editondisk);
    forceload = opts.forceload || false;
    as.push(forceload);
    
    if (opts.session_type) {
      as.push(opts.session_type);
    }
    if (opts.domain) {
      as.push(opts.domain);
    }
  }
  as.push(cb);
  if (as.length < 4) {
    throw new Error('Invalid arguments');
  }
  ctl.unloadJob.apply(ctl, as);
}

/**
 * Gets the name of the current manager (session)
 *
 * @api public
 */
exports.getManagerName = function() {
  return ctl.getManagerName();
}

/**
 * Gets the uid of the current manager
 *
 * @api public
 */
exports.getManagerUID = function() {
  return ctl.getManagerUID();
}

/**
 * Gets the pid of the current manager
 *
 * @api public
 */
exports.getManagerPID = function() {
  return ctl.getManagerPID();
}