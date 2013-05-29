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
 * Also uses similar error handling as in \/ 
 * https://github.com/joyent/node/blob/master/src/node_file.cc
 *
 */

// Copyright Joyent, Inc. and other Node contributors.
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to permit
// persons to whom the Software is furnished to do so, subject to the
// following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN
// NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
// DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
// OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
// USE OR OTHER DEALINGS IN THE SOFTWARE.

/*
 * launchctl.cc
 * Native bindings to launchctl
 * Built from code available at http://opensource.apple.com/source/launchd/launchd-442.26.2/
 * Code modified to use by Evan Lucas
 * 
 */

#include <v8.h>
#include <node.h>
#include <launch.h>
#include <vproc.h>
#include <NSSystemDirectories.h>
extern "C" {
#include <liblaunchctl.h>
#include <errno.h>
}
using namespace node;
using namespace v8;

// Taken from https://github.com/joyent/node/blob/master/src/node_file.cc
#define TYPE_ERROR(msg) \
ThrowException(Exception::TypeError(String::New(msg)));

#define THROW_BAD_ARGS TYPE_ERROR("Invalid arguments");


#define N_STRING(x) String::New(x)
#define N_NUMBER(x) Number::New(x)
#define N_NULL Local<Value>::New(Null())

extern "C" {
  
  struct GetAllJobsBaton {
    uv_work_t request;
    jobs_list_t jobs;
    int err;
    Persistent<Function> callback;
  };
  
  struct GetJobBaton {
    uv_work_t request;
    const char *label;
    launch_data_t resp;
    int err;
    Persistent<Function> callback;
  };
  
  typedef enum {
    NODE_LAUNCHCTL_CMD_START = 1,
    NODE_LAUNCHCTL_CMD_STOP,
    NODE_LAUNCHCTL_CMD_REMOVE
  } node_launchctl_action_t;
  
  struct SSRBaton {
    uv_work_t request;
    const char *label;
    launch_data_t job;
    int err;
    node_launchctl_action_t action;
    Persistent<Function> callback;
  };
  
  struct LoadJobBaton {
    uv_work_t request;
    char *path;
    bool editondisk;
    bool forceload;
    char *session_type;
    char *domain;
    int err;
    Persistent<Function> callback;
  };
  
  struct UnloadJobBaton {
    uv_work_t request;
    char *path;
    bool editondisk;
    bool forceload;
    char *session_type;
    char *domain;
    int err;
    Persistent<Function> callback;
  };
}

static Persistent<String> errno_symbol;
static Persistent<String> code_symbol;
static Persistent<String> errmsg_symbol;


// Taken from https://github.com/joyent/node/blob/master/src/node.cc
// hack alert! copy of ErrnoException, tuned for launchctl errors
Local<Value> LaunchDException(int errorno, const char *code, const char *msg) {
  if (errno_symbol.IsEmpty()) {
    errno_symbol = NODE_PSYMBOL("errno");
    code_symbol = NODE_PSYMBOL("code");
    errmsg_symbol = NODE_PSYMBOL("msg");
  }
  
  if (!msg || !msg[0]) {
    msg = strerror(errorno);
  }
  
  Local<String> estring = String::NewSymbol(strerror(errorno));
  Local<String> message = String::NewSymbol(msg);
  Local<String> cons1 = String::Concat(estring, String::NewSymbol(", "));
  Local<String> cons2 = String::Concat(cons1, message);
  
  Local<Value> e = Exception::Error(cons2);
  
  Local<Object> obj = e->ToObject();
  
  obj->Set(errno_symbol, Integer::New(errorno));
  obj->Set(code_symbol, estring);
  obj->Set(errmsg_symbol, message);
  return e;
}

