/**
 * Module dependencies
 */
var async = require('async')
	, spawn = require('child_process').spawn
	, unzip = require('unzip')
	, request = require('request')
	, fs = require('fs')
	, path = require('path')
	, liburl = 'https://github.com/evanlucas/liblaunchctl/archive/master.zip';

/**
 * This file was inspired by node-gitteh install.js script
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

var buildDir = path.normalize(path.join(__dirname, "./deps/liblaunchctl/build"));

async.series([
  function(cb) {
    console.log('Downloading liblaunchctl');
    request(liburl).pipe(unzip.Extract({path: './deps/liblaunchctl'}).on('error', function(err) { throw err; }).on('close', cb));
  },
  function(cb) {
    console.log('Building liblaunchctl');
    envpassthru('make', '-f', './deps/liblaunchctl/Makefile', 'VERBOSE=1', cb);
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
