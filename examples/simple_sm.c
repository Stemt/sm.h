
#define SM_IMPLEMENTATION
#define SM_TRACE
#include "../sm.h"

void A_do_action(void* ctx){
  int* value = ctx;
  (*value)+=1;
}

bool A_to_B_guard(void* ctx){
  int* value = ctx;
  fprintf(stderr,"A_to_B_guard: value = %d\n",*value);
  return *value > 4;
}

void B_do_action(void* ctx){
  int* value = ctx;
  (*value)+=2;
}

bool B_to_final_guard(void* ctx){
  int* value = ctx;
  fprintf(stderr, "B_to_A_guard: value = %d\n", *value);
  return *value > 10;
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
  SM_Transition_set_guard(B_to_final, B_to_final_guard);

  int value = 0;
  SM_Context context;
  SM_Context_init(&context, &value);

  SM_run(sm, &context);

  return 0;
}
