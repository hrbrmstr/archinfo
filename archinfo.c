#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mach/machine.h>
#include <sys/types.h>
#include <sys/sysctl.h>

#define noErr 0
#define VERSION "0.2.0"

enum fmt { JSON=0, COLUMNS };

// arch_info() is due to the spelunking work of Patrick Wardle <https://www.patreon.com/posts/45121749>
static char *arch_info(pid_t pid) {

  size_t size;
  cpu_type_t type = -1;
  int mib[CTL_MAXNAME] = {0};
  size_t length = CTL_MAXNAME;
  struct kinfo_proc procInfo = {0};

  if (noErr != sysctlnametomib("sysctl.proc_cputype", mib, &length)) return("unknown");

  mib[length] = pid;
  length++;
  size = sizeof(cpu_type_t);

  if (noErr != sysctl(mib, (u_int)length, &type, &size, 0, 0)) return("unknown");
  
  if (CPU_TYPE_X86_64 == type) return("x86_64");

  if (CPU_TYPE_ARM64 == type) {

    mib[0] = CTL_KERN;  //(re)init mib
    mib[1] = KERN_PROC;
    mib[2] = KERN_PROC_PID;
    mib[3] = pid;
    
    length = 4; //(re)set length
    
    size = sizeof(procInfo); //(re)set size
    
    if(noErr != sysctl(mib, (u_int)length, &procInfo, &size, NULL, 0)) return("arm64"); //get proc info
    
    //'P_TRANSLATED' set? set architecture to 'Intel'
    return( (P_TRANSLATED == (P_TRANSLATED & procInfo.kp_proc.p_flag)) ? "x86_64" : "arm64");
    
  }

  return("unknown");

}

// The below is modified code from: <https://gist.github.com/s4y/1173880/9ea0ed9b8a55c23f10ecb67ce288e09f08d9d1e5>
// and is Copyright (c) 2020 DeepTech, Inc. (MIT License).
// 
// The code below the standard way to do this and I originally only copied it due to some Objective-C components.
// While those components are no longer in use, the ACK stays b/c I'm thankful I didn't have to write Objective-C
// for the initial version :-)

int enumerate_processes(enum fmt output_type) {

  static int maxArgumentSize = 0;
  
  if (maxArgumentSize == 0) {
    size_t size = sizeof(maxArgumentSize);
    if (sysctl((int[]) { CTL_KERN, KERN_ARGMAX }, 2, &maxArgumentSize, &size, NULL, 0) == -1) {
      perror("sysctl argument size");
      maxArgumentSize = 4096; // Default
    }
  }

  int mib[3] = { CTL_KERN, KERN_PROC, KERN_PROC_ALL };
  struct kinfo_proc *info;
  size_t length;
  int count;

  if (sysctl(mib, 3, NULL, &length, NULL, 0) < 0) return(1);

  if (!(info = malloc(length))) return(1);

  if (sysctl(mib, 3, info, &length, NULL, 0) < 0) {
    free(info);
    return(1);
  }

  count = (int)(length / sizeof(struct kinfo_proc));
  
  for (int i = 0; i < count; i++) {

    pid_t pid = info[i].kp_proc.p_pid;

    if (pid == 0) continue;

    size_t size = maxArgumentSize;

    char* buffer = (char *)malloc(length);

    if (sysctl((int[]){ CTL_KERN, KERN_PROCARGS2, pid }, 3, buffer, &size, NULL, 0) == 0) {
      if (output_type == COLUMNS) {
        printf("%7d %6s %s\n", pid, arch_info(pid), buffer+sizeof(int));
      } else if (output_type == JSON) {
        printf("{\"pid\":%d,\"arch\":\"%s\",\"name\":\"%s\"}\n", pid, arch_info(pid), buffer+sizeof(int));
      }
    }
    
    free(buffer);

  }
  
  free(info);
  
  return(0);

}

void help() {

  printf("archinfo %s\n", VERSION);
  printf("boB Rudis <bob@rud.is>\n");
  printf("\n");
  printf("archinfo outputs a list of running processes with the architecture (x86_64/amd64)\n");
  printf("\n");
  printf("USAGE:\n");
  printf("    archinfo [FLAG]\n");
  printf("\n");
  printf("FLAG:\n");
  printf("    --json            Output process list in ndjson\n");
  printf("    --columns         Output process list in columns\n");
  printf("    --help            Display this help text\n");

}

int main(int argc, char** argv) {

  if (argc >= 2) {
    if (strcmp("--json", argv[1]) == 0) {
      return(enumerate_processes(JSON));      
    } else if (strcmp("--columns", argv[1]) == 0) {
      return(enumerate_processes(COLUMNS));
    } else if (strcmp("--help", argv[1]) == 0) {
      help();
      return(0);
    }
  } else {
      return(enumerate_processes(COLUMNS));
  }

}