Local<Value> GetJobDetail(launch_data_t obj, const char *key) {
  size_t i, c;
  if (obj == NULL) {
    return N_NULL;
  }
  switch (launch_data_get_type(obj)) {
	  case LAUNCH_DATA_STRING:
	  {
      Local<Value> y = N_STRING(launch_data_get_string(obj));
      return y;
	  }
	    break;
	  case LAUNCH_DATA_INTEGER:
	  {
      Local<Value> y = N_NUMBER(launch_data_get_integer(obj));
      return y;
	  }
	    break;
	  case LAUNCH_DATA_REAL:
	  {
      Local<Value> y = N_NUMBER(launch_data_get_real(obj));
      return y;
	  }
      break;
	  case LAUNCH_DATA_BOOL:
	  {
      Local<Value> y = launch_data_get_bool(obj) ? N_NUMBER(1) : N_NUMBER(0);
      return y;
	  }
      break;
	  case LAUNCH_DATA_ARRAY:
	  {	      
      c = launch_data_array_get_count(obj);
      Local<Array> a = Array::New(c);
      for (i=0; i<c; i++) {
        Local<Value> y = GetJobDetail(launch_data_array_get_index(obj, i), NULL);
        a->Set(N_NUMBER(i), y);
      }
      return a;
	  }
      break;
	  case LAUNCH_DATA_DICTIONARY:
	  {
      size_t count = obj->_array_cnt;
      Local<Object> q = Object::New();
      for (i=0; i<count; i +=2) {
        launch_data_t d = obj->_array[i+1];
        const char *t = obj->_array[i]->string;
        Local<Value> v = GetJobDetail(d, t);
        q->Set(N_STRING(t), v);
      }
      return q;
	  }
      break;
	  case LAUNCH_DATA_FD:
	  {
      Local<Value> s = N_STRING("file-descriptor-object");
      return s;
	  }
      break;
	  case LAUNCH_DATA_MACHPORT:
	  {
      Local<Value> s = N_STRING("mach-port-object");
      return s;
	  }
      break;
	  default:
      printf("Unknown type\n");
      return N_NUMBER(0);
      break;
  }
}

// Gets a single job matching job label
Handle<Value> GetJobSync(const Arguments& args) {
  HandleScope scope;
  launch_data_t result = NULL;
  if (args.Length() != 1) {
    return THROW_BAD_ARGS;
  }
  
  if (!args[0]->IsString()) {
    return TYPE_ERROR("Job label must be a string");
  }
  
  String::Utf8Value job(args[0]);
  
  const char* label = *job;
  result = launchctl_list_job(label);
  if (result == NULL) {
    Local<Value> e = LaunchDException(errno, strerror(errno), NULL);
    return ThrowException(e);
  }
  Local<Value> res = GetJobDetail(result, NULL);
  if (result)
    launch_data_free(result);
  return scope.Close(res);
}

// Get Job Worker
void GetJobWork(uv_work_t* req) {
  GetJobBaton *baton = static_cast<GetJobBaton *>(req->data);
  baton->resp = launchctl_list_job(baton->label);
}

// Get Job Callback
void GetJobAfterWork(uv_work_t *req) {
  HandleScope scope;
  GetJobBaton *baton = static_cast<GetJobBaton *>(req->data);
  if (baton->resp == NULL) {
    baton->err = errno;
  }
  if (!baton->err) {
    Local<Value> res = GetJobDetail(baton->resp, NULL);
    if (res == N_NULL) {
      // No such process
      Local<Value> s = LaunchDException(3, strerror(3), NULL);
      Handle<Value> argv[1] = {
        s
      };
      TryCatch try_catch;
      baton->callback->Call(Context::GetCurrent()->Global(), 2, argv);
      if (try_catch.HasCaught()) {
        node::FatalException(try_catch);
      }
    } else {
      Handle<Value> argv[2] = {
        N_NULL,
        res
      };
      if (baton->resp)
        launch_data_free(baton->resp);
      TryCatch try_catch;
      baton->callback->Call(Context::GetCurrent()->Global(), 2, argv);
      if (try_catch.HasCaught()) {
        node::FatalException(try_catch);
      }
    }
  } else {
    Local<Value> s = LaunchDException(baton->err, strerror(baton->err), NULL);
    Handle<Value> argv[1] = {
      s
    };
    if (baton->resp)
      launch_data_free(baton->resp);
    TryCatch try_catch;
    baton->callback->Call(Context::GetCurrent()->Global(), 1, argv);
    if (try_catch.HasCaught()) {
      node::FatalException(try_catch);
    }
  }
  delete req;
}

