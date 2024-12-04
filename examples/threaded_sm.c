#include <pthread.h>
#include <time.h>
#include <unistd.h>

#define SM_IMPLEMENTATION
#define SM_TRACE
#include "sm.h"

typedef struct{
  int event_1;
  int event_2;
} Counter;

// remember that guards only trigger when SM_step can acquire a lock
bool wait_to_final_guard(void* ctx){
  Counter* counter = ctx;
  return counter->event_1 > 5 || counter->event_2 > 5;
}

bool wait_event_1_trigger(void* ctx, void* event){
  (void)(ctx);
  int* event_value = event;
  return *event_value == 1;
}

void wait_event_1_effect(void* ctx){
  Counter* counter = ctx;
  counter->event_1++;
}

bool wait_event_2_trigger(void* ctx, void* event){
  (void)(ctx);
  int* event_value = event;
  return *event_value == 2;
}

void wait_event_2_effect(void* ctx){
  Counter* counter = ctx;
  counter->event_2++;
}

SM_def(sm);

void short_wait(){
  struct timespec remaining_wait, request_wait = {
    .tv_nsec = 1000,
    .tv_sec = 0,
  };
  nanosleep(&request_wait, &remaining_wait);
}

void* thread_worker(void* ctx){
  SM_Context* context = ctx;
  while(!SM_Context_is_halted(context)){

    // steps may not be performed for the following reasons:
    // - mutex lock couldn't be acquired
    // - state machine is halted
    if(!SM_step(sm, context)){
      printf("worker: was not able to perform step!\n");
      short_wait();
    }else{
    }
  }
  return NULL;
}

void* thread_event_generator_1(void* ctx){
  SM_Context* context = ctx;
  int event_value = 1;
  while(!SM_Context_is_halted(context)){

    // events may not be handled for the following reasons:
    // - mutex lock couldn't be acquired
    // - no transitions were triggered
    // - state machine is halted
    if(!SM_notify(sm, context, &event_value)){
      printf("event_generator_1: generated event not handled!\n");
      short_wait();
    }
  }
  return NULL;
}

void* thread_event_generator_2(void* ctx){
  SM_Context* context = ctx;
  int event_value = 2;
  while(!SM_Context_is_halted(context)){
    if(!SM_notify(sm, context, &event_value)){
      printf("event_generator_2: generated event not handled!\n");
      short_wait();
    }
  }
  return NULL;
}

#define BLOCKING_LOCK

// depending on your application you may choose to use a blocking lock or not
// do note that this will make SM_step and SM_notify block
// non-block behaviour is less predictable though, so make sure your state machine is well defined!
#ifdef BLOCKING_LOCK
bool mutex_try_lock(void* mutex){
  pthread_mutex_t* pthread_mutex = mutex;
  pthread_mutex_lock(pthread_mutex);
  return true;
}
#else
bool mutex_try_lock(void* mutex){
  pthread_mutex_t* pthread_mutex = mutex;
  bool lock_acquired = pthread_mutex_trylock(pthread_mutex) == 0;
  return lock_acquired;
}
#endif

void mutex_unlock(void* mutex){
  pthread_mutex_t* pthread_mutex = mutex;
  pthread_mutex_unlock(pthread_mutex); // errors are ignored for simplicity
}

int main(void){
  SM_State_create(wait);

  SM_Transition_create(sm, initial_to_wait, SM_INITIAL_STATE, wait);
  
  SM_Transition_create(sm, wait_event_1, wait, wait);
  SM_Transition_set_trigger(wait_event_1, wait_event_1_trigger);
  SM_Transition_set_effect(wait_event_1, wait_event_1_effect);
  
  SM_Transition_create(sm, wait_event_2, wait, wait);
  SM_Transition_set_trigger(wait_event_2, wait_event_2_trigger);
  SM_Transition_set_effect(wait_event_2, wait_event_2_effect);

  SM_Transition_create(sm, wait_to_final, wait, SM_FINAL_STATE);
  SM_Transition_set_guard(wait_to_final, wait_to_final_guard);

  pthread_mutex_t mutex;
  pthread_mutex_init(&mutex, NULL);
  SM_Context context = {0};
  Counter counter = {0};
  SM_Context_init(&context, &counter);
  SM_Context_set_mutex(&context, &mutex, mutex_try_lock, mutex_unlock);

  SM_step(sm, &context); // initial transiiton

  pthread_t threads[3];
  pthread_create(&threads[0], NULL, thread_worker, &context);
  pthread_create(&threads[1], NULL, thread_event_generator_1, &context);
  pthread_create(&threads[2], NULL, thread_event_generator_2, &context);

  pthread_join(threads[0], NULL);
  pthread_join(threads[1], NULL);
  pthread_join(threads[2], NULL);

  printf("final counts: event_1 = %d, event_2 = %d\n", counter.event_1, counter.event_2);
  return 0;
}
