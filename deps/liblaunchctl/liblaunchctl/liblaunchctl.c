//
//  liblaunchctl.m
//  liblaunchctl
//
//  Created by Evan Lucas on 5/18/13.
//  Copyright (c) 2013 Hattiesburg Clinic. All rights reserved.
//

#include <launch.h>
#include "liblaunchctl.h"
#include <CoreFoundation/CoreFoundation.h>
#include <NSSystemDirectories.h>
#ifdef __MAC_10_7
#include <fcntl.h>
#endif
#define LAUNCH_ENV_KEEPCONTEXT	"LaunchKeepContext"
#define LAUNCH_ENV_BOOTSTRAPPINGSYSTEM "LaunchBootstrappingSystem"

#define	BOOTSTRAP_MAX_LOOKUP_COUNT		20

#define	BOOTSTRAP_SUCCESS				0
#define	BOOTSTRAP_NOT_PRIVILEGED		1100
#define	BOOTSTRAP_NAME_IN_USE			1101
#define	BOOTSTRAP_UNKNOWN_SERVICE		1102
#define	BOOTSTRAP_SERVICE_ACTIVE		1103
#define	BOOTSTRAP_BAD_COUNT				1104
#define	BOOTSTRAP_NO_MEMORY				1105
#define BOOTSTRAP_NO_CHILDREN			1106

#define BOOTSTRAP_STATUS_INACTIVE		0
#define BOOTSTRAP_STATUS_ACTIVE			1
#define BOOTSTRAP_STATUS_ON_DEMAND		2

static bool _launchctl_system_bootstrap;
static bool _launchctl_peruser_bootstrap;
static bool _launchctl_overrides_db_changed = false;
static CFMutableDictionaryRef _launchctl_overrides_db = NULL;
static char *_launchctl_job_overrides_db_path;
static char *_launchctl_managername = NULL;
static bool sysctl_hw_streq(int mib_slot, const char *str);
static void limitloadtohardware_iterator(launch_data_t val, const char *key, void *ctx);
static launch_data_t read_plist_file(const char *file, bool editondisk, bool load);
static void myCFDictionaryApplyFunction(const void *key, const void *value, void *context);
static launch_data_t CF2launch_data(CFTypeRef);
static void job_disabled_dict_logic(launch_data_t obj, const char *key, void *context);
static void job_override(CFTypeRef key, CFTypeRef val, CFMutableDictionaryRef job);
static bool job_disabled_logic(launch_data_t obj);
static void do_mgroup_join(int fd, int family, int socktype, int protocol, const char *mgroup);
static int _fd(int);
static void do_application_firewall_magic(int sfd, launch_data_t thejob);
static mach_port_t str2bsport(const char *s);
CFTypeRef CFTypeCreateFromLaunchData(launch_data_t obj);
CFArrayRef CFArrayCreateFromLaunchArray(launch_data_t arr);
CFDictionaryRef CFDictionaryCreateFromLaunchDictionary(launch_data_t dict);
bool launch_data_array_append(launch_data_t a, launch_data_t o);
void insert_event(launch_data_t, const char *, const char *, launch_data_t);
void distill_jobs(launch_data_t);
void distill_config_file(launch_data_t);
void distill_fsevents(launch_data_t);
void sock_dict_cb(launch_data_t what, const char *key, void *context);
void sock_dict_edit_entry(launch_data_t tmp, const char *key, launch_data_t fdarray, launch_data_t thejob);
CFPropertyListRef CreateMyPropertyListFromFile(const char *);
CFPropertyListRef CFPropertyListCreateFromFile(CFURLRef plistURL);
void WriteMyPropertyListToFile(CFPropertyListRef, const char *);
bool path_goodness_check(const char *path, bool forceload);
void readpath(const char *, struct load_unload_state *);
void readfile(const char *, struct load_unload_state *);
int submit_job_pass(launch_data_t jobs);
bool path_check(const char *path);


kern_return_t bootstrap_parent(mach_port_t bp, mach_port_t *parent_port);

static inline Boolean _is_launch_data_t(launch_data_t obj) {
	Boolean result = true;
  
	switch (launch_data_get_type(obj)) {
		case LAUNCH_DATA_STRING		: break;
		case LAUNCH_DATA_INTEGER	: break;
		case LAUNCH_DATA_REAL		: break;
		case LAUNCH_DATA_BOOL		: break;
		case LAUNCH_DATA_ARRAY		: break;
		case LAUNCH_DATA_DICTIONARY	: break;
		case LAUNCH_DATA_FD 		: break;
		case LAUNCH_DATA_MACHPORT	: break;
		default						: result = false;
	}
  
	return result;
}
static void _launch_data_iterate(launch_data_t obj, const char *key, CFMutableDictionaryRef dict) {
	if (obj && _is_launch_data_t(obj)) {
		CFStringRef cfKey = CFStringCreateWithCString(NULL, key, kCFStringEncodingUTF8);
		CFTypeRef cfVal = CFTypeCreateFromLaunchData(obj);
    
		if (cfVal) {
			CFDictionarySetValue(dict, cfKey, cfVal);
			CFRelease(cfVal);
		}
		CFRelease(cfKey);
	}
}


launch_data_t launchctl_list_job(const char *job) {
	launch_data_t resp, msg = NULL;
	int r = 0;
  if (geteuid() == 0) {
    setup_system_context();
  }
	msg = launch_data_alloc(LAUNCH_DATA_DICTIONARY);
	launch_data_dict_insert(msg, launch_data_new_string(job), LAUNCH_KEY_GETJOB);
	resp = launch_msg(msg);
	launch_data_free(msg);
  
	if (resp == NULL) {
		fprintf(stderr, "launch_msg(): %s\n", strerror(errno));
		r = 1;
	} else if (launch_data_get_type(resp) == LAUNCH_DATA_DICTIONARY) {
		r = 0;
	} else {
    r = 1;
  }
	
	if (r == 1) {
		return NULL;
	}
  
	return resp;
}

launch_data_status_t getjob(launch_data_t job) {
  launch_data_status_t result = calloc(1, sizeof(struct ldtstatus));
  if (result == NULL) {
    fprintf(stderr, "Unable to allocate memory: %s\n", "launch_data_status_t getjob()");
    return NULL;
  }
  launch_data_t lo = launch_data_dict_lookup(job, LAUNCH_JOBKEY_LABEL);
  launch_data_t pido = launch_data_dict_lookup(job, LAUNCH_JOBKEY_PID);
  launch_data_t stato = launch_data_dict_lookup(job, LAUNCH_JOBKEY_LASTEXITSTATUS);
  result->label = strdup(launch_data_get_string(lo));
  if (pido) {
    // Running -> Has a PID
    result->pid = (int)launch_data_get_integer(pido);
    result->status = -1;
  } else if (stato) {
    // Has a last exit status
    int wstatus = (int)launch_data_get_integer(stato);
    result->pid = -1;
    if (WIFEXITED(wstatus)) {
      result->status = WEXITSTATUS(wstatus);
    } else if (WIFSIGNALED(wstatus)) {
      result->status = WTERMSIG(wstatus);
    } else {
      result->status = -1;
    }
  } else {
    // Does not have a PID or Last Exit Status
    result->pid = -1;
    result->status = -1;
  }
  
  return result;
}

void launch_data_status_free(launch_data_status_t j) {
  if (!j) {
    return;
  }
  if (j->label) {
    free(j->label);
  }
}

int launchctl_start_job(const char *job) {
  
	if (geteuid() == 0) {
		setup_system_context();
	}
	
	launch_data_t resp, msg;
  int e, r = 0;
  msg = launch_data_alloc(LAUNCH_DATA_DICTIONARY);
  launch_data_dict_insert(msg, launch_data_new_string(job), LAUNCH_KEY_STARTJOB);
  resp = launch_msg(msg);
  launch_data_free(msg);
  
  if (resp == NULL) {
    r = errno;
    return r;
  } else if (launch_data_get_type(resp) == LAUNCH_DATA_ERRNO) {
    if ((e = launch_data_get_errno(resp))) {
      r = e;
    }
  } else {
    r = -1;
  }
  launch_data_free(resp);
  return r;
}

