/*
Copyright 2013-present Barefoot Networks, Inc.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/
#include "crash.h"

#include <config.h>
#include <errno.h>
#if HAVE_EXECINFO_H
#include <execinfo.h>
#endif
#include <fcntl.h>
#include <limits.h>
#include <signal.h>
#include <string.h>

#include <sys/wait.h>
#if HAVE_UCONTEXT_H
#include <ucontext.h>
#endif
#include <unistd.h>
#if HAVE_LIBBACKTRACE
#include <backtrace.h>
#endif
#if HAVE_CXXABI_H
#include <cxxabi.h>
#endif

#include <iostream>

#include "exceptions.h"
#include "exename.h"
#include "hex.h"
#include "log.h"

static const char *signames[] = {
    "NONE", "HUP",  "INT",  "QUIT", "ILL",    "TRAP",   "ABRT",  "BUS",  "FPE",  "KILL", "USR1",
    "SEGV", "USR2", "PIPE", "ALRM", "TERM",   "STKFLT", "CHLD",  "CONT", "STOP", "TSTP", "TTIN",
    "TTOU", "URG",  "XCPU", "XFSZ", "VTALRM", "PROF",   "WINCH", "POLL", "PWR",  "SYS"};

#ifdef MULTITHREAD
#include <pthread.h>

#include <mutex>
std::vector<pthread_t> thread_ids;
__thread int my_id;

void register_thread() {
    static std::mutex lock;
    std::lock_guard<std::mutex> acquire(lock);
    my_id = thread_ids.size();
    thread_ids.push_back(pthread_self());
}
#define MTONLY(...) __VA_ARGS__
#else
#define MTONLY(...)
#endif  // MULTITHREAD

#if HAVE_LIBBACKTRACE
struct backtrace_state *global_backtrace_state = nullptr;
#endif

static MTONLY(__thread) int shutdown_loop = 0;  // avoid infinite loop if shutdown crashes

static void sigint_shutdown(int sig, siginfo_t *, void *) {
    if (shutdown_loop++) _exit(-1);
    LOG1("Exiting with SIG" << signames[sig]);
    _exit(sig + 0x80);
}

#if HAVE_LIBBACKTRACE
static int backtrace_log(void *, uintptr_t pc, const char *fname, int lineno, const char *func) {
    char *demangled = nullptr;
#if HAVE_CXXABI_H
    int status;
    demangled = func ? abi::__cxa_demangle(func, 0, 0, &status) : nullptr;
#endif
    LOG1("  0x" << hex(pc) << " " << (demangled ? demangled : func ? func : "??"));
    free(demangled);
    if (fname) {
        LOG1("    " << fname << ":" << lineno);
    }
    return 0;
}
static void backtrace_error(void *, const char *msg, int) { perror(msg); }

#elif HAVE_EXECINFO_H
/*
 * call external program addr2line WITHOUT using malloc or stdio or anything
 * else that might be problematic if there's memory corruption or exhaustion
 */
const char *addr2line(void *addr, const char *text) {
    MTONLY(static std::mutex lock; std::lock_guard<std::mutex> acquire(lock);)
    static pid_t child = 0;
    static int to_child, from_child;
    static char binary[PATH_MAX];
    static char buffer[PATH_MAX];
    const char *t;

    if (!text || !(t = strchr(text, '('))) {
        text = exename();
        t = text + strlen(text);
    }
    memcpy(buffer, text, t - text);
    buffer[t - text] = 0;
    if (child && strcmp(binary, buffer)) {
        child = 0;
        close(to_child);
        close(from_child);
    }
    memcpy(binary, buffer, (t - text) + 1);
    text = binary;
    if (!child) {
        int pfd1[2], pfd2[2];
        char *p = buffer;
        const char *argv[4] = {"/bin/sh", "-c", buffer, 0};
        strcpy(p, "addr2line ");  // NOLINT
        p += strlen(p);
        strcpy(p, " -Cfspe ");  // NOLINT
        p += strlen(p);
        t = text + strlen(text);
        if (!memchr(text, '/', t - text)) {
            strcpy(p, "$(which ");  // NOLINT
            p += strlen(p);
        }
        memcpy(p, text, t - text);
        p += t - text;
        if (!memchr(text, '/', t - text)) *p++ = ')';
        *p = 0;
        child = -1;
#if HAVE_PIPE2
        if (pipe2(pfd1, O_CLOEXEC) < 0) return 0;
        if (pipe2(pfd2, O_CLOEXEC) < 0) return 0;
#else
        if (pipe(pfd1) < 0) return 0;
        if (pipe(pfd2) < 0) return 0;
        fcntl(pfd1[0], F_SETFD, FD_CLOEXEC | fcntl(pfd1[0], F_GETFD));
        fcntl(pfd1[1], F_SETFD, FD_CLOEXEC | fcntl(pfd1[1], F_GETFD));
        fcntl(pfd2[0], F_SETFD, FD_CLOEXEC | fcntl(pfd2[0], F_GETFD));
        fcntl(pfd2[1], F_SETFD, FD_CLOEXEC | fcntl(pfd2[1], F_GETFD));
#endif
        while ((child = fork()) == -1 && errno == EAGAIN) {
        }
        if (child == -1) return 0;
        if (child == 0) {
            dup2(pfd1[1], 1);
            dup2(pfd1[1], 2);
            dup2(pfd2[0], 0);
            execvp(argv[0], (char *const *)argv);
            _exit(-1);
        }
        close(pfd1[1]);
        from_child = pfd1[0];
        close(pfd2[0]);
        to_child = pfd2[1];
    }
    if (child == -1) return 0;
    char *p = buffer;
    uintptr_t a = (uintptr_t)addr;
    int shift = (CHAR_BIT * sizeof(uintptr_t) - 1) & ~3;
    while (shift > 0 && (a >> shift) == 0) shift -= 4;
    while (shift >= 0) {
        *p++ = "0123456789abcdef"[(a >> shift) & 0xf];
        shift -= 4;
    }
    *p++ = '\n';
    auto _unused = write(to_child, buffer, p - buffer);
    (void)_unused;
    p = buffer;
    int len;
    while (p < buffer + sizeof(buffer) - 1 &&
           (len = read(from_child, p, buffer + sizeof(buffer) - p - 1)) > 0 && (p += len) &&
           !memchr(p - len, '\n', len)) {
    }
    *p = 0;
    if ((p = strchr(buffer, '\n'))) *p = 0;
    if (buffer[0] == 0 || buffer[0] == '?') return 0;
    return buffer;
}
#endif /* HAVE_EXECINFO_H */

