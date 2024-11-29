#define SM_IMPLEMENTATION
#include "sm.h"

#include "utest.h"

UTEST(SM_Transitions, initialization){
  SM_def(sm);

  SM_State_create(A);

  SM_Transition_create(sm, transition, SM_INITIAL_STATE, A);

  SM_Context context;
  SM_Context_init(&context, NULL);

  // current state after Context_init() should always be SM_INITIAL_STATE
  ASSERT_EQ(context.current_state, SM_INITIAL_STATE);
}

UTEST(SM_Transitions, initial_to_other){
  SM_def(sm);

  SM_State_create(A);
  
  SM_Transition_create(sm, transition, SM_INITIAL_STATE, A);

  SM_Context context;
  SM_Context_init(&context, NULL);
  
  // a lone transition from a state without guard or trigger will always fire during SM_step()
  SM_step(sm, &context);

  ASSERT_EQ(context.current_state, A);
}

bool TEST_SM_Transitions_guard(void* ctx){
  bool* test_context = ctx;
  return *test_context;
}

UTEST(SM_Transitions, initial_to_other_with_guard){
  SM_def(sm);

  SM_State_create(A);
  
  SM_Transition_create(sm, transition, SM_INITIAL_STATE, A);
  SM_Transition_set_guard(transition, TEST_SM_Transitions_guard);
  
  // context variable passed to guard
  bool test_context = false;

  SM_Context context;
  SM_Context_init(&context, &test_context);

  // guard return false due to context so no transition is triggered
  SM_step(sm, &context);
  ASSERT_EQ(context.current_state, SM_INITIAL_STATE);

  // once guard does return true transition will trigger and the current state changes
  test_context = true;
  SM_step(sm, &context);
  ASSERT_EQ(context.current_state, A);
}

bool TEST_SM_Transitions_trigger(void* ctx, void* event){
  bool* test_event = event;
  return *test_event;
}

UTEST(SM_Transitions, initial_to_other_with_trigger){
  SM_def(sm);

  SM_State_create(A);
  
  SM_Transition_create(sm, transition, SM_INITIAL_STATE, A);
  SM_Transition_set_trigger(transition, TEST_SM_Transitions_trigger);

  SM_Context context;
  SM_Context_init(&context, NULL);

  // SM_step() has no effect on transition with trigger so no transition happens
  SM_step(sm, &context);
  ASSERT_EQ(context.current_state, SM_INITIAL_STATE);
  
  // trigger is called but returns false due to event meaning no transition occurs
  bool test_event = false;
  SM_notify(sm, &context, &test_event);
  ASSERT_EQ(context.current_state, SM_INITIAL_STATE);

  // trigger is called and returns true causing the transition to happen
  test_event = true;
  SM_notify(sm, &context, &test_event);
  ASSERT_EQ(context.current_state, A);
}

UTEST(SM_Transitions, initial_to_other_with_guard_and_trigger){
  SM_def(sm);

  SM_State_create(A);
  
  SM_Transition_create(sm, transition, SM_INITIAL_STATE, A);
  SM_Transition_set_guard(transition, TEST_SM_Transitions_guard);
  SM_Transition_set_trigger(transition, TEST_SM_Transitions_trigger);

  bool test_context = true;
  SM_Context context;
  SM_Context_init(&context, &test_context);

  // SM_step() here should have no effect because the transition has a trigger
  SM_step(sm, &context);
  ASSERT_EQ(context.current_state, SM_INITIAL_STATE);
  
  // transition does not occur because trigger returned false due to event
  bool test_event = false;
  SM_notify(sm, &context, &test_event);
  ASSERT_EQ(context.current_state, SM_INITIAL_STATE);

  // transition does not occur because guard returned false even though the trigger returned true
  test_context = false;
  test_event = true;
  SM_notify(sm, &context, &test_event);
  ASSERT_EQ(context.current_state, SM_INITIAL_STATE);

  // transition occurs because guard and trigger return true
  test_context = true;
  SM_notify(sm, &context, &test_event);
  ASSERT_EQ(context.current_state, A);
}

void TEST_SM_Transitions_effect(void* ctx){
  bool* test_context = ctx;
  *test_context = true;
}

UTEST(SM_Transitions, initial_to_other_with_effect){
  SM_def(sm);

  SM_State_create(A);
  
  SM_Transition_create(sm, transition, SM_INITIAL_STATE, A);
  SM_Transition_set_effect(transition, TEST_SM_Transitions_effect);

  bool test_context = false;
  SM_Context context;
  SM_Context_init(&context, &test_context);

  SM_step(sm, &context);
  ASSERT_EQ(context.current_state, A);
  ASSERT_TRUE(test_context);
}