// Get Job by name
Handle<Value> GetJob(const Arguments& args) {
  HandleScope scope;
  if (args.Length() != 2) {
    return ThrowException(Exception::Error(N_STRING("Invalid args")));
  }
  
  if (!args[0]->IsString()) {
    return ThrowException(Exception::TypeError(N_STRING("Job must be a string")));
  }
  
  if (!args[1]->IsFunction()) {
    return ThrowException(Exception::TypeError(N_STRING("Callback must be a function")));
  }
  
  String::Utf8Value job(args[0]);  
  const char* label = *job;
  
  GetJobBaton *baton = new GetJobBaton;
  
  baton->request.data = baton;
  baton->label = label;
  baton->err = 0;
  baton->resp = NULL;
  baton->callback = Persistent<Function>::New(Local<Function>::Cast(args[1]));
  uv_queue_work(uv_default_loop(), &baton->request, GetJobWork, (uv_after_work_cb)GetJobAfterWork);
  
  return Undefined();
}


// Gets all jobs
Handle<Value> GetAllJobsSync(const Arguments& args) {
  HandleScope scope;
  jobs_list_t s = launchctl_list_jobs();
  if (s == NULL) {
    Local<Value> e = LaunchDException(errno, strerror(errno), "Launchctl returned no jobs");
    return ThrowException(e);
  }
  int count = s->count;
  Handle<Array> output = Array::New(count);
  for (int i=0; i<count; i++) {
    launch_data_status_t job = &s->jobs[i];
    Handle<Object> o = Object::New();
    o->Set(N_STRING("label"), N_STRING(job->label));
    int pid = job->pid;
    if (pid == -1) {
      o->Set(N_STRING("pid"), N_STRING("-"));
    } else {
      o->Set(N_STRING("pid"), N_NUMBER(pid));
    }
    int status = job->status;
    if (status == -1) {
      o->Set(N_STRING("status"), N_STRING("-"));
    } else {
      o->Set(N_STRING("status"), N_NUMBER(status));
    }
    output->Set(N_NUMBER(i), o);
  }
  return scope.Close(output);
}

// Get All Jobs Worker
void GetAllJobsWork(uv_work_t* req) {
  GetAllJobsBaton *baton = static_cast<GetAllJobsBaton *>(req->data);
  baton->jobs = launchctl_list_jobs();
}

// Get All Jobs Callback
void GetAllJobsAfterWork(uv_work_t* req) {
  GetAllJobsBaton *baton = static_cast<GetAllJobsBaton *>(req->data);
  jobs_list_t jobs = baton->jobs;
  if (jobs == NULL) {
    baton->err = errno;
  }
  if (!baton->err) {
    int count = jobs->count;
    Local<Array> output = Array::New(count);
    for (int i=0; i<count; i++) {
      launch_data_status_t job = &jobs->jobs[i];
      Handle<Object> o = Object::New();
      o->Set(N_STRING("label"), N_STRING(job->label));
      int pid = job->pid;
      if (pid == -1) {
        o->Set(N_STRING("pid"), N_STRING("-"));
      } else {
        o->Set(N_STRING("pid"), N_NUMBER(pid));
      }
      int status = job->status;
      if (status == -1) {
        o->Set(N_STRING("status"), N_STRING("-"));
      } else {
        o->Set(N_STRING("status"), N_NUMBER(status));
      }
      output->Set(N_NUMBER(i), o);
    }
    Local<Value> argv[2] = {
      N_NULL,
      output
    };
    launch_data_status_free(jobs->jobs);
    jobs_list_free(jobs);
    TryCatch try_catch;
    baton->callback->Call(Context::GetCurrent()->Global(), 2, argv);
    if (try_catch.HasCaught()) {
      node::FatalException(try_catch);
    }
  } else {
    Local<Value> e = LaunchDException(baton->err, strerror(baton->err), NULL);
    Handle<Value> argv[1] = {
      e
    };
    jobs_list_free(jobs);
    TryCatch try_catch;
    baton->callback->Call(Context::GetCurrent()->Global(), 1, argv);
    if (try_catch.HasCaught()) {
      node::FatalException(try_catch);
    }
  }
  delete req;
}

