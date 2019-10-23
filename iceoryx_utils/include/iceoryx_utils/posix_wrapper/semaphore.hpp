// Copyright (c) 2019 by Robert Bosch GmbH. All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#include "iceoryx_utils/cxx/helplets.hpp"
#include "iceoryx_utils/cxx/smart_c.hpp"
#include "iceoryx_utils/design_pattern/creation.hpp"

#include <cstring>
#include <fcntl.h>
#include <semaphore.h>
#include <sys/stat.h>

namespace iox
{
namespace posix
{
enum class SemaphoreError
{
    CREATION_FAILED,
    UNDEFINED
};

/// @brief Posix semaphore C++ Wrapping class
/// @code
///     auto semaphore = posix::Semaphore::CreateUnnamed(false, 5);
///     int value;
///     if ( semaphore.getValue(value) ) // no error has occurred
///     {
///         std::cout << value << std::endl;
///     }
/// @endcode
class Semaphore : public DesignPattern::Creation<Semaphore, SemaphoreError>
{
  public:
    /// @brief Default constructor which creates an uninitialized semaphore.
    ///         This semaphore object is unusable you need to reassign it with
    ///         an object created by the semaphore factory methods
    Semaphore() noexcept;

    /// @brief Move constructor.
    Semaphore(Semaphore&& rhs) noexcept;

    /// @brief Move assignment operator.
    Semaphore& operator=(Semaphore&& rhs) noexcept;

    /// @brief We are denying Semaphore copy since it manages the semaphore resource
    ///         and the underlying concept did not include copying
    Semaphore(const Semaphore&) = delete;

    /// @brief We are denying Semaphore copy since it manages the semaphore resource
    ///         and the underlying concept did not include copying
    Semaphore& operator=(const Semaphore&) = delete;

    /// @brief Destructor
    ~Semaphore() noexcept;

    /// @brief calls sem_getvalue which gets the value of a semaphore
    /// From the sem_getvalue manpage: sem_getvalue() places the current value
    /// of the semaphore pointed to sem into the integer pointed to by sval.
    ///
    /// If  one or more processes or threads are blocked waiting to lock the
    /// semaphore with sem_wait(3), POSIX.1 permits two possibilities for the
    /// value returned in sval: either 0 is returned; or a negative number whose
    /// absolute value is the count of the number of processes and threads
    /// currently blocked in sem_wait(3).  Linux adopts the former behavior.
    ///
    /// @param[in] value reference in which the value of the semaphore is
    ///                     written to
    ///
    /// @return the optional is set if sem_getvalue succeeded otherwise an unset
    /// optional<int> is returned
    bool getValue(int& value) const noexcept;

    /// @brief calls sem_post which unlocks a semaphore
    /// From the sem_post manpage: sem_post()  increments  (unlocks) the
    /// semaphore pointed to by sem.  If the semaphore's value consequently
    /// becomes greater than zero, then another process or thread blocked in a
    /// sem_wait(3) call will be woken up and proceed to lock the semaphore.
    ///
    /// @return returns false when sem_post fails otherwise true
    bool post() noexcept;

    /// @brief see wait()
    /// @param[in] abs_timeout timeout of the wait
    /// @param[in] doContinueOnInterrupt implements the feature described in the
    ///                sem_wait manpage:
    ///      while ((s = sem_timedwait(&sem, &ts)) == -1 && errno == EINTR)
    ///      continue; /* Restart if interrupted by handler */
    ///             true = restart till we aren't interrupted anymore
    ///             false = return on any error
    /// @return returns false when not initialized,has errors and timed out otherwise true
    bool timedWait(const struct timespec* abs_timeout, const bool doContinueOnInterrupt) const noexcept;

    /// @brief see wait()
    bool tryWait() const noexcept;

