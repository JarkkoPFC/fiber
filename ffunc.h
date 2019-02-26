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
// config
//============================================================================
//#define FFUNC_ENABLE_ASSERTS   // enable asserts (useful for debugging, but adds memory & perf overhead)
//#define FFUNC_ENABLE_MEM_TRACK // enable stack memory tracking (to optimize stack memory usage)
//#define FFUNC_DISABLE_LOGS     // disable logs (reduces program size)
//#define FFUNC_DISABLE_PNEW     // disable placement new operator definition (must be defined outside of ffunc.h)
//----------------------------------------------------------------------------


//============================================================================
// interface
//============================================================================
// external
#include <stdlib.h>
#ifndef FFUNC_DISABLE_LOGS
#include <stdio.h>
#endif

// new
class ffunc_callstack;
struct ffunc_sleep;
#define FFUNC() size_t __m_ffunc_active_line; bool tick(ffunc_callstack &__ffunc_cs_, unsigned __ffunc_stack_pos_, float &__ffunc_dtime_)
#define FFUNC_IMPL(ffunc__) bool ffunc__::tick(ffunc_callstack &__ffunc_cs_, unsigned __ffunc_stack_pos_, float &__ffunc_dtime_)
#define FFUNC_BEGIN switch(__m_ffunc_active_line) {case 0:
#define FFUNC_END break;} return false;
#define FFUNC_CALL(ffunc__, ffunc_args__)\
  case __LINE__:\
  {\
    if(__ffunc_cs_.size()==__ffunc_stack_pos_)\
    {\
      __m_ffunc_active_line=__LINE__;\
      new(__ffunc_cs_.push<ffunc__ >())ffunc__ ffunc_args__;\
    }\
    if(__ffunc_cs_.tick<ffunc__ >(__ffunc_stack_pos_, __ffunc_dtime_))\
      return true;\
  }
#define FFUNC_START(ffunc_callstack__, ffunc__, ffunc_args__) new(ffunc_callstack__.start<ffunc__ >())ffunc__ ffunc_args__
// inline
#ifdef __GNUC__
#define FFUNC_INLINE inline __attribute__((always_inline))
#elif defined(_MSC_VER)
#define FFUNC_INLINE __forceinline
#else
#define FFUNC_INLINE inline
#endif
// placement new
#ifndef FFUNC_DISABLE_PNEW
#if defined(ARDUINO) && !defined(CORE_TEENSY) // Arduino (except Teensy) doesn't have "new" header to define placement new operator
FFUNC_INLINE void *operator new(size_t, void *ptr_) throw() {return ptr_;}
#else
#include <new>
#endif
#endif // FFUNC_DISABLE_PNEW
// aborting
#ifdef __GNUC__
#define FFUNC_ABORT() __builtin_trap()
#elif defined(_MSC_VER)
#define FFUNC_ABORT() __debugbreak()
#else
#define FFUNC_ABORT() abort()
#endif
// logging
#ifdef FFUNC_DISABLE_LOGS
#define FFUNC_LOG(msg__) {}
#define FFUNC_LOGF(...)  {}
#else // FFUNC_DISABLE_LOGS
#ifdef ARDUINO
#define FFUNC_LOG(msg__) {Serial.print(msg__);}
#define FFUNC_LOGF(...)  {char buf[64]; sprintf(buf, __VA_ARGS__); Serial.print(buf);}
#elif defined(_MSC_VER)
#define FFUNC_LOG(msg__) {OutputDebugString(msg__);}
#define FFUNC_LOGF(...)  {char buf[64]; sprintf(buf, __VA_ARGS__); OutputDebugString(buf);}
#else
#define FFUNC_LOG(msg__) {printf(msg__);}
#define FFUNC_LOGF(...)  {printf(__VA_ARGS__);}
#endif
#endif // FFUNC_DISABLE_LOGS
// asserts
#ifndef FFUNC_ENABLE_ASSERTS
#define FFUNC_ASSERT(expr__)           {}
#define FFUNC_ASSERT_LOG(expr__, ...)  {}
#define FFUNC_ASSERT_LOGF(expr__, ...) {}
#else // FFUNC_ENABLE_ASSERTS
#define FFUNC_STR(v__) FFUNC_STR_DO(v__)
#define FFUNC_STR_DO(v__) #v__
#define FFUNC_ASSERT_PREFIX()           {FFUNC_LOG(__FILE__ "(" FFUNC_STR(__LINE__) ") : assert failed: ");}
#define FFUNC_ASSERT(expr__)            {if(!(expr__)) {FFUNC_ASSERT_PREFIX(); FFUNC_LOG(#expr__); FFUNC_ABORT();}}
#define FFUNC_ASSERT_LOG(expr__, msg__) {if(!(expr__)) {FFUNC_ASSERT_PREFIX(); FFUNC_LOG(msg__); FFUNC_ABORT();}}
#define FFUNC_ASSERT_LOGF(expr__, ...)  {if(!(expr__)) {FFUNC_ASSERT_PREFIX(); FFUNC_LOGF(__VA_ARGS__); FFUNC_ABORT();}}
#endif // FFUNC_ENABLE_ASSERTS
//----------------------------------------------------------------------------


