/* EMK 1.0 Copyright (c) 2025 Piraterna */
#ifndef ERRNO_H
#define ERRNO_H

typedef unsigned int errno_t;

#define EOK 0     // No error
#define EFAULT 1  // Bad address
#define EINVAL 2  // Invalid argument
#define EBADPID 3 // Bad pid
#define ESRCH 4   // No such process
#define ENOSYS 5  // Function not implemented
#define EAGAIN 6  // Try again (resource temporarily unavailable)
#define EINTR 7   // Interrupted system call

#define ERRNO_TO_STR(errno)                                                    \
    ((errno) == EOK       ? "No error"                                         \
     : (errno) == EFAULT  ? "Bad address"                                      \
     : (errno) == EINVAL  ? "Invalid argument"                                 \
     : (errno) == EBADPID ? "Bad process id"                                   \
     : (errno) == ESRCH   ? "No such process"                                  \
     : (errno) == ENOSYS  ? "Function not implemented"                         \
     : (errno) == EAGAIN  ? "Resource temporarily unavailable"                 \
     : (errno) == EINTR   ? "Interrupted system call"                          \
                          : "Unknown error")

#endif // ERRNO_H
