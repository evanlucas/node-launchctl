/*!
 * Module dependencies
 */
var ctl     = require('bindings')('bindings')
  , errno   = require('syserrno')
  , util    = require('util')
  , plist   = require('launchd.plist')

/*!
 * Expose LaunchCTL
 */
var LaunchCTL = exports


/*!
 * Expose errno
 */
LaunchCTL.strerror = errno.strerror
LaunchCTL.errorFromCode = errno.errorForCode
LaunchCTL.errorFromErrno = errno.errorForErrno

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
    , regex = (util.isRegExp(args[args.length-1])) && args.pop()
    , name = (typeof args[args.length-1] === 'string') && args.pop()

  if (name) {
    return ctl.getJobSync(name)
  } else if (regex) {
    var results = []
    var jobs = ctl.getAllJobsSync()
    return jobs.filter(function(job) {
      return job.Label && regex.test(job.Label)
    })
  } else {
    return ctl.getAllJobsSync()
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
    , regex = (util.isRegExp(args[args.length-1])) && args.pop()
    , name = (typeof args[args.length-1] === 'string') && args.pop()

  if (name) {
    ctl.getJob(name, function(err, data) {
      if (err) {
        return cb(err)
      } else {
        return cb(null, data)
      }
    })
  } else if (regex) {
    // regex
    var results = []
    ctl.getAllJobs(function(err, jobs) {
      jobs = jobs.filter(function(job) {
        return job.Label && regex.test(job.Label)
      })
      return cb(null, jobs)
    })
  } else {
    return ctl.getAllJobs(function(err, data) {
      if (err) return cb(err)
      return cb(null, data)
    })
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
  var res
  try {
    res = ctl.startStopRemoveSync(label, 1)
  }
  catch (e) {
    if (e.msg && e.errno) {
      var err = errno.errorForErrno(e.errno)
      if (err.errno > 0) {
        throw err
      }
    }
    throw e
  }
  return res
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
        var e = errno.errorForErrno(err.errno)
        if (e.errno > 0) return cb(e)
      }
      return cb(err)
    }
    return cb(null)
  })
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
  var res
  try {
    res = ctl.startStopRemoveSync(label, 2)
  }
  catch (e) {
    if (e.msg && e.errno) {
      var err = errno.errorForErrno(e.errno)
      if (err.errno > 0) {
        throw err
      }
    }
    throw e
  }
  return res
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
        var e = errno.errorForErrno(err.errno)
        if (e.errno > 0) return cb(e)
      }
      return cb(err)
    }
    return cb(null)
  })
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
  var res
  try {
    res = ctl.startStopRemoveSync(label, 3)
  }
  catch (e) {
    if (e.msg && e.errno) {
      var err = errno.errorForErrno(e.errno)
      if (err.errno > 0) {
        throw err
      }
    }
    throw e
  }
  return res
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
        var e = errno.errorForErrno(err.errno)
        if (e.errno > 0) return cb(e)
      }
      return cb(err)
    }
    return cb(null)
  })
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
    , as = []

  if (args.length == 1) {
    jobpath = args[0]
    as.push(jobpath)
    as.push(false)
    as.push(false)
  } else if (args.length == 2) {
    if (typeof args[0] !== 'string') throw new Error('Job path must be a string')
    jobpath = args[0]
    as.push(jobpath)
    if (typeof args[1] !== 'object') throw new Error('Options must be an object')
    var opts = args[1]
    editondisk = opts.editondisk || false
    as.push(editondisk)
    forceload = opts.forceload || false
    as.push(forceload)

    if (opts.session_type) {
      as.push(opts.session_type)
    }
    if (opts.domain) {
      as.push(opts.domain)
    }
  } else {
    throw new Error('Invalid arguments')
  }
  if (as.length < 3) {
    throw new Error('Invalid arguments.')
  }
  return ctl.loadJobSync.apply(ctl, as)
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
    , as = []
  if (!cb) {
    throw new Error('Callback must be a function')
  }
  jobpath = args[0]
  as.push(jobpath)
  if (args.length == 1) {
    as.push(false)
    as.push(false)
  } else if (args.length == 2) {
    if (typeof args[0] !== 'string') throw new Error('Job path must be a string')
    if (typeof args[1] !== 'object') throw new Error('Options must be an object')
    var opts = args[1]
    editondisk = opts.editondisk || false
    as.push(editondisk)
    forceload = opts.forceload || false
    as.push(forceload)

    if (opts.session_type) {
      as.push(opts.session_type)
    }
    if (opts.domain) {
      as.push(opts.domain)
    }
  }
  as.push(cb)
  if (as.length < 4) {
    throw new Error('Invalid arguments')
  }
  ctl.loadJob.apply(ctl, as)
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
    , as = []

  if (args.length == 1) {
    jobpath = args[0]
    as.push(jobpath)
    as.push(false)
    as.push(false)
  } else if (args.length == 2) {
    if (typeof args[0] !== 'string') throw new Error('Job path must be a string')
    jobpath = args[0]
    as.push(jobpath)
    if (typeof args[1] !== 'object') throw new Error('Options must be an object')
    var opts = args[1]
    editondisk = opts.editondisk || false
    as.push(editondisk)
    forceload = opts.forceload || false
    as.push(forceload)

    if (opts.session_type) {
      as.push(opts.session_type)
    }
    if (opts.domain) {
      as.push(opts.domain)
    }
  } else {
    throw new Error('Invalid arguments')
  }
  if (as.length < 3) {
    throw new Error('Invalid arguments.')
  }
  return ctl.unloadJobSync.apply(ctl, as)
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
    , as = []
  if (!cb) {
    throw new Error('Callback must be a function')
  }
  jobpath = args[0]
  as.push(jobpath)
  if (args.length == 1) {
    as.push(false)
    as.push(false)
  } else if (args.length == 2) {
    if (typeof args[0] !== 'string') throw new Error('Job path must be a string')
    if (typeof args[1] !== 'object') throw new Error('Options must be an object')
    var opts = args[1]
    editondisk = opts.editondisk || false
    as.push(editondisk)
    forceload = opts.forceload || false
    as.push(forceload)

    if (opts.session_type) {
      as.push(opts.session_type)
    }
    if (opts.domain) {
      as.push(opts.domain)
    }
  }
  as.push(cb)
  if (as.length < 4) {
    throw new Error('Invalid arguments')
  }
  ctl.unloadJob.apply(ctl, as)
}

