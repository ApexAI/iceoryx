// Copyright (c) 2020 by Apex.AI Inc. All rights reserved.
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

#ifndef IOX_UTILS_CXX_METHOD_CALLBACK_HPP
#define IOX_UTILS_CXX_METHOD_CALLBACK_HPP

#include "iceoryx_utils/cxx/function_ref.hpp"

namespace iox
{
namespace cxx
{
class GenericClass
{
};

template <typename ReturnValue, typename... Args>
class ConstMethodCallback
{
  public:
    ConstMethodCallback() = default;
    template <typename ClassType>
    ConstMethodCallback(ClassType* classPtr, ReturnValue (ClassType::*methodPtr)(Args...) const) noexcept;

    ConstMethodCallback(const ConstMethodCallback& rhs) = default;
    ConstMethodCallback& operator=(const ConstMethodCallback& rhs) = default;

    ConstMethodCallback(ConstMethodCallback&& rhs) noexcept;
    ConstMethodCallback& operator=(ConstMethodCallback&& rhs) noexcept;

    ~ConstMethodCallback() = default;

    ReturnValue operator()(Args... args) const noexcept;
    bool operator==(const ConstMethodCallback& rhs) const noexcept;

    explicit operator bool() const noexcept;

  private:
    void* m_classPtr{nullptr};
    ReturnValue (GenericClass::*m_methodPtr)(Args...) const;
    cxx::function_ref<ReturnValue(void*, ReturnValue (GenericClass::*)(Args...) const, Args...)> m_callback;
};

template <typename ReturnValue, typename... Args>
class MethodCallback
{
  public:
    MethodCallback() = default;
    template <typename ClassType>
    MethodCallback(ClassType* classPtr, ReturnValue (ClassType::*methodPtr)(Args...)) noexcept;

    MethodCallback(const MethodCallback& rhs) = default;
    MethodCallback& operator=(const MethodCallback& rhs) = default;

    MethodCallback(MethodCallback&& rhs) noexcept;
    MethodCallback& operator=(MethodCallback&& rhs) noexcept;

    ~MethodCallback() = default;


    ReturnValue operator()(Args... args) noexcept;
    bool operator==(const MethodCallback& rhs) const noexcept;

    explicit operator bool() const noexcept;

  private:
    void* m_classPtr{nullptr};
    ReturnValue (GenericClass::*m_methodPtr)(Args...);
    cxx::function_ref<ReturnValue(void*, ReturnValue (GenericClass::*)(Args...), Args...)> m_callback;
};

} // namespace cxx
} // namespace iox

#include "iceoryx_utils/internal/cxx/method_callback.inl"

#endif