# Fiber
*A portable single header C++ fiber function library for co-operative multitasking.*

[Fibers](https://en.wikipedia.org/wiki/Fiber_(computer_science)) and [coroutines](https://en.wikipedia.org/wiki/Coroutine) can be useful for implementing non-blocking long-running tasks such as script sequences with delays or other waits between function invocations. As opposed to [threads](https://en.wikipedia.org/wiki/Thread_(computing)), fibers are quite light weight constructs used to implement [co-operative multitasking](https://en.wikipedia.org/wiki/Cooperative_multitasking) (as opposed to [preempive multitasking](https://en.wikipedia.org/wiki/Preemption_(computing)) of threads).

Because fibers are not natively supported in the C++ language, this library tries to provide the functionality in form of reusable and somewhat accessible pattern without the need to manually write code to exhibit fiber-like behavior (e.g. using custom state machines). Particularly with deep function call hierarhies and complex logic the manual implementation can get quite difficult.

The fiber library is written in portable C++ code and should compile on all C++ standard conforming compilers, but let me know if there are issues.

# Examples
You might want to implement something along the lines of the following to blink a light N times (half a second on, half a second off) and then pause for a second:
```
void blink_light(unsigned num_times_)
{
  for(unsigned i=0; i<num_times_; ++i)
  {
    // switch light on and wait for half a second
    light_on();
    delay(0.5);
    // switch light off and wait for half a second
    light_off();
    delay(0.5);
  }
  delay(1.0);
}
```
However, this will pause the control flow execution upon *delay()* calls which might be undesirable because no other logic can be executed simultaneously. Instead it may be preferable to return from *blink_light()* upon the delays and repeatedly re-enter to the function at the exit locations until the desired delays have been completed, enabling other logic to be executed during the delays. This could also be implemented with threads and relying on operating system pre-emptive scheduler, but threads might be too heavy weight for the purpose or simply not available in some environments.

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
      // switch light on and wait for 0.5 seconds
      light_on();
      FFUNC_CALL(ffunc_sleep, (0.5f));
      // switch light off and wait for 0.5 seconds
      light_off();
      FFUNC_CALL(ffunc_sleep, (0.5f));
    }
    // wait 1 second before exit
    FFUNC_CALL(ffunc_sleep, (1.0f));
    FFUNC_END
  }
};
```
Note how the function is defined as struct *ffunc_blink_light* and all local variables (*num_times* and *i*) of *blink_light()* are defined as member variables of the struct instead of local variables of the function. The actual function code is defined in *FFUNC()* function of the struct and sandwitched between *FFUNC_BEGIN* and *FFUNC_END* macros within the function. Once the function is defined as above, it can be executed as follows:
```
// allocate callstack to hold the state of the fiber
ffunc_callstack cs;
// start the "blink light" fiber with parameter 3 to blink the light 3 times
FFUNC_START(cs, ffunc_blink_light, (3));
// variables for current and previous time to calculate delta time for the fiber ticking
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
This starts the fiber function execution and keeps re-entering the function in non-blocking *cs.tick()* call until the function completes. The *tick()* function returns *true* while the fiber function is executing and takes the delta time between the *tick()* calls to update the state of the fiber.

The *ffunc_callstack* holds the state of the fiber function while the function is running and needs to reserve some pre-determined amount of memory for the state. By default the callstack size is 256 bytes, but it can be defined upon the construction of the stack. The amount of memory required for the stack depends on the depth of the fiber function calls and amount of state each function requires.

Fiber functions can be also called with *FFUNC_CALL()* from other fiber functions which enables creating more complex logic with other fiber functions. For example the above *blink_light* fiber function could be used as follows:
```
struct ffunc_light_show
{
  FFUNC()
  {
    FFUNC_BEGIN
    // switch the light to red color and blink once
    set_light_color(RED);
    FFUNC_CALL(ffunc_blink_light, (1));
     // switch the light to green color and blink twice
    set_light_color(GREEN);
    FFUNC_CALL(ffunc_blink_light, (2));
     // switch the light to blue color and blink three times
    set_light_color(BLUE);
    FFUNC_CALL(ffunc_blink_light, (3));
    FFUNC_END
  }
};
```

# Arduino Example
In Arduino community it's quite common to see people implementing various scripted sequences, so below is a comprehensive example how this library can be used on Arduino. First you need to copy the [ffunc.h](https://github.com/JarkkoPFC/fiber/blob/master/ffunc.h) file to the same directory as the .ino file.

Arduino has setup() and loop() functions that needs to be implemented for the sketches. Also for timing purposes Arduino provides millis() function which returns the number of milliseconds since the program start. To use the above "blink light" function on Arduino, you can copy the following code in its entirety into your ino file. This will cause the LED, that's connected to pin 13, to blink 3 times and then pause for a second before restarting the blink sequence.

