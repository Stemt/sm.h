# sm.h

A simple single header library for creating statically allocated state machines.

## Usage

### Quick Example

```c
#define SM_IMPLEMENTATION
#define SM_TRACE
#include "../sm.h"

void A_do_action(void* ctx){
  int* value = ctx;
  (*value)+=1;
}

void A_exit_action(void* ctx){
  printf("exiting state A");
}

bool A_to_B_guard(void* ctx){
  int* value = ctx;
  fprintf(stderr,"A_to_B_guard: value = %d\n",*value);
  return *value > 4;
}

void B_enter_action(void* ctx){
  printf("exiting state B\n");
}

void B_do_action(void* ctx){
  int* value = ctx;
  (*value)+=2;
}

bool B_to_final_trigger(void* ctx, void* event){
  int* event_value = event;
  printf("B_to_final_trigger: event_value = %d\n", *event_value);
  return *event_value > 10;
}

int main(void){
  SM_def(sm);

  SM_State_create(A);
  SM_State_set_do_action(A, A_do_action);
  
  SM_State_create(B);
  SM_State_set_do_action(B, B_do_action);

  SM_Transition_create(sm, initial, SM_INITIAL_STATE, A);

  SM_Transition_create(sm, A_to_B, A, B);
  SM_Transition_set_guard(A_to_B, A_to_B_guard);

  SM_Transition_create(sm, B_to_final, B, SM_FINAL_STATE);
  SM_Transition_set_trigger(B_to_final, B_to_final_trigger);

  int value = 0;
  int event_value = 0;
  SM_Context context;
  SM_Context_init(&context, &value);

  while(!SM_Context_is_halted(&context)){
    SM_step(sm, &context);
    SM_notify(sm, &context, &event_value);
    event_value++;
  }

  return 0;
}
```

### Detailed Usage

#### Including

Start by including the header and defining `SM_IMPLEMENTATION` in one of your source files.
Defining `SM_TRACE` will cause the state machine to log any state transitions to stderr.

```c
#define SM_IMPLEMENTATION // only in 1 source file
#define SM_TRACE // log transitions to stderr (for debugging)
#include "sm.h"
```

#### Defining a State Machine

Defining a state machine may be done in either a global or local scope.

```c
SM_def(example_state_machine);
```

#### Creating States

States must be created in a local scope.

```c
... {
    ...
    SM_State_create(example_state);
    ...
}
```

States can have different kinds of actions assigned to them.
- **enter_action**: Is called once when the state is entered.
- **do_action**: Is called when the state is active and `SM_step()` is called, **but not when a transition is triggered**.
- **exit_action**: Is called once when the state is entered.

These actions can be defined as follows.

```c
void enter_action(void* user_context){
    // enter action
}

void do_action(void* user_context){
    // do action
}

void exit_action(void* user_context){
    // exit action
}

... {
    ...

    SM_State_create(example_state);
    SM_State_set_enter_action(  example_state, enter_action );
    SM_State_set_do_action(     example_state, do_action    );
    SM_State_set_exit_action(   example_state, exit_action  );

    ...
}
```

#### Creating Transitions

States are used to create transitions between two states;

```c
    SM_Transition_create(example_state_machine, initial_to_example_state, SM_INITIAL_STATE, example_state);
```
> [!NOTE]
> Also note that there are two special states: `SM_INITIAL_STATE` and `SM_FINAL_STATE`.
> `SM_INITIAL_STATE` is always the first state that a state machine starts with.
> That also means that the state machine **must** contain a transition from `SM_FINAL_STATE` to another state.
> When a transition is defined and triggered to `SM_FINAL_STATE` the state machine will halt.

There are 3 ways that a transition can be triggered.
- **guard without trigger**: If a transition has a guard but no trigger, the transition will be triggered as soon as the guard returns `true` during an `SM_step()` call.
- **trigger**: With a trigger, a transition will only be triggerd if the trigger returns true during an `SM_notify()` call. Though only if it doesn't have a guard or the set guard returns true.
- **no guard or trigger**: Without a guard or trigger the transition will be triggered during an `SM_step()` call, but only if no other transition **with** a guard can be triggered during that same call.

Aside from the guard and trigger the transition can also have an affect.
An effect is an action which is called when the transition is triggered.

```c
bool example_guard(void* user_context){
    return /* condition */;
}

bool example_trigger(void* user_context, void* event){
    return /* condition */;
}

void effect_action(void* user_context){
    // effect action
}

... {
    ...

    SM_Transition_create(example_state_machine, initial_to_example_state, SM_INITIAL_STATE, example_state);
    SM_Transition_set_guard(    initial_to_example_state, example_guard     );
    SM_Transition_set_trigger(  initial_to_example_state, example_trigger   );
    SM_Transition_set_effect(   initial_to_example_state, effect_action     );

    ...
}
```

#### Running Your State Machine

Running a state machine requires a `SM_Context`.
`SM_Context` is used to keep track of the current state of a state machine and if it has been halted or not.
It can also be initialized with a `void*` pointer to your own data, which is passed to all guards, triggers and actions when those are called.

```c
    SM_Context context;
    int custom_data = 0;
    SM_Context_init(&context, &custom_data);
```

Using the `SM_step()` function, the state machine can be made to perform one transition with it's associated actions or one do action.

```c
    SM_step(example_state_machine, &context);
```

For convenience, `SM_run()` can be called to keep calling `SM_step()` automatically until the state machine halts.

```c
    SM_run(example_state_machine, &context);
```

`SM_notify()` can trigger triggers with the associated actions.

```c
    int example_event = 42;
    SM_notify(example_state_machine, &context, &example_event);
```





