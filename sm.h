
#ifndef SM_H_
#define SM_H_

#include <stdbool.h>
#include <stdlib.h>
#include <assert.h>

#ifdef SM_TRACE
#include <stdio.h>
#endif

#ifndef SM_PREFIX
#define SM_PREFIX SM_
#endif

typedef void (*SM_ActionCallback)(void* user_context);

typedef struct{
  SM_ActionCallback enter_action;
  SM_ActionCallback do_action;
  SM_ActionCallback exit_action;
  const char* trace_name;
  void* transition;
  bool init;
} SM_State;

/**
 * \brief         The SM_State_create macro is used to define and init a new state
 * \param state:  name of the new state to be created
 */
#define SM_State_create(state)\
  static SM_State (SM_PREFIX##state) = {0};\
  SM_State* const (state) = &(SM_PREFIX##state);\
  assert((state)->init == false && "attempted redefinition of state: "#state);\
  SM_State_set_trace_name((state), (#state));\
  SM_State_init(state)

/**
 * \brief         initializes given state
 * \param self:   state handle
 */
void SM_State_init(SM_State* self);

/**
 * \brief           sets the enter_action callback 
 * \note            enter_aciton is called upon state entry right after a transition
 * \param self:     state handle
 * \param action:   action callback
 */
void SM_State_set_enter_action(SM_State* self, SM_ActionCallback action);

/**
 * \brief             sets the do_action callback 
 * \note              do_action is called if the state is active and no transition occurs during an SM_step() call
 * \param self:       state handle
 * \param do_action:  action callback
 */
void SM_State_set_do_action(SM_State* self, SM_ActionCallback action);

/**
 * \brief                 sets the exit_action callback 
 * \note                  exit_aciton is called right before a transition occurs
 * \param self:           state handle
 * \param enter_action:   action callback
 */
void SM_State_set_exit_action(SM_State* self, SM_ActionCallback action);

typedef bool (*SM_GuardCallback)(void* user_context);
typedef bool (*SM_TriggerCallback)(void* user_context, void* event);

typedef struct{
  SM_TriggerCallback trigger;
  SM_GuardCallback guard;
  SM_ActionCallback effect;  
  SM_State* source;
  SM_State* target;
  void* next_transition;
  bool init;
} SM_Transition;

/**
 * \brief                 defines, initializes a new transition and adds it to the given statemachine 
 * \param sm:             a state machine handle
 * \param transition:     name of the transition to be defined
 * \param source_state:   state to transition from
 * \param target_state:   state to transition to
 */
#define SM_Transition_create(sm, transition, source_state, target_state)\
  static SM_Transition (SM_PREFIX##transition) = {0};\
  SM_Transition* const (transition) = &(SM_PREFIX##transition);\
  assert((transition)->init == false && "attempted redefinition of transition: "#transition);\
  SM_Transition_init((transition), (source_state), (target_state));\
  SM_add_transition((sm), (transition))

/**
 * \brief               initializes a transition to the given statemachine 
 * \param self:         transition handle
 * \param source_state: state to transition from
 * \param target_state: state to transition to
 */
void SM_Transition_init(SM_Transition* self, SM_State* source_state, SM_State* target_state);

/**
 * \brief           sets the trigger callback
 * \note            trigger is called during a SM_notify() call when the source state is active and a guard is not set or the guard returns true.
 * \param self:     transition handle
 * \param trigger:  trigger callback
 */
void SM_Transition_set_trigger(SM_Transition* self, SM_TriggerCallback trigger);

/**
 * \brief         sets the guard callback
 * \note          a guard is called during either a SM_notify() or SM_step()
 * \param self:   transition handle
 * \param guard:  guard callback
 */
void SM_Transition_set_guard(SM_Transition* self, SM_GuardCallback guard);

/**
 * \brief           sets the effect callback
 * \note            an effect is called in the middle of a state transition
 * \param self:     transition handle
 * \param effect:   effect action callback
 */
void SM_Transition_set_effect(SM_Transition* self, SM_ActionCallback effect);

typedef struct{
  void* user_context;
  SM_State* current_state;
  bool halted;
} SM_Context;

/**
 * \brief                 initializes the given context
 * \param self:           context handle
 * \param user_context:   custom pointer which will be passed to any action, guard and trigger callbacks when those are called
 */
void SM_Context_init(SM_Context* self, void* user_context);

/**
 * \brief         reinitializes the context but keeps the user_context
 * \param self:   context handle
 */
void SM_Context_reset(SM_Context* self);

/**
 * \brief         checks if the current state machine context is halted
 * \param self:   context handle
 */
bool SM_Context_is_halted(SM_Context* self);

#define SM_INITIAL_STATE NULL
#define SM_FINAL_STATE NULL

typedef struct{
  size_t transition_count;
  SM_Transition** transitions;
  SM_Transition* initial_transition;
  bool init;
} SM;

/**
 * \brief       defines a new state machine
 * \note        can be defined in global and local scope
 * \param sm:   new statemachine name
 */
#define SM_def(sm)\
  static SM (SM_PREFIX##sm) = {0};\
  static SM* (sm) = &(SM_PREFIX##sm)

/**
 * \brief               adds the given transition to the state machine linked graph structure
 * \param self:         state machine handle
 * \param transition:   transition handle
 */
void SM_add_transition(SM* self, SM_Transition* transition);

/**
 * \brief           performs one transition if possible or executes the do_action of the current state
 * \param self:     state machine handle
 * \param context:  context handle
 */
void SM_step(SM* self, SM_Context* context);

/**
 * \brief notifies triggers of transitions from the current state of the given event
 * \param self: state machine handle
 * \param context: context handle
 * \param event: pointer to custom event type that is passed to the triggers that are checked during this call
 */
void SM_notify(SM* self, SM_Context* context, void* event);

/**
 * \brief           runs SM_step() continuously until SM_Context_is_halted() returns false
 * \param self:     state machine handle
 * \param context:  context handle
 */
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

void SM_State_set_enter_action(SM_State* self, SM_ActionCallback action){
  self->enter_action = action;
}

void SM_State_set_do_action(SM_State* self, SM_ActionCallback action){
  self->do_action = action;
}

void SM_State_set_exit_action(SM_State* self, SM_ActionCallback action){
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

void SM_Transition_set_trigger(SM_Transition* self, SM_TriggerCallback trigger){
  self->trigger = trigger;
}

void SM_Transition_set_guard(SM_Transition* self, SM_GuardCallback guard){
  self->guard = guard;
}

void SM_Transition_set_effect(SM_Transition* self, SM_ActionCallback effect){
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
  if(self->transition == NULL){
    self->transition = new_transition;
  }else{
    SM_Transition_add_to_chain(self->transition, new_transition);
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
  (void)(self);
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


SM_Transition* SM_get_next_transition(SM* self, SM_Context* context, SM_Transition* transition){
  if(transition == NULL){
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
    if( (!SM_Transition_has_guard(transition) || SM_Transition_check_guard(transition, context->user_context)) &&
        SM_Transition_check_trigger(transition, context->user_context, event))
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