int launchctl_stop_job(const char *job) {
  if (geteuid() == 0) {
		setup_system_context();
	}
	
	launch_data_t resp, msg;
  int e, r = 0;
  msg = launch_data_alloc(LAUNCH_DATA_DICTIONARY);
  launch_data_dict_insert(msg, launch_data_new_string(job), LAUNCH_KEY_STOPJOB);
  resp = launch_msg(msg);
  launch_data_free(msg);
  
  if (resp == NULL) {
    r = errno;
    return r;
  } else if (launch_data_get_type(resp) == LAUNCH_DATA_ERRNO) {
    if ((e = launch_data_get_errno(resp))) {
      r = e;
    }
  } else {
    r = -1;
  }
  launch_data_free(resp);
  return r;
}

int launchctl_remove_job(const char *job) {
  if (geteuid() == 0) {
		setup_system_context();
	}
	
	launch_data_t resp, msg;
  int e, r = 0;
  msg = launch_data_alloc(LAUNCH_DATA_DICTIONARY);
  launch_data_dict_insert(msg, launch_data_new_string(job), LAUNCH_KEY_REMOVEJOB);
  resp = launch_msg(msg);
  launch_data_free(msg);
  
  if (resp == NULL) {
    r = errno;
    return r;
  } else if (launch_data_get_type(resp) == LAUNCH_DATA_ERRNO) {
    if ((e = launch_data_get_errno(resp))) {
      r = e;
    }
  } else {
    r = -1;
  }
  launch_data_free(resp);
  return r;
}

