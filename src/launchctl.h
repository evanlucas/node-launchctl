/*
 * launchctl related code used from Apple
 */

/*
 * Copyright (c) 2005-2011 Apple Inc. All rights reserved.
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
 * launchctl.cc
 * Native bindings to launchctl
 * Built from code available at http://opensource.apple.com/source/launchd/launchd-442.26.2/
 * Code modified to use by Evan Lucas
 *
 */



#include <v8.h>
#include <node.h>
extern "C" {
#include <liblaunchctl.h>
#include <errno.h>
}

namespace launchctl {
  
typedef enum {
  NODE_LAUNCHCTL_CMD_START = 1,
  NODE_LAUNCHCTL_CMD_STOP,
  NODE_LAUNCHCTL_CMD_REMOVE
} node_launchctl_action_t;
  
struct GetAllJobsBaton {
  uv_work_t request;
  jobs_list_t jobs;
  int err;
  v8::Persistent<v8::Function> callback;
};

struct GetJobBaton {
  uv_work_t request;
  const char *label;
  launch_data_t resp;
  int err;
  v8::Persistent<v8::Function> callback;
};
  
struct SSRBaton {
  uv_work_t request;
  const char *label;
  launch_data_t job;
  int err;
  node_launchctl_action_t action;
  v8::Persistent<v8::Function> callback;
};

struct LoadJobBaton {
  uv_work_t request;
  char *path;
  bool editondisk;
  bool forceload;
  char *session_type;
  char *domain;
  int err;
  v8::Persistent<v8::Function> callback;
};


struct UnloadJobBaton {
  uv_work_t request;
  char *path;
  bool editondisk;
  bool forceload;
  char *session_type;
  char *domain;
  int err;
  v8::Persistent<v8::Function> callback;
};


} // namespace liblaunchctl