    /// @brief calls sem_wait which locks a semaphore
    /// From the sem_wait manpage: sem_wait()  decrements  (locks) the semaphore
    /// pointed to by sem.  If the semaphore's value is greater than zero, then
    /// the decrement proceeds, and the function returns, immediately.  If the
    /// semaphore
    /// currently has the value zero, then the call blocks until either it
    /// becomes possible to perform the decrement (i.e., the semaphore value
    /// rises above zero), or a signal handler interrupts the call.
    ///
    /// sem_trywait() is the same as sem_wait(), except that if the decrement
    /// cannot be immediately performed, then call returns an error (errno set
    /// to EAGAIN) instead of blocking.
    ///
    /// sem_timedwait() is the same as sem_wait(), except that abs_timeout
    /// specifies a limit on the amount of time that the call should block if
    /// the decrement cannot be immediately performed.  The abs_time‐ out
    /// argument points to a structure that specifies an absolute timeout in
    /// seconds and nanoseconds since the Epoch, 1970-01-01 00:00:00 +0000
    /// (UTC).  This structure is defined as follows:
    ///
    ///     struct timespec {
    ///         time_t tv_sec;      /* Seconds */
    ///         long   tv_nsec;     /* Nanoseconds [0 .. 999999999] */
    ///     };
    ///
    /// If the timeout has already expired by the time of the call, and the
    /// semaphore could not be locked immediately, then sem_timedwait() fails
    /// with a timeout error (errno set to ETIMEDOUT).
    ///
    /// If the operation can be performed immediately, then sem_timedwait()
    /// never fails with a timeout error, regardless of the value of
    /// abs_timeout.  Furthermore, the validity of abs_timeout is not checked in
    /// this case.
    ///
    /// @return returns false if sem_wait fails otherwise true
    bool wait() const noexcept;

    /// @brief returns the pointer to the managed semaphore. You can use this
    ///         pointer with all the sem_** functions.
    sem_t* getHandle() noexcept;

  private:
    static constexpr int m_nameSize = 128;
    char m_name[m_nameSize] = {'\0'};
    bool m_isCreated = true;
    bool m_isNamedSemaphore = true;
    bool m_isShared = false;

    mutable sem_t m_handle;
    mutable sem_t* m_handlePtr = &m_handle;

  private:
    friend class DesignPattern::Creation<Semaphore, SemaphoreError>;

    /// @brief Returns false if the given name length is smaller than m_nameSize
    ///         otherwise true.
    bool hasSemaphoreNameOverflow(const char* name) noexcept;

    /// @brief Creates a local unnamed semaphore.
    ///         The Semaphore should be initialized but that has to be verified
    ///         via IsInitialized()
    ///         For details see man sem_init.
    /// @param[in] value initial value of the semaphore
    Semaphore(const unsigned int value) noexcept;

    /// @brief Creates unnamed semaphore in the shared memory.
    ///         The Semaphore should be initialized but that has to be verified
    ///         via IsInitialized()
    ///         For details see man sem_init.
    /// @param[in] handle pointer to a handle which is in the shared memory
    /// @param[in] value initial value of the semaphore
    Semaphore(sem_t* handle, const unsigned int value) noexcept;

    /// @brief Opens an already existing named semaphore. If a semaphore with
    ///         name does not exist an uninitialized Semaphore is returned
    ///         otherwise the Semaphore can be initialized but that has to be
    ///         verified via IsInitialized().
    ///         For details see man sem_open.
    /// @param[in] name name of the semaphore
    /// @param[in] oflag specifies flags that control the operation of the call
    ///                 O_CREAT flag is not allowed here
    Semaphore(const char* name, const int oflag) noexcept;

    /// @brief Creates an exclusive named semaphore. If a semaphore with name
    ///         already exists then the Semaphore returned is not initialized
    ///         and not usable!
    ///         You always have to verify if the semaphore returned by this
    ///         factory is initialized via the IsInitialized() method.
    ///         For details see man sem_open.
    /// @param[in] name name of the semaphore
    /// @param[in] mode specifies the permissions to be placed on the new
    ///                 semaphore, see man 2 open to get a detailed description
    ///                 on mode_t
    /// @param[in] value the initial value of the semaphore
    /// @return Semaphore object which can be initialized, if a semaphore
    ///         named name exists it is definitly an uninitialized semaphore.
    Semaphore(const char* name, const mode_t mode, const unsigned int value) noexcept;

    /// @brief calls sem_close which closes a named semaphore
    /// From the sem_close manpage: sem_close() closes the named semaphore
    /// referred to by sem, allowing any resources that the system has allocated
    /// to the calling process for this semaphore to be freed.
    ///
    /// @return returns false when sem_close fails otherwise true
    bool close() noexcept;

