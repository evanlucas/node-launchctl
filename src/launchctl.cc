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
#include <launch.h>
#include <vproc.h>
#include <NSSystemDirectories.h>
extern "C" {
#include <liblaunchctl.h>
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
    launch_data_t msg;
    launch_data_t resp;
    int err;
    node_launchctl_action_t action;
    Persistent<Function> callback;
  };
}

Local<Value> GetJobDetail(launch_data_t obj, const char *key) {
  size_t i, c;
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
      return N_NUMBER(0);
      break;
  }
}

// Gets a single job matching job label
Handle<Value> GetJobSync(const Arguments& args) {
  HandleScope scope;
  launch_data_t result = NULL;
  if (args.Length() != 1) {
    return ThrowException(Exception::Error(N_STRING("Invalid args")));
  }
  
  if (!args[0]->IsString()) {
    return ThrowException(Exception::TypeError(N_STRING("Job must be a string")));
  }
  
  String::Utf8Value job(args[0]);
  
  const char* label = *job;
  result = launchctl_list_job(label);
  if (result == NULL) {
    return ThrowException(Exception::Error(N_STRING(strerror(errno))));
  }
  Local<Value> res = GetJobDetail(result, NULL);
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
    Handle<Value> argv[2] = {
      N_NULL,
      res
    };
    launch_data_free(baton->resp);
    TryCatch try_catch;
    baton->callback->Call(Context::GetCurrent()->Global(), 2, argv);
    if (try_catch.HasCaught()) {
      node::FatalException(try_catch);
    }
  } else {
    Local<Value> s = Exception::Error(N_STRING(strerror(baton->err)));
    Handle<Value> argv[1] = {
      s
    };
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
  jobsl s = launchctl_list_jobs();
  int count = s->count;
  Handle<Array> output = Array::New(count);
  for (int i=0; i<count; i++) {
    lstatus job = &s->jobs[i];
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
  //vproc_swap_complex(NULL, VPROC_GSK_ALLJOBS, NULL, &baton->resp);
  jobs_list_t s = launchctl_list_jobs();
  baton->jobs = s;
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
      lstatus job = &jobs->jobs[i];
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
    TryCatch try_catch;
    baton->callback->Call(Context::GetCurrent()->Global(), 2, argv);
    if (try_catch.HasCaught()) {
      node::FatalException(try_catch);
    }
  } else {
    Local<Value> s = Exception::Error(N_STRING(strerror(baton->err)));
    Handle<Value> argv[1] = {
      s
    };
    launch_data_free(baton->resp);
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
  if (args.Length() != 1 || !args[0]->IsFunction()) {
    return ThrowException(Exception::Error(String::New("Requires callback and must be a function")));
  }
  
  GetAllJobsBaton *baton = new GetAllJobsBaton;
  baton->request.data = baton;
  baton->err = 0;
  baton->callback = Persistent<Function>::New(Local<Function>::Cast(args[0]));
  
  uv_queue_work(uv_default_loop(), &baton->request, GetAllJobsWork, (uv_after_work_cb)GetAllJobsAfterWork);
  
  return Undefined();
}

Handle<Value> GetLastError(const Arguments& args) {
  HandleScope scope;
  Handle<Object> ret = Object::New();
  ret->Set(N_STRING("errno"), N_NUMBER(errno));
  ret->Set(N_STRING("strerror"), N_STRING(strerror(errno)));
  return scope.Close(ret);
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
  
  launch_data_t resp, msg = NULL;
  
  node_launchctl_action_t cmd_v = (node_launchctl_action_t)args[1]->Int32Value();
  
  const char *lmsgcmd;
  switch (cmd_v) {
    case NODE_LAUNCHCTL_CMD_START:
      lmsgcmd = LAUNCH_KEY_STARTJOB;
      break;
    case NODE_LAUNCHCTL_CMD_STOP:
      lmsgcmd = LAUNCH_KEY_STOPJOB;
      break;
    case NODE_LAUNCHCTL_CMD_REMOVE:
      lmsgcmd = LAUNCH_KEY_REMOVEJOB;
      break;
    default:
      return TYPE_ERROR("Invalid command");
      break;
  }
  
  
  msg = launch_data_alloc(LAUNCH_DATA_DICTIONARY);
  launch_data_dict_insert(msg, launch_data_new_string(label), lmsgcmd);
  
  resp = launch_msg(msg);
  launch_data_free(msg);
  int e, r = 0;
  Local<Object> ret = Object::New();
  
  if (resp == NULL) {
    launch_data_free(resp);
    Local<Value> x = String::New("status");
    ret->Set(x, String::New("error"));
    ret->Set(String::New("message"), String::New(strerror(errno)));
    return scope.Close(ret);
  } else if (launch_data_get_type(resp) == LAUNCH_DATA_ERRNO) {
    if ((e = launch_data_get_errno(resp))) {
      launch_data_free(resp);
      ret->Set(String::New("status"), String::New("error"));
      ret->Set(String::New("message"), String::New(strerror(e)));
      return scope.Close(ret);
    }
  } else {
    launch_data_free(resp);
    ret->Set(String::New("status"), String::New("error"));
    ret->Set(String::New("message"), String::New("launchd returned unknown response"));
    return scope.Close(ret);
  }
  
  return scope.Close(Number::New(r));
  
}

void StartStopRemoveWork(uv_work_t *req) {
  SSRBaton *baton = static_cast<SSRBaton *>(req->data);
  const char *lmsgcmd;
  switch (baton->action) {
    case NODE_LAUNCHCTL_CMD_START:
      lmsgcmd = LAUNCH_KEY_STARTJOB;
      break;
    case NODE_LAUNCHCTL_CMD_STOP:
      lmsgcmd = LAUNCH_KEY_STOPJOB;
      break;
    case NODE_LAUNCHCTL_CMD_REMOVE:
      lmsgcmd = LAUNCH_KEY_REMOVEJOB;
      break;
    default:
      break;
  }
  baton->msg = launch_data_alloc(LAUNCH_DATA_DICTIONARY);
  launch_data_dict_insert(baton->msg, launch_data_new_string(baton->label), lmsgcmd);
  baton->resp = launch_msg(baton->msg);
  launch_data_free(baton->msg);
}

void StartStopRemoveAfterWork(uv_work_t *req) {
  HandleScope scope;
  SSRBaton *baton = static_cast<SSRBaton *>(req->data);
  int e = 0;
  if (baton->resp == NULL) {
    Local<Object> ret = Object::New();
    launch_data_free(baton->resp);
    Local<Value> argv[1] = {
      Exception::Error(String::New(strerror(errno)))
    };
    TryCatch try_catch;
    baton->callback->Call(Context::GetCurrent()->Global(), 1, argv);
    if (try_catch.HasCaught()) {
      node::FatalException(try_catch);
    }
  } else if (launch_data_get_type(baton->resp) == LAUNCH_DATA_ERRNO) {
    if ((e = launch_data_get_errno(baton->resp))) {
      launch_data_free(baton->resp);
      Local<Value> argv[1] = {
        Exception::Error(String::New(strerror(e)))
      };
      TryCatch try_catch;
      baton->callback->Call(Context::GetCurrent()->Global(), 1, argv);
      if (try_catch.HasCaught()) {
        node::FatalException(try_catch);
      }
    } else {
      Local<Value> argv[1] = {
        Local<Value>::New(Null())
      };
      TryCatch try_catch;
      baton->callback->Call(Context::GetCurrent()->Global(), 1, argv);
      if (try_catch.HasCaught()) {
        node::FatalException(try_catch);
      }
    }
  } else {
    launch_data_free(baton->resp);
    Local<Value> argv[1] = {
      Exception::Error(String::New("Unknown response"))
    };
    TryCatch try_catch;
    baton->callback->Call(Context::GetCurrent()->Global(), 1, argv);
    if (try_catch.HasCaught()) {
      node::FatalException(try_catch);
    }
  }
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
  baton->resp = NULL;
  baton->label = label;
  baton->msg = NULL;
  baton->action = cmd_v;
  baton->err = 0;
  baton->callback = Persistent<Function>::New(Local<Function>::Cast(args[2]));
  uv_queue_work(uv_default_loop(), &baton->request, StartStopRemoveWork, (uv_after_work_cb)StartStopRemoveAfterWork);
  return Undefined();
}

void init(Handle<Object> target) {
  target->Set(String::NewSymbol("getJob"), FunctionTemplate::New(GetJob)->GetFunction());
  target->Set(String::NewSymbol("getJobSync"), FunctionTemplate::New(GetJobSync)->GetFunction());
  target->Set(String::NewSymbol("getAllJobs"), FunctionTemplate::New(GetAllJobs)->GetFunction());
  target->Set(String::NewSymbol("getAllJobsSync"), FunctionTemplate::New(GetAllJobsSync)->GetFunction());
  target->Set(String::NewSymbol("startStopRemoveSync"), FunctionTemplate::New(StartStopRemoveSync)->GetFunction());
  target->Set(String::NewSymbol("startStopRemove"), FunctionTemplate::New(StartStopRemove)->GetFunction());
  target->Set(String::NewSymbol("getLastError"), FunctionTemplate::New(GetLastError)->GetFunction());
}
NODE_MODULE(launchctl, init);
