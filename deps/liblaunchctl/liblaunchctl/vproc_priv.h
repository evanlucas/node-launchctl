/*
 * Copyright (c) 2006 Apple Computer, Inc. All rights reserved.
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
 * Only includes the necessary function definitions
 *
 * Modified by Evan Lucas
 *
 * Last modified on 5/5/2013
 */

#include <launch.h>
#include <vproc.h>

#define VPROCMGR_SESSION_LOGINWINDOW "LoginWindow"
#define VPROCMGR_SESSION_BACKGROUND "Background"
#define VPROCMGR_SESSION_AQUA "Aqua"
#define VPROCMGR_SESSION_STANDARDIO "StandardIO"
#define VPROCMGR_SESSION_SYSTEM "System"

#define XPC_DOMAIN_TYPE_SYSTEM	 "XPCSystem"
#define XPC_DOMAIN_TYPE_PERUSER "XPCPerUser"
#define XPC_DOMAIN_TYPE_PERSESSION "XPCPerSession"
#define XPC_DOMAIN_TYPE_PERAPPLICATION "XPCPerApplication"


typedef enum {
	VPROC_GSK_ZERO,
	VPROC_GSK_LAST_EXIT_STATUS,
	VPROC_GSK_GLOBAL_ON_DEMAND,
	VPROC_GSK_MGR_UID,
	VPROC_GSK_MGR_PID,
	VPROC_GSK_IS_MANAGED,
	VPROC_GSK_MGR_NAME,
	VPROC_GSK_BASIC_KEEPALIVE,
	VPROC_GSK_START_INTERVAL,
	VPROC_GSK_IDLE_TIMEOUT,
	VPROC_GSK_EXIT_TIMEOUT,
	VPROC_GSK_ENVIRONMENT,
	VPROC_GSK_ALLJOBS,
	VPROC_GSK_GLOBAL_LOG_MASK,
	VPROC_GSK_GLOBAL_UMASK,
	VPROC_GSK_ABANDON_PROCESS_GROUP,
	VPROC_GSK_TRANSACTIONS_ENABLED,
	VPROC_GSK_WEIRD_BOOTSTRAP,
	VPROC_GSK_WAITFORDEBUGGER,
	VPROC_GSK_SECURITYSESSION,
	VPROC_GSK_SHUTDOWN_DEBUGGING,
	VPROC_GSK_VERBOSE_BOOT,
	VPROC_GSK_PERUSER_SUSPEND,
	VPROC_GSK_PERUSER_RESUME,
	VPROC_GSK_JOB_OVERRIDES_DB,
	VPROC_GSK_JOB_CACHE_DB,
	VPROC_GSK_EMBEDDEDROOTEQUIVALENT,
} vproc_gsk_t;

vproc_err_t
vproc_swap_string(vproc_t vp, vproc_gsk_t key,
                  const char *instr, char **outstr);

vproc_err_t
_vproc_send_signal_by_label(const char *label, int sig);

vproc_err_t vproc_swap_integer(vproc_t vp, vproc_gsk_t key, int64_t *inval, int64_t *outval);
const char *vproc_strerror(vproc_err_t r);