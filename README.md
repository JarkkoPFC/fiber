# Fiber
*A portable single header C++ fiber function library for co-operative multitasking.*

[Fibers](https://en.wikipedia.org/wiki/Fiber_(computer_science)) can be useful for implementing non-blocking long-running tasks such as script sequences with delays or other waits between function invocations. As opposed to [threads](https://en.wikipedia.org/wiki/Thread_(computing)), fibers are quite light weight constructs used to implement [co-operative multitasking](https://en.wikipedia.org/wiki/Cooperative_multitasking) (as opposed to [preempive multitasking](https://en.wikipedia.org/wiki/Preemption_(computing)) of threads).

Because fibers are not natively supported in the C++ language, this library tries to provide the functionality in form of reusable and somewhat accessible pattern without the need to manually write code to exhibit fiber-like behavior. Particularly with deep nesting of function calls manual implementation can get quite difficult.

The fiber library is written in portable C++ code and should compile on all C++ standard conforming compilers, but let me know if there are issues.

# Examples
You might want to implement something along the lines of:
```
void blink_light(unsigned num_times_)
{
  for(unsigned i=0; i<num_times_; ++i)
  {
    // switch light on and wait for 1 second
    light_on();
    delay(1.0);
    // switch light off and wait for 1 second
    light_off();
    delay(1.0);
  }
}
```
However, this will pause the control flow execution upon *delay()* calls which might be undesirable because no other logic can be executed simultaneously. Instead it may be preferable to return from *blink_light()* upon the delays and repeatedly re-enter to the function at the exit locations until the desired delays have been completed, enabling other logic to be executed during the delays. This could be implemented with threads and relying on operating system pre-emptive scheduler, but threads might be too heavy weight for the purpose or simply not available in some environments.

With the fiber library you can implement the *blink_light()* function as follows:
```
struct ffunc_blink_light
{
  // define local variables here
  unsigned num_times, i;
  // the fiber function takes num_times as the argument
  ffunc_blink_light(unsigned num_times_)  :num_times(num_times_), i(0) {}
  // the implementation of the fiber function
  FFUNC()
  {
    FFUNC_BEGIN
    for(i=0; i<num_times; ++i)
    {
      // switch light on and wait for 1 second
      light_on();
      FFUNC_CALL(ffunc_sleep, (1.0f));
      // switch light off and wait for 1 second
      light_off();
      FFUNC_CALL(ffunc_sleep, (1.0f));
    }
    FFUNC_END
  }
};
```
Note how the function is defined as struct *ffunc_blink_light* and all local variables (*num_times* and *i*) of *blink_light()* are defined as member variables of the struct instead of local variables of the function. The actual function code is defined in *FFUNC()* function of the struct and sandwitched between *FFUNC_BEGIN* and *FFUNC_END* macros within the function. Once the function is defined as above, it can be executed as follows:
```
// allocate callstack to hold the state of the fiber. by default allocates 256 bytes for the stack.
ffunc_callstack cs;
// start the "blink light" fiber with parameter 3 to blink the light 3 times
FFUNC_START(cs, ffunc_blink_light, (3));
// variables current and previous time to calculate delta time for the fiber ticking
float cur_time=get_time(), prev_time=cur_time;
// loop while ticking (non-blocking) the fiber with the delta time
while(cs.tick(cur_time-prev_time))
{
  /* do some other work here while the fiber runs */

  // update previous and current time for the delta time
  prev_time=cur_time;
  cur_time=get_time();
}
```
This starts the fiber function execution and keeps re-entering the function in non-blocking *cs.tick()* call until the function completes. The *tick()* function returns *true* while the function is executing and takes the delta time between the *tick()* calls to update the state of the fiber.

The *ffunc_callstack* holds the state of the fiber function while the function is running and needs to reserve some pre-determined amount of memory for the state. By default the callstack size is 256 bytes, but it can be defined upon the construction of the stack. The amount of memory required for the stack depends on the depth of the fiber function calls and amount of state each function requires.

Fiber functions can be also called with *FFUNC_CALL()* from other fiber functions enabling nesting and implementation of more complex logic. For example the above *blink_light* fiber function could be used as follows:
```
struct ffunc_light_show
{
  FFUNC()
  {
    FFUNC_BEGIN
    // switch light to red color and blink the light once
    set_light_color(RED);
    FFUNC_CALL(ffunc_blink_light, (1));
     // switch the light to green color and blink the light twice
    set_light_color(GREEN);
    FFUNC_CALL(ffunc_blink_light, (2));
     // switch the light to blue color and blight the light three times
    set_light_color(BLUE);
    FFUNC_CALL(ffunc_blink_light, (3));
    FFUNC_END
  }
};
```
