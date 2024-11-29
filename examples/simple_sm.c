#include <stdio.h>

#define SM_IMPLEMENTATION // only in 1 source file
#define SM_TRACE // log transitions to stderr (for debugging)
#include "sm.h"

void A_do_action(void* ctx){
  int* value = ctx;
  (*value)+=1;
}

void A_exit_action(void* ctx){
  (void)(ctx);
  printf("exiting state A");
}

bool A_to_B_guard(void* ctx){
  int* value = ctx;
  fprintf(stderr,"A_to_B_guard: value = %d\n",*value);
  return *value > 4;
}

void B_enter_action(void* ctx){
  (void)(ctx);
  printf("exiting state B\n");
}

void B_do_action(void* ctx){
  int* value = ctx;
  (*value)+=2;
}

bool B_to_final_trigger(void* ctx, void* event){
  (void)(ctx);
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
