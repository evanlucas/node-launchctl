# node-launchctl

Provides native bindings to launchctl commands

*Please note that this module is in the earlier stages. It could change.*

## Install

		npm install launchctl

## API

		var launchctl = require('launchctl')

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

## TODO

Make API more complete

- load
- unload
- submit


## Contributions
- Please feel free to fork/contribute :]

 