#if HAVE_UCONTEXT_H

#if defined(__CYGWIN__)
#define REGNAME(regname) regname
#else
#define REGNAME(regname) mc_##regname
#endif

static void dumpregs(mcontext_t *mctxt) {
#if defined(REG_EAX)
    LOG1(" eax=" << hex(mctxt->gregs[REG_EAX], 8, '0')
                 << " ebx=" << hex(mctxt->gregs[REG_EBX], 8, '0')
                 << " ecx=" << hex(mctxt->gregs[REG_ECX], 8, '0')
                 << " edx=" << hex(mctxt->gregs[REG_EDX], 8, '0'));
    LOG1(" edi=" << hex(mctxt->gregs[REG_EDI], 8, '0')
                 << " esi=" << hex(mctxt->gregs[REG_ESI], 8, '0')
                 << " ebp=" << hex(mctxt->gregs[REG_EBP], 8, '0')
                 << " esp=" << hex(mctxt->gregs[REG_ESP], 8, '0'));
#elif defined(REG_RAX)
    LOG1(" rax=" << hex(mctxt->gregs[REG_RAX], 16, '0')
                 << " rbx=" << hex(mctxt->gregs[REG_RBX], 16, '0')
                 << " rcx=" << hex(mctxt->gregs[REG_RCX], 16, '0'));
    LOG1(" rdx=" << hex(mctxt->gregs[REG_RDX], 16, '0')
                 << " rdi=" << hex(mctxt->gregs[REG_RDI], 16, '0')
                 << " rsi=" << hex(mctxt->gregs[REG_RSI], 16, '0'));
    LOG1(" rbp=" << hex(mctxt->gregs[REG_RBP], 16, '0')
                 << " rsp=" << hex(mctxt->gregs[REG_RSP], 16, '0')
                 << "  r8=" << hex(mctxt->gregs[REG_R8], 16, '0'));
    LOG1("  r9=" << hex(mctxt->gregs[REG_R9], 16, '0')
                 << " r10=" << hex(mctxt->gregs[REG_R10], 16, '0')
                 << " r11=" << hex(mctxt->gregs[REG_R11], 16, '0'));
    LOG1(" r12=" << hex(mctxt->gregs[REG_R12], 16, '0')
                 << " r13=" << hex(mctxt->gregs[REG_R13], 16, '0')
                 << " r14=" << hex(mctxt->gregs[REG_R14], 16, '0'));
    LOG1(" r15=" << hex(mctxt->gregs[REG_R15], 16, '0'));
#elif defined(__i386__)
    LOG1(" eax=" << hex(mctxt->REGNAME(eax), 8, '0') << " ebx=" << hex(mctxt->REGNAME(ebx), 8, '0')
                 << " ecx=" << hex(mctxt->REGNAME(ecx), 8, '0')
                 << " edx=" << hex(mctxt->REGNAME(edx), 8, '0'));
    LOG1(" edi=" << hex(mctxt->REGNAME(edi), 8, '0') << " esi=" << hex(mctxt->REGNAME(esi), 8, '0')
                 << " ebp=" << hex(mctxt->REGNAME(ebp), 8, '0')
                 << " esp=" << hex(mctxt->REGNAME(esp), 8, '0'));
#elif defined(__amd64__)
    LOG1(" rax=" << hex(mctxt->REGNAME(rax), 16, '0')
                 << " rbx=" << hex(mctxt->REGNAME(rbx), 16, '0')
                 << " rcx=" << hex(mctxt->REGNAME(rcx), 16, '0'));
    LOG1(" rdx=" << hex(mctxt->REGNAME(rdx), 16, '0')
                 << " rdi=" << hex(mctxt->REGNAME(rdi), 16, '0')
                 << " rsi=" << hex(mctxt->REGNAME(rsi), 16, '0'));
    LOG1(" rbp=" << hex(mctxt->REGNAME(rbp), 16, '0')
                 << " rsp=" << hex(mctxt->REGNAME(rsp), 16, '0')
                 << "  r8=" << hex(mctxt->REGNAME(r8), 16, '0'));
    LOG1("  r9=" << hex(mctxt->REGNAME(r9), 16, '0') << " r10=" << hex(mctxt->REGNAME(r10), 16, '0')
                 << " r11=" << hex(mctxt->REGNAME(r11), 16, '0'));
    LOG1(" r12=" << hex(mctxt->REGNAME(r12), 16, '0')
                 << " r13=" << hex(mctxt->REGNAME(r13), 16, '0')
                 << " r14=" << hex(mctxt->REGNAME(r14), 16, '0'));
    LOG1(" r15=" << hex(mctxt->REGNAME(r15), 16, '0'));
#else
#warning "unknown machine type"
#endif
}
#endif

