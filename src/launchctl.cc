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

#define TYPE_ERROR(msg) NanThrowTypeError(msg);

#define THROW_BAD_ARGS TYPE_ERROR("Invalid arguments");


#define N_STRING(x) String::New(x)
#define N_NUMBER(x) Number::New(x)
#define N_NULL NanNewLocal<v8::Value>(v8::Null())

#define ERROR_CB(baton, s) { \
	Local<Value> e = LaunchDException(baton->err, strerror(baton->err), s); \
	Local<Value> argv[1]; \
	argv[0] = e; \
	TryCatch try_catch; \
	baton->callback->Call(1, argv); \
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
#if !NODE_VERSION_AT_LEAST(0, 11, 0)
  if (errno_symbol.IsEmpty()) {
    errno_symbol = NODE_PSYMBOL("errno");
    code_symbol = NODE_PSYMBOL("code");
    errmsg_symbol = NODE_PSYMBOL("msg");
  }
#endif
	switch (errorno) {
		case 144:
			msg = "Job already loaded";
			break;
		case 145:
			msg = "Job not loaded";
			break;
		case 146:
			msg = "Unable to set security session";
			break;
		case 147:
			msg = "Job not unloaded";
			break;
		case 148:
			msg = "Invalid domain";
			break;
		case 149:
			msg = "Job not found";
			break;
		case 150:
			msg = "Invalid command";
			break;
		case 151:
			msg = "Invalid arguments";
			break;
		case 152:
			msg = "Invalid limit";
			break;
		case 153:
			msg = "Unknown response from launchd";
			break;
		case 154:
			msg = "Invalid umask";
			break;
		default:
			msg = strerror(errorno);
			break;
	}

  if (!msg || !msg[0]) {
    msg = strerror(errorno);
  }

  Local<String> estring = NanSymbol(strerror(errorno));
  Local<String> message = NanSymbol(msg);
  Local<String> cons1 = String::Concat(estring, NanSymbol(", "));
  Local<String> cons2 = String::Concat(cons1, message);

  Local<Value> e = Exception::Error(cons2);

  Local<Object> obj = e->ToObject();
#if NODE_VERSION_AT_LEAST(0, 11, 0)
	obj->Set(N_STRING("errno"), Integer::New(errorno));
	obj->Set(N_STRING("code"), estring);
	obj->Set(N_STRING("msg"), message);
#else
  obj->Set(errno_symbol, Integer::New(errorno));
  obj->Set(code_symbol, estring);
  obj->Set(errmsg_symbol, message);
#endif
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
NAN_METHOD(GetJobSync) {
  NanScope();
  launch_data_t result = NULL;
  if (args.Length() != 1) {
    THROW_BAD_ARGS
  }

  if (!args[0]->IsString()) {
    TYPE_ERROR("Job label must be a string")
  }

  String::Utf8Value job(args[0]);

  const char* label = *job;
  result = launchctl_list_job(label);
  if (result == NULL) {
    Local<Value> e = LaunchDException(errno, strerror(errno), NULL);
    ThrowException(e);
  }
  Local<Value> res = GetJobDetail(result, NULL);
  if (result)
    launch_data_free(result);
  NanReturnValue(res);
}

// Get Job Worker
void GetJobWork(uv_work_t* req) {
  GetJobBaton *baton = static_cast<GetJobBaton *>(req->data);
  baton->resp = launchctl_list_job(baton->label);
  if (baton->resp == NULL) {
    baton->err = errno;
  }
}

// Get Job Callback
void GetJobAfterWork(uv_work_t *req) {
  NanScope();
  GetJobBaton *baton = static_cast<GetJobBaton *>(req->data);
  if (!baton->err) {
    Local<Value> res = GetJobDetail(baton->resp, NULL);
    if (res == N_NULL) {
      // No such process
      Local<Value> s = LaunchDException(3, strerror(3), NULL);
      Local<Value> argv[] = {
        s
      };
      TryCatch try_catch;
      baton->callback->Call(2, argv);
      if (try_catch.HasCaught()) {
        node::FatalException(try_catch);
      }
    } else {
      Local<Value> argv[2] = {
        N_NULL,
        res
      };
      if (baton->resp)
        launch_data_free(baton->resp);
      TryCatch try_catch;
      baton->callback->Call(2, argv);
      if (try_catch.HasCaught()) {
        node::FatalException(try_catch);
      }
    }
  } else {
    Local<Value> s = LaunchDException(baton->err, strerror(baton->err), NULL);
    Local<Value> argv[] = {
      s
    };
    if (baton->resp)
      launch_data_free(baton->resp);
    TryCatch try_catch;
    baton->callback->Call(1, argv);
    if (try_catch.HasCaught()) {
      node::FatalException(try_catch);
    }
  }
  delete req;
}

