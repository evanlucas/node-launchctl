/**
 * Module dependencies
 */
var async = require('async')
	, spawn = require('child_process').spawn
	, unzip = require('unzip')
	, request = require('request')
	, fs = require('fs')
	, path = require('path')
	, args = process.argv.splice(2)
	, branch = (args.length === 0) ? 'master' : 'xcodeproj'
	, liburl = (branch == 'master') ? 'https://github.com/evanlucas/liblaunchctl/archive/master.zip' : 'https://github.com/evanlucas/liblaunchctl/archive/xcodeproj.zip';

console.log('Building from the '+branch+' branch');


/**
 * This file was inspired by node-gitteh install.js script and is very, very similar
 * The main difference being that instead of using git, we are download zipped archives
 * Since it will only run on a mac (unless you've got launchd compiled on BSD and working)
 * 
 * This was an attempt to prevent the user from having to already be in a git repo
 * When he/she installs this.  It also just makes the build process easier in general
 * By not changing directories over and over and over again.
 *
 * All in all,
 * Seemed to be a good way to build and link a dynamic library
 *
 */
function passthru() {
	var args = Array.prototype.slice.call(arguments);
	var cb = args.splice(-1)[0];
	var cmd = args.splice(0, 1)[0];
	var opts = {};
	if(typeof(args.slice(-1)[0]) === "object") {
		opts = args.splice(-1)[0];
	}
	var child = spawn(cmd, args, opts);

	child.stdout.pipe(process.stdout);
	child.stderr.pipe(process.stderr);
	child.on("exit", cb);
}

function shpassthru() {
	var cmd = 
	passthru.apply(null, ["/bin/sh", "-c"].concat(Array.prototype.slice.call(arguments)));
}

function envpassthru() {
	passthru.apply(null, ["/usr/bin/env"].concat(Array.prototype.slice.call(arguments)));
}

var buildDir = path.normalize(path.join(__dirname, (branch == 'master') ? './deps/liblaunchctl-master/build' : "./deps/liblaunchctl-xcodeproj/build"));

async.series([
  function(cb) {
    console.log('Removing old dependencies');
    envpassthru('rm', '-rf', './liblaunchctl.zip', function(err) {
      envpassthru('rm', '-rf', './deps', cb);
    });
  },
  function(cb) {
    console.log('Downloading liblaunchctl');
    var req = request(liburl);
    var x = fs.createWriteStream('./liblaunchctl.zip');
    x.on('close', function() {
      console.log('closed');
      return cb(null);
    });
    
    x.on('error', function(err) {
      throw err;
    });
    
    req.pipe(x);
  },
  function(cb) {
    console.log('Extracting liblaunchctl');
    envpassthru('unzip', './liblaunchctl.zip', '-d', path.normalize(path.join(__dirname, './deps')), function(err) {
      if (err) {
        return cb(err);
      }
      console.log('Cleaning up');
      envpassthru('rm', '-rf', './liblaunchctl.zip', cb);  
    });
  },
  function(cb) {
    console.log('Building liblaunchctl');
    envpassthru('xcodebuild', '-configuration', 'Release', 'BUILT_PRODUCTS_DIR='+path.normalize(__dirname), { cwd: './deps/liblaunchctl-'+branch}, cb);  
  },
  function(cb) {
    console.log('Building native module.');
    shpassthru('./node_modules/.bin/node-gyp configure --debug', cb);
  },
  function(cb) {
    shpassthru('./node_modules/.bin/node-gyp build --debug', cb);
  }
], function(err) {
  if (err) process.exit(err);
});
