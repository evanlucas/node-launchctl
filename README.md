# node-launchctl

Provides native bindings to launchctl commands

*Please note that this module is in the earlier stages. It could change.*

## Install

		npm install launchctl

## Test
  
    make test

If you run into errors with the install, `cd` to the node_modules dir and run `make install`. This will install liblaunchctl.dylib to `/usr/local/lib`.

## API

		var launchctl = require('launchctl')


**Do keep in mind that the synchronous functions probably should be run in a try/catch block, because they WILL throw an error if necessary**

### list

**List all jobs**

		launchctl.list(function(err, jobs){
			if (err) {
				console.error(err);
			} else {
				console.log(jobs);
			}
		});
		
*Will return something like:*

		[
			{ 
				label: 'com.apple.KerberosHelper.LKDCHelper',
				pid: '-',
				status: 1
			},
			{
				label: 'com.apple.tccd',
				pid: 304,
				status: '-'
			},
			...
		]

**List all jobs matching regex**

		launchctl.list(/^com.apple.(.*)/, function(err, jobs) {
			if (err) {
				console.error(err);
			} else {
				console.log(jobs);
			}
		});

*Will return an array of jobs like above*


**List a job by name**

		launchctl.list('com.apple.Dock.agent', function(err, job) {
			if (err) {
				console.error(err);
			} else {
				console.log(job);	
			}
		});

*Will return something like:*

		{
			Label: 'com.apple.Dock.agent',
			LimitLoadToSessionType: 'Aqua',
			OnDemand: 1,
			LastExitStatus: 256,
			PID: 6601,
			TimeOut: 30,
			Program: '/System/Library/CoreServices/Dock.app/Contents/MacOS/Dock',
			MachServices: {
				'com.apple.dock.appstore': 'mach-port-object',
				'com.apple.dock.server': 'mach-port-object',
				'com.apple.dock.fullscreen': 'mach-port-object',
				'com.apple.dock.downloads': 'mach-port-object',
				'com.apple.dock.notificationcenter': 'mach-port-object'
			},
			PerJobMachServices: {
				'com.apple.CFPastboardClient': 'mach-port-object',
				'com.apple.tsm.portname': 'mach-port-object',
				'com.apple.coredrag': 'mach-port-object',
				'com.apple.axserver': 'mach-port-object'
			}
		}

<hr>

### listSync

Synchronous version of `list`

- returns either an array of jobs or a job object

<hr>

### start

**Start job with given label**

		launchctl.start('com.apple.Dock.agent', function(err){
			if (err) {
				console.error(err);
			} else {
				console.log('Successfully started job');
			}
		});

<hr>

### startSync

Synchronous version of `start`

		var res = launchctl.startSync('com.apple.Dock.agent');
		if (e = launchctl.error(res)) {
			console.log(e);
		} else {
			console.log('Successfully started');
		}

<hr>

### stop

**Stop job with given label**

		launchctl.stop('homebrew.mxcl.redis', function(err) {
			if (err) {
				console.error(err);
			} else {
				console.log('Successfully stopped job');
			}
		});

<hr>

### stopSync

Synchronous version of `stop`

		var res = launchctl.startSync('com.apple.Dock.agent');
		if (e = launchctl.error(res)) {
			console.log(e);
		} else {
			console.log('Successfully stopped');
		}

<hr>

### restart

*NOTE: Keep in mind what stop does in `launchctl`. If the process is set to be kept alive, then restart is not necessary.  Stop will work, because it will restart it as soon as it stops it.*

**Restart job with given label**

		launchctl.restart('homebrew.mxcl.redis', function(err){
			if (err) {
				console.error(err);
			} else {
				console.log('Successfully restarted job');
			}
		});

<hr>

### restartSync

Synchronous version of `restart`

<hr>

### remove

**Remove job with given label**

		launchctl.remove('homebrew.mxcl.redis', function(err) {
			if (err) {
				console.log(err);
			} else {
				console.log('Successfully removed job');
			}
		});

<hr>

### removeSync

Synchronous version of `remove`

		var res = launchctl.removeSync('homebrew.mxcl.redis');
		if (e = launchctl.error(res)) {
			console.log(e);
		} else {
			console.log('Successfully removed');
		}

<hr>

### load

**Loads a job**

**Params:**

- {String} path (required)
- {Object} opts (optional)
	- {Boolean} editondisk (default: false)
	- {Boolean} forceload (default: false)
	- {String} session_type
		- Aqua
		- Background
		- LoginWindow
		- StandardIO
		- System
	- {String} domain
		- system
		- local
		- network
		- all
		- user (default)

<hr>

### loadSync

Synchronous version of `load`

<hr>

### unload

**Unloads a job**

**Params**

- {String} path (required)
- {Object} opts (optional)
	- {Boolean} editondisk (default: false)
	- {Boolean} forceload (default: false)
	- {String} session_type
		- Aqua
		- Background
		- LoginWindow
		- StandardIO
		- System
	- {String} domain
		- system
		- local
		- network
		- all
		- user (default)

<hr>

### unloadSync

Synchronous version of `unload`

<hr>

### getManagerName

**Gets the manager/session name**

Will return one of the following:

- Aqua
- System
- Background
- LoginWindow
- StandardIO

<hr>

### getManagerUID

**Gets the uid of the current launchd manager**

<hr>

### getManagerPID

**Gets the pid of the current launchd manager**

<hr>

## exports

### errorFromCode

**Params:**

- {String} code The error code

<hr>

### strerror

**Params:**

- {Number} errno The error number

<hr>

## TODO

Make API more complete

- submit


## Contributions
- Please feel free to fork/contribute :]

 

