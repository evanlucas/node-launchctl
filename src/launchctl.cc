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
using namespace node;
using namespace v8;



extern "C" {
    
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
    
    typedef unsigned int vproc_flags_t;
    
    vproc_err_t vproc_swap_complex(vproc_t vp, vproc_gsk_t key, launch_data_t inval, launch_data_t *outval);
    
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
    
    struct GetAllJobsBaton {
        uv_work_t request;
        launch_data_t resp;
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
    extern int * __error(void);
    #define errno (*__error())
}


Local<Value> GetJobDetail(launch_data_t obj, const char *key) {
    size_t i, c;
    switch (launch_data_get_type(obj)) {
        case LAUNCH_DATA_STRING:
        {
            Local<Value> y = String::New(launch_data_get_string(obj));
            return y;
        }
            break;
        case LAUNCH_DATA_INTEGER:
        {
            Local<Value> y = Number::New(launch_data_get_integer(obj));
            return y;
        }
            break;
        case LAUNCH_DATA_REAL:
        {
            Local<Value> y = Number::New(launch_data_get_real(obj));
            return y;
        }
            break;
        case LAUNCH_DATA_BOOL:
        {
            Local<Value> y = launch_data_get_bool(obj) ? Number::New(1) : Number::New(0);
            return y;
        }
            break;
        case LAUNCH_DATA_ARRAY:
        {
            
            c = launch_data_array_get_count(obj);
            Local<Array> a = Array::New(c);
            for (i=0; i<c; i++) {
                Local<Value> y = GetJobDetail(launch_data_array_get_index(obj, i), NULL);
                a->Set(Number::New(i), y);
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
                q->Set(String::New(t), v);
            }
            return q;
        }
            break;
        case LAUNCH_DATA_FD:
        {
            Local<Value> s = String::New("file-descriptor-object");
            return s;
        }
            break;
        case LAUNCH_DATA_MACHPORT:
        {
            Local<Value> s = String::New("mach-port-object");
            return s;
        }
            break;
        default:
            return Number::New(0);
            break;
    }
}

// Gets a single job matching job label
Handle<Value> GetJobSync(const Arguments& args) {
    HandleScope scope;
    launch_data_t resp, msg = NULL;
    if (args.Length() != 1) {
        return ThrowException(Exception::Error(String::New("Invalid args")));
    }
    
    if (!args[0]->IsString()) {
        return ThrowException(Exception::TypeError(String::New("Job must be a string")));
    }
    
    String::Utf8Value job(args[0]);
    
    const char* label = *job;
    
    msg = launch_data_alloc(LAUNCH_DATA_DICTIONARY);
    launch_data_dict_insert(msg, launch_data_new_string(label), LAUNCH_KEY_GETJOB);
    
    resp = launch_msg(msg);
    launch_data_free(msg);
    
    if (resp == NULL) {
        return ThrowException(Exception::Error(String::New(strerror(errno))));
    }
    
    Local<Value> res = GetJobDetail(resp, NULL);
    launch_data_free(resp);
    return scope.Close(res);
    
    
}

// Get Job Worker
void GetJobWork(uv_work_t* req) {
    GetJobBaton *baton = static_cast<GetJobBaton *>(req->data);
    launch_data_t msg = NULL;
    const char *label = baton->label;
    msg = launch_data_alloc(LAUNCH_DATA_DICTIONARY);
    launch_data_dict_insert(msg, launch_data_new_string(label), LAUNCH_KEY_GETJOB);
    
    baton->resp = launch_msg(msg);
    launch_data_free(msg);
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
            Local<Value>::New(Null()),
            res
        };
        launch_data_free(baton->resp);
        TryCatch try_catch;
        baton->callback->Call(Context::GetCurrent()->Global(), 2, argv);
        if (try_catch.HasCaught()) {
            node::FatalException(try_catch);
        }
    } else {
        Local<Value> s = Exception::Error(String::New(strerror(baton->err)));
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
        return ThrowException(Exception::Error(String::New("Invalid args")));
    }
    
    if (!args[0]->IsString()) {
        return ThrowException(Exception::TypeError(String::New("Job must be a string")));
    }
    
    if (!args[1]->IsFunction()) {
        return ThrowException(Exception::TypeError(String::New("Callback must be a function")));
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
    if (vproc_swap_complex(NULL, VPROC_GSK_ALLJOBS, NULL, &resp) == NULL) {
        size_t i = 0;
        
        if (LAUNCH_DATA_DICTIONARY != resp->type) {
            return scope.Close(Array::New());
        }
        size_t count = resp->_array_cnt;
        Local<Array> output = Array::New(count);
        
        for (i=0; i<resp->_array_cnt; i+=2) {
            launch_data_t d = resp->_array[i+1];
            launch_data_t lo = launch_data_dict_lookup(d, LAUNCH_JOBKEY_LABEL);
            launch_data_t pido = launch_data_dict_lookup(d, LAUNCH_JOBKEY_PID);
            launch_data_t stato = launch_data_dict_lookup(d, LAUNCH_JOBKEY_LASTEXITSTATUS);
            const char *label = launch_data_get_string(lo);
            Local<Object> o = Object::New();
            Local<Value> nameX = String::New("label");
            Local<Value> nameY = String::New(label);
            o->Set(nameX, nameY);
            Local<Value> pidX = String::New("pid");
            Local<Value> statX = String::New("status");
            if (pido) {
                Local<Value> pidY = Number::New(launch_data_get_integer(pido));
                o->Set(pidX, pidY);
                Local<Value> statY = String::New("-");
                o->Set(statX, statY);
            } else if (stato) {
                int wstatus = (int)launch_data_get_integer(stato);
                Local<Value> pidY = String::New("-");
                o->Set(pidX, pidY);
                if (WIFEXITED(wstatus)) {
                    Local<Value> sY = Number::New(WEXITSTATUS(wstatus));
                    o->Set(statX, sY);
                } else if (WIFSIGNALED(wstatus)) {
                    Local<Value> sY = Number::New(WTERMSIG(wstatus));
                    o->Set(statX, sY);
                } else {
                    Local<Value> sY = String::New("-");
                    o->Set(statX, sY);
                }
            } else {
                Local<Value> pidY = String::New("-");
                o->Set(pidX, pidY);
                o->Set(statX, String::New("-"));
            }
            
            output->Set(Number::New(i), o);
        }
        
        launch_data_free(resp);
        return scope.Close(output);
    } else {
        return ThrowException(Exception::Error(String::New("vproc_swap_complex !== NULL")));
    }
}