```
#include "ffunc.h"

static uint8_t s_led_pin=13; // change this to the LED pin you want to animate

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
      // switch light on and wait for 0.5 seconds
      digitalWrite(s_led_pin, HIGH);
      FFUNC_CALL(ffunc_sleep, (0.5f));
      // switch light off and wait for 0.5 seconds
      digitalWrite(s_led_pin, LOW);
      FFUNC_CALL(ffunc_sleep, (0.5f));
    }
    // wait for 1 second before exit
    FFUNC_CALL(ffunc_sleep, (1.0f));
    FFUNC_END
  }
};

static unsigned long s_prev_time;
void setup()
{
  pinMode(s_led_pin, OUTPUT);
  s_prev_time=millis();
}

static ffunc_callstack s_cs;
void loop()
{
  // update the callstack with the delta time betleen loop() calls.
  // tick() function returns false if no fiber function is running, so
  // this keeps restarting the function when the previous completes.
  unsigned long cur_time=millis();
  if(!s_cs.tick((cur_time-s_prev_time)/1000.0f)) // convert delta in millis to seconds
    FFUNC_START(s_cs, ffunc_blink_light, (3)); // start the blink light function
  s_prev_time=cur_time;
}
```
Note that you can have multiple callstacks to run multiple fiber functions simultaneously. For example if you want one fiber to animate a LED as shown in this example, you can create another fiber which independently animates another LED, or motor, or something else entirely!

# Optimizing Stack Memory
Especially for memory constrained systems it's important to optimize the memory usage of the fiber callstack. This is very similar to regular functions and how they use the stack. The memory of fiber callstack can be optimized in few different ways:
- Optimize the state info of each fiber function (analoguous to function local variables)
- Avoid deep function hierarchies (analoguous to stack usage of regular function hierarchies)
- Reduce the stack size to the minimum (analoguous to thread stacks config)
- Allocate stack from static vs dynamic store

### Optimize function state info
Because the state of the fiber functions (local variables) are stored recursively into the fiber callstack upon fiber function invocations (i.e. upon *FFUNC_START()* and *FFUNC_CALL()* calls), it's good to optimize the amount of state information in each function. You can do this by storing the least amount of variables for function, using the smallest sufficient data types for the variables, and organizing the variables for efficient storage. Just like with regular functions this will reduce stack usage. You can check the size of the state data used by the function by simply checking the size of each function struct with *sizeof()* operator, e.g.
```
unsigned state_size=sizeof(ffunc_sleep);
```
### Avoid deep function hierarchies
As with regular functions the stack usage of fibers depends on the depth of the function call hierarchy. For example recursive function calls can have very deep call hierarchies and consume a lot of stack, thus it's good to use iterative algorithms instead where feasible.

There's an additional overhead that regular functions don't have: re-entering and exiting the functions upon waits. Every time the callstack is being ticked, all the functions in the callstack up to the final re-entering location are being iterated through, which adds some performance overhead.

### Reducing the stack size
By default the fiber callstack allocates 256 bytes from the dynamic store, but this may not be optimal depending on fiber function call hierarchy depth and how much state information is stored for each function. Thus it's good to optimize both state data and call hierarchy with the previous guidelines before reducing the stack size. It's also good to leave the stack size trimming closer to the end of the project if possible, so that changes to the fiber functions don't overflow the stack, and keep good amount of spare stack memory during development.

It's also good to keep asserts enabled for the library so that the stack overflows are properly checked. You can enable asserts by defining *FFUNC_ENABLE_ASSERTS* before including *ffunc.h* or by uncommenting the define in *ffunc.h*.
```
#define FFUNC_ENABLE_ASSERTS
#include "ffunc.h"
```

To minimize the memory usage you can enable stack memory tracking for the project by defining *FFUNC_ENABLE_MEM_TRACK* before *#include "ffunc.h"*, or by uncommenting the define in the beginning of *ffunc.h*
```
#define FFUNC_ENABLE_MEM_TRACK
#include "ffunc.h"
```
This will keep track of the peak stack usage, which can be then queried with *peak_size()* function of the stack. After this you can print out the peak size of the stack for example at the end of the program or upon the stack destruction, and use the value to trim the stack size to the optimal size.
```
FFUNC_LOGF("Peak stack size: %i bytes\r\n", s_cs.peak_size());
```
For example if the above code prints 40 bytes, this value can be then passed to the constructor of *ffunc_callstack* as follows:
```
ffunc_callstack cs(40); // allocate 40 bytes for the callstack
```
### Allocating stack data from static store
The fiber callstack enables explicit definition of the callstack storage for further memory optimizations and for avoiding dynamic memory allocations. This can be used for example to address memory fragmentation issues of dynamic allocations, especially if callstacks are frequently created and destroyed by the program.

*ffunc_callstack* has a constructor which takes a pointer to a memory buffer and the size of the buffer (in bytes) as arguments, and leaves the memory management of the buffer to the caller. An important note is that the buffer must be properly aligned in memory to avoid memory alignment issues/crashes. This is the reason why the buffer is passed by *size_t** and not by *void** to the constructor to underline the fact. For example to properly create the callstack with a buffer in static store you could do the following:
```
static size_t s_cs_buf[16]; // 16*sizeof(size_t) bytes
ffunc_callstack cs(s_cs_buf, sizeof(s_cs_buf));
```