// Get Job by name
NAN_METHOD(GetJob) {
  NanScope();
  if (args.Length() != 2) {
    ThrowException(Exception::Error(N_STRING("Invalid args")));
  }

  if (!args[0]->IsString()) {
    ThrowException(Exception::TypeError(N_STRING("Job must be a string")));
  }

  if (!args[1]->IsFunction()) {
    ThrowException(Exception::TypeError(N_STRING("Callback must be a function")));
  }

  String::Utf8Value job(args[0]);
  const char* label = *job;

  GetJobBaton *baton = new GetJobBaton;

  baton->request.data = baton;
  baton->label = label;
  baton->err = 0;
  baton->resp = NULL;
  baton->callback = new NanCallback(Local<Function>::Cast(args[1]));
  uv_queue_work(uv_default_loop(), &baton->request, GetJobWork, (uv_after_work_cb)GetJobAfterWork);

  NanReturnUndefined();
}


// Gets all jobs
NAN_METHOD(GetAllJobsSync) {
  NanScope();
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
			NanReturnValue(N_NULL);
		}
		Handle<Array> output = Array::New(count/2);

		int a = 0;
		for (int i=0; i<count; i+=2) {
			launch_data_t job = resp->_array[i+1];
			Local<Value> res = GetJobDetail(job, NULL);
			output->Set(N_NUMBER(a), res);
			a++;
		}

		NanReturnValue(output);
	}

	NanReturnValue(N_NULL);
}

// Get All Jobs Worker
void GetAllJobsWork(uv_work_t* req) {
  GetAllJobsBaton *baton = static_cast<GetAllJobsBaton *>(req->data);
	baton->resp = NULL;
	if (geteuid() == 0) {
		setup_system_context();
	}
	if (vproc_swap_complex(NULL, VPROC_GSK_ALLJOBS, NULL, &baton->resp) == NULL) {
		baton->count = (int)baton->resp->_array_cnt;
	}
}

// Get All Jobs Callback
void GetAllJobsAfterWork(uv_work_t* req) {
	NanScope();
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
		baton->callback->Call(2, argv);
		if (try_catch.HasCaught()) {
			node::FatalException(try_catch);
		}
	} else {
		Local<Value> e = LaunchDException(baton->err, strerror(baton->err), NULL);
		Local<Value> argv[1] = {
			e
		};
		TryCatch try_catch;
		baton->callback->Call(1, argv);
		if (try_catch.HasCaught()) {
			node::FatalException(try_catch);
		}
	}

	delete req;
}

// Get all jobs
NAN_METHOD(GetAllJobs) {
  NanScope();
  if (args.Length() != 1) {
    THROW_BAD_ARGS;
  }

  if (!args[0]->IsFunction()) {
    TYPE_ERROR("Callback must be a function");
  }

  GetAllJobsBaton *baton = new GetAllJobsBaton;
  baton->request.data = baton;
  baton->err = 0;
  baton->callback = new NanCallback(Local<Function>::Cast(args[0]));

  uv_queue_work(uv_default_loop(), &baton->request, GetAllJobsWork, (uv_after_work_cb)GetAllJobsAfterWork);

  NanReturnUndefined();
}

NAN_METHOD(GetManagerName) {
  NanScope();
  char * mgmr = launchctl_get_managername();
  if (mgmr == NULL) {
    Local<Value> e = LaunchDException(errno, strerror(errno), "Unable to get manager name");
    ThrowException(e);
  }
  Local<Value> r = N_STRING(mgmr);
  free(mgmr);
  NanReturnValue(r);
}

NAN_METHOD(GetManagerUID) {
  NanScope();
  int u = launchctl_get_manageruid();
  if (u < 0) {
    Local<Value> e = LaunchDException(errno, strerror(errno), "Unable to get manager uid");
    ThrowException(e);
  }
  Local<Value> r = N_NUMBER(u);
  NanReturnValue(r);
}

NAN_METHOD(GetManagerPID) {
  NanScope();
  int p = launchctl_get_managerpid();
  if (p < 0) {
    Local<Value> e = LaunchDException(errno, strerror(errno), "Unable to get manager uid");
    ThrowException(e);
  }
  Local<Value> r = N_NUMBER(p);
  NanReturnValue(r);
}

