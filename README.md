# sm.h

A single header library for simply creating statically allocated state machines.

## Usage

### Quick Example

```c
#include <stdio.h>

#define SM_IMPLEMENTATION // only in 1 source file
#define SM_TRACE // log transitions to stderr (for debugging)
#include "sm.h"

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

// define the state machine
SM_def(sm);

int main(void){
  
  // --- create states ---
  
  SM_State_create(A);
  SM_State_set_do_action(A, A_do_action); 
  SM_State_set_exit_action(A, A_exit_action); 

  SM_State_create(B);
  SM_State_set_enter_action(B, B_enter_action); 
  SM_State_set_do_action(B, B_do_action); 

  
  // --- create transitions ---

  // required transition from SM_INITIAL_STATE, 
  SM_Transition_create(sm, initial, SM_INITIAL_STATE, A); 
 
  SM_Transition_create(sm, A_to_B, A, B);
  SM_Transition_set_guard(A_to_B, A_to_B_guard); 

  SM_Transition_create(sm, B_to_final, B, SM_FINAL_STATE);
  SM_Transition_set_trigger(B_to_final, B_to_final_trigger);

  
  // --- setup context ---
  
  int value = 0;
  int event_value = 0;
  SM_Context context;
  SM_Context_init(&context, &value);

  
  // --- running the statemachine ---
  
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
> - `SM_INITIAL_STATE` is always the first state that a state machine starts with.
> That also means that the state machine **must** contain a transition from `SM_INITIAL_STATE` to another state.
> - When a transition to `SM_FINAL_STATE` is triggered, the state machine will halt.
> A halted state machine will not do anything when either `SM_step()` or `SM_notify()` is called.

There are 3 ways that a transition can be triggered.
- **guard without trigger**: If a transition has a guard but no trigger, the transition will be triggered as soon as the guard returns `true` during an `SM_step()` call.
- **trigger**: With a trigger, a transition will only be triggered if the trigger returns true during an `SM_notify()` call. Though only if it doesn't have a guard or the set guard returns true.
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
... {
    ...
    SM_Context context;
    int custom_data = 0;
    SM_Context_init(&context, &custom_data);
    ...
}
```

Using the `SM_step()` function, the state machine can be made to perform one transition with it's associated actions or one do action.

```c
... {
    ...
    SM_step(example_state_machine, &context);
    ...
}
```

For convenience, `SM_run()` can be called to keep calling `SM_step()` automatically until the state machine halts.

```c
... {
    ...
    SM_run(example_state_machine, &context);
    ...
}
```

`SM_notify()` can trigger triggers with the associated actions.

```c
... {
    ...
    int example_event = 42;
    SM_notify(example_state_machine, &context, &example_event);
    ...
}
```

## How Does it Work?

All structures, except for `SM_Context` are statically allocated when using the `def` and `create` macros and are linked to other structures when passed into the respective macros.

For example when taking the quick example above it can modelled as follows in UML.

```mermaid
stateDiagram-v2
  A : A
  A : do / A_do_action()
  A : exit / A_exit_action()
  
  B : B
  B : enter / B_enter_action()
  B : do / B_do_action()


  [*] --> A
  A --> B : [ A_to_B_guard() ]
  B --> [*] : B_to_final_trigger()

```

At runtime the example produces the following linked graph structure.
Note that the classses here actuallly represent statically allocated instances and the stereotype represent the actual type of the instance.

```mermaid
classDiagram
  class sm
  <<SM>> sm
  sm --> initial : initial_transition

  class initial
  <<SM_Transition>> initial
  initial --> A : target
  
  class A
  <<SM_State>> A

  class A_do_action
  <<SM_ActionCallback>> A_do_action
  A --> A_do_action : do_action

  class A_exit_action
  <<SM_ActionCallback>> A_exit_action
  A --> A_exit_action : exit_action

  class A_to_B
  <<SM_Transition>> A_to_B
  A --> A_to_B : first_transition
  A_to_B --> A : source
  A_to_B --> B : target

  class A_to_B_guard
  <<SM_GuardCallback>> A_to_B_guard
  A_to_B --> A_to_B_guard : guard
  
  class B
  <<SM_State>> B
  B --> B_enter_action : enter_action
  B --> B_do_action : do_action

  class B_enter_action
  <<SM_ActionCallback>> B_enter_action
  class B_do_action
  <<SM_ActionCallback>> B_do_action

  class B_to_final
  <<SM_Transition>> B_to_final
  B --> B_to_final : first_transition
  B_to_final --> B : source
  B_to_final --> B_to_final_trigger : trigger

  class B_to_final_trigger
  <<SM_TriggerCallback>> B_to_final_trigger

```

It's also possible to have multiple transitions.
For example to create the following state machine.

```mermaid
stateDiagram-v2
  [*] --> A
  A --> B
  A --> C
```

You'd do the following.

```c
    SM_State_create(A);
    SM_State_create(B);
    SM_State_create(C);

    SM_Transition_create(sm, initial_to_A, SM_INITIAL_STATE, A);

    SM_Transition_create(sm, A_to_B, A, B);
    
    SM_Transition_create(sm, A_to_C, A, C);
```

Which in the state machine graph chains the two transitions together so they can be accessed without the state machine having to allocate memory for an array of transition pointers.
At the same time the transitions themselves will keep track of their source state so they can call the exit action during a transition.

```mermaid
classDiagram
  class A
  <<SM_State>> A
  A --> A_to_B : first_transition

  class A_to_B
  <<SM_Transition>> A_to_B
  A_to_B --> A : source
  A_to_B --> A_to_C : next_transition

  class A_to_C
  <<SM_Transition>> A_to_C
  A_to_C --> A : source
```

## Why Does it Work This Way?

A state machine only describes behavior and does not itself, have to be statefull.
Therefore the behavior can be described using statically allocated nodes as described above.
This is beneficial for memory management as you don't have to worry about allocating space for the the nodes themselves.

The actual state of the machine can then be maintained in a seperate structure which is what `SM_Context` is for.
This can be allocated any way that the user sees fit.
For example if you're entire application only requires one instance of a state machine the context could also be allocated statically.
But if you wan't to be able to have multiple instances you'd want to allocate it on the stack or heap.

For a good example of running multiple instances of a state machine see [examples/game_of_life.c](./examples/game_of_life.c) where each cell of the grid has its own `SM_Context`.

## Examples

To build the example you need to have gnu-make installed and a C compiler that supports atleast C99.

Then to build simply run:
```
make
```
This creates the executables in the `build` directory, which also created if it doesn't yet exist.

### simple_sm.c

Same as the quick example above.

### game_of_life.c

Implements [Game of Life](https://en.wikipedia.org/wiki/Conway%27s_Game_of_Life) by John Conway using a simple state machine for each cell of the grid.
The size of the grid can be modified by changing the `SIZE` define and the amount of steps simulated by setting the `int steps` in the main function.

This example shows how you can use multiple `SM_Context`s to run multiple instances of a statemachine.

### lexer.c

Implements a rudimentary lexer I have previously used to parse CSV files.

## TODO

- Implement event queue for `SM_notify` as currently events are discarded if not immediately handled