// Get all jobs
Handle<Value> GetAllJobs(const Arguments& args) {
  HandleScope scope;
  if (args.Length() != 1) {
    return THROW_BAD_ARGS;
  }
  
  if (!args[0]->IsFunction()) {
    return TYPE_ERROR("Callback must be a function");
  }
  
  GetAllJobsBaton *baton = new GetAllJobsBaton;
  baton->request.data = baton;
  baton->err = 0;
  baton->callback = Persistent<Function>::New(Local<Function>::Cast(args[0]));
  
  uv_queue_work(uv_default_loop(), &baton->request, GetAllJobsWork, (uv_after_work_cb)GetAllJobsAfterWork);
  
  return Undefined();
}

Handle<Value> GetManagerName(const Arguments& args) {
  HandleScope scope;
  char * mgmr = launchctl_get_managername();
  if (mgmr == NULL) {
    Local<Value> e = LaunchDException(errno, strerror(errno), "Unable to get manager name");
    return ThrowException(e);
  }
  Local<Value> r = N_STRING(mgmr);
  free(mgmr);
  return scope.Close(r);
}

Handle<Value> GetManagerUID(const Arguments& args) {
  HandleScope scope;
  int u = launchctl_get_manageruid();
  if (!u) {
    Local<Value> e = LaunchDException(errno, strerror(errno), "Unable to get manager uid");
    return ThrowException(e);
  }
  Local<Value> r = N_NUMBER(u);
  return scope.Close(r);
}

Handle<Value> GetManagerPID(const Arguments& args) {
  HandleScope scope;
  int p = launchctl_get_managerpid();
  if (!p) {
    Local<Value> e = LaunchDException(errno, strerror(errno), "Unable to get manager uid");
    return ThrowException(e);
  }
  Local<Value> r = N_NUMBER(p);
  return scope.Close(r);
}

Handle<Value> GetLastError(const Arguments& args) {
  HandleScope scope;
  Local<Value> s;
  if (errno == 9) {
    s = LaunchDException(errno, strerror(errno), "No such process");
  } else {
    s = LaunchDException(errno, strerror(errno), NULL);
  }
  return scope.Close(s);
}

Handle<Value> StartStopRemoveSync(const Arguments& args) {
  HandleScope scope;
  if (args.Length() != 2) {
    return THROW_BAD_ARGS;
  }
  
  if (!args[0]->IsString()) {
    return TYPE_ERROR("Job label must be a string");
  }
  
  if (!args[1]->IsInt32()) {
    return TYPE_ERROR("Command must be an integer");
  }
  
  String::Utf8Value job(args[0]);
  
  const char* label = *job;
  
  node_launchctl_action_t cmd_v = (node_launchctl_action_t)args[1]->Int32Value();
  int result = 0;
  switch (cmd_v) {
    case NODE_LAUNCHCTL_CMD_START:
      result = launchctl_start_job(label);
      break;
    case NODE_LAUNCHCTL_CMD_STOP:
      result = launchctl_stop_job(label);
      break;
    case NODE_LAUNCHCTL_CMD_REMOVE:
      result = launchctl_remove_job(label);
      break;
    default:
      return ThrowException(LaunchDException(146, "EINCMD", "Invalid command"));
      break;
  }
  return scope.Close(N_NUMBER(result));
}