//============================================================================
// ffunc_callstack
//============================================================================
class ffunc_callstack
{
public:
  // construction
  FFUNC_INLINE ffunc_callstack(unsigned capacity_=256);
  FFUNC_INLINE ffunc_callstack(size_t *buffer_, unsigned capacity_=0);
  FFUNC_INLINE ~ffunc_callstack();
  FFUNC_INLINE unsigned peak_size() const;
  //--------------------------------------------------------------------------

  // ticking
  FFUNC_INLINE bool tick(float delta_time_);
  //--------------------------------------------------------------------------

  // fiber func private API
  template<class FFunc> FFUNC_INLINE void *start();
  template<class FFunc> FFUNC_INLINE void *push();
  FFUNC_INLINE unsigned size() const {return m_size;}
  template<class FFunc> FFUNC_INLINE bool tick(unsigned stack_pos_, float dtime_);
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
  bool(*m_ffunc_tick)(ffunc_callstack&, float delta_time_);
  int m_capacity;
  unsigned m_size;
#ifdef FFUNC_ENABLE_MEM_TRACK
  unsigned m_peak_size;
#endif
};
//----------------------------------------------------------------------------

ffunc_callstack::ffunc_callstack(unsigned capacity_)
{
  m_stack=malloc(capacity_);
  m_ffunc_tick=&ffunc_tick_null;
  m_capacity=int(capacity_);
  m_size=0;
#ifdef FFUNC_ENABLE_MEM_TRACK
  m_peak_size=0;
#endif
}
//----

ffunc_callstack::ffunc_callstack(size_t *buffer_, unsigned capacity_)
{
  m_stack=buffer_;
  m_ffunc_tick=&ffunc_tick_null;
  m_capacity=-int(capacity_); // negative capacity to indicate the buffer isn't dynamically allocated by the class
  m_size=0;
#ifdef FFUNC_ENABLE_MEM_TRACK
  m_peak_size=0;
#endif
}
//----

ffunc_callstack::~ffunc_callstack()
{
  FFUNC_ASSERT_LOG(m_size==0, "Destroying stack while in use by a fiber\r\n");
  if(m_capacity>0)
    free(m_stack);
}
//----

unsigned ffunc_callstack::peak_size() const
{
#ifdef FFUNC_ENABLE_MEM_TRACK
  return m_peak_size;
#else
  return 0;
#endif
}
//----------------------------------------------------------------------------

bool ffunc_callstack::tick(float delta_time_)
{
  if(!m_ffunc_tick(*this, delta_time_))
  {
    m_ffunc_tick=&ffunc_tick_null;
    m_size=0;
    return false;
  }
  return true;
}
//----------------------------------------------------------------------------

template<class FFunc>
void *ffunc_callstack::start()
{
  FFUNC_ASSERT_LOG(m_size==0, "Stack in use by another fiber\r\n");
  FFUNC_ASSERT_LOGF(sizeof(FFunc)<=abs(m_capacity), "Stack overflow by %i bytes\r\n", unsigned(sizeof(FFunc)-abs(m_capacity)));
  m_ffunc_tick=&ffunc_tick<FFunc>;
  m_size+=sizeof(FFunc);
#ifdef FFUNC_ENABLE_MEM_TRACK
  m_peak_size=max(m_peak_size, unsigned(sizeof(FFunc)));
#endif
  ((FFunc*)m_stack)->__m_ffunc_active_line=0;
  return m_stack;
}
//----

template<class FFunc>
void *ffunc_callstack::push()
{
  FFUNC_ASSERT_LOGF(m_size+sizeof(FFunc)<=abs(m_capacity), "Stack overflow by %i bytes\r\n", unsigned(m_size+sizeof(FFunc)-abs(m_capacity)));
  FFunc *v=(FFunc*)(((char*)m_stack)+m_size);
  m_size+=sizeof(FFunc);
#ifdef FFUNC_ENABLE_MEM_TRACK
  m_peak_size=max(m_peak_size, m_size);
#endif
  v->__m_ffunc_active_line=0;
  return v;
}
//----

template<class FFunc>
bool ffunc_callstack::tick(unsigned stack_pos_, float dtime_)
{
  FFunc *ff=(FFunc*)((char*)m_stack+stack_pos_);
  if(ff->tick(*this, stack_pos_+sizeof(FFunc), dtime_))
    return true;
  m_size-=sizeof(FFunc);
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
#endif // FFUNC_LIB