/**
 * Submit a job
 *
 * Examples:
 *
 *      try {
 *        ctl.submitSync({
 *            label: 'com.test.label'
 *          , program: '/bin/ls'
 *          , stderr: '/var/log/test.err.log'
 *          , stdout: '/var/log/test.out.log'
 *          , args: ['-l', '-a', '-h']
 *        })
 *      }
 *      catch(e) {
 *        throw e
 *      }
 *
 * @param {Object} args `label`, `program`, `stderr`, `stdout`, `args` (args must be an array)
 * @api public
 */
LaunchCTL.submitSync = function(args) {
  if (!args) throw new Error('Invalid arguments')
  var res
  try {
    res = ctl.submitJobSync(args)
  }

  catch (e) {
    if (e.msg && e.errno) {
      var err = errno.errorForErrno(e.errno)
      if (err.errno > 0) throw err

    }
    throw e
  }
  return res
}

/**
 * Submit a job
 *
 * Examples:
 *
 *      ctl.submit({
 *          label: 'com.test.label'
 *        , program: '/bin/ls'
 *        , stderr: '/var/log/test.err.log'
 *        , stdout: '/var/log/test.out.log'
 *        , args: ['-l', '-a', '-h']
 *      }, function(err) {
 *        if (err) {
 *          console.log(err)
 *        } else {
 *          console.log('Success')
 *        }
 *      })
 *
 * @param {Object} data `label`, `program`, `stderr`, `stdout`, `args` (args must be an array)
 * @param {Function} cb function(err)
 * @api public
 */
LaunchCTL.submit = function() {
  var args = Array.prototype.slice.call(arguments)
    , cb = (typeof args[args.length-1] === 'function') && args.pop()
    , data = (typeof args[args.length-1] === 'object') && args.pop()
  if (!cb) cb = function() {}
  if (!data) return cb(new Error('Invalid arguments'))
  return ctl.submitJob(data, cb)
}

/**
 * Gets the name of the current manager (session)
 * `launchctl managername`
 *
 * Examples:
 *
 *      var name = ctl.managername()
 *      // => 'Aqua'
 *
 * @api public
 */
LaunchCTL.managername = function() {
  return ctl.getManagerName()
}

/**
 * Gets the uid of the current manager
 * `launchctl manageruid`
 *
 * Examples:
 *
 *      var uid = ctl.manageruid()
 *      // => 501
 *
 * @api public
 */
LaunchCTL.manageruid = function() {
  return ctl.getManagerUID()
}

/**
 * Gets the pid of the current manager
 * `launchctl managerpid`
 *
 * Examples:
 *
 *      var pid = ctl.managerpid()
 *      // => 263
 *
 * @api public
 */
LaunchCTL.managerpid = function() {
  return ctl.getManagerPID()
}