void StartStopRemoveWork(uv_work_t *req) {
  SSRBaton *baton = static_cast<SSRBaton *>(req->data);
  int result = 0;
  switch (baton->action) {
    case NODE_LAUNCHCTL_CMD_START:
      result = launchctl_start_job(baton->label);
      break;
    case NODE_LAUNCHCTL_CMD_STOP:
      result = launchctl_stop_job(baton->label);
      break;
    case NODE_LAUNCHCTL_CMD_REMOVE:
      result = launchctl_remove_job(baton->label);
      break;
    default:
      break;
  }
  if (result != 0) {
    // Failed for some reason
    baton->err = result;
  } else {
    // If we are starting, start getting list_job...
    if (baton->action == NODE_LAUNCHCTL_CMD_START) {
      baton->job = launchctl_list_job(baton->label);
      if (baton->job == NULL) {
        // Error
        baton->err = errno;
      }
    }
  }
}

void StartStopRemoveAfterWork(uv_work_t *req) {
  HandleScope scope;
  SSRBaton *baton = static_cast<SSRBaton *>(req->data);
  
  if (!baton->err) {
    if (baton->action == NODE_LAUNCHCTL_CMD_START) {
      Local<Value> res = GetJobDetail(baton->job, NULL);
      Handle<Value> argv[2] = {
        N_NULL,
        res
      };
      launch_data_free(baton->job);
      TryCatch try_catch;
      baton->callback->Call(Context::GetCurrent()->Global(), 2, argv);
      if (try_catch.HasCaught()) {
        node::FatalException(try_catch);
      }
    } else {
      Handle<Value> argv[2] = {
        N_NULL,
        N_NUMBER(1)
      };
      TryCatch try_catch;
      baton->callback->Call(Context::GetCurrent()->Global(), 2, argv);
      if (try_catch.HasCaught()) {
        node::FatalException(try_catch);
      }
    }
  } else {
    // We have an error
    Local<Value> s;
    if (baton->err == 9) {
      // Bad file descriptor
      // Typically throws No such process
      s = LaunchDException(baton->err, strerror(baton->err), "No such process");
    } else {
      s = LaunchDException(baton->err, strerror(baton->err), NULL);
    }
    Handle<Value> argv[1] = {
      s
    };

    TryCatch try_catch;
    baton->callback->Call(Context::GetCurrent()->Global(), 1, argv);
    if (try_catch.HasCaught()) {
      node::FatalException(try_catch);
    }
  }
  
  delete req;

}

Handle<Value> StartStopRemove(const Arguments& args) {
  HandleScope scope;
  if (args.Length() != 3) {
    return THROW_BAD_ARGS;
  }
  
  if (!args[0]->IsString()) {
    return TYPE_ERROR("Job label must be a string");
  }
  
  if (!args[1]->IsInt32()) {
    return TYPE_ERROR("Command must be an integer");
  }
  
  if (!args[2]->IsFunction()) {
    return TYPE_ERROR("Callback must be a function");
  }
  
  String::Utf8Value job(args[0]);
  
  const char* label = *job;
  
  node_launchctl_action_t cmd_v = (node_launchctl_action_t)args[1]->Int32Value();
  
  SSRBaton *baton = new SSRBaton;
  baton->request.data = baton;
  baton->label = label;
  baton->action = cmd_v;
  baton->err = 0;
  baton->callback = Persistent<Function>::New(Local<Function>::Cast(args[2]));
  uv_queue_work(uv_default_loop(), &baton->request, StartStopRemoveWork, (uv_after_work_cb)StartStopRemoveAfterWork);
  return Undefined();
}

