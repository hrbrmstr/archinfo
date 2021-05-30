#include <unistd.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <mach/machine.h>
#include <sys/types.h>
#include <sys/sysctl.h>
#include <libgen.h>

#define VERSION "0.4.0"

#define noErr 0
#define DEFAULT_BUFFER_SIZE 4096
#define SYSCTL_ERROR 1

#define is_match(x) (x == 0)

enum fmt { JSON=0, COLUMNS };
enum show { ALL=0, ARM64, X86_64 };

static int max_arg_size = 0;

typedef struct ProcInfo {
  bool ok;
  char *name;
  char arch[10];  
} procinfo;

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
    
    //'P_TRANSLATED' set? set architecture to 'x86_64'
    return( (P_TRANSLATED == (P_TRANSLATED & procInfo.kp_proc.p_flag)) ? "x86_64" : "arm64");
    
  }

  return("unknown");

}

// retrieve process info

procinfo proc_info(pid_t pid) {
 
  size_t size = max_arg_size;
  procinfo p;
  int mib[3] = { CTL_KERN, KERN_PROCARGS2, pid };
  struct kinfo_proc *info;
  size_t length;
  int count;

  // get size for buffer
  (void)sysctl(mib, 3, NULL, &length, NULL, 0);

  char* buffer = (char *)calloc(length+32, sizeof(char)); // need +32 b/c this is busted on Big Sur

  // get the info
  if (sysctl(mib, 3, buffer, &size, NULL, 0) == 0) {
    
    // copy the info
    p.ok = TRUE;
    p.name = buffer;
    strncpy(p.arch, arch_info(pid), 10);

  } else {
    free(buffer);
    p.ok = FALSE;
  }

  return(p);

}

// output one line of process info with architecture info

void output_one(enum fmt output_type, pid_t pid, procinfo p, bool only_basename) {
  if (output_type == COLUMNS) {
    printf(
      "%7d %6s %s\n", 
      pid,
      p.arch, 
      (only_basename ? basename((char *)(p.name+sizeof(int))) : p.name+sizeof(int))
    );
  } else if (output_type == JSON) {
    printf(
      "{\"pid\":%d,\"arch\":\"%s\",\"name\":\"%s\"}\n", 
      pid, 
      p.arch, 
      (only_basename ? basename((char *)(p.name+sizeof(int))) : p.name+sizeof(int))
    );
  }
}

// walk through process list, get PID and name, then pass that on to output_one() to
// grab the architecture and do the actual output

int enumerate_processes(enum fmt output_type, enum show to_show, bool only_basename) {

  int mib[3] = { CTL_KERN, KERN_PROC, KERN_PROC_ALL };
  struct kinfo_proc *info;
  size_t length;
  int count;

  // get the running process list
  if (sysctl(mib, 3, NULL, &length, NULL, 0) < 0) return(SYSCTL_ERROR);

  // make some room for results
  if (!(info = calloc(length, sizeof(char)))) return(SYSCTL_ERROR);

  // get the results
  if (sysctl(mib, 3, info, &length, NULL, 0) < 0) {
    free(info);
    return(SYSCTL_ERROR);
  }

  // how many results?
  count = (int)(length / sizeof(struct kinfo_proc));
  
  // for each result
  for (int i = 0; i < count; i++) {

    pid_t pid = info[i].kp_proc.p_pid; // get PID

    if (pid == 0) continue;

    procinfo p = proc_info(pid); // get process info

    if (p.ok) {
      if (
           (to_show == ALL) || 
           ((to_show == ARM64) && is_match(strncmp("arm", p.arch, 3))) || 
           ((to_show == X86_64) && is_match(strncmp("x86", p.arch, 3)))
         ) {
        output_one(output_type, pid, p, only_basename);
      }
      free(p.name);
    }
      
  }
  
  free(info);
  
  return(0);

}

// call to display cmdline tool help

void help() {

  printf("archinfo %s\n", VERSION);
  printf("boB Rudis <bob@rud.is>\n");
  printf("\n");
  printf("archinfo outputs a list of running processes with architecture (x86_64/amd64) information\n");
  printf("\n");
  printf("USAGE:\n");
  printf("    archinfo [--arm|--x86] [--basename] [--columns|--json] [--pid #]\n");
  printf("\n");
  printf("FLAGS/OPTIONS:\n");
  printf("    --arm             Only show arm64 processes (default is to show both)\n");
  printf("    --x86             Only show x86_64 processes  (default is to show both)\n");
  printf("    --basename        Only show basename of process executable\n");
  printf("    --columns         Output process list in columns (default)\n");
  printf("    --json            Output process list in ndjson\n");
  printf("    --pid #           Output process architecture info for the specified process\n");
  printf("    --help            Display this help text\n");

}

void init() {
   
  if (max_arg_size == 0) {
    size_t size = sizeof(max_arg_size);
    if (sysctl((int[]) { CTL_KERN, KERN_ARGMAX }, 2, &max_arg_size, &size, NULL, 0) == -1) {
      perror("sysctl argument size");
      max_arg_size = DEFAULT_BUFFER_SIZE;
    }
  }

}

int main(int argc, char** argv) {

  int c;
  enum show to_show = ALL;
  bool show_help = FALSE;
  bool do_one = FALSE;
  bool only_basename = FALSE;
  pid_t pid = -1;
  enum fmt output_type = COLUMNS;

  while(true) {

    int this_option_optind = optind ? optind : 1;
    int option_index = 0;

    static struct option long_options[] = {
      { "arm",      no_argument,       0,  'a' },
      { "x86",      no_argument,       0,  'x' },
      { "basename", no_argument,       0,  'b' },
      { "json",     no_argument,       0,  'j' },
      { "columns",  no_argument,       0,  'c' },
      { "help",     no_argument,       0,  'h' },
      { "pid",      required_argument, 0,  'p' },
      { 0,          0,                 0,  0 }
    };

    c = getopt_long(argc, argv, "axbjchp:", long_options, &option_index);

    if (c == -1) break;

    switch(c) {

      case 'a': to_show = ARM64; break;
      case 'x': to_show = X86_64; break;
      case 'b': only_basename = TRUE; break;
      case 'p': do_one = TRUE; pid = atoi(optarg); break;
      case 'h': show_help = TRUE; break;
      case 'j': output_type = JSON; break;
      case 'c': output_type = COLUMNS; break;

    }

  }

  init();

  // only show help if --help is in the arg list

  if (show_help) {
    help();
    return(0);
  }

  // only do one process if --pid is in the arg list

  if (do_one) {
    if (pid > 0) {
      procinfo p = proc_info(pid);
      if (p.ok) {
        output_one(output_type, pid, p, only_basename);
        free(p.name);
        return(0);
      }
    }
    return(SYSCTL_ERROR);
  } 

 // otherwise do them all

  return(enumerate_processes(output_type, to_show, only_basename));

}