void TEST_SM_Transitions_unguarded_effect(void* ctx){
  bool* test_context = ctx;
  test_context[1] = true;
}

bool TEST_SM_Transitions_priority_guard(void* ctx){
  return true;
}

void TEST_SM_Transitions_guarded_effect(void* ctx){
  bool* test_context = ctx;
  test_context[0] = true;
}

UTEST(SM_Transitions, guard_vs_no_guard_priority){
  SM_def(sm);

  SM_State_create(A);
  
  SM_Transition_create(sm, unguarded_transition, SM_INITIAL_STATE, A);
  SM_Transition_set_effect(unguarded_transition, TEST_SM_Transitions_unguarded_effect);

  SM_Transition_create(sm, guarded_transition, SM_INITIAL_STATE, A);
  SM_Transition_set_guard(guarded_transition, TEST_SM_Transitions_priority_guard);
  SM_Transition_set_effect(guarded_transition, TEST_SM_Transitions_guarded_effect);

  bool test_context[2] = {0};
  SM_Context context;
  SM_Context_init(&context, test_context);

  // the bool at index 0 should be set true by the guarded effect
  // and the one at index 1 should be false because the unguarded transition didn't trigger
  SM_step(sm, &context);
  ASSERT_EQ(context.current_state, A);
  ASSERT_TRUE(test_context[0]);
  ASSERT_FALSE(test_context[1]);
}

void TEST_SM_States_enter(void* ctx){
  bool* test_context = ctx;
  *test_context = true;
}

UTEST(SM_States, state_enter){
  SM_def(sm);

  SM_State_create(A);
  SM_State_set_enter_action(A, TEST_SM_States_enter);

  SM_Transition_create(sm, initial_to_A, SM_INITIAL_STATE, A);
  SM_Transition_create(sm, A_to_final, A, SM_FINAL_STATE);

  bool test_context = false;
  SM_Context context;
  SM_Context_init(&context, &test_context);
  
  // enter is called during the first transition (initial_to_A)
  ASSERT_FALSE(test_context);
  SM_step(sm, &context);
  ASSERT_TRUE(test_context);
}

void TEST_SM_States_exit(void* ctx){
  bool* test_context = ctx;
  *test_context = true;
}

UTEST(SM_States, state_exit){
  SM_def(sm);

  SM_State_create(A);
  SM_State_set_exit_action(A, TEST_SM_States_exit);

  SM_Transition_create(sm, initial_to_A, SM_INITIAL_STATE, A);
  SM_Transition_create(sm, A_to_final, A, SM_FINAL_STATE);

  bool test_context = false;
  SM_Context context;
  SM_Context_init(&context, &test_context);

  // exit is called on the second transition (A_to_final)
  ASSERT_FALSE(test_context);
  SM_step(sm, &context);
  ASSERT_FALSE(test_context);
  SM_step(sm, &context);
  ASSERT_TRUE(test_context);
}

void TEST_SM_States_do(void* ctx){
  bool* test_context = ctx;
  *test_context = true;
}

UTEST(SM_States, state_skip_do){
  SM_def(sm);

  SM_State_create(A);
  SM_State_set_do_action(A, TEST_SM_States_do);

  SM_Transition_create(sm, initial_to_A, SM_INITIAL_STATE, A);
  SM_Transition_create(sm, A_to_final, A, SM_FINAL_STATE);

  bool test_context = false;
  SM_Context context;
  SM_Context_init(&context, &test_context);

  // do is not called as a transition occurs in both steps
  ASSERT_FALSE(test_context);
  SM_step(sm, &context);
  ASSERT_FALSE(test_context);
  SM_step(sm, &context);
  ASSERT_FALSE(test_context);
  ASSERT_EQ(context.current_state, SM_FINAL_STATE);
}

UTEST(SM_States, state_do){
  SM_def(sm);

  SM_State_create(A);
  SM_State_set_do_action(A, TEST_SM_States_do);

  SM_Transition_create(sm, initial_to_A, SM_INITIAL_STATE, A);
  SM_Transition_create(sm, A_to_final, A, SM_FINAL_STATE);
  SM_Transition_set_guard(A_to_final, TEST_SM_Transitions_guard);

  bool test_context = false;
  SM_Context context;
  SM_Context_init(&context, &test_context);

  // do is called as the A_to_final is blocked by the guard 
  ASSERT_FALSE(test_context);
  SM_step(sm, &context);
  ASSERT_FALSE(test_context);
  SM_step(sm, &context);
  ASSERT_TRUE(test_context);
  SM_step(sm, &context);
  ASSERT_EQ(context.current_state, SM_FINAL_STATE);
}

UTEST_MAIN();