Handle<Value> LoadJobSync(const Arguments& args) {
  HandleScope scope;
  // Job, editondisk, forceload, session_type, domain
  if (args.Length() < 3) {
    return THROW_BAD_ARGS;
  }
  
  if (!args[0]->IsString()) {
    return TYPE_ERROR("Job path must be a string");
  }
  
  String::Utf8Value job(args[0]);
  const char *jobpath = *job;
  
  if (!args[1]->IsBoolean()) {
    return TYPE_ERROR("Edit On Disk must be a bool");
  }
  
  bool editondisk = (args[1]->ToBoolean() == True()) ? true : false;
  if (!args[2]->IsBoolean()) {
    return TYPE_ERROR("Force Load must be a bool");
  }
  
  bool forceload = (args[2]->ToBoolean() == True()) ? true : false;
  
  const char *session_type = NULL;
  const char *domain = NULL;
  
  if (args.Length() == 4 || args.Length() == 5) {
    if (!args[3]->IsString() || !args[3]->IsNull()) {
      return TYPE_ERROR("Session type must be a string");
    } else {
      String::Utf8Value sesstype(args[3]);
      session_type = *sesstype;
    }
  }
  
  if (args.Length() == 5) {
    if (!args[4]->IsString()) {
      return TYPE_ERROR("Domain must be a string");
    } else {
      String::Utf8Value dm(args[4]);
      domain = *dm;
    }
  }
  
  int result = launchctl_load_job(jobpath, editondisk, forceload, session_type, domain);
  if (result != 0) {
    return ThrowException(LaunchDException(errno, strerror(errno), NULL));
  }
  return scope.Close(N_NUMBER(result));
}

void LoadJobWorker(uv_work_t *req) {
  LoadJobBaton *baton = static_cast<LoadJobBaton *>(req->data);
  int res = launchctl_load_job(baton->path, baton->editondisk, baton->forceload, baton->session_type, baton->domain);
  printf("Result: %d\n", res);
  baton->err = res;
}

void LoadJobAfterWork(uv_work_t *req) {
  LoadJobBaton *baton = static_cast<LoadJobBaton *>(req->data);
  if (baton->err == 0) {
    printf("No error occurred\n");
    // Success
    // TODO:
    // Since we are loading the job, lets see if we can (at this point) get the job's details :]
    // This will take reading the plist and getting the label
    Handle<Value> argv[2] = {
      N_NULL,
      N_NUMBER(0)
    };
    TryCatch try_catch;
    baton->callback->Call(Context::GetCurrent()->Global(), 2, argv);
    if (try_catch.HasCaught()) {
      node::FatalException(try_catch);
    }
  } else {
    printf("Error did occur\n");
    printf("Error: %d, %s\n", baton->err, strerror(baton->err));
    // Some kind of error
    Local<Value> e;
    if (baton->err == 17) {
      e = LaunchDException(baton->err, strerror(baton->err), "Job already loaded");
    } else {
      e = LaunchDException(baton->err, strerror(baton->err), NULL);
    }
    Handle<Value> argv[1] = {
      e
    };
    TryCatch try_catch;
    baton->callback->Call(Context::GetCurrent()->Global(), 1, argv);
    if (try_catch.HasCaught()) {
      node::FatalException(try_catch);
    }
  }
}