NAN_METHOD(StartStopRemoveSync) {
  NanScope();
  if (args.Length() != 2) {
    THROW_BAD_ARGS;
  }

  if (!args[0]->IsString()) {
    TYPE_ERROR("Job label must be a string");
  }

  if (!args[1]->IsInt32()) {
    TYPE_ERROR("Command must be an integer");
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
      ThrowException(LaunchDException(150, "EINCMD", "Invalid command"));
      break;
  }
  if (result != 0) {
    ThrowException(LaunchDException(result, NULL, NULL));
  }
  NanReturnValue(N_NUMBER(result));
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
  NanScope();
  SSRBaton *baton = static_cast<SSRBaton *>(req->data);

  if (!baton->err) {
    if (baton->action == NODE_LAUNCHCTL_CMD_START) {
      Local<Value> res = GetJobDetail(baton->job, NULL);
      Local<Value> argv[2] = {
        N_NULL,
        res
      };
      launch_data_free(baton->job);
      TryCatch try_catch;
      baton->callback->Call(2, argv);
      if (try_catch.HasCaught()) {
        node::FatalException(try_catch);
      }
    } else {
      Local<Value> argv[2] = {
        N_NULL,
        N_NUMBER(1)
      };
      TryCatch try_catch;
      baton->callback->Call(2, argv);
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
    Local<Value> argv[1] = {
      s
    };

    TryCatch try_catch;
    baton->callback->Call(1, argv);
    if (try_catch.HasCaught()) {
      node::FatalException(try_catch);
    }
  }

  delete req;

}

NAN_METHOD(StartStopRemove) {
  NanScope();
  if (args.Length() != 3) {
    THROW_BAD_ARGS;
  }

  if (!args[0]->IsString()) {
    TYPE_ERROR("Job label must be a string");
  }

  if (!args[1]->IsInt32()) {
    TYPE_ERROR("Command must be an integer");
  }

  if (!args[2]->IsFunction()) {
    TYPE_ERROR("Callback must be a function");
  }

  String::Utf8Value job(args[0]);

  const char* label = *job;

  node_launchctl_action_t cmd_v = (node_launchctl_action_t)args[1]->Int32Value();

  SSRBaton *baton = new SSRBaton;
  baton->request.data = baton;
  baton->label = label;
  baton->action = cmd_v;
  baton->err = 0;
  baton->callback = new NanCallback(Local<Function>::Cast(args[2]));
  uv_queue_work(uv_default_loop(), &baton->request, StartStopRemoveWork, (uv_after_work_cb)StartStopRemoveAfterWork);
  NanReturnUndefined();
}

NAN_METHOD(LoadJobSync) {
  NanScope();
  // Job, editondisk, forceload, session_type, domain
  if (args.Length() < 3) {
    THROW_BAD_ARGS;
  }

  if (!args[0]->IsString()) {
    TYPE_ERROR("Job path must be a string");
  }

  String::Utf8Value job(args[0]);
  const char *jobpath = *job;

  if (!args[1]->IsBoolean()) {
    TYPE_ERROR("Edit On Disk must be a bool");
  }

  bool editondisk = (args[1]->ToBoolean() == True()) ? true : false;
  if (!args[2]->IsBoolean()) {
    TYPE_ERROR("Force Load must be a bool");
  }

  bool forceload = (args[2]->ToBoolean() == True()) ? true : false;

  const char *session_type = NULL;
  const char *domain = NULL;

  if (args.Length() == 4 || args.Length() == 5) {
    if (!args[3]->IsString() || !args[3]->IsNull()) {
      TYPE_ERROR("Session type must be a string");
    } else {
      String::Utf8Value sesstype(args[3]);
      session_type = *sesstype;
    }
  }

  if (args.Length() == 5) {
    if (!args[4]->IsString()) {
      TYPE_ERROR("Domain must be a string");
    } else {
      String::Utf8Value dm(args[4]);
      domain = *dm;
    }
  }

  int result = launchctl_load_job(jobpath, editondisk, forceload, session_type, domain);
  if (result != 0) {
    ThrowException(LaunchDException(errno, strerror(errno), NULL));
  }
  NanReturnValue(N_NUMBER(result));
}

void LoadJobWorker(uv_work_t *req) {
  LoadJobBaton *baton = static_cast<LoadJobBaton *>(req->data);
  int res = launchctl_load_job(baton->path, baton->editondisk, baton->forceload, baton->session_type, baton->domain);
  baton->err = res;
}

void LoadJobAfterWork(uv_work_t *req) {
	NanScope();
  LoadJobBaton *baton = static_cast<LoadJobBaton *>(req->data);
  if (baton->err == 0) {
    // Success
    // TODO:
    // Since we are loading the job, lets see if we can (at this point) get the job's details :]
    // This will take reading the plist and getting the label
    Local<Value> argv[] = {
      N_NULL,
      N_NUMBER(0)
    };
    TryCatch try_catch;
    baton->callback->Call(2, argv);
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
    Local<Value> argv[1] = {
      e
    };
    TryCatch try_catch;
    baton->callback->Call(1, argv);
    if (try_catch.HasCaught()) {
      node::FatalException(try_catch);
    }
  }
}

NAN_METHOD(LoadJob) {
  NanScope();
  // Job, editondisk, forceload, session_type, domain
  if (args.Length() < 4) {
    THROW_BAD_ARGS;
  }

  if (!args[0]->IsString()) {
    TYPE_ERROR("Job path must be a string");
  }

  String::Utf8Value job(args[0]);
  char *jobpath = *job;

  if (!args[1]->IsBoolean()) {
    TYPE_ERROR("Edit On Disk must be a bool");
  }

  bool editondisk = (args[1]->ToBoolean() == True()) ? true : false;
  if (!args[2]->IsBoolean()) {
    TYPE_ERROR("Force Load must be a bool");
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
    TYPE_ERROR("Callback must be a function");
  }
  baton->callback = new NanCallback(Local<Function>::Cast(args[args.Length()-1]));
  if (args.Length() == 4) {
    baton->session_type = NULL;
    baton->domain = NULL;
  } else if (args.Length() == 5) {
    if (!args[3]->IsString()) {
      TYPE_ERROR("Session type must be a string");
    } else {
      String::Utf8Value sesstype(args[3]);
      session_type = *sesstype;
      baton->session_type = strdup(session_type);
    }
  } else if (args.Length() == 6) {
    if (!args[3]->IsString()) {
      TYPE_ERROR("Session type must be a string");
    } else {
      String::Utf8Value sesstype(args[3]);
      session_type = *sesstype;
      baton->session_type = strdup(session_type);
    }

    if (!args[4]->IsString()) {
      TYPE_ERROR("Domain must be a string");
    } else {
      String::Utf8Value dm(args[4]);
      domain = *dm;
      baton->domain = strdup(domain);
    }
  }

  uv_queue_work(uv_default_loop(), &baton->request, LoadJobWorker, (uv_after_work_cb)LoadJobAfterWork);
  NanReturnUndefined();
}

NAN_METHOD(SubmitJobSync) {
	NanScope();
	if (args.Length() != 1) {
		THROW_BAD_ARGS;
	}
	int r = 0;
	if (!args[0]->IsObject()) {
		TYPE_ERROR("Argument must be an object");
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
			TYPE_ERROR("Label must be a string");
		}
		String::Utf8Value labelS(obj->Get(label_key));
		const char *label = strdup(*labelS);
		launch_data_dict_insert(job, launch_data_new_string(label), LAUNCH_JOBKEY_LABEL);
	}

	if (obj->Has(program_key)) {
		if (!obj->Get(program_key)->IsString()) {
			TYPE_ERROR("Program must be a string");
		}
		String::Utf8Value progS(obj->Get(program_key));
		const char *prog = strdup(*progS);
		launch_data_dict_insert(job, launch_data_new_string(prog), LAUNCH_JOBKEY_PROGRAM);
	}

	if (obj->Has(stderr_key)) {
		if (!obj->Get(stderr_key)->IsString()) {
			TYPE_ERROR("stderr must be a string");
		}
		String::Utf8Value stderrS(obj->Get(stderr_key));
		const char *stde = strdup(*stderrS);
		launch_data_dict_insert(job, launch_data_new_string(stde), LAUNCH_JOBKEY_STANDARDERRORPATH);
	}

	if (obj->Has(stdout_key)) {
		if (!obj->Get(stdout_key)->IsString()) {
			TYPE_ERROR("stdout must be a string");
		}
		String::Utf8Value stdoutS(obj->Get(stdout_key));
		const char *stdo = strdup(*stdoutS);
		launch_data_dict_insert(job, launch_data_new_string(stdo), LAUNCH_JOBKEY_STANDARDOUTPATH);
	}

	if (obj->Has(args_key)) {
		if (!obj->Get(args_key)->IsArray()) {
			TYPE_ERROR("args must be an array");
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
		NanReturnValue(N_NUMBER(r));
	} else if (launch_data_get_type(resp) == LAUNCH_DATA_ERRNO) {
		errno = launch_data_get_errno(resp);
		if (errno) {
			r = errno;
		}
	} else {
		r = 0;
	}
	launch_data_free(resp);
	NanReturnValue(N_NUMBER(r));
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
	NanScope();
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
		baton->callback->Call(1, argv);
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

NAN_METHOD(SubmitJob) {
	NanScope();
	if (args.Length() != 2) {
		THROW_BAD_ARGS;
	}
	if (!args[0]->IsObject()) {
		TYPE_ERROR("Argument must be an object");
	}

	if (!args[1]->IsFunction()) {
		fprintf(stderr, "args[1] is not a function\n");
		TYPE_ERROR("Callback must be a function");
	}

	Local<Object> obj = args[0]->ToObject();

	Local<String> label_key = String::NewSymbol("label");
	Local<String> program_key = String::NewSymbol("program");
	Local<String> stderr_key = String::NewSymbol("stderr");
	Local<String> stdout_key = String::NewSymbol("stdout");
	Local<String> args_key = String::NewSymbol("args");
	SubmitJobBaton *baton = new SubmitJobBaton;
	baton->request.data = baton;
	baton->callback = new NanCallback(Local<Function>::Cast(args[args.Length()-1]));


	baton->job = launch_data_alloc(LAUNCH_DATA_DICTIONARY);
	baton->largv = launch_data_alloc(LAUNCH_DATA_ARRAY);
	if (obj->Has(label_key)) {
		if (!obj->Get(label_key)->IsString()) {
			TYPE_ERROR("Label must be a string");
		}
		String::Utf8Value labelS(obj->Get(label_key));
		const char *label = strdup(*labelS);
		launch_data_dict_insert(baton->job, launch_data_new_string(label), LAUNCH_JOBKEY_LABEL);
	}

	if (obj->Has(program_key)) {
		if (!obj->Get(program_key)->IsString()) {
			TYPE_ERROR("Program must be a string");
		}
		String::Utf8Value progS(obj->Get(program_key));
		const char *prog = strdup(*progS);
		launch_data_dict_insert(baton->job, launch_data_new_string(prog), LAUNCH_JOBKEY_PROGRAM);
	}

	if (obj->Has(stderr_key)) {
		if (!obj->Get(stderr_key)->IsString()) {
			TYPE_ERROR("stderr must be a string");
		}
		String::Utf8Value stderrS(obj->Get(stderr_key));
		const char *stde = strdup(*stderrS);
		launch_data_dict_insert(baton->job, launch_data_new_string(stde), LAUNCH_JOBKEY_STANDARDERRORPATH);
	}

	if (obj->Has(stdout_key)) {
		if (!obj->Get(stdout_key)->IsString()) {
			TYPE_ERROR("stdout must be a string");
		}
		String::Utf8Value stdoutS(obj->Get(stdout_key));
		const char *stdo = strdup(*stdoutS);
		launch_data_dict_insert(baton->job, launch_data_new_string(stdo), LAUNCH_JOBKEY_STANDARDOUTPATH);
	}

	if (obj->Has(args_key)) {
		if (!obj->Get(args_key)->IsArray()) {
			TYPE_ERROR("args must be an array");
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
	NanReturnUndefined();
}
NAN_METHOD(UnloadJobSync) {
  NanScope();
  // Job, editondisk, forceload, session_type, domain
  if (args.Length() < 3) {
    THROW_BAD_ARGS;
  }

  if (!args[0]->IsString()) {
    TYPE_ERROR("Job path must be a string");
  }

  String::Utf8Value job(args[0]);
  const char *jobpath = *job;

  if (!args[1]->IsBoolean()) {
    TYPE_ERROR("Edit On Disk must be a bool");
  }

  bool editondisk = (args[1]->ToBoolean() == True()) ? true : false;
  if (!args[2]->IsBoolean()) {
    TYPE_ERROR("Force Load must be a bool");
  }

  bool forceload = (args[2]->ToBoolean() == True()) ? true : false;

  const char *session_type = NULL;
  const char *domain = NULL;

  if (args.Length() == 4 || args.Length() == 5) {
    if (!args[3]->IsString() || !args[3]->IsNull()) {
      TYPE_ERROR("Session type must be a string");
    } else {
      String::Utf8Value sesstype(args[3]);
      session_type = *sesstype;
    }
  }

  if (args.Length() == 5) {
    if (!args[4]->IsString()) {
      TYPE_ERROR("Domain must be a string");
    } else {
      String::Utf8Value dm(args[4]);
      domain = *dm;
    }
  }

  int result = launchctl_unload_job(jobpath, editondisk, forceload, session_type, domain);
  if (result != 0) {
    ThrowException(LaunchDException(errno, strerror(errno), NULL));
  }
  NanReturnValue(N_NUMBER(result));
}


void UnloadJobWorker(uv_work_t *req) {
  UnloadJobBaton *baton = static_cast<UnloadJobBaton *>(req->data);
  int res = launchctl_unload_job(baton->path, baton->editondisk, baton->forceload, baton->session_type, baton->domain);
  baton->err = res;
}

void UnloadJobAfterWork(uv_work_t *req) {
	NanScope();
  UnloadJobBaton *baton = static_cast<UnloadJobBaton *>(req->data);
  if (baton->err == 0) {
    Local<Value> argv[2] = {
      N_NULL,
      N_NUMBER(0)
    };
    TryCatch try_catch;
    baton->callback->Call(2, argv);
    if (try_catch.HasCaught()) {
      node::FatalException(try_catch);
    }
  } else {
    // Some kind of error
    Local<Value> e = LaunchDException(baton->err, strerror(baton->err), NULL);
    Local<Value> argv[1] = {
      e
    };
    TryCatch try_catch;
    baton->callback->Call(1, argv);
    if (try_catch.HasCaught()) {
      node::FatalException(try_catch);
    }
  }
}
NAN_METHOD(UnloadJob) {
  NanScope();
  // Job, editondisk, forceload, session_type, domain
  if (args.Length() < 4) {
    THROW_BAD_ARGS;
  }

  if (!args[0]->IsString()) {
    TYPE_ERROR("Job path must be a string");
  }

  String::Utf8Value job(args[0]);
  char *jobpath = *job;

  if (!args[1]->IsBoolean()) {
    TYPE_ERROR("Edit On Disk must be a bool");
  }

  bool editondisk = (args[1]->ToBoolean() == True()) ? true : false;
  if (!args[2]->IsBoolean()) {
    TYPE_ERROR("Force Load must be a bool");
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
    TYPE_ERROR("Callback must be a function");
  }
  baton->callback = new NanCallback(Local<Function>::Cast(args[args.Length()-1]));
  if (args.Length() == 4) {
    baton->session_type = NULL;
    baton->domain = NULL;
  } else if (args.Length() == 5) {
    if (!args[3]->IsString()) {
      TYPE_ERROR("Session type must be a string");
    } else {
      String::Utf8Value sesstype(args[3]);
      session_type = *sesstype;
      baton->session_type = strdup(session_type);
    }
  } else if (args.Length() == 6) {
    if (!args[3]->IsString()) {
      TYPE_ERROR("Session type must be a string");
    } else {
      String::Utf8Value sesstype(args[3]);
      session_type = *sesstype;
      baton->session_type = strdup(session_type);
    }

    if (!args[4]->IsString()) {
      TYPE_ERROR("Domain must be a string");
    } else {
      String::Utf8Value dm(args[4]);
      domain = *dm;
      baton->domain = strdup(domain);
    }
  }

  uv_queue_work(uv_default_loop(), &baton->request, UnloadJobWorker, (uv_after_work_cb)UnloadJobAfterWork);
  NanReturnUndefined();
}

NAN_METHOD(GetLimitSync) {
  NanScope();
	char slimstr[100];
	char hlimstr[100];
	struct rlimit *lmts = NULL;
	launch_data_t resp, msg;
	int r = 0;
	size_t i, lsz = -1;
  if (geteuid() == 0) {
		setup_system_context();
	}
	msg = launch_data_new_string(LAUNCH_KEY_GETRESOURCELIMITS);
	resp = launch_msg(msg);
	launch_data_free(msg);
	if (resp == NULL) {
		r = errno;
		Local<Value> e = LaunchDException(r, strerror(r), NULL);
		ThrowException(e);
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
		NanReturnValue(output);
	} else if (launch_data_get_type(resp) == LAUNCH_DATA_STRING) {
		Local<Value> e = LaunchDException(1000, launch_data_get_string(resp), NULL);
		launch_data_free(resp);
		ThrowException(e);
	}
  Local<Value> e = LaunchDException(errno, strerror(errno), NULL);
  ThrowException(e);
  NanReturnValue(N_NUMBER(errno));
}

NAN_METHOD(SetLimitSync) {
  NanScope();
  if (geteuid() == 0) {
    setup_system_context();
  }
  if (args.Length() < 2) {
    THROW_BAD_ARGS;
  }

  if (!args[0]->IsString()) {
    TYPE_ERROR("Limit name must be a string");
  }

  String::Utf8Value nameS(args[0]);
  const char *name = *nameS;

  if (!args[1]->IsString()) {
    TYPE_ERROR("Soft limit must be a string");
  }

  String::Utf8Value softS(args[1]);
  const char *soft = *softS;


  if (!args[2]->IsString()) {
    TYPE_ERROR("Hard limit must be a string");
  }

  String::Utf8Value hardS(args[2]);
  const char *hard = *hardS;


  struct rlimit *lmts = NULL;
  launch_data_t resp, msg, tmp, resp1 = NULL;
  int r = 0;
  size_t lsz = -1;
  ssize_t which = 0;
  rlim_t slim = -1, hlim = -1;
  if (-1 == (which = name2num(name))) {
    fprintf(stderr, "Unable to find name2num\n");
    Local<Value> e = LaunchDException(152, "EINVLIM", NULL);
    ThrowException(e);
  }
  msg = launch_data_new_string(LAUNCH_KEY_GETRESOURCELIMITS);
  resp = launch_msg(msg);
  launch_data_free(msg);
  if (resp == NULL) {
    r = errno;
    Local<Value> e = LaunchDException(r, strerror(r), NULL);
    ThrowException(e);
  } else if (launch_data_get_type(resp) == LAUNCH_DATA_OPAQUE) {
    lmts = (struct rlimit *)launch_data_get_opaque(resp);
    lsz = launch_data_get_opaque_size(resp);
  } else if (launch_data_get_type(resp) == LAUNCH_DATA_STRING) {
    Local<Value> e = LaunchDException(153, "EUNKRES", launch_data_get_string(resp));
    launch_data_free(resp);
    ThrowException(e);
  } else {
    Local<Value> e = LaunchDException(errno, strerror(errno), NULL);
    ThrowException(e);
  }
  resp1 = resp;

  if (str2lim(soft, &slim)) {
    Local<Value> e = LaunchDException(152, "EINVLIM", "Invalid soft limit");
    ThrowException(e);
  }
  if (str2lim(hard, &hlim)) {
    Local<Value> e = LaunchDException(152, "EINVLIM", "Invalid hard limit");
    ThrowException(e);
  }

  lmts[which].rlim_cur = slim;
  lmts[which].rlim_max = hlim;

  msg = launch_data_alloc(LAUNCH_DATA_DICTIONARY);
  tmp = launch_data_new_opaque(lmts, lsz);
  launch_data_dict_insert(msg, tmp, LAUNCH_KEY_SETRESOURCELIMITS);
  resp = launch_msg(msg);
  launch_data_free(msg);

  if (resp == NULL) {
    Local<Value> e = LaunchDException(errno, strerror(errno), NULL);
    ThrowException(e);
  } else if (launch_data_get_type(resp) == LAUNCH_DATA_STRING) {
    Local<Value> e = LaunchDException(153, "EUNKRES", launch_data_get_string(resp));
    ThrowException(e);
  } else if (launch_data_get_type(resp) != LAUNCH_DATA_OPAQUE) {
    Local<Value> e = LaunchDException(153, "EUNKRES", "Unknown response from launchd");
    ThrowException(e);
  }

  launch_data_free(resp);
  launch_data_free(resp1);

  NanReturnValue(N_NUMBER(0));
}

NAN_METHOD(SetEnvVar) {
	NanScope();

	if (args.Length() != 2) {
		THROW_BAD_ARGS;
	}

	if (!args[0]->IsString()) {
		TYPE_ERROR("Key must be a string");
	}

	if (!args[1]->IsString()) {
		TYPE_ERROR("Value must be a string");
	}

	String::Utf8Value keyS(args[0]);
	String::Utf8Value valS(args[1]);
	const char *key = *keyS;
	const char *val = *valS;

	launch_data_t resp, tmp, tmpv, msg;

	msg = launch_data_alloc(LAUNCH_DATA_DICTIONARY);
	tmp = launch_data_alloc(LAUNCH_DATA_DICTIONARY);
	int r = 0;
	tmpv = launch_data_new_string(val);
	launch_data_dict_insert(tmp, tmpv, key);
	launch_data_dict_insert(msg, tmp, LAUNCH_KEY_SETUSERENVIRONMENT);

	resp = launch_msg(msg);
	launch_data_free(msg);

	if (resp) {
		launch_data_free(resp);
	} else {
		r = errno;
		Local<Value> e = LaunchDException(errno, strerror(errno), NULL);
		ThrowException(e);
	}
	NanReturnValue(N_NUMBER(r));
}

NAN_METHOD(UnsetEnvVar) {
	NanScope();
	int r = 0;

	if (args.Length() != 1) {
		THROW_BAD_ARGS;
	}

	if (!args[0]->IsString()) {
		TYPE_ERROR("Key must be a string");
	}


	String::Utf8Value keyS(args[0]);
	const char *key = *keyS;

	launch_data_t resp, tmp, msg;
	msg = launch_data_alloc(LAUNCH_DATA_DICTIONARY);
	tmp = launch_data_new_string(key);
	launch_data_dict_insert(msg, tmp, LAUNCH_KEY_UNSETUSERENVIRONMENT);

	resp = launch_msg(msg);
	launch_data_free(msg);

	if (resp) {
		launch_data_free(resp);
	} else {
		r = errno;
		Local<Value> e = LaunchDException(errno, strerror(errno), NULL);
		ThrowException(e);
	}

	NanReturnValue(N_NUMBER(r));
}

NAN_METHOD(GetRUsage) {
  NanScope();
  int r = 0;
  launch_data_t resp, msg;
  if (args.Length() != 1) {
    THROW_BAD_ARGS;
  }

  if (!args[0]->IsString()) {
    TYPE_ERROR("who must be a string containing either `self` or `children`");
  }
  String::Utf8Value whoS(args[0]);
  const char *who = *whoS;

  if (!strcmp(who, "self")) {
    msg = launch_data_new_string(LAUNCH_KEY_GETRUSAGESELF);
  } else {
    msg = launch_data_new_string(LAUNCH_KEY_GETRUSAGECHILDREN);
  }
  resp = launch_msg(msg);
  launch_data_free(msg);

  if (resp == NULL) {
    Local<Value> e = LaunchDException(errno, strerror(errno), NULL);
    ThrowException(e);
  } else if (launch_data_get_type(resp) == LAUNCH_DATA_ERRNO) {
    r = launch_data_get_errno(resp);
    Local<Value> e = LaunchDException(r, strerror(r), NULL);
    ThrowException(e);
  } else if (launch_data_get_type(resp) == LAUNCH_DATA_OPAQUE){
    struct rusage *rusage = (struct rusage *)launch_data_get_opaque(resp);
    Local<Object> output = Object::New();
    double usertimeused = (double)rusage->ru_utime.tv_sec + (double)rusage->ru_utime.tv_usec / (double)1000000;
    output->Set(N_STRING("user_time_used"), N_NUMBER(usertimeused));
    double systemtimeused = (double)rusage->ru_stime.tv_sec + (double)rusage->ru_stime.tv_usec / (double)1000000;
    output->Set(N_STRING("system_time_used"), N_NUMBER(systemtimeused));

    output->Set(N_STRING("max_resident_set_size"), N_NUMBER(rusage->ru_maxrss));
    output->Set(N_STRING("shared_text_memory_size"), N_NUMBER(rusage->ru_ixrss));
    output->Set(N_STRING("unshared_data_size"), N_NUMBER(rusage->ru_idrss));
    output->Set(N_STRING("unshared_stack_size"), N_NUMBER(rusage->ru_isrss));
    output->Set(N_STRING("page_reclaims"), N_NUMBER(rusage->ru_minflt));
    output->Set(N_STRING("page_faults"), N_NUMBER(rusage->ru_majflt));
    output->Set(N_STRING("swaps"), N_NUMBER(rusage->ru_nswap));
    output->Set(N_STRING("block_input_operations"), N_NUMBER(rusage->ru_inblock));
    output->Set(N_STRING("block_output_operations"), N_NUMBER(rusage->ru_oublock));
    output->Set(N_STRING("messages_sent"), N_NUMBER(rusage->ru_msgsnd));
    output->Set(N_STRING("messages_received"), N_NUMBER(rusage->ru_msgrcv));
    output->Set(N_STRING("signals_received"), N_NUMBER(rusage->ru_nsignals));
    output->Set(N_STRING("voluntary_context_switches"), N_NUMBER(rusage->ru_nvcsw));
    output->Set(N_STRING("involuntary_context_switches"), N_NUMBER(rusage->ru_nivcsw));
    launch_data_free(resp);
    NanReturnValue(output);
  }
  Local<Value> e = LaunchDException(153, "EUNKRES", "Unknown response from launchd");
  ThrowException(e);
  NanReturnValue(N_NUMBER(153));
}

NAN_METHOD(Umask) {
  NanScope();


  if (args.Length() > 1) {
    THROW_BAD_ARGS;
  }

  if (args.Length() == 1) {
    // Set
    if (!args[0]->IsString()) {
      TYPE_ERROR("Umask must be a string");
    }
    String::Utf8Value umaskS(args[0]);
		const char *u = *umaskS;
		int res = launchctl_setumask(u);
		if (res != 0) {
			Local<Value> e = LaunchDException(res, strerror(res), NULL);
			ThrowException(e);
		}
    NanReturnValue(N_NUMBER(res));
  } else {
    // Get
		int64_t res = launchctl_getumask();
		if (res == -1) {
			Local<Value> e = LaunchDException(errno, strerror(errno), NULL);
			ThrowException(e);
		}
    NanReturnValue(N_NUMBER(res));
  }
}

NAN_METHOD(GetEnv) {
	NanScope();
	launch_data_t resp;

	if (vproc_swap_complex(NULL, VPROC_GSK_ENVIRONMENT, NULL, &resp) == NULL) {
		size_t i;
		if (LAUNCH_DATA_DICTIONARY != resp->type) {
			NanReturnValue(N_NUMBER(0));
		}
		Local<Object> output = Object::New();
		for (i=0; i<resp->_array_cnt; i+=2) {
			launch_data_t d = resp->_array[i+1];
			const char *k = resp->_array[i]->string;
			output->Set(N_STRING(k), N_STRING(launch_data_get_string(d)));
		}
		launch_data_free(resp);
		NanReturnValue(output);
	} else {
		NanReturnValue(N_NUMBER(0));
	}
}

void InitLaunchctl(Handle<Object> target) {
  NanScope();
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
	NODE_SET_METHOD(target, "setLimitSync", SetLimitSync);
	NODE_SET_METHOD(target, "setEnvVar", SetEnvVar);
	NODE_SET_METHOD(target, "unsetEnvVar", UnsetEnvVar);
	NODE_SET_METHOD(target, "getEnv", GetEnv);
	NODE_SET_METHOD(target, "getRUsage", GetRUsage);
  NODE_SET_METHOD(target, "umask", Umask);
}

//NODE_MODULE(launchctl, init);

} // namespace liblaunchctl