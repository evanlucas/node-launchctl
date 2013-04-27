#include <v8.h>
#include <node.h>
extern "C" {
#include <sys/cdefs.h>
#include <sys/reboot.h>
#include <reboot2.h>
#include <launch.h>
#include <vproc.h>
//#include <CoreFoundation/CoreFoundation.h>
}
#include <stdint.h>
#include <stdlib.h>

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
    
    extern int * __error(void);
    #define errno (*__error())
}
void print_jobs(launch_data_t j, const char *key __attribute__((unused)), void *context __attribute__((unused))) {
	static size_t depth = 0;
	launch_data_t lo = launch_data_dict_lookup(j, LAUNCH_JOBKEY_LABEL);
	launch_data_t pido = launch_data_dict_lookup(j, LAUNCH_JOBKEY_PID);
	launch_data_t stato = launch_data_dict_lookup(j, LAUNCH_JOBKEY_LASTEXITSTATUS);
	const char *label = launch_data_get_string(lo);
	size_t i;
	if (pido) {
		fprintf(stdout, "%lld\t-\t%s\n", launch_data_get_integer(pido), label);
	} else if (stato) {
		int wstatus = (int)launch_data_get_integer(stato);
		if (WIFEXITED(wstatus)) {
			fprintf(stdout, "-\t%d\t%s\n", WEXITSTATUS(wstatus), label);
		} else if (WIFSIGNALED(wstatus)) {
			fprintf(stdout, "-\t-%d\t%s\n", WTERMSIG(wstatus), label);
		} else {
			fprintf(stdout, "-\t???\t%s\n", label);
		}
	} else {
		fprintf(stdout, "-\t-\t%s\n", label);
	}
	for (i = 0; i < depth; i++) {
		fprintf(stdout, "\t");
	}
}

Local<Value> GetJobDetail(launch_data_t obj, const char *key) {
    size_t i, c;
    
    
//    if (!key) key = "data";
    
//    Local<Value> k = String::New(key);
    
    switch (launch_data_get_type(obj)) {
        case LAUNCH_DATA_STRING:
        {
            //fprintf(stderr, "STRING: %s\n", launch_data_get_string(obj));
            Local<Value> y = String::New(launch_data_get_string(obj));
            //o->Set(k, y);
            return y;
        }
            break;
        case LAUNCH_DATA_INTEGER:
        {
            //fprintf(stderr, "INTEGER: %lld\n", launch_data_get_integer(obj));
            Local<Value> y = Number::New(launch_data_get_integer(obj));
            //o->Set(k, y);
            return y;
        }
            break;
        case LAUNCH_DATA_REAL:
        {
            //fprintf(stderr, "REAL: %f\n", launch_data_get_real(obj));
            Local<Value> y = Number::New(launch_data_get_real(obj));
            //o->Set(k, y);
            return y;
        }
            break;
        case LAUNCH_DATA_BOOL:
        {
            Local<Value> y = launch_data_get_bool(obj) ? Number::New(1) : Number::New(0);
            //o->Set(k, y);
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
            //o->Set(k, a);
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
            Local<Object> o = Object::New();
            Local<Value> k;
            if (key) {
                k = String::New(key);
            } else {
                k = String::New("data");
            }
            o->Set(k, q);
            return o;
        }
            break;
        default:
            return Number::New(0);
            break;
    }
}
Handle<Value> GetJob(const Arguments& args) {
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

Handle<Value> GetAllJobs(const Arguments& args) {
    HandleScope scope;
	launch_data_t resp, msg = NULL;
    if (vproc_swap_complex(NULL, VPROC_GSK_ALLJOBS, NULL, &resp) == NULL) {
        fprintf(stdout, "[getJobs]\n");

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
        return scope.Close(String::New("vproc_swap_complex returned NON-NULL\n"));
    }
}



void init(Handle<Object> target) {
    target->Set(String::NewSymbol("getJob"), FunctionTemplate::New(GetJob)->GetFunction());
    target->Set(String::NewSymbol("getAllJobs"), FunctionTemplate::New(GetAllJobs)->GetFunction());
}
NODE_MODULE(launchctl, init);
