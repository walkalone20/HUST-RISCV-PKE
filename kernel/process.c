/*
 * Utility functions for process management. 
 *
 * Note: in Lab1, only one process (i.e., our user application) exists. Therefore, 
 * PKE OS at this stage will set "current" to the loaded user application, and also
 * switch to the old "current" process after trap handling.
 */

#include "riscv.h"
#include "strap.h"
#include "config.h"
#include "process.h"
#include "elf.h"
#include "string.h"

#include "spike_interface/spike_utils.h"

//Two functions defined in kernel/usertrap.S
extern char smode_trap_vector[];
extern void return_to_user(trapframe*);

// current points to the currently running user-mode application.
process* current = NULL;

//
// switch to a user-mode process
//
void switch_to(process* proc) {
  assert(proc);
  current = proc;

  write_csr(stvec, (uint64)smode_trap_vector);
  // set up trapframe values that smode_trap_vector will need when
  // the process next re-enters the kernel.
  proc->trapframe->kernel_sp = proc->kstack;  // process's kernel stack
  proc->trapframe->kernel_trap = (uint64)smode_trap_handler;

  // set up the registers that strap_vector.S's sret will use
  // to get to user space.

  // set S Previous Privilege mode to User.
  unsigned long x = read_csr(sstatus);
  x &= ~SSTATUS_SPP;  // clear SPP to 0 for user mode
  x |= SSTATUS_SPIE;  // enable interrupts in user mode

  write_csr(sstatus, x);

  // set S Exception Program Counter to the saved user pc.
  write_csr(sepc, proc->trapframe->epc);

  // switch to user mode with sret.
  return_to_user(proc->trapframe);
}



// print the given line
void print_line(addr_line *line) {
  char filepath[1000], code[2000];
  struct stat temp;
  int l = strlen(current->dir[current->file[line->file].dir]);
  strcpy(filepath, current->dir[current->file[line->file].dir]); // copy the file path
  filepath[l] = '/';
  strcpy(filepath + l + 1, current->file[line->file].file); // concat the file name
  
  // read the code line and print
  spike_file_t *f = spike_file_open(filepath, O_RDONLY, 0);
  spike_file_stat(f, &temp);
  spike_file_read(f, code, temp.st_size);
  spike_file_close(f);
  int cnt = 0;
  for (int i = 0; i < temp.st_size;) {
    int j = i;
    while (j < temp.st_size && code[j] != '\n') j++;
    cnt++;
    if (cnt == line->line) {
      code[j] = '\0';
      sprint("Runtime error at %s:%d\n%s\n", filepath, line->line, code + i);
      break;
    }
    i = j + 1;
  }
}

void print_error_line() {
  uint64 mepc = read_csr(mepc);
  for (int i = 0; i < current->line_ind; i++) {
    if (mepc < current->line[i].addr) {
      print_line(current->line + i - 1);
      break;
    }
  }
}