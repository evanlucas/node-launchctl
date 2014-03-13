//
//  liblaunchctl_tests.m
//  liblaunchctl tests
//
//  Created by Evan Lucas on 6/21/13.
//  Copyright (c) 2013 Hattiesburg Clinic. All rights reserved.
//

#import <SenTestingKit/SenTestingKit.h>
#import "liblaunchctl.h"

@interface liblaunchctl_tests : SenTestCase
@end

@implementation liblaunchctl_tests

- (void)setUp
{
  [super setUp];
}

- (void)tearDown
{
  // Tear-down code here.
  
  [super tearDown];
}

int print_obj(launch_data_t job, const char *key, void *context) {
  static size_t indent = 0;
  size_t i,c;
  if (!job) {
    printf("Unknown job\n");
    return -1;
  }
  
  for (i=0; i<indent; i++) {
    fprintf(stdout, "\t");
  }
  
  if (key) {
    fprintf(stdout, "\"%s\" = ", key);
  }
  
  switch(launch_data_get_type(job)) {
		case LAUNCH_DATA_STRING:
			fprintf(stdout, "\"%s\";\n", launch_data_get_string(job));
			break;
		case LAUNCH_DATA_INTEGER:
			fprintf(stdout, "%lld;\n", launch_data_get_integer(job));
			break;
		case LAUNCH_DATA_REAL:
			fprintf(stdout, "%f;\n", launch_data_get_real(job));
			break;
		case LAUNCH_DATA_BOOL:
			fprintf(stdout, "%s;\n", launch_data_get_bool(job) ? "true" : "false");
			break;
		case LAUNCH_DATA_ARRAY:
			c = launch_data_array_get_count(job);
			fprintf(stdout, "(\n");
			indent++;
			for (i=0; i<c; i++) {
				print_obj(launch_data_array_get_index(job, i), NULL, NULL);
			}
			indent--;
			for (i=0; i<indent; i++) {
				fprintf(stdout, "\t");
			}
			fprintf(stdout, ");\n");
			break;
		case LAUNCH_DATA_DICTIONARY:
			fprintf(stdout, "{\n");
			indent++;
			launch_data_dict_iterate(job, print_obj, NULL);
			indent--;
			for (i=0; i<indent; i++) {
				fprintf(stdout, "\t");
			}
			fprintf(stdout, "};\n");
			break;
		case LAUNCH_DATA_FD:
			fprintf(stdout, "file-descriptor-object;\n");
			break;
		case LAUNCH_DATA_MACHPORT:
			fprintf(stdout, "mach-port-object;\n");
			break;
		default:
			fprintf(stdout, "???;\n");
			break;
	}
  
  return 0;
}

- (void)testListDock {
  launch_data_t res = launchctl_list_job("com.apple.Dock.agent");
  if (!res) {
    STFail(@"Invalid job");
  }
  print_obj(res, NULL, NULL);
  launch_data_free(res);
}

- (void)testFakeJob {
  launch_data_t res = launchctl_list_job("com.thisisafakejob.test");
  if (!res) {
    NSLog(@"Invalid job");
  } else {
    STFail(@"Should be an invalid job");
  }
}

- (void)testStartFakeJob {
  int res = launchctl_start_job("com.thisisafakejob.test");
  if (res == 0) {
    STFail(@"Should return non-zero");
  }
  NSLog(@"%d: %s", res, strerror(res));
}

- (void)testStopFakeJob {
  int res = launchctl_stop_job("com.thisisafakejob.test");
  if (res == 0) {
    STFail(@"Should return non-zero");
  }
  NSLog(@"%d: %s", res, strerror(res));
}

- (void)testLoadFakeJob {
  int res = launchctl_load_job("/System/Library/LaunchDaemons/com.thisisafakejob.test.plist", false, false, NULL, NULL);
  if (res == 0) {
    STFail(@"Should return non-zero");
  }
  NSLog(@"%d: %s", res, strerror(res));
}

- (void)testGetManagerName {
  char *name = launchctl_get_managername();
  if (strcmp(name, "Aqua")) {
    STFail(@"Should return Aqua");
  }
  NSLog(@"%s", name);
}


@end