// Get All Jobs Worker
void GetAllJobsWork(uv_work_t* req) {
    GetAllJobsBaton *baton = static_cast<GetAllJobsBaton *>(req->data);
    vproc_swap_complex(NULL, VPROC_GSK_ALLJOBS, NULL, &baton->resp);
}

// Get All Jobs Callback
void GetAllJobsAfterWork(uv_work_t* req) {
    GetAllJobsBaton *baton = static_cast<GetAllJobsBaton *>(req->data);
    
    if (LAUNCH_DATA_DICTIONARY != baton->resp->type) {
        Local<Value> res = Array::New();
        Local<Value> argv[2] = {
            Local<Value>::New(Null()),
            res
        };
        launch_data_free(baton->resp);
        TryCatch try_catch;
        baton->callback->Call(Context::GetCurrent()->Global(), 2, argv);
        if(try_catch.HasCaught()) {
            node::FatalException(try_catch);
        }
    } else {
        size_t count = baton->resp->_array_cnt;
        Local<Array> res = Array::New(count);
        int i = 0;
        for (i=0; i<count; i+=2) {
            launch_data_t d = baton->resp->_array[i+1];
            launch_data_t lo = launch_data_dict_lookup(d, LAUNCH_JOBKEY_LABEL);
            launch_data_t pido = launch_data_dict_lookup(d, LAUNCH_JOBKEY_PID);
            launch_data_t stato = launch_data_dict_lookup(d, LAUNCH_JOBKEY_LASTEXITSTATUS);
            const char *label = launch_data_get_string(lo);
            Local<Object> o = Object::New();
            o->Set(String::New("label"), String::New(label));
            if (pido) {
                o->Set(String::New("pid"), Number::New(launch_data_get_integer(pido)));
                o->Set(String::New("status"), String::New("-"));
            } else if (stato) {
                int wstatus = (int)launch_data_get_integer(stato);
                o->Set(String::New("pid"), String::New("-"));
                if (WIFEXITED(wstatus)) {
                    o->Set(String::New("status"), Number::New(WEXITSTATUS(wstatus)));
                } else if (WIFSIGNALED(wstatus)) {
                    o->Set(String::New("status"), Number::New(WTERMSIG(wstatus)));
                } else {
                    o->Set(String::New("status"), String::New("-"));
                }
            } else {
                o->Set(String::New("status"), String::New("-"));
                o->Set(String::New("pid"), String::New("-"));
            }
            
            res->Set(Number::New(i), o);
        }
        launch_data_free(baton->resp);
        Local<Value> argv[2] = {
            Local<Value>::New(Null()),
            res
        };
        
        TryCatch try_catch;
        baton->callback->Call(Context::GetCurrent()->Global(), 2, argv);
        if(try_catch.HasCaught()) {
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
    baton->resp = NULL;
    baton->callback = Persistent<Function>::New(Local<Function>::Cast(args[0]));
    
    uv_queue_work(uv_default_loop(), &baton->request, GetAllJobsWork, (uv_after_work_cb)GetAllJobsAfterWork);
    
    return Undefined();
}

// Start job with the given label
Handle<Value> StartJobSync(const Arguments& args) {
    HandleScope scope;
    if (args.Length() != 1) {
        return ThrowException(Exception::Error(String::New("Invalid args")));
    }
    
    if (!args[0]->IsString()) {
        return ThrowException(Exception::TypeError(String::New("Job label must be a string")));
    }
    
    String::Utf8Value job(args[0]);
    
    const char* label = *job;
    
    launch_data_t resp, msg = NULL;
    
    const char *lmsgcmd = LAUNCH_KEY_STARTJOB;
    
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

// Stop job with the given label
Handle<Value> StopJobSync(const Arguments& args) {
    HandleScope scope;
    if (args.Length() != 1) {
        return ThrowException(Exception::Error(String::New("Invalid args")));
    }
    
    if (!args[0]->IsString()) {
        return ThrowException(Exception::TypeError(String::New("Job label must be a string")));
    }
    
    String::Utf8Value job(args[0]);
    
    const char* label = *job;
    
    launch_data_t resp, msg = NULL;
    
    const char *lmsgcmd = LAUNCH_KEY_STOPJOB;
    
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



void init(Handle<Object> target) {
    target->Set(String::NewSymbol("getJob"), FunctionTemplate::New(GetJob)->GetFunction());
    target->Set(String::NewSymbol("getJobSync"), FunctionTemplate::New(GetJobSync)->GetFunction());
    target->Set(String::NewSymbol("getAllJobs"), FunctionTemplate::New(GetAllJobs)->GetFunction());
    target->Set(String::NewSymbol("getAllJobsSync"), FunctionTemplate::New(GetAllJobsSync)->GetFunction());
    target->Set(String::NewSymbol("startJobSync"), FunctionTemplate::New(StartJobSync)->GetFunction());
    target->Set(String::NewSymbol("stopJobSync"), FunctionTemplate::New(StopJobSync)->GetFunction());
}
NODE_MODULE(launchctl, init);
