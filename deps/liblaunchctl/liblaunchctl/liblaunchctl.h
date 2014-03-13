/*
 * Copyright (c) 2005 Apple Computer, Inc. All rights reserved.
 *
 * @APPLE_APACHE_LICENSE_HEADER_START@
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * @APPLE_APACHE_LICENSE_HEADER_END@
 */

/*
 * Based on the launchd program
 *
 * Most of this comes from launchctl
 * The original source can be found at
 * http://opensource.apple.com/source/launchd/launchd-442.26.2/
 *
 * Modified By Evan Lucas
 *
 * Last modified on 5/5/2013
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/sysctl.h>
#include <sys/socket.h>
#include <launch.h>
#include "launch_priv.h"
#include <vproc.h>
#include "vproc_priv.h"
#include <net/if.h>
#include <netinet/in.h>
#include <netinet/in_var.h>
#include <netdb.h>
#include <sys/un.h>
#include <CoreFoundation/CoreFoundation.h>
#include <dirent.h>
#include <fnmatch.h>
#include <glob.h>
#include <pwd.h>
#include <sys/syslimits.h>
#include "assumes.h"
#include <errno.h>

#define EALLOAD 144 // Job already loaded
#define ENOLOAD 145 // Job not loaded
#define ESETSEC 146 // Unable to set security session
#define ENOUNLO 147 // Job not unloaded
#define EIVALDO 148 // Invalid domain
#define EJNFOUN 149 // Job not found
#define EINCMD 150 // Invalid command
#define EINVARG 151 // Invalid arguments

struct _launch_data {
  uint64_t type;
  union {
    struct {
      union {
        launch_data_t *_array;
        char *string;
        void *opaque;
        int64_t __junk;
      };
      union {
        uint64_t _array_cnt;
        uint64_t string_len;
        uint64_t opaque_size;
      };
    };
    int64_t fd;
    uint64_t  mp;
    uint64_t err;
    int64_t number;
    uint64_t boolean; /* We'd use 'bool' but this struct needs to be used under Rosetta, and sizeof(bool) is different between PowerPC and Intel */
    double float_num;
  };
};

vproc_err_t vproc_swap_complex(vproc_t vp, vproc_gsk_t key, launch_data_t inval, launch_data_t *outval);



struct ldtstatus {
  char *label;
  int pid;
  int status;
};

typedef struct ldtstatus *launch_data_status_t;


struct load_unload_state {
	launch_data_t pass1;
	char *session_type;
	bool editondisk:1, load:1, forceload:1;
};

#define CFTypeCheck(cf, type) (CFGetTypeID(cf) == type ## GetTypeID())
#define CFReleaseIfNotNULL(cf) if (cf) CFRelease(cf);
#define VPROC_MAGIC_UNLOAD_SIGNAL 0x4141504C
#define LAUNCH_SECDIR _PATH_TMP "launch-XXXXXX"
#define	SO_EXECPATH	0x1085

#pragma mark Public Functions

/*!
 @function launchctl_list_job
 @discussion Lists the job with the given job label
 @param job
  The job label (ex. com.apple.Dock.agent)
 @return launch_data_t
 */
launch_data_t launchctl_list_job(const char *job);
void launch_data_status_free(launch_data_status_t j);
int launchctl_start_job(const char *job);
int launchctl_stop_job(const char *job);
int launchctl_remove_job(const char *job);
int launchctl_load_job(const char *job, bool editondisk, bool forceload, const char *session_type, const char *domain);
int launchctl_unload_job(const char *job, bool editondisk, bool forceload, const char *session_type, const char *domain);
char *launchctl_get_managername();
int launchctl_get_managerpid();
int64_t launchctl_get_manageruid();
char *launchctl_get_session();
int launchctl_submit_job(int argc, char *const argv[]);
int64_t launchctl_getumask();
int launchctl_setumask(const char *mask);
void setup_system_context(void);
static const struct {
	const char *name;
	int lim;
} limlookup[] = {
	{ "cpu",	RLIMIT_CPU },
	{ "filesize",	RLIMIT_FSIZE },
	{ "data",	RLIMIT_DATA },
	{ "stack",	RLIMIT_STACK },
	{ "core",	RLIMIT_CORE },
	{ "rss", 	RLIMIT_RSS },
	{ "memlock",	RLIMIT_MEMLOCK },
	{ "maxproc",	RLIMIT_NPROC },
	{ "maxfiles",	RLIMIT_NOFILE }
};

static const size_t limlookupcnt = sizeof limlookup / sizeof limlookup[0];

ssize_t name2num(const char *n);
const char *num2name(int n);
const char *lim2str(rlim_t val, char *buf);
bool str2lim(const char *buf, rlim_t *res);