/**
 * Gets/sets the launchd resource limits
 *
 * Examples:
 *
 *      var limits = ctl.limit()
 *      // => {
 *      // =>   cpu: { soft: 'unlimited', hard: 'unlimited' },
 *      // =>   filesize: { soft: 'unlimited', hard: 'unlimited' },
 *      // =>   data: { soft: 'unlimited', hard: 'unlimited' },
 *      // =>   stack: { soft: '8388608', hard: '67104768' },
 *      // =>   core: { soft: '0', hard: 'unlimited' },
 *      // =>   rss: { soft: 'unlimited', hard: 'unlimited' },
 *      // =>   memlock: { soft: 'unlimited', hard: 'unlimited' },
 *      // =>   maxproc: { soft: '1000', hard: '2000' },
 *      // =>   maxfiles: { soft: '8192', hard: '20480' }
 *      // => }
 *
 *
 *  Get `maxproc`
 *
 *      var res = ctl.limit('maxproc')
 *      // => { soft: '1000', hard: '2000' }
 *
 * Set `maxproc` limit
 *
 *      var res = ctl.limit('maxproc', '1200', '2000')
 *      // => 0
 *
 * @param {String} limtype The specific type to get (optional)
 * @param {String|Number} soft The soft limit (optional)
 * @param {String|Number} hard The hard limit (optional)
 * @api public
 */
LaunchCTL.limit = function(limtype, soft, hard) {
  var err
  if (limtype && !soft && !hard) {
    try {
      var lims = ctl.getLimitSync()
      var keys = Object.keys(lims)
      limtype = limtype.toLowerCase()
      if (~keys.indexOf(limtype)) {
        return lims[limtype]
      } else {
        err = LaunchCTL.errorFromCode('EINVLIM')
        throw err
      }
    }
    catch(e) {
      throw e
    }
  } else if (limtype && (soft || hard)) {
    // We are setting a limit
    try {
      if ('number' === typeof soft) {
        soft = soft.toString()
      }
      if (args.length === 2) hard = soft
      if (limtype === 'maxfiles' && (soft === 'unlimited' || hard === 'unlimited')) {
        err = LaunchCTL.errorFromCode('EINVLIM')
        err.msg = 'Invalid limit. The limit for `maxfiles` cannot be set to `unlimited`'
        throw err
      }
      return ctl.setLimitSync(limtype, soft, hard)
    }
    catch (e) {
      throw e
    }
  } else {
    // List all limits
    return ctl.getLimitSync()
  }
}

/**
 * Sets a launchd environment variable
 *
 * @param {String} key The key
 * @param {String} val The value
 * @api public
 */
LaunchCTL.setEnvVar = function(key, val) {
  var res
  try {
    res = ctl.setEnvVar(key, val)
  }
  catch (e) {
    if (e.msg && e.errno) {
      var err = errno.errorForErrno(e.errno)
      if (err.errno > 0) throw err
    }
    throw e
  }
  return res
}

/**
 * Unsets a launchd environment variable
 *
 * @param {String} key The key
 * @api public
 */
LaunchCTL.unsetEnvVar = function(key) {
  var res
  try {
    res = ctl.unsetEnvVar(key)
  }
  catch(e) {
    if (e.msg && e.errno) {
      var err = errno.errorForErrno(e.errno)
      if (err.errno > 0) throw err
    }
    throw e
  }
  return res
}

/**
 * Gets an Environment Variable (or all of them)
 *
 * If no `key` is passed, it will return an Object
 * If `key` is passed, if the key exists, it will return a string
 * If `key` is passed, but does not exist, it will return false
 *
 * @param {String} key (optional)
 * @api public
 * @returns Object|String|false
 */
LaunchCTL.getEnvVar = function(key) {
  if (key) {
    var res = ctl.getEnv()
    if (res.hasOwnProperty(key)) {
      return res[key]
    } else {
      return false
    }
  }
  return ctl.getEnv()
}

/**
 * Gets rusage for either `self` or `children`
 *
 * @param {String} who Either `self` or `children`
 * @api public
 */
LaunchCTL.getRUsage = function(who) {
  var res
  try {
    res = ctl.getRUsage(who)
  }
  catch(e) {
    if (e.msg && e.errno) {
      var err = errno.errorForErrno(e.errno)
      if (err.errno > 0) throw err
    }
    throw e
  }
  return res
}

/**
 * Gets or sets the `umask`
 *
 * Example:
 *
 *     var res = ctl.umask('22')
 *
 *     var res = ctl.umask()
 *
 * @param {String} arg The umask (optional)
 * @api public
 */
LaunchCTL.umask = function(arg) {
  var res
  try {
    if (arg) {
      res = ctl.umask(arg)
    } else {
      res = ctl.umask()
    }
  }
  catch(e) {
    if (e.msg && e.errno) {
      var err = errno.errorForErrno(e.errno)
      if (err.errno > 0) throw err
    }
    throw e
  }
  return res
}
/**
 * Construct launchctl plist object
 */
LaunchCTL.Plist = plist