static void crash_shutdown(int sig, siginfo_t *info, void *uctxt) {
    if (shutdown_loop++) _exit(-1);
    MTONLY(static std::recursive_mutex lock; static int threads_dumped = 0;
           static bool killed_all_threads = false; lock.lock(); if (!killed_all_threads) {
               killed_all_threads = true;
               for (int i = 0; i < int(thread_ids.size()); i++)
                   if (i != my_id - 1) {
                       pthread_kill(thread_ids[i], SIGABRT);
                   }
           })
    LOG1(MTONLY("Thread #" << my_id << " " <<) "exiting with SIG" << signames[sig] << ", trace:");
    if (sig == SIGILL || sig == SIGFPE || sig == SIGSEGV || sig == SIGBUS || sig == SIGTRAP)
        LOG1("  address = " << hex(info->si_addr));
#if HAVE_UCONTEXT_H
    dumpregs(&(static_cast<ucontext_t *>(uctxt)->uc_mcontext));
#else
    (void)uctxt;  // Suppress unused parameter warning.
#endif

#if HAVE_LIBBACKTRACE
    if (LOGGING(1)) {
        backtrace_full(global_backtrace_state, 1, backtrace_log, backtrace_error, nullptr);
    }
#elif HAVE_EXECINFO_H
    if (LOGGING(1)) {
        static void *buffer[64];
        int size = backtrace(buffer, 64);
        char **strings = backtrace_symbols(buffer, size);
        for (int i = 1; i < size; i++) {
            if (strings) LOG1("  " << strings[i]);
            if (const char *line = addr2line(buffer[i], strings ? strings[i] : 0))
                LOG1("    " << line);
        }
        if (size < 1) LOG1("backtrace failed");
        free(strings);
    }
#endif
    MTONLY(
        if (++threads_dumped < int(thread_ids.size())) {
            lock.unlock();
            pthread_exit(0);
        } else { lock.unlock(); })
    if (sig != SIGABRT) BUG("Exiting with SIG%s", signames[sig]);
    _exit(sig + 0x80);
}

void setup_signals() {
    struct sigaction sigact;
    sigact.sa_sigaction = sigint_shutdown;
    sigact.sa_flags = SA_SIGINFO;
    sigemptyset(&sigact.sa_mask);
    sigaction(SIGHUP, &sigact, 0);
    sigaction(SIGINT, &sigact, 0);
    sigaction(SIGQUIT, &sigact, 0);
    sigaction(SIGTERM, &sigact, 0);
    sigact.sa_sigaction = crash_shutdown;
    sigaction(SIGILL, &sigact, 0);
    sigaction(SIGABRT, &sigact, 0);
    sigaction(SIGFPE, &sigact, 0);
    sigaction(SIGSEGV, &sigact, 0);
    sigaction(SIGBUS, &sigact, 0);
    sigaction(SIGTRAP, &sigact, 0);
    signal(SIGPIPE, SIG_IGN);
#if HAVE_LIBBACKTRACE
    if (LOGGING(1)) global_backtrace_state = backtrace_create_state(exename(), 1, nullptr, nullptr);
#endif
}