int launchctl_load_job(const char *job, bool editondisk, bool forceload, const char *session_type, const char *domain) {
  if (geteuid() == 0) {
		setup_system_context();
	}
	
	NSSearchPathEnumerationState es = 0;
  char nspath[PATH_MAX * 2];
  struct load_unload_state lus;
  int res = 0;
  size_t i;
  memset(&lus, 0, sizeof(lus));
  lus.load = true;
  lus.editondisk = editondisk;
  lus.forceload = forceload;
  lus.session_type = (char *)session_type;
  if (domain == NULL) {
    es &= ~NSUserDomainMask;
  } else {
    if (strcasecmp(domain, "all") == 0) {
      es |= NSAllDomainsMask;
    } else if (strcasecmp(domain, "user") == 0) {
      es |= NSUserDomainMask;
    } else if (strcasecmp(domain, "local") == 0) {
      es |= NSLocalDomainMask;
    } else if (strcasecmp(domain, "network") == 0) {
      es |= NSNetworkDomainMask;
    } else if (strcasecmp(domain, "system") == 0) {
      es |= NSSystemDomainMask;
    } else {
      fprintf(stderr, "Invalid domain: %s\n", domain);
      return EIVALDO;
    }
  }
  int dbfd = -1;
  
  vproc_err_t verr = vproc_swap_string(NULL, VPROC_GSK_JOB_OVERRIDES_DB, NULL, &_launchctl_job_overrides_db_path);
  if (verr) {
    if (bootstrap_port) {
      fprintf(stderr, "Could not get location of job overrides database: ppid/bootstrap: %d/0x%x\n", getppid(), bootstrap_port);
    }
  } else {
    dbfd = open(_launchctl_job_overrides_db_path, O_RDONLY | O_EXLOCK | O_CREAT, S_IRUSR | S_IWUSR);
    if (dbfd != -1) {
      _launchctl_overrides_db = (CFMutableDictionaryRef)CreateMyPropertyListFromFile(_launchctl_job_overrides_db_path);
			if (!_launchctl_overrides_db) {
				_launchctl_overrides_db = CFDictionaryCreateMutable(NULL, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
			}
    } else if (errno != EROFS) {
      fprintf(stderr, "Could not open job overrides database at: %s: %d: %s\n", _launchctl_job_overrides_db_path, errno, strerror(errno));
    }
  }
  
  /* Only one pass! */
	lus.pass1 = launch_data_alloc(LAUNCH_DATA_ARRAY);
  
	es = NSStartSearchPathEnumeration(NSLibraryDirectory, es);
	while ((es = NSGetNextSearchPathEnumeration(es, nspath))) {
		glob_t g;
    
		if (lus.session_type) {
			strcat(nspath, "/LaunchAgents");
		} else {
			strcat(nspath, "/LaunchDaemons");
		}
    
		if (glob(nspath, GLOB_TILDE|GLOB_NOSORT, NULL, &g) == 0) {
			for (i = 0; i < g.gl_pathc; i++) {
				readpath(g.gl_pathv[i], &lus);
			}
			globfree(&g);
		}
	}
  
  readpath(job, &lus);
  
	if (launch_data_array_get_count(lus.pass1) == 0) {
		launch_data_free(lus.pass1);
    return EJNFOUN;
	}
  
  distill_jobs(lus.pass1);
  res = submit_job_pass(lus.pass1);
  
	if (_launchctl_overrides_db_changed) {
		WriteMyPropertyListToFile(_launchctl_overrides_db, _launchctl_job_overrides_db_path);
	}
  
	flock(dbfd, LOCK_UN);
	close(dbfd);
	return res;
  
}

int launchctl_unload_job(const char *job, bool editondisk, bool forceload, const char *session_type, const char *domain) {
  if (geteuid() == 0) {
		setup_system_context();
	}
	NSSearchPathEnumerationState es = 0;
  char nspath[PATH_MAX * 2];
  int res = 0;
  struct load_unload_state lus;
  size_t i;
  memset(&lus, 0, sizeof(lus));
  lus.load = false;
  lus.editondisk = editondisk;
  lus.forceload = forceload;
  lus.session_type = (char *)session_type;
  if (domain == NULL) {
    es &= ~NSUserDomainMask;
  } else {
    if (strcasecmp(domain, "all") == 0) {
      es |= NSAllDomainsMask;
    } else if (strcasecmp(domain, "user") == 0) {
      es |= NSUserDomainMask;
    } else if (strcasecmp(domain, "local") == 0) {
      es |= NSLocalDomainMask;
    } else if (strcasecmp(domain, "network") == 0) {
      es |= NSNetworkDomainMask;
    } else if (strcasecmp(domain, "system") == 0) {
      es |= NSSystemDomainMask;
    } else {
      fprintf(stderr, "Invalid domain: %s\n", domain);
      return EIVALDO;
    }
  }

  
  int dbfd = -1;
  
  vproc_err_t verr = vproc_swap_string(NULL, VPROC_GSK_JOB_OVERRIDES_DB, NULL, &_launchctl_job_overrides_db_path);
  if (verr) {
    if (bootstrap_port) {
      fprintf(stderr, "Could not get location of job overrides database: ppid/bootstrap: %d/0x%x\n", getppid(), bootstrap_port);
    }
  } else {
    dbfd = open(_launchctl_job_overrides_db_path, O_RDONLY | O_EXLOCK | O_CREAT, S_IRUSR | S_IWUSR);
    if (dbfd != -1) {
      _launchctl_overrides_db = (CFMutableDictionaryRef)CreateMyPropertyListFromFile(_launchctl_job_overrides_db_path);
			if (!_launchctl_overrides_db) {
				_launchctl_overrides_db = CFDictionaryCreateMutable(NULL, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
			}
    } else if (errno != EROFS) {
      fprintf(stderr, "Could not open job overrides database at: %s: %d: %s\n", _launchctl_job_overrides_db_path, errno, strerror(errno));
    }
  }
  
  /* Only one pass! */
	lus.pass1 = launch_data_alloc(LAUNCH_DATA_ARRAY);
  
	es = NSStartSearchPathEnumeration(NSLibraryDirectory, es);
  
	while ((es = NSGetNextSearchPathEnumeration(es, nspath))) {
		glob_t g;
    
		if (lus.session_type) {
			strcat(nspath, "/LaunchAgents");
		} else {
			strcat(nspath, "/LaunchDaemons");
		}
    
		if (glob(nspath, GLOB_TILDE|GLOB_NOSORT, NULL, &g) == 0) {
			for (i = 0; i < g.gl_pathc; i++) {
				readpath(g.gl_pathv[i], &lus);
			}
			globfree(&g);
		}
	}
  
  readpath(job, &lus);
  
	if (launch_data_array_get_count(lus.pass1) == 0) {
		launch_data_free(lus.pass1);
    return EJNFOUN;
	}
  

  for (i = 0; i < launch_data_array_get_count(lus.pass1); i++) {
    launch_data_t tmps;
    tmps = launch_data_dict_lookup(launch_data_array_get_index(lus.pass1, i), LAUNCH_JOBKEY_LABEL);
    if (!tmps) {
      return -1;
    }
    
    if (_vproc_send_signal_by_label(launch_data_get_string(tmps), VPROC_MAGIC_UNLOAD_SIGNAL) != NULL) {
      return ENOLOAD;
    }
  }
  
	if (_launchctl_overrides_db_changed) {
		WriteMyPropertyListToFile(_launchctl_overrides_db, _launchctl_job_overrides_db_path);
	}
  
	flock(dbfd, LOCK_UN);
	close(dbfd);
	return res;
}

char *launchctl_get_managername() {
  if (geteuid() == 0) {
		setup_system_context();
	}
	char *mgmrname = NULL;
  vproc_err_t verr = vproc_swap_string(NULL, VPROC_GSK_MGR_NAME, NULL, &mgmrname);
  if (verr) {
    return NULL;
  }
  return mgmrname;
}

int64_t launchctl_get_manageruid() {
  if (geteuid() == 0) {
		setup_system_context();
	}
  int64_t manager_uid = 0;
  vproc_err_t verr = vproc_swap_integer(NULL, VPROC_GSK_MGR_UID, NULL, (int64_t *)&manager_uid);
  if (verr) {
    fprintf(stderr, "Unknown job manager\n");
    return -1;
  }
  return manager_uid;
}

int launchctl_get_managerpid() {
  if (geteuid() == 0) {
		setup_system_context();
	}
  int64_t manager_pid = 0;
  vproc_err_t verr = vproc_swap_integer(NULL, VPROC_GSK_MGR_PID, NULL, (int64_t *)&manager_pid);
  if (verr) {
    fprintf(stderr, "Unknown job manager\n");
    return -1;
  }
  return (int)manager_pid;
}

int64_t launchctl_getumask() {
  if (geteuid() == 0) {
		setup_system_context();
	}
  int64_t outval;
  if (vproc_swap_integer(NULL, VPROC_GSK_GLOBAL_UMASK, NULL, &outval) == NULL) {
    return outval;
  } else {
    return -1;
  }
}

int launchctl_setumask(const char *mask) {
  if (geteuid() == 0) {
		setup_system_context();
	}
  char *endptr;
  long m = 0;
  int64_t inval, outval;
  int r = 0;
  m = strtol(mask, &endptr, 8);
  if (*endptr != '\0' || m > 0777) {
    return 154;
  }
  
  inval = m;
  
  if (vproc_swap_integer(NULL, VPROC_GSK_GLOBAL_UMASK, &inval, &outval) == NULL) {
    return r;
  } else {
    r = errno;
    return r;
  }
}

int launchctl_submit_job(int argc, char *const argv[]) {
  if (geteuid() == 0) {
		setup_system_context();
	}
  launch_data_t msg = launch_data_alloc(LAUNCH_DATA_DICTIONARY);
  launch_data_t job = launch_data_alloc(LAUNCH_DATA_DICTIONARY);
  launch_data_t resp, largv = launch_data_alloc(LAUNCH_DATA_ARRAY);
  
  int ch, i, r = 0;
  
  launch_data_dict_insert(job, launch_data_new_bool(false), LAUNCH_JOBKEY_LABEL);
  
  while ((ch = getopt(argc, argv, "l:p:o:e:")) != -1) {
		switch (ch) {
      case 'l':
        launch_data_dict_insert(job, launch_data_new_string(optarg), LAUNCH_JOBKEY_LABEL);
        break;
      case 'p':
        launch_data_dict_insert(job, launch_data_new_string(optarg), LAUNCH_JOBKEY_PROGRAM);
        break;
      case 'o':
        launch_data_dict_insert(job, launch_data_new_string(optarg), LAUNCH_JOBKEY_STANDARDOUTPATH);
        break;
      case 'e':
        launch_data_dict_insert(job, launch_data_new_string(optarg), LAUNCH_JOBKEY_STANDARDERRORPATH);
        break;
      default:
        // TODO: possibly leaking if we get here
        return EINVARG;
		}
	}
  
  argc -= optind;
  argv += optind;
  
  for (i=0; argv[i]; i++) {
    launch_data_array_append(largv, launch_data_new_string(argv[i]));
  }
  
  launch_data_dict_insert(job, largv, LAUNCH_JOBKEY_PROGRAMARGUMENTS);
  
  launch_data_dict_insert(msg, job, LAUNCH_KEY_SUBMITJOB);
  
  resp = launch_msg(msg);
  launch_data_free(msg);
  
  if (resp == NULL) {
    r = errno;
    return r;
  } else if (launch_data_get_type(resp) == LAUNCH_DATA_ERRNO) {
    errno = launch_data_get_errno(resp);
    if (errno) {
      r = errno;
    }
  } else {
    r = -1;
  }
  
  launch_data_free(resp);
  return r;
}


CFTypeRef CFTypeCreateFromLaunchData(launch_data_t obj) {
  CFTypeRef cfObj = NULL;
  
	switch (launch_data_get_type(obj)) {
    case LAUNCH_DATA_STRING: {
      const char *str = launch_data_get_string(obj);
      cfObj = CFStringCreateWithCString(NULL, str, kCFStringEncodingUTF8);
      break;
    }
    case LAUNCH_DATA_INTEGER: {
      long long integer = launch_data_get_integer(obj);
      cfObj = CFNumberCreate(NULL, kCFNumberLongLongType, &integer);
      break;
    }
    case LAUNCH_DATA_REAL: {
      double real = launch_data_get_real(obj);
      cfObj = CFNumberCreate(NULL, kCFNumberDoubleType, &real);
      break;
    }
    case LAUNCH_DATA_BOOL: {
      bool yesno = launch_data_get_bool(obj);
      cfObj = yesno ? kCFBooleanTrue : kCFBooleanFalse;
      break;
    }
    case LAUNCH_DATA_ARRAY: {
      cfObj = (CFTypeRef)CFArrayCreateFromLaunchArray(obj);
      break;
    }
    case LAUNCH_DATA_DICTIONARY: {
      cfObj = (CFTypeRef)CFDictionaryCreateFromLaunchDictionary(obj);
      break;
    }
    case LAUNCH_DATA_FD: {
      int fd = launch_data_get_fd(obj);
      cfObj = CFNumberCreate(NULL, kCFNumberIntType, &fd);
      break;
    }
    case LAUNCH_DATA_MACHPORT: {
      mach_port_t port = launch_data_get_machport(obj);
      cfObj = CFNumberCreate(NULL, kCFNumberIntType, &port);
      break;
    }
    default:
      break;
	}
  
	return cfObj;
}

CFArrayRef CFArrayCreateFromLaunchArray(launch_data_t arr) {
  CFArrayRef result = NULL;
	CFMutableArrayRef mutResult = CFArrayCreateMutable(NULL, 0, &kCFTypeArrayCallBacks);
  
	if (launch_data_get_type(arr) == LAUNCH_DATA_ARRAY) {
		unsigned int count = (unsigned int)launch_data_array_get_count(arr);
		unsigned int i = 0;
    
		for (i = 0; i < count; i++) {
			launch_data_t launch_obj = launch_data_array_get_index(arr, i);
			CFTypeRef obj = CFTypeCreateFromLaunchData(launch_obj);
      
			if (obj) {
				CFArrayAppendValue(mutResult, obj);
				CFRelease(obj);
			}
		}
    
		result = CFArrayCreateCopy(NULL, mutResult);
	}
  
	if (mutResult) {
		CFRelease(mutResult);
	}
	return result;
}

CFDictionaryRef CFDictionaryCreateFromLaunchDictionary(launch_data_t dict) {
  CFDictionaryRef result = NULL;
  
	if (launch_data_get_type(dict) == LAUNCH_DATA_DICTIONARY) {
		CFMutableDictionaryRef mutResult = CFDictionaryCreateMutable(NULL, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
    
		launch_data_dict_iterate(dict, (void (*)(launch_data_t, const char *, void *))_launch_data_iterate, mutResult);
    
		result = CFDictionaryCreateCopy(NULL, mutResult);
		CFRelease(mutResult);
	}
  
	return result;
}

bool path_goodness_check(const char *path, bool forceload) {
	struct stat sb;
  
	if (stat(path, &sb) == -1) {
		//fprintf(stderr, "Couldn't stat(\"%s\"): %s", path, strerror(errno));
		return false;
	}
  
	if (forceload) {
		return true;
	}
  
	if (sb.st_mode & (S_IWOTH|S_IWGRP)) {
		fprintf(stderr, "Dubious permissions on file (skipping): %s", path);
		return false;
	}
  
	if (sb.st_uid != 0 && sb.st_uid != getuid()) {
		fprintf(stderr, "Dubious ownership on file (skipping): %s", path);
		return false;
	}
  
	if (!(S_ISREG(sb.st_mode) || S_ISDIR(sb.st_mode))) {
		fprintf(stderr, "Dubious path. Not a regular file or directory (skipping): %s", path);
		return false;
	}
  
	if ((!S_ISDIR(sb.st_mode)) && (fnmatch("*.plist", path, FNM_CASEFOLD) == FNM_NOMATCH)) {
		fprintf(stderr, "Dubious file. Not of type .plist (skipping): %s", path);
		return false;
	}
  
	return true;
}

void readpath(const char *what, struct load_unload_state *lus) {
	char buf[MAXPATHLEN];
	struct stat sb;
	struct dirent *de;
	DIR *d;
  
	if (!path_goodness_check(what, lus->forceload)) {
		return;
	}
  
	if (stat(what, &sb) == -1) {
		return;
	}
  
	if (S_ISREG(sb.st_mode)) {
		readfile(what, lus);
	} else if (S_ISDIR(sb.st_mode)) {
		if ((d = opendir(what)) == NULL) {
			fprintf(stderr, "opendir() failed to open the directory");
			return;
		}
    
		while ((de = readdir(d))) {
			if (de->d_name[0] == '.') {
				continue;
			}
			snprintf(buf, sizeof(buf), "%s/%s", what, de->d_name);
      
			if (!path_goodness_check(buf, lus->forceload)) {
				continue;
			}
      
			readfile(buf, lus);
		}
		closedir(d);
	}
}

void readfile(const char *what, struct load_unload_state *lus) {
	char ourhostname[1024];
	launch_data_t tmpd, tmps, thejob, tmpa;
	bool job_disabled = false;
	size_t i, c;
  
	gethostname(ourhostname, sizeof(ourhostname));
  
	if (NULL == (thejob = read_plist_file(what, lus->editondisk, lus->load))) {
		fprintf(stderr, "no plist was returned for: %s", what);
		return;
	}
  
  
	if (NULL == launch_data_dict_lookup(thejob, LAUNCH_JOBKEY_LABEL)) {
		fprintf(stderr, "missing the Label key: %s", what);
		goto out_bad;
	}
  
	if ((launch_data_dict_lookup(thejob, LAUNCH_JOBKEY_PROGRAM) == NULL) &&
      (launch_data_dict_lookup(thejob, LAUNCH_JOBKEY_PROGRAMARGUMENTS) == NULL)) {
		fprintf(stderr, "neither a Program nor a ProgramArguments key was specified: %s", what);
		goto out_bad;
	}
  
	if (NULL != (tmpa = launch_data_dict_lookup(thejob, LAUNCH_JOBKEY_LIMITLOADFROMHOSTS))) {
		c = launch_data_array_get_count(tmpa);
    
		for (i = 0; i < c; i++) {
			launch_data_t oai = launch_data_array_get_index(tmpa, i);
			if (!strcasecmp(ourhostname, launch_data_get_string(oai))) {
				goto out_bad;
			}
		}
	}
  
	if (NULL != (tmpa = launch_data_dict_lookup(thejob, LAUNCH_JOBKEY_LIMITLOADTOHOSTS))) {
		c = launch_data_array_get_count(tmpa);
    
		for (i = 0; i < c; i++) {
			launch_data_t oai = launch_data_array_get_index(tmpa, i);
			if (!strcasecmp(ourhostname, launch_data_get_string(oai))) {
				break;
			}
		}
    
		if (i == c) {
			goto out_bad;
		}
	}
  
	if (NULL != (tmpd = launch_data_dict_lookup(thejob, LAUNCH_JOBKEY_LIMITLOADTOHARDWARE))) {
		bool result = false;
		launch_data_dict_iterate(tmpd, limitloadtohardware_iterator, &result);
		if (!result) {
			goto out_bad;
		}
	}
  
	if (NULL != (tmpd = launch_data_dict_lookup(thejob, LAUNCH_JOBKEY_LIMITLOADFROMHARDWARE))) {
		bool result = false;
		launch_data_dict_iterate(tmpd, limitloadtohardware_iterator, &result);
		if (result) {
			goto out_bad;
		}
	}
  
	/* If the manager is Aqua, the LimitLoadToSessionType should default to
	 * "Aqua".
	 *
	 * <rdar://problem/8297909>
	 */
	if (!_launchctl_managername) {
		if (vproc_swap_string(NULL, VPROC_GSK_MGR_NAME, NULL, &_launchctl_managername)) {
			if (bootstrap_port) {
				/* This is only an error if we are running with a neutered
				 * bootstrap port, otherwise we wouldn't expect this operating to
				 * succeed.
				 *
				 * <rdar://problem/10514286>
				 */
				fprintf(stderr, "Could not obtain manager name: ppid/bootstrap: %d/0x%x", getppid(), bootstrap_port);
			}
      
			_launchctl_managername = "";
		}
	}
  
	if (!lus->session_type) {
		if (strcmp(_launchctl_managername, "Aqua") == 0) {
			lus->session_type = "Aqua";
		}
	}
  
	if (lus->session_type && !(tmpa = launch_data_dict_lookup(thejob, LAUNCH_JOBKEY_LIMITLOADTOSESSIONTYPE))) {
		tmpa = launch_data_new_string("Aqua");
		launch_data_dict_insert(thejob, tmpa, LAUNCH_JOBKEY_LIMITLOADTOSESSIONTYPE);
	}
  
	if ((tmpa = launch_data_dict_lookup(thejob, LAUNCH_JOBKEY_LIMITLOADTOSESSIONTYPE))) {
		const char *allowed_session;
		bool skipjob = true;
    
		/* My sincere apologies to anyone who has to deal with this
		 * LimitLoadToSessionType madness. It was like this when I got here, but
		 * I've knowingly made it worse, hopefully to the benefit of the end
		 * user.
		 *
		 * See <rdar://problem/8769211> and <rdar://problem/7114980>.
		 */
		if (!lus->session_type && launch_data_get_type(tmpa) == LAUNCH_DATA_STRING) {
			if (strcasecmp("System", _launchctl_managername) == 0 && strcasecmp("System", launch_data_get_string(tmpa)) == 0) {
				skipjob = false;
			}
		}
    
		if (lus->session_type) switch (launch_data_get_type(tmpa)) {
      case LAUNCH_DATA_ARRAY:
        c = launch_data_array_get_count(tmpa);
        for (i = 0; i < c; i++) {
          tmps = launch_data_array_get_index(tmpa, i);
          allowed_session = launch_data_get_string(tmps);
          if (strcasecmp(lus->session_type, allowed_session) == 0) {
            skipjob = false;
            /* we have to do the following so job_reparent_hack() works within launchd */
            tmpa = launch_data_new_string(lus->session_type);
            launch_data_dict_insert(thejob, tmpa, LAUNCH_JOBKEY_LIMITLOADTOSESSIONTYPE);
            break;
          }
        }
        break;
      case LAUNCH_DATA_STRING:
        allowed_session = launch_data_get_string(tmpa);
        if (strcasecmp(lus->session_type, allowed_session) == 0) {
          skipjob = false;
        }
        break;
      default:
        break;
		}
    
		if (skipjob) {
			goto out_bad;
		}
	}
  
	if ((tmpd = launch_data_dict_lookup(thejob, LAUNCH_JOBKEY_DISABLED))) {
		job_disabled = job_disabled_logic(tmpd);
	}
  
	if (lus->forceload) {
		job_disabled = false;
	}
  
	if (job_disabled && lus->load) {
		goto out_bad;
	}
  
	if (_launchctl_system_bootstrap || _launchctl_peruser_bootstrap) {
		uuid_t uuid;
		uuid_clear(uuid);
    
		launch_data_t lduuid = launch_data_new_opaque(uuid, sizeof(uuid_t));
		launch_data_dict_insert(thejob, lduuid, LAUNCH_JOBKEY_SECURITYSESSIONUUID);
	}
  
	launch_data_array_append(lus->pass1, thejob);
  
  
	return;
out_bad:
	launch_data_free(thejob);
}

static void limitloadtohardware_iterator(launch_data_t val, const char *key, void *ctx) {
	bool *result = ctx;
  
	char name[128];
	(void)snprintf(name, sizeof(name), "hw.%s", key);
  
	int mib[2];
	size_t sz = 2;
	if (*result != true && osx_assumes_zero(sysctlnametomib(name, mib, &sz)) == 0) {
		if (launch_data_get_type(val) == LAUNCH_DATA_ARRAY) {
			size_t c = launch_data_array_get_count(val);
      
			size_t i = 0;
			for (i = 0; i < c; i++) {
				launch_data_t oai = launch_data_array_get_index(val, i);
				if (sysctl_hw_streq(mib[1], launch_data_get_string(oai))) {
					*result = true;
					i = c;
				}
			}
		}
	}
}

bool job_disabled_logic(launch_data_t obj) {
	bool r = false;
  
	switch (launch_data_get_type(obj)) {
		case LAUNCH_DATA_DICTIONARY:
			launch_data_dict_iterate(obj, job_disabled_dict_logic, &r);
			break;
		case LAUNCH_DATA_BOOL:
			r = launch_data_get_bool(obj);
			break;
		default:
			break;
	}
  
	return r;
}

void job_override(CFTypeRef key, CFTypeRef val, CFMutableDictionaryRef job) {
	if (!CFTypeCheck(key, CFString)) {
		return;
	}
	if (CFStringCompare(key, CFSTR(LAUNCH_JOBKEY_LABEL), kCFCompareCaseInsensitive) == 0) {
		return;
	}
  
	CFDictionarySetValue(job, key, val);
}

static void job_disabled_dict_logic(launch_data_t obj, const char *key, void *context) {
	bool *r = context;
  
	if (launch_data_get_type(obj) != LAUNCH_DATA_STRING) {
		return;
	}
  
	if (strcasecmp(key, LAUNCH_JOBKEY_DISABLED_MACHINETYPE) == 0) {
		if (sysctl_hw_streq(HW_MACHINE, launch_data_get_string(obj))) {
			*r = true;
		}
	} else if (strcasecmp(key, LAUNCH_JOBKEY_DISABLED_MODELNAME) == 0) {
		if (sysctl_hw_streq(HW_MODEL, launch_data_get_string(obj))) {
			*r = true;
		}
	}
}

static bool sysctl_hw_streq(int mib_slot, const char *str) {
	char buf[1000];
	size_t bufsz = sizeof(buf);
	int mib[] = { CTL_HW, mib_slot };
  
	if (sysctl(mib, 2, buf, &bufsz, NULL, 0) != -1) {
		if (strcmp(buf, str) == 0) {
			return true;
		}
	}
  
	return false;
}

void myCFDictionaryApplyFunction(const void *key, const void *value, void *context) {
	launch_data_t ik, iw, where = context;
  
	ik = CF2launch_data(key);
	iw = CF2launch_data(value);
  
	launch_data_dict_insert(where, iw, launch_data_get_string(ik));
	launch_data_free(ik);
}

launch_data_t CF2launch_data(CFTypeRef cfr) {
	launch_data_t r;
	CFTypeID cft = CFGetTypeID(cfr);
  
	if (cft == CFStringGetTypeID()) {
		char buf[4096];
		CFStringGetCString(cfr, buf, sizeof(buf), kCFStringEncodingUTF8);
		r = launch_data_alloc(LAUNCH_DATA_STRING);
		launch_data_set_string(r, buf);
	} else if (cft == CFBooleanGetTypeID()) {
		r = launch_data_alloc(LAUNCH_DATA_BOOL);
		launch_data_set_bool(r, CFBooleanGetValue(cfr));
	} else if (cft == CFArrayGetTypeID()) {
		CFIndex i, ac = CFArrayGetCount(cfr);
		r = launch_data_alloc(LAUNCH_DATA_ARRAY);
		for (i = 0; i < ac; i++) {
			CFTypeRef v = CFArrayGetValueAtIndex(cfr, i);
			if (v) {
				launch_data_t iv = CF2launch_data(v);
				launch_data_array_set_index(r, iv, i);
			}
		}
	} else if (cft == CFDictionaryGetTypeID()) {
		r = launch_data_alloc(LAUNCH_DATA_DICTIONARY);
		CFDictionaryApplyFunction(cfr, myCFDictionaryApplyFunction, r);
	} else if (cft == CFDataGetTypeID()) {
		r = launch_data_alloc(LAUNCH_DATA_OPAQUE);
		launch_data_set_opaque(r, CFDataGetBytePtr(cfr), CFDataGetLength(cfr));
	} else if (cft == CFNumberGetTypeID()) {
		long long n;
		double d;
		CFNumberType cfnt = CFNumberGetType(cfr);
		switch (cfnt) {
      case kCFNumberSInt8Type:
      case kCFNumberSInt16Type:
      case kCFNumberSInt32Type:
      case kCFNumberSInt64Type:
      case kCFNumberCharType:
      case kCFNumberShortType:
      case kCFNumberIntType:
      case kCFNumberLongType:
      case kCFNumberLongLongType:
        CFNumberGetValue(cfr, kCFNumberLongLongType, &n);
        r = launch_data_alloc(LAUNCH_DATA_INTEGER);
        launch_data_set_integer(r, n);
        break;
      case kCFNumberFloat32Type:
      case kCFNumberFloat64Type:
      case kCFNumberFloatType:
      case kCFNumberDoubleType:
        CFNumberGetValue(cfr, kCFNumberDoubleType, &d);
        r = launch_data_alloc(LAUNCH_DATA_REAL);
        launch_data_set_real(r, d);
        break;
      default:
        r = NULL;
        break;
		}
	} else {
		r = NULL;
	}
	return r;
}

CFPropertyListRef CreateMyPropertyListFromFile(const char *posixfile) {
	CFPropertyListRef propertyList;
	CFStringRef       errorString;
	CFDataRef         resourceData;
	SInt32            errorCode;
	CFURLRef          fileURL;
  
	fileURL = CFURLCreateFromFileSystemRepresentation(kCFAllocatorDefault, (const UInt8 *)posixfile, strlen(posixfile), false);
	if (!fileURL) {
		fprintf(stderr, "CFURLCreateFromFileSystemRepresentation(%s) failed", posixfile);
	}
	if (!CFURLCreateDataAndPropertiesFromResource(kCFAllocatorDefault, fileURL, &resourceData, NULL, NULL, &errorCode)) {
		fprintf(stderr, "CFURLCreateDataAndPropertiesFromResource(%s) failed: %d", posixfile, (int)errorCode);
	}
  
	propertyList = CFPropertyListCreateFromXMLData(kCFAllocatorDefault, resourceData, kCFPropertyListMutableContainersAndLeaves, &errorString);
	if (fileURL) {
		CFRelease(fileURL);
	}
  
	if (resourceData) {
		CFRelease(resourceData);
	}
  
	return propertyList;
}

void WriteMyPropertyListToFile(CFPropertyListRef plist, const char *posixfile) {
	CFDataRef	resourceData;
	CFURLRef	fileURL;
	SInt32		errorCode;
  
	fileURL = CFURLCreateFromFileSystemRepresentation(kCFAllocatorDefault, (const UInt8 *)posixfile, strlen(posixfile), false);
	if (!fileURL) {
		fprintf(stderr, "CFURLCreateFromFileSystemRepresentation(%s) failed", posixfile);
	}
	resourceData = CFPropertyListCreateXMLData(kCFAllocatorDefault, plist);
	if (resourceData == NULL) {
		fprintf(stderr, "CFPropertyListCreateXMLData(%s) failed", posixfile);
	}
	if (!CFURLWriteDataAndPropertiesToResource(fileURL, resourceData, NULL, &errorCode)) {
		fprintf(stderr, "CFURLWriteDataAndPropertiesToResource(%s) failed: %d", posixfile, (int)errorCode);
	}
  
	if (resourceData) {
		CFRelease(resourceData);
	}
    if (fileURL) {
        CFRelease(fileURL);
    }
}

bool launch_data_array_append(launch_data_t a, launch_data_t o) {
	size_t offt = launch_data_array_get_count(a);
  
	return launch_data_array_set_index(a, o, offt);
}

static launch_data_t read_plist_file(const char *file, bool editondisk, bool load) {
	CFPropertyListRef plist = CreateMyPropertyListFromFile(file);
	launch_data_t r = NULL;
  
	if (NULL == plist) {
		fprintf(stderr, "no plist was returned for: %s", file);
		return NULL;
	}
  
	CFStringRef label = CFDictionaryGetValue(plist, CFSTR(LAUNCH_JOBKEY_LABEL));
	if (!(label && CFTypeCheck(label, CFString))) {
		return NULL;
	}
  
	if (_launchctl_overrides_db) {
		CFDictionaryRef overrides = CFDictionaryGetValue(_launchctl_overrides_db, label);
		if (overrides && CFTypeCheck(overrides, CFDictionary)) {
			CFDictionaryApplyFunction(overrides, (CFDictionaryApplierFunction)job_override, (void *)plist);
		}
	}
  
	if (editondisk) {
		if (_launchctl_overrides_db) {
			CFMutableDictionaryRef job = (CFMutableDictionaryRef)CFDictionaryGetValue(_launchctl_overrides_db, label);
			if (!job || !CFTypeCheck(job, CFDictionary)) {
				job = CFDictionaryCreateMutable(NULL, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
				CFDictionarySetValue(_launchctl_overrides_db, label, job);
				CFRelease(job);
			}
      
			CFDictionarySetValue(job, CFSTR(LAUNCH_JOBKEY_DISABLED), load ? kCFBooleanFalse : kCFBooleanTrue);
			CFDictionarySetValue((CFMutableDictionaryRef)plist, CFSTR(LAUNCH_JOBKEY_DISABLED), load ? kCFBooleanFalse : kCFBooleanTrue);
			_launchctl_overrides_db_changed = true;
		} else {
			if (load) {
				CFDictionaryRemoveValue((CFMutableDictionaryRef)plist, CFSTR(LAUNCH_JOBKEY_DISABLED));
			} else {
				CFDictionarySetValue((CFMutableDictionaryRef)plist, CFSTR(LAUNCH_JOBKEY_DISABLED), kCFBooleanTrue);
			}
			WriteMyPropertyListToFile(plist, file);
		}
	}
  
	r = CF2launch_data(plist);
  
	CFRelease(plist);
  
	return r;
}

struct distill_context {
	launch_data_t base;
	launch_data_t newsockdict;
};

void distill_jobs(launch_data_t jobs) {
	size_t i, c = launch_data_array_get_count(jobs);
	launch_data_t job;
  
	for (i = 0; i < c; i++) {
		job = launch_data_array_get_index(jobs, i);
		distill_config_file(job);
		distill_fsevents(job);
	}
}

void distill_config_file(launch_data_t id_plist) {
	struct distill_context dc = { id_plist, NULL };
	launch_data_t tmp;
  
	if ((tmp = launch_data_dict_lookup(dc.base, LAUNCH_JOBKEY_SOCKETS))) {
		dc.newsockdict = launch_data_alloc(LAUNCH_DATA_DICTIONARY);
		launch_data_dict_iterate(tmp, sock_dict_cb, &dc);
		launch_data_dict_insert(dc.base, dc.newsockdict, LAUNCH_JOBKEY_SOCKETS);
	}
}

void distill_fsevents(launch_data_t id_plist) {
	launch_data_t copy, newevent;
	launch_data_t tmp, tmp2;
  
	if ((tmp = launch_data_dict_lookup(id_plist, LAUNCH_JOBKEY_QUEUEDIRECTORIES))) {
		copy = launch_data_copy(tmp);
		(void)launch_data_dict_remove(id_plist, LAUNCH_JOBKEY_QUEUEDIRECTORIES);
    
		newevent = launch_data_alloc(LAUNCH_DATA_DICTIONARY);
		launch_data_dict_insert(newevent, copy, LAUNCH_JOBKEY_QUEUEDIRECTORIES);
		insert_event(id_plist, "com.apple.fsevents.matching", "com.apple.launchd." LAUNCH_JOBKEY_QUEUEDIRECTORIES, newevent);
	}
  
	if ((tmp = launch_data_dict_lookup(id_plist, LAUNCH_JOBKEY_WATCHPATHS))) {
		copy = launch_data_copy(tmp);
		(void)launch_data_dict_remove(id_plist, LAUNCH_JOBKEY_WATCHPATHS);
    
		newevent = launch_data_alloc(LAUNCH_DATA_DICTIONARY);
		launch_data_dict_insert(newevent, copy, LAUNCH_JOBKEY_WATCHPATHS);
		insert_event(id_plist, "com.apple.fsevents.matching", "com.apple.launchd." LAUNCH_JOBKEY_WATCHPATHS, newevent);
	}
  
	if ((tmp = launch_data_dict_lookup(id_plist, LAUNCH_JOBKEY_KEEPALIVE))) {
		if ((tmp2 = launch_data_dict_lookup(tmp, LAUNCH_JOBKEY_KEEPALIVE_PATHSTATE))) {
			copy = launch_data_copy(tmp2);
			(void)launch_data_dict_remove(tmp, LAUNCH_JOBKEY_KEEPALIVE_PATHSTATE);
      
			newevent = launch_data_alloc(LAUNCH_DATA_DICTIONARY);
			launch_data_dict_insert(newevent, copy, LAUNCH_JOBKEY_KEEPALIVE_PATHSTATE);
			insert_event(id_plist, "com.apple.fsevents.matching", "com.apple.launchd." LAUNCH_JOBKEY_KEEPALIVE_PATHSTATE, newevent);
		}
	}
}

void insert_event(launch_data_t job, const char *stream, const char *key, launch_data_t event) {
	launch_data_t launchevents, streamdict;
  
	launchevents = launch_data_dict_lookup(job, LAUNCH_JOBKEY_LAUNCHEVENTS);
	if (launchevents == NULL) {
		launchevents = launch_data_alloc(LAUNCH_DATA_DICTIONARY);
		launch_data_dict_insert(job, launchevents, LAUNCH_JOBKEY_LAUNCHEVENTS);
	}
  
	streamdict = launch_data_dict_lookup(launchevents, stream);
	if (streamdict == NULL) {
		streamdict = launch_data_alloc(LAUNCH_DATA_DICTIONARY);
		launch_data_dict_insert(launchevents, streamdict, stream);
	}
  
	launch_data_dict_insert(streamdict, event, key);
}

void sock_dict_cb(launch_data_t what, const char *key, void *context) {
	struct distill_context *dc = context;
	launch_data_t fdarray = launch_data_alloc(LAUNCH_DATA_ARRAY);
  
	launch_data_dict_insert(dc->newsockdict, fdarray, key);
  
	if (launch_data_get_type(what) == LAUNCH_DATA_DICTIONARY) {
		sock_dict_edit_entry(what, key, fdarray, dc->base);
	} else if (launch_data_get_type(what) == LAUNCH_DATA_ARRAY) {
		launch_data_t tmp;
		size_t i;
    
		for (i = 0; i < launch_data_array_get_count(what); i++) {
			tmp = launch_data_array_get_index(what, i);
			sock_dict_edit_entry(tmp, key, fdarray, dc->base);
		}
	}
}

void sock_dict_edit_entry(launch_data_t tmp, const char *key, launch_data_t fdarray, launch_data_t thejob) {
	launch_data_t a, val;
	int sfd, st = SOCK_STREAM;
	bool passive = true;
  
	if ((val = launch_data_dict_lookup(tmp, LAUNCH_JOBSOCKETKEY_TYPE))) {
		if (!strcasecmp(launch_data_get_string(val), "stream")) {
			st = SOCK_STREAM;
		} else if (!strcasecmp(launch_data_get_string(val), "dgram")) {
			st = SOCK_DGRAM;
		} else if (!strcasecmp(launch_data_get_string(val), "seqpacket")) {
			st = SOCK_SEQPACKET;
		}
	}
  
	if ((val = launch_data_dict_lookup(tmp, LAUNCH_JOBSOCKETKEY_PASSIVE))) {
		passive = launch_data_get_bool(val);
	}
  
	if ((val = launch_data_dict_lookup(tmp, LAUNCH_JOBSOCKETKEY_SECUREWITHKEY))) {
		char secdir[] = LAUNCH_SECDIR, buf[1024];
		launch_data_t uenv = launch_data_dict_lookup(thejob, LAUNCH_JOBKEY_USERENVIRONMENTVARIABLES);
    
		if (NULL == uenv) {
			uenv = launch_data_alloc(LAUNCH_DATA_DICTIONARY);
			launch_data_dict_insert(thejob, uenv, LAUNCH_JOBKEY_USERENVIRONMENTVARIABLES);
		}
    
		mkdtemp(secdir);
    
		sprintf(buf, "%s/%s", secdir, key);
    
		a = launch_data_new_string(buf);
		launch_data_dict_insert(tmp, a, LAUNCH_JOBSOCKETKEY_PATHNAME);
		a = launch_data_new_string(buf);
		launch_data_dict_insert(uenv, a, launch_data_get_string(val));
	}
  
	if ((val = launch_data_dict_lookup(tmp, LAUNCH_JOBSOCKETKEY_PATHNAME))) {
		struct sockaddr_un sun;
		mode_t sun_mode = 0;
		mode_t oldmask;
		bool setm = false;
    
		memset(&sun, 0, sizeof(sun));
    
		sun.sun_family = AF_UNIX;
    
		strncpy(sun.sun_path, launch_data_get_string(val), sizeof(sun.sun_path));
    
		if (posix_assumes_zero(sfd = _fd(socket(AF_UNIX, st, 0))) == -1) {
			return;
		}
    
		if ((val = launch_data_dict_lookup(tmp, LAUNCH_JOBSOCKETKEY_PATHMODE))) {
			sun_mode = (mode_t)launch_data_get_integer(val);
			setm = true;
		}
    
		if (passive) {
			if (unlink(sun.sun_path) == -1 && errno != ENOENT) {
				close(sfd);
				return;
			}
			oldmask = umask(S_IRWXG|S_IRWXO);
			if (bind(sfd, (struct sockaddr *)&sun, (socklen_t) sizeof sun) == -1) {
				close(sfd);
				umask(oldmask);
				return;
			}
			umask(oldmask);
			if (setm) {
				chmod(sun.sun_path, sun_mode);
			}
			if ((st == SOCK_STREAM || st == SOCK_SEQPACKET) && listen(sfd, -1) == -1) {
				close(sfd);
				return;
			}
		} else if (connect(sfd, (struct sockaddr *)&sun, (socklen_t) sizeof sun) == -1) {
			close(sfd);
			return;
		}
    
		val = launch_data_new_fd(sfd);
		launch_data_array_append(fdarray, val);
	} else {
		launch_data_t rnames = NULL;
		const char *node = NULL, *serv = NULL, *mgroup = NULL;
		char servnbuf[50];
		struct addrinfo hints, *res0, *res;
		int gerr, sock_opt = 1;
    
		memset(&hints, 0, sizeof(hints));
    
		hints.ai_socktype = st;
		if (passive) {
			hints.ai_flags |= AI_PASSIVE;
		}
    
		if ((val = launch_data_dict_lookup(tmp, LAUNCH_JOBSOCKETKEY_NODENAME))) {
			node = launch_data_get_string(val);
		}
		if ((val = launch_data_dict_lookup(tmp, LAUNCH_JOBSOCKETKEY_MULTICASTGROUP))) {
			mgroup = launch_data_get_string(val);
		}
		if ((val = launch_data_dict_lookup(tmp, LAUNCH_JOBSOCKETKEY_SERVICENAME))) {
			if (LAUNCH_DATA_INTEGER == launch_data_get_type(val)) {
				sprintf(servnbuf, "%lld", launch_data_get_integer(val));
				serv = servnbuf;
			} else {
				serv = launch_data_get_string(val);
			}
		}
		if ((val = launch_data_dict_lookup(tmp, LAUNCH_JOBSOCKETKEY_FAMILY))) {
			if (!strcasecmp("IPv4", launch_data_get_string(val))) {
				hints.ai_family = AF_INET;
			} else if (!strcasecmp("IPv6", launch_data_get_string(val))) {
				hints.ai_family = AF_INET6;
			}
		}
		if ((val = launch_data_dict_lookup(tmp, LAUNCH_JOBSOCKETKEY_PROTOCOL))) {
			if (!strcasecmp("TCP", launch_data_get_string(val))) {
				hints.ai_protocol = IPPROTO_TCP;
			} else if (!strcasecmp("UDP", launch_data_get_string(val))) {
				hints.ai_protocol = IPPROTO_UDP;
			}
		}
		if ((rnames = launch_data_dict_lookup(tmp, LAUNCH_JOBSOCKETKEY_BONJOUR))) {
			if (LAUNCH_DATA_BOOL != launch_data_get_type(rnames) || launch_data_get_bool(rnames)) {
				launch_data_t newevent;
				char eventkey[100];
        
				newevent = launch_data_copy(tmp);
				snprintf(eventkey, sizeof(eventkey), "com.apple.launchd.%s", key);
				insert_event(thejob, "com.apple.bonjour.registration", eventkey, newevent);
			}
		}
    
		if ((gerr = getaddrinfo(node, serv, &hints, &res0)) != 0) {
			fprintf(stderr, "getaddrinfo(): %s", gai_strerror(gerr));
			return;
		}
    
		for (res = res0; res; res = res->ai_next) {
			if ((sfd = _fd(socket(res->ai_family, res->ai_socktype, res->ai_protocol))) == -1) {
				fprintf(stderr, "socket(): %s", strerror(errno));
				return;
			}
      
			do_application_firewall_magic(sfd, thejob);
      
			if (hints.ai_flags & AI_PASSIVE) {
				if (AF_INET6 == res->ai_family && -1 == setsockopt(sfd, IPPROTO_IPV6, IPV6_V6ONLY,
                                                           (void *)&sock_opt, (socklen_t) sizeof sock_opt)) {
					fprintf(stderr, "setsockopt(IPV6_V6ONLY): %m");
					return;
				}
				if (mgroup) {
					if (setsockopt(sfd, SOL_SOCKET, SO_REUSEPORT, (void *)&sock_opt, (socklen_t) sizeof sock_opt) == -1) {
						fprintf(stderr, "setsockopt(SO_REUSEPORT): %s", strerror(errno));
						return;
					}
				} else {
					if (setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, (void *)&sock_opt, (socklen_t) sizeof sock_opt) == -1) {
						fprintf(stderr, "setsockopt(SO_REUSEADDR): %s", strerror(errno));
						return;
					}
				}
				if (bind(sfd, res->ai_addr, res->ai_addrlen) == -1) {
					fprintf(stderr, "bind(): %s", strerror(errno));
					return;
				}
				/* The kernel may have dynamically assigned some part of the
				 * address. (The port being a common example.)
				 */
				if (getsockname(sfd, res->ai_addr, &res->ai_addrlen) == -1) {
					fprintf(stderr, "getsockname(): %s", strerror(errno));
					return;
				}
        
				if (mgroup) {
					do_mgroup_join(sfd, res->ai_family, res->ai_socktype, res->ai_protocol, mgroup);
				}
				if ((res->ai_socktype == SOCK_STREAM || res->ai_socktype == SOCK_SEQPACKET) && listen(sfd, -1) == -1) {
					fprintf(stderr, "listen(): %s", strerror(errno));
					return;
				}
			} else {
				if (connect(sfd, res->ai_addr, res->ai_addrlen) == -1) {
					fprintf(stderr, "connect(): %s", strerror(errno));
					return;
				}
			}
			val = launch_data_new_fd(sfd);
			launch_data_array_append(fdarray, val);
		}
	}
}

int _fd(int fd) {
	if (fd >= 0)
		fcntl(fd, F_SETFD, 1);
	return fd;
}

void do_application_firewall_magic(int sfd, launch_data_t thejob) {
	const char *prog = NULL, *partialprog = NULL;
	char *path, *pathtmp, **pathstmp;
	char *paths[100];
	launch_data_t tmp;
  
	/*
	 * Sigh...
	 * <rdar://problem/4684434> setsockopt() with the executable path as the argument
	 */
  
	if ((tmp = launch_data_dict_lookup(thejob, LAUNCH_JOBKEY_PROGRAM))) {
		prog = launch_data_get_string(tmp);
	}
  
	if (!prog) {
		if ((tmp = launch_data_dict_lookup(thejob, LAUNCH_JOBKEY_PROGRAMARGUMENTS))) {
			if ((tmp = launch_data_array_get_index(tmp, 0))) {
				if ((partialprog = launch_data_get_string(tmp))) {
					if (partialprog[0] == '/') {
						prog = partialprog;
					}
				}
			}
		}
	}
  
	if (!prog) {
		pathtmp = path = strdup(getenv("PATH"));
    
		pathstmp = paths;
    
		while ((*pathstmp = strsep(&pathtmp, ":"))) {
			if (**pathstmp != '\0') {
				pathstmp++;
			}
		}
    
		free(path);
		pathtmp = alloca(MAXPATHLEN);
    
		pathstmp = paths;
    
		for (; *pathstmp; pathstmp++) {
			snprintf(pathtmp, MAXPATHLEN, "%s/%s", *pathstmp, partialprog);
			if (path_check(pathtmp)) {
				prog = pathtmp;
				break;
			}
		}
	}
  
	if (prog != NULL) {
		/* The networking team has asked us to ignore the failure of this API if
		 * errno == ENOPROTOOPT.
		 */
		if (setsockopt(sfd, SOL_SOCKET, SO_EXECPATH, prog, (socklen_t)(strlen(prog) + 1)) == -1 && errno != ENOPROTOOPT) {
			(void)osx_assumes_zero(errno);
		}
	}
}

void do_mgroup_join(int fd, int family, int socktype, int protocol, const char *mgroup) {
	struct addrinfo hints, *res0, *res;
	struct ip_mreq mreq;
	struct ipv6_mreq m6req;
	int gerr;
  
	memset(&hints, 0, sizeof(hints));
  
	hints.ai_flags |= AI_PASSIVE;
	hints.ai_family = family;
	hints.ai_socktype = socktype;
	hints.ai_protocol = protocol;
  
	if ((gerr = getaddrinfo(mgroup, NULL, &hints, &res0)) != 0) {
		fprintf(stderr, "getaddrinfo(): %s", gai_strerror(gerr));
		return;
	}
  
	for (res = res0; res; res = res->ai_next) {
		if (AF_INET == family) {
			memset(&mreq, 0, sizeof(mreq));
			mreq.imr_multiaddr = ((struct sockaddr_in *)res->ai_addr)->sin_addr;
			if (setsockopt(fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, (socklen_t) sizeof mreq) == -1) {
				fprintf(stderr, "setsockopt(IP_ADD_MEMBERSHIP): %s", strerror(errno));
				continue;
			}
			break;
		} else if (AF_INET6 == family) {
			memset(&m6req, 0, sizeof(m6req));
			m6req.ipv6mr_multiaddr = ((struct sockaddr_in6 *)res->ai_addr)->sin6_addr;
			if (setsockopt(fd, IPPROTO_IPV6, IPV6_JOIN_GROUP, &m6req, (socklen_t) sizeof m6req) == -1) {
				fprintf(stderr, "setsockopt(IPV6_JOIN_GROUP): %s", strerror(errno));
				continue;
			}
			break;
		} else {
			fprintf(stderr, "unknown family during multicast group bind!");
			break;
		}
	}
  
	freeaddrinfo(res0);
}

bool path_check(const char *path) {
	struct stat sb;
  
	if (stat(path, &sb) == 0)
		return true;
	return false;
}

int submit_job_pass(launch_data_t jobs) {
	launch_data_t msg, resp;
	size_t i;
	int e = 0;
  
	if (launch_data_array_get_count(jobs) == 0)
		return -1;
  
	msg = launch_data_alloc(LAUNCH_DATA_DICTIONARY);
  
	launch_data_dict_insert(msg, jobs, LAUNCH_KEY_SUBMITJOB);
  
	resp = launch_msg(msg);
  
	if (resp) {
		switch (launch_data_get_type(resp)) {
      case LAUNCH_DATA_ERRNO:
        if ((e = launch_data_get_errno(resp)))
          return e;
          //fprintf(stderr, "%s", strerror(e));
        break;
      case LAUNCH_DATA_ARRAY:
        for (i = 0; i < launch_data_array_get_count(jobs); i++) {
          launch_data_t obatind = launch_data_array_get_index(resp, i);
//          launch_data_t jatind = launch_data_array_get_index(jobs, i);
//          const char *lab4job = launch_data_get_string(launch_data_dict_lookup(jatind, LAUNCH_JOBKEY_LABEL));
          if (LAUNCH_DATA_ERRNO == launch_data_get_type(obatind)) {
            e = launch_data_get_errno(obatind);
            switch (e) {
              case EEXIST:
                //fprintf(stderr, "%s: %s", lab4job, "Already loaded");
                errno = EALLOAD;
                e = EALLOAD;
                break;
              case ESRCH:
                //fprintf(stderr, "%s: %s", lab4job, "Not loaded");
                errno = ENOLOAD;
                e = ENOLOAD;
                break;
              case ENEEDAUTH:
                //fprintf(stderr, "%s: %s", lab4job, "Could not set security session");
                errno = ESETSEC;
                e = ESETSEC;
              default:
                //fprintf(stderr, "%s: %s", lab4job, strerror(e));
              case 0:
                break;
            }
          }
        }
        break;
      default:
        fprintf(stderr, "unknown respose from launchd!");
        return -1;
        break;
		}
		launch_data_free(resp);
	} else {
		fprintf(stderr, "launch_msg(): %s", strerror(errno));
	}
  
	launch_data_free(msg);
  return e;
}

void
setup_system_context(void)
{
	if (getenv(LAUNCHD_SOCKET_ENV)) {
		return;
	}
  
	if (getenv(LAUNCH_ENV_KEEPCONTEXT)) {
		return;
	}
  
	if (geteuid() != 0) {
		fprintf(stderr, "You must be the root user to perform this operation.");
		return;
	}
  
	/* Use the system launchd's socket. */
	setenv("__USE_SYSTEM_LAUNCHD", "1", 0);
  
	/* Put ourselves in the system launchd's bootstrap. */
	mach_port_t rootbs = str2bsport("/");
	mach_port_deallocate(mach_task_self(), bootstrap_port);
	task_set_bootstrap_port(mach_task_self(), rootbs);
	bootstrap_port = rootbs;
}

mach_port_t
str2bsport(const char *s)
{
	bool getrootbs = strcmp(s, "/") == 0;
	mach_port_t last_bport, bport = bootstrap_port;
	task_t task = mach_task_self();
	kern_return_t result;
  
	if (strcmp(s, "..") == 0 || getrootbs) {
		do {
			last_bport = bport;
			result = bootstrap_parent(last_bport, &bport);
      
			if (result == BOOTSTRAP_NOT_PRIVILEGED) {
				fprintf(stderr, "Permission denied");
				return 1;
			} else if (result != BOOTSTRAP_SUCCESS) {
				fprintf(stderr, "bootstrap_parent() %d", result);
				return 1;
			}
		} while (getrootbs && last_bport != bport);
	} else if (strcmp(s, "0") == 0 || strcmp(s, "NULL") == 0) {
		bport = MACH_PORT_NULL;
	} else {
		int pid = atoi(s);
    
		result = task_for_pid(mach_task_self(), pid, &task);
    
		if (result != KERN_SUCCESS) {
			fprintf(stderr, "task_for_pid() %s", mach_error_string(result));
			return 1;
		}
    
		result = task_get_bootstrap_port(task, &bport);
    
		if (result != KERN_SUCCESS) {
			fprintf(stderr, "Couldn't get bootstrap port: %s", mach_error_string(result));
			return 1;
		}
	}
  
	return bport;
}

ssize_t
name2num(const char *n)
{
	size_t i;
  
	for (i = 0; i < limlookupcnt; i++) {
		if (!strcmp(limlookup[i].name, n)) {
			return limlookup[i].lim;
		}
	}
	return -1;
}

const char *
num2name(int n)
{
	size_t i;
  
	for (i = 0; i < limlookupcnt; i++) {
		if (limlookup[i].lim == n)
			return limlookup[i].name;
	}
	return NULL;
}

const char *
lim2str(rlim_t val, char *buf)
{
	if (val == RLIM_INFINITY)
		strcpy(buf, "unlimited");
	else
		sprintf(buf, "%lld", val);
	return buf;
}

bool
str2lim(const char *buf, rlim_t *res)
{
	char *endptr;
	*res = strtoll(buf, &endptr, 10);
	if (!strcmp(buf, "unlimited")) {
		*res = RLIM_INFINITY;
		return false;
	} else if (*endptr == '\0') {
    return false;
	}
	return true;
}
