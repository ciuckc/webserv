#pragma once
#include <csignal>
#include <sys/wait.h>
#include "IOException.h"

namespace Sig {
struct Mask {
 public:
  Mask(std::initializer_list<int> sigs) noexcept : set() {
    sigemptyset(&set);
    for (auto sig : sigs) {
      sigaddset(&set, sig);
    }
  }
  ~Mask() = default;

  sigset_t set;

  void install(int how) const {
    if (sigprocmask(how, &set, nullptr))
      throw IOException("sigprocmask", errno);
  }
};
static const Mask empty {};
static const Mask blocked {SIGINT, SIGCHLD}; // feel free to add more

template<int signo, bool info = true> struct Action {
 private:
  using action_t = struct sigaction;
  action_t action;

 public:
  using info_handler = void(*)(int, siginfo_t*, void*);
  using handler_t = typename std::conditional<info, info_handler, sighandler_t>::type;

  Action(int flags, handler_t handler, const Mask& mask = {}) noexcept : action() {
    action.sa_flags = flags;
    action.sa_mask = mask.set;
    if constexpr (info)
      action.sa_sigaction = handler;
    else
      action.sa_handler = handler;
  };
  ~Action() = default;

  void install() const {
    if (sigaction(signo, &action, nullptr) != 0)
      throw IOException("sigaction", errno); // this should be impossible
  }
};


static void child_handler(int, siginfo_t* info, void*) {
  int status;
  pid_t pid = info->si_pid;
  if (waitpid(pid, &status, WNOHANG) != pid)
    abort(); // possible??
}
static const Action<SIGCHLD> child_action {
  SA_SIGINFO | SA_NOCLDSTOP | SA_RESTART, child_handler
};

static inline void setup_signals() {
  blocked.install(SIG_BLOCK);
  child_action.install();
}
}
