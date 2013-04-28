### node-launchctl

Provides native bindings to launchctl commands

#### Install

		npm install launchctl

#### Usage

		var ctl = require('launchctl');
		ctl.list(function(err, jobs) {
			console.log(jobs);
		});

#### Functions

- list
	- list(function(err, jobs) {})
		- Get all jobs
	- list(<JOB_LABEL>, function(err, job) {})
		- Get job with label <JOB_LABEL>
	- list(/regex/, function(err, jobs) {})
		- Get jobs matching provided regex

- listSync
	- Synchronous version of list

- startSync
	- startSync(<JOB_LABEL>)
		- Starts a job with the given label

- stopSync
	-stopSync(<JOB_LABEL>)
		- Stops a job with the given label

#### TODO

Add more functions
- load
- unload
- submit

#### Contributions
- Please feel free to fork/contribute :]

 