Handle<Value> LoadJob(const Arguments& args) {
  HandleScope scope;
  // Job, editondisk, forceload, session_type, domain
  if (args.Length() < 4) {
    return THROW_BAD_ARGS;
  }
  
  if (!args[0]->IsString()) {
    return TYPE_ERROR("Job path must be a string");
  }
  
  String::Utf8Value job(args[0]);
  char *jobpath = *job;
  
  if (!args[1]->IsBoolean()) {
    return TYPE_ERROR("Edit On Disk must be a bool");
  }
  
  bool editondisk = (args[1]->ToBoolean() == True()) ? true : false;
  if (!args[2]->IsBoolean()) {
    return TYPE_ERROR("Force Load must be a bool");
  }
  
  bool forceload = (args[2]->ToBoolean() == True()) ? true : false;
  
  char *session_type = NULL;
  char *domain = NULL;
  
  LoadJobBaton *baton = new LoadJobBaton;
  baton->request.data = baton;
  baton->path = strdup(jobpath);
  baton->editondisk = editondisk;
  baton->forceload = forceload;
  
  if (!args[args.Length()-1]->IsFunction()) {
    return TYPE_ERROR("Callback must be a function");
  }
  baton->callback = Persistent<Function>::New(Local<Function>::Cast(args[args.Length()-1]));
  if (args.Length() == 4) {
    baton->session_type = NULL;
    baton->domain = NULL;
  } else if (args.Length() == 5) {
    if (!args[3]->IsString()) {
      return TYPE_ERROR("Session type must be a string");
    } else {
      String::Utf8Value sesstype(args[3]);
      session_type = *sesstype;
      baton->session_type = strdup(session_type);
    }
  } else if (args.Length() == 6) {
    if (!args[3]->IsString()) {
      return TYPE_ERROR("Session type must be a string");
    } else {
      String::Utf8Value sesstype(args[3]);
      session_type = *sesstype;
      baton->session_type = strdup(session_type);
    }
    
    if (!args[4]->IsString()) {
      return TYPE_ERROR("Domain must be a string");
    } else {
      String::Utf8Value dm(args[4]);
      domain = *dm;
      baton->domain = strdup(domain);
    }
  }
  
  uv_queue_work(uv_default_loop(), &baton->request, LoadJobWorker, (uv_after_work_cb)LoadJobAfterWork);
  return Undefined();
}

Handle<Value> UnloadJobSync(const Arguments& args) {
  HandleScope scope;
  // Job, editondisk, forceload, session_type, domain
  if (args.Length() < 3) {
    return THROW_BAD_ARGS;
  }
  
  if (!args[0]->IsString()) {
    return TYPE_ERROR("Job path must be a string");
  }
  
  String::Utf8Value job(args[0]);
  const char *jobpath = *job;
  
  if (!args[1]->IsBoolean()) {
    return TYPE_ERROR("Edit On Disk must be a bool");
  }
  
  bool editondisk = (args[1]->ToBoolean() == True()) ? true : false;
  if (!args[2]->IsBoolean()) {
    return TYPE_ERROR("Force Load must be a bool");
  }
  
  bool forceload = (args[2]->ToBoolean() == True()) ? true : false;
  
  const char *session_type = NULL;
  const char *domain = NULL;
  
  if (args.Length() == 4 || args.Length() == 5) {
    if (!args[3]->IsString() || !args[3]->IsNull()) {
      return TYPE_ERROR("Session type must be a string");
    } else {
      String::Utf8Value sesstype(args[3]);
      session_type = *sesstype;
    }
  }
  
  if (args.Length() == 5) {
    if (!args[4]->IsString()) {
      return TYPE_ERROR("Domain must be a string");
    } else {
      String::Utf8Value dm(args[4]);
      domain = *dm;
    }
  }
  
  int result = launchctl_unload_job(jobpath, editondisk, forceload, session_type, domain);
  if (result != 0) {
    return ThrowException(LaunchDException(errno, strerror(errno), NULL));
  }
  return scope.Close(N_NUMBER(result));}

//
// TODO
// Add asynchronous version of unloadJob
//

void init(Handle<Object> target) {
  NODE_SET_METHOD(target, "getJob", GetJob);
  NODE_SET_METHOD(target, "getJobSync", GetJobSync);
  NODE_SET_METHOD(target, "getAllJobs", GetAllJobs);
  NODE_SET_METHOD(target, "getAllJobsSync", GetAllJobsSync);
  NODE_SET_METHOD(target, "getManagerName", GetManagerName);
  NODE_SET_METHOD(target, "startStopRemove", StartStopRemove);
  NODE_SET_METHOD(target, "startStopRemoveSync", StartStopRemoveSync);
  NODE_SET_METHOD(target, "loadJob", LoadJob);
  NODE_SET_METHOD(target, "loadJobSync", LoadJobSync);
  NODE_SET_METHOD(target, "unloadJobSync", UnloadJobSync);
}
NODE_MODULE(launchctl, init);
