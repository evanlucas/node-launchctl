/*!
 * Module dependencies
 */
var debug = require('debug')('launchctl')
  , ctl = require('bindings')('bindings')
  , errno = require('syserrno')

/*!
 * Expose LaunchCTL
 */
var LaunchCTL = exports;


/*!
 * Expose errno
 */
LaunchCTL.strerror = errno.strerror;
LaunchCTL.errorFromCode = errno.errorForCode;
LaunchCTL.errorFromErrno = errno.errorForErrno;

/**
 * `launchctl list`
 *
 * Examples:
 *
 * #### List All
 *
 *      try {
 *        var res = ctl.listSync()
 *      }
 *      catch (e) {
 *        throw e
 *      }
 *
 * #### List specific job
 *
 *      try {
 *        var res = ctl.listSync('com.apple.Dock.agent')
 *      }
 *      catch (e) {
 *        throw e
 *      }
 *
 * #### List by regular expression
 *
 *      try {
 *        var res = ctl.listSync(/com.apple.(.*)/)
 *      }
 *      catch (e) {
 *        throw e
 *      }
 * 
 * @param {String} name The job label or regular expression (optional)
 * @api public
 */
LaunchCTL.listSync = function() {
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
 * `launchctl list`
 * 
 *
 * Examples:
 *
 * #### List all
 *
 *      ctl.list(function(err, jobs) {
 *        if (err) throw err
 *        console.log(jobs)
 *      })
 *
 * #### List specific job
 *
 *      ctl.list('com.apple.Dock.agent', function(err, job) {
 *        if (err) throw err
 *        console.log(job)
 *      })
 *
 * #### List by regex
 *
 *      ctl.list(/com.apple.(.*)/, function(err, jobs) {
 *        if (err) throw err
 *        console.log(jobs)
 *      })
 *
 * @param {String} name The job label or regex (optional)
 * @param {Function} cb function(err, data)
 *
 *
 * @api public
 */
LaunchCTL.list = function() {
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
 * `launchctl start`
 *
 * Examples:
 *
 *      try {
 *        ctl.startSync('com.apple.Dock.agent')
 *      }
 *      catch (e) {
 *        throw e
 *      }
 *
 * @param {String} label The job label
 * @api public
 */
LaunchCTL.startSync = function(label) {
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
 * `launchctl start`
 *
 * Examples:
 *
 *      ctl.start('com.apple.Dock.agent', function(err) {
 *        if (err) throw err
 *        // Your code
 *      })
 *
 * @param {String} label The job label
 * @param {Function} cb function(err, res)
 * @api public
 */
LaunchCTL.start = function(label, cb) {
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
 * `launchctl stop`
 *
 * Examples:
 *
 *      try {
 *        var res = ctl.stopSync('com.apple.Dock.agent')
 *      }
 *      catch (e) {
 *        throw e
 *      }
 *
 * **Note:**
 * Keep in mind what launchd actually does on a job that has the `KeepAlive`
 * set to true.  It will simply restart the job, not actually stop it
 *
 * @param {String} label The job label
 * @api public
 */
LaunchCTL.stopSync = function(label) {
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
 * `launchctl stop`
 *
 * Examples:
 *
 *      ctl.stop('com.apple.Dock.agent', function(err) {
 *        if (err) throw err
 *        // Your code
 *      })
 *
 *
 * **Note:**
 * Keep in mind what launchd actually does on a job that has the `KeepAlive`
 * set to true.  It will simply restart the job, not actually stop it
 *
 * @param {String} label The job label
 * @param {Function} cb function(err)
 * @api public
 */
LaunchCTL.stop = function(label, cb) {
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
 * Removes a job with the given label from launchd
 * Equivalent to `launchctl remove`
 * 
 * Examples:
 *
 *      try {
 *        ctl.removeSync('com.jobname.test')
 *      }
 *      catch (e) {
 *        throw e
 *      }
 *
 * @param {String} label The job label
 * @api public
 */
LaunchCTL.removeSync = function(label) {
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
 * Equivalent to `launchctl remove`
 * 
 * Examples:
 *
 *      ctl.remove('com.jobname.test', function(err) {
 *        if (err) throw err
 *        // Your code
 *      })
 *
 * @param {String} label The job label
 * @param {Function} cb function(err)
 * @api public
 */
LaunchCTL.remove = function(label, cb) {
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
 * `launchctl load`
 *
 * Examples:
 *
 *      try {
 *        ctl.loadSync('/System/Library/...', {
 *          editondisk: false, // default
 *          forceload: false, // default
 *          session_type: 'Aqua',
 *          domain: 'user'
 *        })
 *      catch (e) {
 *        throw e
 *      }
 *
 * @param {String} path The path to a plist specifying job info
 * @param {Object} opts editondisk, forceload, session_type, domain
 *
 * @api public
 */
LaunchCTL.loadSync = function() {
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
 * Examples:
 *
 *      ctl.load('/System/Library/...', {
 *        editondisk: false,
 *        forceload: false,
 *        session_type: 'Aqua',
 *        domain: 'user'
 *      }, function(err) {
 *        if (err) throw err
 *        // Your code
 *      })
 *
 * @param {String} path The path to a plist specifying job info
 * @param {Object} opts editondisk, forceload, session_type, domain
 * @param {Function} cb function(err)
 *
 * @api public
 */
LaunchCTL.load = function() {
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
 * Examples:
 *
 *      try {
 *        ctl.unloadSync('/System/Library/...', {
 *          editondisk: false,
 *          forceload: false,
 *          session_type: 'Aqua',
 *          domain: 'user'
 *        })
 *      }
 *      catch (e) {
 *        throw e
 *      }
 *
 * @param {String} path The path to a plist specifying job info
 * @param {Object} opts editondisk, forceload, session_type, domain
 *
 * @api public
 */
LaunchCTL.unloadSync = function() {
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
 * Examples:
 *
 *      ctl.unload('/System/Library/..', {
 *        editondisk: false,
 *        forceload: false,
 *        session_type: 'Aqua',
 *        domain: 'user'
 *      }, function(err) {
 *        if (err) throw err
 *        // Your code
 *      })
 *
 * @param {String} path The path to a plist specifying job info
 * @param {Object} opts editondisk, forceload, session_type, domain
 * @param {Function} cb function(err)
 *
 * @api public
 */
LaunchCTL.unload = function() {
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
 * `launchctl managername`
 *
 * Examples:
 *
 *      var name = ctl.getManagerName()
 *      // => 'Aqua'
 *
 * @api public
 */
LaunchCTL.getManagerName = function() {
  return ctl.getManagerName();
}

/**
 * Gets the uid of the current manager
 * `launchctl manageruid`
 *
 * Examples:
 *
 *      var uid = ctl.getManagerUID()
 *      // => 501
 *
 * @api public
 */
LaunchCTL.getManagerUID = function() {
  return ctl.getManagerUID();
}

/**
 * Gets the pid of the current manager
 * `launchctl managerpid`
 *
 * Examples:
 *
 *      var pid = ctl.getManagerPID()
 *      // => 263
 *
 * @api public
 */
LaunchCTL.getManagerPID = function() {
  return ctl.getManagerPID();
}

/**
 * Construct launchctl plist object
 */
LaunchCTL.Plist = require('./plist')