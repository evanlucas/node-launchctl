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
#include "launchctl.h"
using namespace node;
using namespace v8;

namespace launchctl {

// Taken from https://github.com/joyent/node/blob/master/src/node_file.cc

#define TYPE_ERROR(msg) ThrowException(Exception::TypeError(String::New(msg)));

#define THROW_BAD_ARGS TYPE_ERROR("Invalid arguments");


#define N_STRING(x) String::New(x)
#define N_NUMBER(x) Number::New(x)
#define N_NULL Local<Value>::New(Null())

#define ERROR_CB(baton, s) { \
	Local<Value> e = LaunchDException(baton->err, strerror(baton->err), s); \
	Handle<Value> argv[1]; \
	argv[0] = e; \
	TryCatch try_catch; \
	baton->callback->Call(Context::GetCurrent()->Global(), 1, argv); \
	if (try_catch.HasCaught()) { \
		node::FatalException(try_catch); \
	} \
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
  
  if (errno == 144) {
    msg = "Job already loaded";
  } else if (errorno == 145) {
    msg = "Job not loaded";
  } else if (errorno == 146) {
    msg = "Unable to set security session";
  } else if (errorno == 147) {
    msg = "Job not unloaded";
  } else if (errorno == 148) {
    msg = "Invalid domain";
  } else if (errorno == 149) {
    msg = "Job not found";
  } else if (errorno == 150) {
		msg = "Invalid command";
	} else if (errorno == 151) {
		msg = "Invalid arguments";
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
  return obj;
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
  
  return N_NUMBER(0);
}

// Gets a single job matching job label
Handle<Value> GetJobSync(const Arguments& args) {
  HandleScope scope;
  launch_data_t result = NULL;
  if (args.Length() != 1) {
    return THROW_BAD_ARGS
  }
  
  if (!args[0]->IsString()) {
    return TYPE_ERROR("Job label must be a string")
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
	launch_data_t resp = NULL;
	if (geteuid() == 0) {
		setup_system_context();
	}
	if (vproc_swap_complex(NULL, VPROC_GSK_ALLJOBS, NULL, &resp) == NULL) {
		int count = (int)resp->_array_cnt;
		if (LAUNCH_DATA_DICTIONARY != resp->type) {
			if (resp != NULL) {
				launch_data_free(resp);
			}
			return scope.Close(N_NULL);
		}
		Handle<Array> output = Array::New(count/2);
		
		int a = 0;
		for (int i=0; i<count; i+=2) {
			launch_data_t job = resp->_array[i+1];
			Local<Value> res = GetJobDetail(job, NULL);
			output->Set(N_NUMBER(a), res);
			a++;
		}
		
		return scope.Close(output);
	}
	
	return scope.Close(N_NULL);
}

// Get All Jobs Worker
void GetAllJobsWork(uv_work_t* req) {
  GetAllJobsBaton *baton = static_cast<GetAllJobsBaton *>(req->data);
	baton->resp = NULL;
	if (getuid() == 0) {
		setup_system_context();
	}
	if (vproc_swap_complex(NULL, VPROC_GSK_ALLJOBS, NULL, &baton->resp) == NULL) {
		baton->count = (int)baton->resp->_array_cnt;
	}
}

// Get All Jobs Callback
void GetAllJobsAfterWork(uv_work_t* req) {
  GetAllJobsBaton *baton = static_cast<GetAllJobsBaton *>(req->data);
	if (baton->resp == NULL) {
		baton->err = errno;
	}
	
	if (!baton->err) {
		int count = baton->count;
		Local<Array> output = Array::New(count/2);
		int a = 0;
		for (int i=0; i<count; i+=2) {
			launch_data_t j = baton->resp->_array[i+1];
			Local<Value> res = GetJobDetail(j, NULL);
			output->Set(N_NUMBER(a), res);
			a++;
		}
		Local<Value> argv[2] = {
			N_NULL,
			output
		};
		
		if (baton->resp) {
			launch_data_free(baton->resp);
		}
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
  if (u < 0) {
    Local<Value> e = LaunchDException(errno, strerror(errno), "Unable to get manager uid");
    return ThrowException(e);
  }
  Local<Value> r = N_NUMBER(u);
  return scope.Close(r);
}

Handle<Value> GetManagerPID(const Arguments& args) {
  HandleScope scope;
  int p = launchctl_get_managerpid();
  if (p < 0) {
    Local<Value> e = LaunchDException(errno, strerror(errno), "Unable to get manager uid");
    return ThrowException(e);
  }
  Local<Value> r = N_NUMBER(p);
  return scope.Close(r);
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
      return ThrowException(LaunchDException(150, "EINCMD", "Invalid command"));
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
  baton->err = res;
}

void LoadJobAfterWork(uv_work_t *req) {
  LoadJobBaton *baton = static_cast<LoadJobBaton *>(req->data);
  if (baton->err == 0) {
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

Handle<Value> SubmitJobSync(const Arguments& args) {
	HandleScope scope;
	if (args.Length() != 1) {
		return THROW_BAD_ARGS;
	}
	int r = 0;
	if (!args[0]->IsObject()) {
		return TYPE_ERROR("Argument must be an object");
	}
	Local<Object> obj = args[0]->ToObject();
	
	Local<String> label_key = String::NewSymbol("label");
	Local<String> program_key = String::NewSymbol("program");
	Local<String> stderr_key = String::NewSymbol("stderr");
	Local<String> stdout_key = String::NewSymbol("stdout");
	Local<String> args_key = String::NewSymbol("args");
	
	if (geteuid() == 0) {
		setup_system_context();
	}
	
	launch_data_t msg = launch_data_alloc(LAUNCH_DATA_DICTIONARY);
	launch_data_t job = launch_data_alloc(LAUNCH_DATA_DICTIONARY);
	launch_data_t resp, largv = launch_data_alloc(LAUNCH_DATA_ARRAY);
	if (obj->Has(label_key)) {
		if (!obj->Get(label_key)->IsString()) {
			return TYPE_ERROR("Label must be a string");
		}
		String::Utf8Value labelS(obj->Get(label_key));
		const char *label = strdup(*labelS);
		launch_data_dict_insert(job, launch_data_new_string(label), LAUNCH_JOBKEY_LABEL);
	}
	
	if (obj->Has(program_key)) {
		if (!obj->Get(program_key)->IsString()) {
			return TYPE_ERROR("Program must be a string");
		}
		String::Utf8Value progS(obj->Get(program_key));
		const char *prog = strdup(*progS);
		launch_data_dict_insert(job, launch_data_new_string(prog), LAUNCH_JOBKEY_PROGRAM);
	}
	
	if (obj->Has(stderr_key)) {
		if (!obj->Get(stderr_key)->IsString()) {
			return TYPE_ERROR("stderr must be a string");
		}
		String::Utf8Value stderrS(obj->Get(stderr_key));
		const char *stde = strdup(*stderrS);
		launch_data_dict_insert(job, launch_data_new_string(stde), LAUNCH_JOBKEY_STANDARDERRORPATH);
	}
	
	if (obj->Has(stdout_key)) {
		if (!obj->Get(stdout_key)->IsString()) {
			return TYPE_ERROR("stdout must be a string");
		}
		String::Utf8Value stdoutS(obj->Get(stdout_key));
		const char *stdo = strdup(*stdoutS);
		launch_data_dict_insert(job, launch_data_new_string(stdo), LAUNCH_JOBKEY_STANDARDOUTPATH);
	}
	
	if (obj->Has(args_key)) {
		if (!obj->Get(args_key)->IsArray()) {
			return TYPE_ERROR("args must be an array");
		}
		Local<Array> argsArray = Local<Array>::Cast(obj->Get(args_key));
		int32_t num = argsArray->Length();
		int32_t i = 0;
		for (i = 0; i < num; i++) {
			String::Utf8Value vS(argsArray->Get(N_NUMBER(i)));
			const char *v = strdup(*vS);
			size_t offt = launch_data_array_get_count(largv);
			launch_data_array_set_index(largv, launch_data_new_string(v), offt);
		}
		launch_data_dict_insert(job, largv, LAUNCH_JOBKEY_PROGRAMARGUMENTS);
	}
	
	launch_data_dict_insert(msg, job, LAUNCH_KEY_SUBMITJOB);
	
	resp = launch_msg(msg);
	launch_data_free(msg);
	
	if (resp == NULL) {
		r = errno;
		return scope.Close(N_NUMBER(r));
	} else if (launch_data_get_type(resp) == LAUNCH_DATA_ERRNO) {
		errno = launch_data_get_errno(resp);
		if (errno) {
			r = errno;
		}
	} else {
		r = 0;
	}
	launch_data_free(resp);
	return scope.Close(N_NUMBER(r));
}

void SubmitJobWorker(uv_work_t *req) {
	SubmitJobBaton *baton = static_cast<SubmitJobBaton *>(req->data);
	if (geteuid() == 0) {
		setup_system_context();
	}
	launch_data_t msg = launch_data_alloc(LAUNCH_DATA_DICTIONARY);
	launch_data_dict_insert(msg, baton->job, LAUNCH_KEY_SUBMITJOB);
	baton->resp = launch_msg(msg);
	launch_data_free(msg);
}

void SubmitJobAfterWork(uv_work_t *req) {
	SubmitJobBaton *baton = static_cast<SubmitJobBaton *>(req->data);
	if (baton->resp == NULL) {
		baton->err = errno;
	} else if (launch_data_get_type(baton->resp) == LAUNCH_DATA_ERRNO) {
		errno = launch_data_get_errno(baton->resp);
		if (errno) {
			baton->err = errno;
		}
	} else {
		baton->err = 0;
	}
	
	if (!baton->err) {
		Local<Value> argv[1];
		argv[0] = N_NULL;
		if (baton->resp) {
			launch_data_free(baton->resp);
		}
		TryCatch try_catch;
		baton->callback->Call(Context::GetCurrent()->Global(), 1, argv);
		if (try_catch.HasCaught()) {
			node::FatalException(try_catch);
		}
	} else {
		if (baton->resp) {
			launch_data_free(baton->resp);
		}
		ERROR_CB(baton, NULL);
	}
	
	delete req;
}

Handle<Value> SubmitJob(const Arguments& args) {
	HandleScope scope;
	if (args.Length() != 2) {
		return THROW_BAD_ARGS;
	}
	if (!args[0]->IsObject()) {
		return TYPE_ERROR("Argument must be an object");
	}
	
	if (!args[1]->IsFunction()) {
		fprintf(stderr, "args[1] is not a function\n");
		return TYPE_ERROR("Callback must be a function");
	}
	
	Local<Object> obj = args[0]->ToObject();
	
	Local<String> label_key = String::NewSymbol("label");
	Local<String> program_key = String::NewSymbol("program");
	Local<String> stderr_key = String::NewSymbol("stderr");
	Local<String> stdout_key = String::NewSymbol("stdout");
	Local<String> args_key = String::NewSymbol("args");
	SubmitJobBaton *baton = new SubmitJobBaton;
	baton->request.data = baton;
	baton->callback = Persistent<Function>::New(Local<Function>::Cast(args[args.Length()-1]));
	
	
	
	
	baton->job = launch_data_alloc(LAUNCH_DATA_DICTIONARY);
	baton->largv = launch_data_alloc(LAUNCH_DATA_ARRAY);
	if (obj->Has(label_key)) {
		if (!obj->Get(label_key)->IsString()) {
			return TYPE_ERROR("Label must be a string");
		}
		String::Utf8Value labelS(obj->Get(label_key));
		const char *label = strdup(*labelS);
		launch_data_dict_insert(baton->job, launch_data_new_string(label), LAUNCH_JOBKEY_LABEL);
	}
	
	if (obj->Has(program_key)) {
		if (!obj->Get(program_key)->IsString()) {
			return TYPE_ERROR("Program must be a string");
		}
		String::Utf8Value progS(obj->Get(program_key));
		const char *prog = strdup(*progS);
		launch_data_dict_insert(baton->job, launch_data_new_string(prog), LAUNCH_JOBKEY_PROGRAM);
	}
	
	if (obj->Has(stderr_key)) {
		if (!obj->Get(stderr_key)->IsString()) {
			return TYPE_ERROR("stderr must be a string");
		}
		String::Utf8Value stderrS(obj->Get(stderr_key));
		const char *stde = strdup(*stderrS);
		launch_data_dict_insert(baton->job, launch_data_new_string(stde), LAUNCH_JOBKEY_STANDARDERRORPATH);
	}
	
	if (obj->Has(stdout_key)) {
		if (!obj->Get(stdout_key)->IsString()) {
			return TYPE_ERROR("stdout must be a string");
		}
		String::Utf8Value stdoutS(obj->Get(stdout_key));
		const char *stdo = strdup(*stdoutS);
		launch_data_dict_insert(baton->job, launch_data_new_string(stdo), LAUNCH_JOBKEY_STANDARDOUTPATH);
	}
	
	if (obj->Has(args_key)) {
		if (!obj->Get(args_key)->IsArray()) {
			return TYPE_ERROR("args must be an array");
		}
		Local<Array> argsArray = Local<Array>::Cast(obj->Get(args_key));
		int32_t num = argsArray->Length();
		int32_t i = 0;
		for (i = 0; i < num; i++) {
			String::Utf8Value vS(argsArray->Get(N_NUMBER(i)));
			const char *v = strdup(*vS);
			size_t offt = launch_data_array_get_count(baton->largv);
			launch_data_array_set_index(baton->largv, launch_data_new_string(v), offt);
		}
		launch_data_dict_insert(baton->job, baton->largv, LAUNCH_JOBKEY_PROGRAMARGUMENTS);
	}
	uv_queue_work(uv_default_loop(), &baton->request, SubmitJobWorker, (uv_after_work_cb)SubmitJobAfterWork);
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
  return scope.Close(N_NUMBER(result));
}


void UnloadJobWorker(uv_work_t *req) {
  UnloadJobBaton *baton = static_cast<UnloadJobBaton *>(req->data);
  int res = launchctl_unload_job(baton->path, baton->editondisk, baton->forceload, baton->session_type, baton->domain);
  baton->err = res;
}

void UnloadJobAfterWork(uv_work_t *req) {
  UnloadJobBaton *baton = static_cast<UnloadJobBaton *>(req->data);
  if (baton->err == 0) {
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
    // Some kind of error
    Local<Value> e = LaunchDException(baton->err, strerror(baton->err), NULL);
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
Handle<Value> UnloadJob(const Arguments& args) {
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
  
  UnloadJobBaton *baton = new UnloadJobBaton;
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
  
  uv_queue_work(uv_default_loop(), &baton->request, UnloadJobWorker, (uv_after_work_cb)UnloadJobAfterWork);
  return Undefined();
}

Handle<Value> GetLimitSync(const Arguments& args) {
  HandleScope scope;
	char slimstr[100];
	char hlimstr[100];
	struct rlimit *lmts = NULL;
	launch_data_t resp, msg;
	int r = 0;
	size_t i, lsz = -1;
	msg = launch_data_new_string(LAUNCH_KEY_GETRESOURCELIMITS);
	resp = launch_msg(msg);
	launch_data_free(msg);
	if (resp == NULL) {
		r = errno;
		Local<Value> e = LaunchDException(r, strerror(r), NULL);
		return ThrowException(e);
	} else if (launch_data_get_type(resp) == LAUNCH_DATA_OPAQUE) {
		lmts = (struct rlimit *)launch_data_get_opaque(resp);
		lsz = launch_data_get_opaque_size(resp);
		Handle<Object> output = Object::New();
		for (i = 0; i<(lsz/sizeof(struct rlimit)); i++) {
			const char *l = num2name((int)i);
			Local<Object> inside = Object::New();
			inside->Set(N_STRING("soft"), N_STRING(lim2str(lmts[i].rlim_cur, slimstr)));
			inside->Set(N_STRING("hard"), N_STRING(lim2str(lmts[i].rlim_max, hlimstr)));
			output->Set(N_STRING(l), inside);
		}
		launch_data_free(resp);
		return scope.Close(output);
	} else if (launch_data_get_type(resp) == LAUNCH_DATA_STRING) {
		Local<Value> e = LaunchDException(1000, launch_data_get_string(resp), NULL);
		launch_data_free(resp);
		return ThrowException(e);
	} else {
		Local<Value> e = LaunchDException(errno, strerror(errno), NULL);
		return ThrowException(e);
	}
}

void InitLaunchctl(Handle<Object> target) {
  HandleScope scope;
  NODE_SET_METHOD(target, "getJob", GetJob);
  NODE_SET_METHOD(target, "getJobSync", GetJobSync);
  NODE_SET_METHOD(target, "getAllJobs", GetAllJobs);
  NODE_SET_METHOD(target, "getAllJobsSync", GetAllJobsSync);
  NODE_SET_METHOD(target, "getManagerName", GetManagerName);
  NODE_SET_METHOD(target, "getManagerPID", GetManagerPID);
  NODE_SET_METHOD(target, "getManagerUID", GetManagerUID);
  NODE_SET_METHOD(target, "startStopRemove", StartStopRemove);
  NODE_SET_METHOD(target, "startStopRemoveSync", StartStopRemoveSync);
  NODE_SET_METHOD(target, "loadJob", LoadJob);
  NODE_SET_METHOD(target, "loadJobSync", LoadJobSync);
  NODE_SET_METHOD(target, "unloadJob", UnloadJob);
  NODE_SET_METHOD(target, "unloadJobSync", UnloadJobSync);
	NODE_SET_METHOD(target, "submitJob", SubmitJob);
	NODE_SET_METHOD(target, "submitJobSync", SubmitJobSync);
	NODE_SET_METHOD(target, "getLimitSync", GetLimitSync);
}

//NODE_MODULE(launchctl, init);

} // namespace liblaunchctl