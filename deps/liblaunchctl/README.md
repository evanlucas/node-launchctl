## liblaunchctl

### Install

		git clone git://github.com/evanlucas/liblaunchctl.git
		cd liblaunchctl
		node-gyp configure
		node-gyp build

### Dependencies

- Xcode (including CLI tools)
- Mac OS X 10.8+
- [Node.js](http://nodejs.org)
- [node-gyp](https://github.com/TooTallNate/node-gyp)

### API

#### launch_data_t launchctl_list_job(const char *job)

Gets a single job's details

Example:

		launch_data_t job = launchctl_list_job("com.apple.Dock.agent");
		
#### jobs_list_t launchctl_list_jobs()

Gets all jobs registered with launchd

Example:

		jobs_list_t jobs = launchctl_list_jobs();
		if (jobs == NULL) {
			printf("Error listing jobs\n");
		} else {
			int count = jobs->count;
			for (int i=0; i<count; i++) {
				launch_data_status_t job = &s->jobs[i];
				printf("JOB: %s\t PID: %d\t STATUS: %d\n", job->label, job->pid, job->status);
			}
		}

#### int launchctl_start_job(const char *job);

Starts the job with the given job label

Example:

		int result = launchctl_start_job("com.test.job");

#### int launchctl_stop_job(const char *job);

Stops the job with the given label
*Keep in mind the way that launchd works.  If the job is set to be kept alive, this will simply restart it.*

Example:

		int result = launchctl_stop_job("com.test.job");

#### int launchctl_load_job(const char *path, bool editondisk, bool forceload, const char *session_type, const char *domain);

Load the job at the given path

Example:

		int result = launchctl_load_job("/Library/LaunchDaemons/com.test.job.plist", true, true, NULL, NULL);

#### int launchctl_unload_job(const char *path)

Unloads the job at the given path

Example:

		int result = launchctl_unload_job("/Library/LaunchDaemons/com.test.job.plist");

### Tests

		make
		make test

### TODO

- Better memory management
- Clean up code
- Add more options
