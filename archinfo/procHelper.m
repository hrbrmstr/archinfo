#import <sys/sysctl.h>
#import <Foundation/Foundation.h>

#include "procHelper.h"

// archInfo() is due to the spelunking work of Patrick Wardle <https://www.patreon.com/posts/45121749>
NSString* archInfo(pid_t pid) {
    
  size_t size;
  cpu_type_t type = -1;
  int mib[CTL_MAXNAME] = {0};
  size_t length = CTL_MAXNAME;
  struct kinfo_proc procInfo = {0};
  if (noErr != sysctlnametomib("sysctl.proc_cputype", mib, &length)) return(@"unknown");

  mib[length] = pid;
  length++;
  size = sizeof(cpu_type_t);
  if (noErr != sysctl(mib, (u_int)length, &type, &size, 0, 0)) return(@"unknown");
  
  if (CPU_TYPE_X86_64 == type) return(@"x86_64");
  if (CPU_TYPE_ARM64 == type) {
    mib[0] = CTL_KERN;  //(re)init mib
    mib[1] = KERN_PROC;
    mib[2] = KERN_PROC_PID;
    mib[3] = pid;
    
    length = 4; //(re)set length
    
    size = sizeof(procInfo); //(re)set size
    
    if(noErr != sysctl(mib, (u_int)length, &procInfo, &size, NULL, 0)) return(@"amd64"); //get proc info
    
    //'P_TRANSLATED' set? set architecture to 'Intel'
    return( (P_TRANSLATED == (P_TRANSLATED & procInfo.kp_proc.p_flag)) ? @"x86_64" : @"amd64");
    
  }
  
  return(@"unknown");
  
}

// The below is modified code from: <https://gist.github.com/s4y/1173880/9ea0ed9b8a55c23f10ecb67ce288e09f08d9d1e5>
//
// Copyright (c) 2020 DeepTech, Inc.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated
// documentation files (the "Software"), to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and
// to permit persons to whom the Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
// TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//

NSArray* allProcesses(){
  
  static int maxArgumentSize = 0;
  
  if (maxArgumentSize == 0) {
    size_t size = sizeof(maxArgumentSize);
    if (sysctl((int[]) { CTL_KERN, KERN_ARGMAX }, 2, &maxArgumentSize, &size, NULL, 0) == -1) {
      perror("sysctl argument size");
      maxArgumentSize = 4096; // Default
    }
  }
  
  NSMutableArray *processes = [NSMutableArray array];
  int mib[3] = { CTL_KERN, KERN_PROC, KERN_PROC_ALL };
  struct kinfo_proc *info;
  size_t length;
  int count;
  
  if (sysctl(mib, 3, NULL, &length, NULL, 0) < 0) return nil;
  if (!(info = malloc(length))) return nil;
  if (sysctl(mib, 3, info, &length, NULL, 0) < 0) {
    free(info);
    return nil;
  }
  
  count = (int)(length / sizeof(struct kinfo_proc));
  
  for (int i = 0; i < count; i++) {
    pid_t pid = info[i].kp_proc.p_pid;
    if (pid == 0) continue;
    size_t size = maxArgumentSize;
    char* buffer = (char *)malloc(length);
    if (sysctl((int[]){ CTL_KERN, KERN_PROCARGS2, pid }, 3, buffer, &size, NULL, 0) == 0) {
      
      NSString* executable = [NSString stringWithCString:(buffer+sizeof(int)) encoding:NSUTF8StringEncoding];
      [processes addObject:[NSDictionary dictionaryWithObjectsAndKeys:
                            [NSNumber numberWithInt:pid], @"pid",
                            executable, @"executable",
                            archInfo(pid), @"arch",
                            nil]];
    }
    
    free(buffer);

  }
  
  free(info);
  
  return processes;
}

NSString *processListToJSON(NSArray * processListDict) {
  NSError * err;
  NSData * jsonData = [NSJSONSerialization dataWithJSONObject:processListDict options:0 error:&err];
  return([[NSString alloc] initWithData:jsonData encoding:NSUTF8StringEncoding]);
}
