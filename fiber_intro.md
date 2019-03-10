# Introduction to Fibers in C++

Fibers and coroutines are a mechanism to implement multitasking by co-operatively executing multiple functions simultaneously. They enable implementation of non-blocking functions in a similar way as regular blocking functions without the need to explicitly cater the non-blocking requirement. This significantly simplifies the implementation of functions making it both easier to write and read thus enabling more complex logic in your programs.

If you are already familiar with [threads](https://en.wikipedia.org/wiki/Thread_(computing)), this is a very similar concept. For threads the scheduling is done [preemptively](https://en.wikipedia.org/wiki/Preemption_(computing)) by the operating system, i.e. the OS can switch over to another thread at any time without consent of the executing function. In contrast the scheduling of fibers/coroutines is done [co-operatively](https://en.wikipedia.org/wiki/Cooperative_multitasking) by the user code and executing functions voluterily "yield" the control back to the scheduler by calling a yielding function. The advantage of threads is the ability to take advantage of multi-core systems, but they are not available on all environments, have more expensive [context switching](https://en.wikipedia.org/wiki/Context_switch) and because of the preemptive scheduling need care in avoiding [race conditions](https://en.wikipedia.org/wiki/Race_condition).

An alternative way commonly used to implement non-blocking functions is state machines, where the non-blocking requirement is explicitly coded into the functions. However, this adds a lot of complexity to the function implementation and makes it difficult to follow the program flow. To illustrate this point, below is a simple example of a function which blinks a light 3 times. The code below is written in blocking way, i.e. the program execution pauses upon delays and no other progress can happen simultaneously:
```
void blink_light()
{
  for(int i=0; i<3; ++i)
  {
    light_on();
    delay(1.0f);
    light_off();
    delay(1.0f);
  }
}
```
The above code is very easy to write and read. For fibers/coroutines the code looks very similar and is thus quite easy to write and read as well. However, with state machines even this kind of simple example turns into quite a complex code to follow:
```
int blink_light(float time_, bool start_)
{
  static int state=2, count;
  static float state_start_time;
  if(start_)
  {
    state=0;
    count=0;
    state_start_time=time_;
    light_on();
  }
  switch(state)
  {
    // light on state
    case 0:
    {
      if(time_-state_start_time>1.0f) // 1 sec delay
      {
        state=1;
        state_start_time=time_;
        light_off();
      }
    } break;
    // light off state
    case 1:
    {
      if(time_-state_start_time>1.0f) // 1 sec delay
      {
        state=0;
        state_start_time=time_;
        if(++count==3)
          state=2;
        else
          light_on();
      }
    } break;
  }
  return state;
}
```
Obviously the first form is much preferred because of its simplicity and scalability to more complex logic. The problem is that C++ doesn't natively support fibers, because each fiber requires its own stack to store the current state of the function (local variables) and C/C++ supports only one stack. However, it's possible to implement support for fibers in C++ with macros and templates, and with some compromises in the syntax. For the purpose I created a [C++ fiber library](https://github.com/JarkkoPFC/fiber) which enables writing C++ fiber functions. Using the library the code looks quite similar to the above blocking function implementation:
```
struct ffunc_blink_light
{
  int i;
  FFUNC()
  {
    FFUNC_BEGIN
    for(i=0; i<3; ++i)
    {
      light_on();
      FFUNC_CALL(ffunc_sleep, (1.0f)); // delay for 1 second (yields)
      light_off();
      FFUNC_CALL(ffunc_sleep, (1.0f)); // delay for 1 second (yields)
    }
    FFUNC_END
  }
};
```
This may appear as a strange form at first for defining a function, but it's almost as easy to write & read as the blocking example code above. Instead of a function, the *blink_light()* is defined as a struct and the function body resides inside *FFUNC()* sandwitched between *FFUNC_BEGIN* and *FFUNC_END* macros. All function local variables must be defined as the struct variables outside of *FFUNC()* definition so that the the state can be stored into a fiber stack upon yielding. Here the *FFUNC_CALL* is a yielding function call that may return control back to the calling function and upon successive calls to the function the execution is resumed to the location where the yielding happened. Furthermore, other fiber functions can call this function with *FFUNC_CALL(ffunc_blink_light, ())* to compose increasingly complex fiber functions.

For each simultaneously executing fiber a stack must be allocated, which retains the state of the fiber function. Then the fiber can be started with *FFUNC_START()* macro and the function is progressed by given delta time between loops with *tick()* function of the stack. Calling *tick()* continues the function execution from the previously yielded location until the function exits.
```
ffunc_callstack cs; // allocate stack to hold the function state
FFUNC_START(cs, ffunc_blink_light, ()); // start blink_light fiber function in the stack
float cur_time=time(), prev_time=cur_time;
while(cs.tick(cur_time-prev_time)) // tick function in the stack until it completes
{
  prev_time=cur_time;
  cur_time=time();
}
```
