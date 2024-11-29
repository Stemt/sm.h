#define SM_IMPLEMENTATION

#ifndef SM_H_
#define SM_H_

#include <stdbool.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>

typedef void (*ActionCallback)(void* user_context);

typedef struct{
  ActionCallback enter_action;
  ActionCallback do_action;
  ActionCallback exit_action;
  const char* trace_name;
  void* transition;
  bool init;
} SM_State;

#define SM_State_create(state)\
  static SM_State (_##state) = {0};\
  SM_State* const (state) = &(_##state);\
  assert((state)->init == false && "attempted redefinition of state: "#state);\
  SM_State_set_trace_name((state), (#state));\
  SM_State_init(state)

void SM_State_init(SM_State* self);
void SM_State_set_enter_action(SM_State* self, ActionCallback action);
void SM_State_set_do_action(SM_State* self, ActionCallback action);
void SM_State_set_exit_action(SM_State* self, ActionCallback action);
void SM_State_enter(SM_State* self, void* user_context);
void SM_State_do(SM_State* self, void* user_context);
void SM_State_exit(SM_State* self, void* user_context);

typedef bool (*GuardCallback)(void* user_context);
typedef bool (*TriggerCallback)(void* user_context, void* event);

typedef struct{
  TriggerCallback trigger;
  GuardCallback guard;
  ActionCallback effect;  
  SM_State* source;
  SM_State* target;
  void* next_transition;
  bool init;
} SM_Transition;

#define SM_Transition_create(sm, transition, source_state, target_state)\
  static SM_Transition (_##transition) = {0};\
  SM_Transition* const (transition) = &(_##transition);\
  assert((transition)->init == false && "attempted redefinition of transition: "#transition);\
  SM_Transition_init((transition), (source_state), (target_state));\
  SM_add_transition((sm), (transition))

void SM_Transition_set_trigger(SM_Transition* self, TriggerCallback trigger);
void SM_Transition_set_guard(SM_Transition* self, GuardCallback guard);
void SM_Transition_set_effect(SM_Transition* self, ActionCallback effect);
bool SM_Transition_has_trigger_or_guard(SM_Transition* self);
bool SM_Transition_check_guard(SM_Transition* self, void* user_context);
bool SM_Transition_check_trigger(SM_Transition* self, void* user_context, void* event);
void SM_Transition_apply_effect(SM_Transition* self, void* user_context);

typedef struct{
  void* user_context;
  SM_State* current_state;
  bool halted;
} SM_Context;

void SM_Context_init(SM_Context* self, void* user_context);
void SM_Context_reset(SM_Context* self);
bool SM_Context_is_halted(SM_Context* self);

#define SM_INITIAL_STATE NULL
#define SM_FINAL_STATE NULL

typedef struct{
  size_t transition_count;
  SM_Transition** transitions;
  SM_Transition* initial_transition;
  bool init;
} SM;

#define SM_def(sm)\
  static SM (_##sm) = {0};\
  static SM* (sm) = &(_##sm)

#define SM_init(sm, ...)\
  assert((sm)->init == false && "attempted redefinition of state machine: "#sm);\
  _SM_init(&(sm))

void _SM_init(SM* self);
void SM_step(SM* self, SM_Context* context);
void SM_notify(SM* self, SM_Context* context, void* event);
void SM_run(SM* self, SM_Context* context);

#ifdef SM_IMPLEMENTATION

void SM_State_init(SM_State* self){
  self->init = true;
}

void SM_State_set_trace_name(SM_State* self, const char* trace_name){
  self->trace_name = trace_name;
}

const char* SM_State_get_trace_name(SM_State* self){
  if(self != SM_INITIAL_STATE){
    if(self->trace_name == NULL) return "!state missing trace name!";
    return self->trace_name;
  }
  return "initial/final";
}

void SM_State_set_enter_action(SM_State* self, ActionCallback action){
  self->enter_action = action;
}

void SM_State_set_do_action(SM_State* self, ActionCallback action){
  self->do_action = action;
}

void SM_State_set_exit_action(SM_State* self, ActionCallback action){
  self->exit_action = action;
}

void SM_State_enter(SM_State* self, void* user_context){
  if(self && self->enter_action) self->enter_action(user_context); 
}

void SM_State_do(SM_State* self, void* user_context){
  if(self && self->do_action) self->do_action(user_context); 
}

void SM_State_exit(SM_State* self, void* user_context){
  if(self && self->exit_action) self->exit_action(user_context); 
}

void SM_Transition_init(SM_Transition* self, SM_State* source, SM_State* target){
  self->source = source;
  self->target = target;
  self->init = true;
}

void SM_Transition_set_trigger(SM_Transition* self, TriggerCallback trigger){
  self->trigger = trigger;
}

void SM_Transition_set_guard(SM_Transition* self, GuardCallback guard){
  self->guard = guard;
}

void SM_Transition_set_effect(SM_Transition* self, ActionCallback effect){
  self->effect = effect;
}

bool SM_Transition_has_trigger(SM_Transition* self){
  return self->trigger != NULL;
}

bool SM_Transition_has_guard(SM_Transition* self){
  return self->guard != NULL;
}

bool SM_Transition_has_trigger_or_guard(SM_Transition* self){
  return SM_Transition_has_trigger(self) || SM_Transition_has_guard(self);
}

bool SM_Transition_check_guard(SM_Transition* self, void* user_context){
  if(self->guard) 
    return self->guard(user_context);
  return false;
}

bool SM_Transition_check_trigger(SM_Transition* self, void* user_context, void* event){
  if(self->trigger) 
    return self->trigger(user_context, event);
  return false;
}

void SM_Transition_apply_effect(SM_Transition* self, void* user_context){
  if(self->effect) self->effect(user_context);
}

void SM_Transition_add_to_chain(SM_Transition* current, SM_Transition* new_transition){
  while(current->next_transition != NULL){
    current = current->next_transition;
  }
  current->next_transition = new_transition;
}

void SM_State_add_transition(SM_State* self, SM_Transition* new_transition){
  //printf("sm state (%p) add transition: %p\n", self, new_transition);
  if(self->transition == NULL){
    self->transition = new_transition;
  }else{
    SM_Transition_add_to_chain(self->transition, new_transition);
  }
}

void SM_add_transition(SM* self, SM_Transition* transition){
  if(transition->source != SM_INITIAL_STATE){
    SM_State_add_transition(transition->source, transition);
  }else{
    if(self->initial_transition == NULL){
      self->initial_transition = transition;
    }else{
      SM_Transition_add_to_chain(self->initial_transition, transition);
    }
  }
}

void SM_Context_init(SM_Context* self, void* user_context){
  self->user_context = user_context;
  self->current_state = SM_INITIAL_STATE;
  self->halted = false;
}

void SM_Context_reset(SM_Context* self){
  self->current_state = SM_INITIAL_STATE;
  self->halted = false;
}

bool SM_Context_is_halted(SM_Context* self){
  return self->halted;
}

void _SM_init(SM* self){
  self->init = true;
}

void SM_transition(SM* self, SM_Transition* transition, SM_Context* context){
#ifdef SM_TRACE
  fprintf(stderr,"SM_TRACE: transition triggered: '%s' -> '%s'\n", 
      SM_State_get_trace_name(transition->source),
      SM_State_get_trace_name(transition->target));
#endif
  SM_State_exit(context->current_state, context->user_context);
  SM_Transition_apply_effect(transition, context->user_context);
  SM_State_enter(transition->target, context->user_context);
  context->current_state = transition->target;
  if(context->current_state == SM_FINAL_STATE){
    context->halted = true;
  }
}

SM_Transition* SM_get_next_transition(SM* self, SM_Context* context, SM_Transition* transition){
  if(!transition){
    if(context->current_state == SM_INITIAL_STATE){
      return self->initial_transition;
    }else{
      return (SM_Transition*) context->current_state->transition;
    }
  }else{
    return (SM_Transition*) transition->next_transition;
  }
}

void SM_step(SM* self, SM_Context* context){
  assert(self->initial_transition && "atleast one transition from SM_INITIAL_STATE must be created");
  if(context->halted) return;
  
  // check all guards without triggers first
  for(SM_Transition* transition = SM_get_next_transition(self, context, NULL); 
      transition != NULL; 
      transition = SM_get_next_transition(self, context, transition))
  {
    assert(transition->source == context->current_state && "transition not valid for current state");
    if(!SM_Transition_has_trigger(transition) &&
        SM_Transition_check_guard(transition, context->user_context))
    {
      SM_transition(self, transition, context);
      return;
    }
  }

  // check any transitions without triggers and guards
  for(SM_Transition* transition = SM_get_next_transition(self, context, NULL); 
      transition != NULL; 
      transition = SM_get_next_transition(self, context, transition))
  {
    assert(transition->source == context->current_state && "transition not valid for current state");
    if(!SM_Transition_has_trigger_or_guard(transition))
    {
      SM_transition(self, transition, context);
      return;
    }
  }

  SM_State_do(context->current_state, context->user_context);
}

// TODO add event to an eventqueue so events dont expire if not handled
void SM_notify(SM* self, SM_Context* context, void* event){
  if(context->halted) return;

  for(SM_Transition* transition = SM_get_next_transition(self, context, NULL); 
      transition != NULL; 
      transition = SM_get_next_transition(self, context, transition))
  {
    assert(transition->source == context->current_state);
    if(SM_Transition_check_trigger(transition, context->user_context, event))
    {
      SM_transition(self, transition, context);
      return;
    }
  }
}

void SM_run(SM* self, SM_Context* context){
  while(!context->halted){
    SM_step(self, context);
  };
}

#endif // SM_IMPLEMENTATION

#endif // SM_H_
