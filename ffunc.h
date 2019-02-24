//============================================================================
// Fiber function library for co-operative multithreading
//
// Copyright (c) 2019, Profoundic Technologies, Inc.
// All rights reserved.
//----------------------------------------------------------------------------
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//     * Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above copyright
//       notice, this list of conditions and the following disclaimer in the
//       documentation and/or other materials provided with the distribution.
//     * Neither the name of Profoundic Technologies nor the names of its
//       contributors may be used to endorse or promote products derived from
//       this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL PROFOUNDIC TECHNOLOGIES BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
// THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//============================================================================

#ifndef FFUNC_LIB
#define FFUNC_LIB
//----------------------------------------------------------------------------


//============================================================================
// interface
//============================================================================
// new
class ffunc_callstack;
struct ffunc_sleep;
#define FFUNC() size_t __m_ffunc_active_line; bool tick(ffunc_callstack &__ffunc_cs_, size_t __ffunc_stack_pos_, float &__ffunc_dtime_)
#define FFUNC_IMPL(ffunc__) bool ffunc__::tick(ffunc_callstack &__ffunc_cs_, size_t __ffunc_stack_pos_, float &__ffunc_dtime_)
#define FFUNC_BEGIN switch(__m_ffunc_active_line) {case 0:
#define FFUNC_END break;} return false;
#define FFUNC_CALL(ffunc__, ffunc_args__)\
  case __LINE__:\
  {\
    if(__ffunc_cs_.stack_size()==__ffunc_stack_pos_)\
    {\
      __m_ffunc_active_line=__LINE__;\
      new(__ffunc_cs_.push<ffunc__ >())ffunc__ ffunc_args__;\
    }\
    if(__ffunc_cs_.tick<ffunc__ >(__ffunc_stack_pos_, __ffunc_dtime_))\
      return true;\
  }
#define FFUNC_START(ffunc_callstack__, ffunc__, ffunc_args__) new(ffunc_callstack__.start<ffunc__>())ffunc__ ffunc_args__
// inline
#ifdef __GNUC__
#define FFUNC_INLINE inline __attribute__((always_inline))
#elif defined(_MSC_VER)
#define FFUNC_INLINE __forceinline
#else
#define FFUNC_INLINE inline
#endif
// new definition
#ifndef FFUNC_NO_PNEW
#if defined(ARDUINO) && !defined(CORE_TEENSY)
FFUNC_INLINE void *operator new(size_t, void *ptr_) throw() {return ptr_;}
#else
#include <new>
#endif
#endif // FFUNC_NO_PNEW
//----------------------------------------------------------------------------


//============================================================================
// ffunc_callstack
//============================================================================
class ffunc_callstack
{
public:
  // construction
  FFUNC_INLINE ffunc_callstack(size_t stack_capacity_=256);
  FFUNC_INLINE ~ffunc_callstack();
  //--------------------------------------------------------------------------

  // ticking
  FFUNC_INLINE bool tick(float delta_time_);
  //--------------------------------------------------------------------------

  // fiber func private API
  template<class FFunc> FFUNC_INLINE void *start();
  template<class FFunc> FFUNC_INLINE void *push();
  FFUNC_INLINE size_t stack_size() const {return m_stack_size;}
  template<class FFunc> FFUNC_INLINE bool tick(size_t stack_pos_, float dtime_);
  //--------------------------------------------------------------------------

private:
  static bool ffunc_tick_null(ffunc_callstack&, float delta_time_) {return false;}
  template<typename FFunc> static bool ffunc_tick(ffunc_callstack &cs_, float delta_time_)
  {
    // tick re-entrant function callstack
    FFunc *ff=(FFunc*)cs_.m_stack;
    if(ff->tick(cs_, sizeof(FFunc), delta_time_))
      return true;
    ff->~FFunc();
    return false;
  }
  //--------------------------------------------------------------------------

  void *m_stack;
  size_t m_stack_capacity;
  bool(*m_ffunc_tick)(ffunc_callstack&, float delta_time_);
  size_t m_stack_size;
};
//----------------------------------------------------------------------------

ffunc_callstack::ffunc_callstack(size_t stack_capacity_)
{
  m_stack=malloc(stack_capacity_);
  m_stack_capacity=stack_capacity_;
  m_ffunc_tick=&ffunc_tick_null;
  m_stack_size=0;
}
//----

ffunc_callstack::~ffunc_callstack()
{
  free(m_stack);
}
//----------------------------------------------------------------------------

bool ffunc_callstack::tick(float delta_time_)
{
  if(!m_ffunc_tick(*this, delta_time_))
  {
    m_ffunc_tick=&ffunc_tick_null;
    m_stack_size=0;
    return false;
  }
  return true;
}
//----------------------------------------------------------------------------

template<class FFunc>
void *ffunc_callstack::start()
{
//  assert(m_stack_size==0);
//  assert(sizeof(FFunc)<=m_stack_capacity);
  m_ffunc_tick=&ffunc_tick<FFunc>;
  m_stack_size+=sizeof(FFunc);
  ((FFunc*)m_stack)->__m_ffunc_active_line=0;
  return m_stack;
}
//----

template<class FFunc>
void *ffunc_callstack::push()
{
//  assert(m_stack_size+sizeof(FFunc)<=m_stack_capacity);
  FFunc *v=(FFunc*)(((char*)m_stack)+m_stack_size);
  m_stack_size+=sizeof(FFunc);
  v->__m_ffunc_active_line=0;
  return v;
}
//----

template<class FFunc>
bool ffunc_callstack::tick(size_t stack_pos_, float dtime_)
{
  FFunc *ff=(FFunc*)((char*)m_stack+stack_pos_);
  if(ff->tick(*this, stack_pos_+sizeof(FFunc), dtime_))
    return true;
  m_stack_size-=sizeof(FFunc);
  ff->~FFunc();
  return false;
}
//----------------------------------------------------------------------------


//============================================================================
// ffunc_sleep
//============================================================================
struct ffunc_sleep
{
  float sleep_time;
  FFUNC_INLINE ffunc_sleep(float t_) :sleep_time(t_) {}
  FFUNC();
};
//----------------------------------------------------------------------------

FFUNC_IMPL(ffunc_sleep)
{
  // count down the sleep time
  sleep_time-=__ffunc_dtime_;
  if(sleep_time<=0.0f)
  {
    __ffunc_dtime_+=sleep_time;
    return false;
  }
  __ffunc_dtime_=0.0f;
  return true;
}
//----------------------------------------------------------------------------

//============================================================================
#endif
