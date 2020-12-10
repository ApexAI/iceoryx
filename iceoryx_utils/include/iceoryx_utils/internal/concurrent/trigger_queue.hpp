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
#ifndef IOX_UTILS_CONCURRENT_TRIGGER_QUEUE_HPP
#define IOX_UTILS_CONCURRENT_TRIGGER_QUEUE_HPP

#include "iceoryx_utils/cxx/optional.hpp"
#include "iceoryx_utils/internal/concurrent/smart_lock.hpp"
#include "iceoryx_utils/posix_wrapper/semaphore.hpp"

#include <cstdint>
#include <queue>

namespace iox
{
namespace concurrent
{
/// @brief TriggerQueue is behaves exactly like a normal queue (fifo) except that
///         this queue is threadsafe and offers a blocking pop which blocks the
///         the caller until the queue contains at least one element which can
///         be pop'ed.
///
/// @code
///     #include "iceoryx_utils/internal/concurrent/trigger_queue.hpp"
///     #include <atomic>
///     #include <iostream>
///     #include <thread>
///     #include <vector>
///
///     concurrent::TriggerQueue<int, 10>   trigger;
///
///     std::atomic_bool                    keepRunning{true};
///     std::vector<int>                    outputVector;
///
///     void OutputToConsoleThread() {
///         while(keepRunning) {
///             int value;
///             // if this returns false then it is caused by the wakeup trigger
///             // otherwise an element would be in the queue
///             if ( trigger.blocking_pop(value) )
///                 std::cout << value << std::endl;
///         }
///     }
///
///     void OutputToVectorThread() {
///         while(keepRunning) {
///             int value;
///             // if this returns false then it is caused by the wakeup trigger
///             // otherwise an element would be in the queue
///             if ( trigger.blocking_pop(value) )
///                 outputVector.push_back(value);
///         }
///     }
///
///     int main() {
///         std::thread outputConsole(OutputToConsoleThread);
///         std::thread outputVector(OutputToVectorThread);
///
///         if ( trigger.is_initialized() == false ) {
///             // semaphore of the trigger could not be initialized
///         }
///
///         trigger.push(1);
///         std::this_thread::sleep_for(std::chrono::milliseconds(1000));
///         trigger.push(2);
///         std::this_thread::sleep_for(std::chrono::milliseconds(1000));
///         keepRunning = false;
///
///         // exit push to ensure that the output thread really terminates
///         // we need to send 2 wakeup_trigger since 2 threads are running and
///         // wakeup_trigger sends only a single trigger
///         trigger.send_wakeup_trigger();
///         trigger.send_wakeup_trigger();
///
///         outputConsole.join();
///         outputVector.join();
///     }
/// @endcode
template <typename T, uint64_t CAPACITY>
class TriggerQueue
{
    /// To gain the advantages of the rule of zero (no explicit definition of
    /// methods which can be autogenerated by the compiler, ctor, dtor, copy,
    /// move) we check CAPACITY > 0 by declaring a member variable via a
    /// lambda which itselfs contains the check.
    static_assert(CAPACITY > 0, "The trigger queue must have at least one element!");

  public:
    /// @todo replace this with a multi push / multi pop lockfree fifo
    using queue_t = concurrent::smart_lock<std::queue<T>>;

    /// @brief Creates a TriggerQueue. If the TriggerQueue could not be
    ///         initialized, which only happens when the semaphore could not
    ///         be created, then the optional contains nothing.
    ///         Before using a TriggerQueue created by this factory you have to
    ///         verify the success of CreateTriggerQueue by calling
    ///         the optional method has_value().
    static cxx::optional<TriggerQueue> CreateTriggerQueue();

    /// The default constructor needs to be private since we should use the
    /// factory methode CreateTriggerQueue to create a new trigger queue. A
    /// trigger queue constructor can fail if the semaphore construction fails
    /// and the factory method handles that case with an optional
    TriggerQueue() = default;

    /// @brief Pushs an element into the trigger queue and notifies one thread
    ///         which is waiting in blocking_pop().
    ///         If the queue is full it returns false and no element is inserted
    ///         and nothing is notified. If the push was successfull, it returns
    ///         true.
    bool push(const T& in);

    bool blocking_push(const T& in);

    /// @brief  This is a blocking pop. If the queue is empty it blocks until
    ///         an element is push'ed into the queue otherwise it returns the
    ///         last element instantly.
    ///         It returns false when the queue was empty. This can happen when
    ///         another thread called send_wakeup_trigger to notify an arbitrary
    ///         thread.
    ///         It returns true if an element could be pop'ed from the queue.
    bool blocking_pop(T& out);

    /// @brief  If the queue already contains an element it writes the contents
    ///         of that element in out and returns true, otherwise false.
    bool try_pop(T& out);

    /// @brief  Returns true if the queue is empty, otherwise false.
    bool empty();

    /// @brief  Returns the number of elements which are currently in the queue.
    uint64_t size();

    /// @brief  Returns the capacity of the trigger queue.
    uint64_t capacity();

    /// @brief  Sends a single wakeup trigger to an arbitrary blocking_pop. This
    ///         is needed when shutting down a thread which works in a while
    ///         loop with blocking_pop.
    ///         You have the responsibility to ensure that every thread which
    ///         is waiting in blocking_pop is notified when needed!
    void send_wakeup_trigger();

    friend class cxx::optional<TriggerQueue<T, CAPACITY>>;

  private:
    cxx::expected<posix::Semaphore, posix::SemaphoreError> m_semaphore =
        posix::Semaphore::create(posix::CreateUnnamedSingleProcessSemaphore, 0);
    bool m_isInitialized = !m_semaphore.has_error();

    /// @todo remove with lockfree fifo implementation
    /// this methods are helper to make the transition to a lockfree fifo easier
    bool stl_queue_pop(T& out);
    bool stl_queue_push(const T& in);
    queue_t m_queue;
};
} // namespace concurrent
} // namespace iox

#include "iceoryx_utils/internal/concurrent/trigger_queue.inl"

#endif // IOX_UTILS_CONCURRENT_TRIGGER_QUEUE_HPP