    /// @brief calls sem_destroy which destroys a unnamed semaphore
    /// From the sem_destroy manpage: sem_destroy() destroys the unnamed
    /// semaphore at the address pointed to by sem.
    ///
    /// Only a semaphore that has been initialized by sem_init(3) should be
    /// destroyed using sem_destroy().
    ///
    /// Destroying a semaphore that other processes or threads are currently
    /// blocked on (in sem_wait(3)) produces undefined behavior.
    ///
    /// Using a semaphore that has been destroyed produces undefined results,
    /// until the semaphore has been reinitialized using sem_init(3).
    ///
    /// @return returns false when sem_destroy fails otherwise true
    bool destroy() noexcept;

    /// @brief calls sem_init which initializes an unnamed semaphore
    /// From the sem_init manpage: sem_init() initializes the unnamed semaphore
    /// at the address pointed to by sem.  The value argument specifies the
    /// initial value for the semaphore.
    ///
    /// The pshared argument indicates whether this semaphore is to be shared
    /// between the threads of a process, or between processes.
    ///
    /// If  pshared  has the value 0, then the semaphore is shared between the
    /// threads of a process, and should be located at some address that is
    /// visible to all threads (e.g., a global variable, or a vari‐ able
    /// allocated dynamically on the heap).
    ///
    /// If pshared is nonzero, then the semaphore is shared between processes,
    /// and should be located in a region of shared memory (see shm_open(3),
    /// mmap(2),  and  shmget(2)).   (Since  a  child  created  by fork(2)
    /// inherits its parent's memory mappings, it can also access the
    /// semaphore.)  Any process that can access the shared memory region can
    /// operate on the semaphore using sem_post(3), sem_wait(3), and so on.
    ///
    /// Initializing a semaphore that has already been initialized results in
    /// undefined behavior.
    ///
    /// @return returns false when sem_init fails otherwise true
    bool init(sem_t* handle, const int pshared, const unsigned int value) noexcept;

    /// @brief calls sem_open which initializes and opens a named semaphore
    /// From the sem_open manpage: sem_open() creates a new POSIX semaphore or
    /// opens an existing semaphore.  The semaphore is identified by name.  For
    /// details of the construction of name, see sem_overview(7).
    ///
    /// The oflag argument specifies flags that control the operation of the
    /// call.  (Definitions of the flags values can be obtained by including
    /// <fcntl.h>.)  If O_CREAT is specified in oflag, then the sem‐ aphore is
    /// created if it does not already exist.  The owner (user ID) of the
    /// semaphore is set to the effective user ID of the calling process.  The
    /// group ownership (group ID) is set to the  effective group ID of the
    /// calling process.  If both O_CREAT and O_EXCL are specified in oflag,
    /// then an error is returned if a semaphore with the given name already
    /// exists.
    ///
    /// If  O_CREAT  is specified in oflag, then two additional arguments must
    /// be supplied.  The mode argument specifies the permissions to be placed
    /// on the new semaphore, as for open(2).  (Symbolic defini‐ tions for the
    /// permissions bits can be obtained by including <sys/stat.h>.)  The
    /// permissions settings are masked against the process umask. Both read and
    /// write permission should be granted  to  each class  of  user  that  will
    /// access the semaphore. The value argument specifies the initial value for
    /// the new semaphore.  If O_CREAT is specified, and a semaphore with the
    /// given name already exists, then mode and value are ignored.
    ///
    /// @return returns false when sem_open fails otherwise true
    bool open(const int oflag) noexcept;

    bool open(const int oflag, const mode_t mode, const unsigned int value) noexcept;

    /// @brief calls sem_unlink which removes a named semaphore
    /// From the sem_unlink manpage: sem_unlink() removes the named semaphore
    /// referred to by name.  The semaphore name is removed immediately.  The
    /// semaphore is destroyed once all other processes that have the semaphore
    /// open close it.
    ///
    /// @return returns false when sem_unlink fails otherwise true
    bool unlink(const char* name) noexcept;

    /// @brief Returns true if the semaphore was created with CreateNamed or
    ///         OpenNamed otherwise it returns false.
    bool isNamedSemaphore() noexcept;

    template <typename SmartC>
    bool setHandleFromCall(const SmartC& call) noexcept;
};
} // namespace posix
} // namespace iox

#include "iceoryx_utils/internal/posix_wrapper/semaphore.inl"
