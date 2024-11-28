#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#define SM_IMPLEMENTATION
#include "sm.h"

typedef enum{
  TokenType_UNKNOWN,
  TokenType_WORD,
  TokenType_NUMBER,
  TokenType_SEPERATOR,
  TokenType_NEWLINE,
  TokenType_EOF,
  TokenType_END,
} TokenType;

const char* TokenType_to_str(TokenType type){
  switch (type) {
    case TokenType_UNKNOWN: return "UNKNOWN";
    case TokenType_WORD: return "WORD";
    case TokenType_NUMBER: return "NUMBER";
    case TokenType_SEPERATOR: return "SEPERATOR";
    case TokenType_NEWLINE: return "NEWLINE";
    case TokenType_EOF: return "EOF";
    case TokenType_END: break;
  }
  assert(false && "invalid TokenType");
}

typedef struct{
  const char* str;
  size_t len;
  TokenType type;
} Token;

typedef struct{
  const char* str;
  char seperator;
  Token current_token;
  SM_Context sm_context;
  void (*token_handler)(void* context, Token* token);
  void* token_handler_context;
} Lexer;

bool Lexer_is_word(char ch){
  return ch >= 'a' && ch <= 'z' || ch >= 'A' && ch <= 'Z';
}

bool Lexer_is_number(char ch){
  return ch >= '0' && ch <= '9';
}

void Lexer_new_token(Lexer* self, TokenType type){
  self->current_token.type = type;
  self->current_token.str = self->str;
  self->current_token.len = 1;
  self->str++;
}

void Lexer_same_token(Lexer* self){
  self->current_token.len++;
  self->str++;
}

void Lexer_finalize_token(Lexer* self){
  if(self->token_handler)
    self->token_handler(self->token_handler_context, &self->current_token);
}

bool Lexer_unknown_to_newline_guard(void* context){
  Lexer* self = context;
  return *self->str == '\n';
}

void Lexer_unknown_to_newline_effect(void* context){
  Lexer_new_token(context, TokenType_NEWLINE);
}

bool Lexer_newline_to_unknown_guard(void* context){
  Lexer* self = context;
  return *self->str != '\n';
}

void Lexer_newline_to_unknown_effect(void* context){
  Lexer_finalize_token(context);
}

bool Lexer_unknown_to_word_guard(void* context){
  Lexer* self = context;
  return Lexer_is_word(*self->str);
}

void Lexer_unknown_to_word_effect(void* context){
  Lexer_new_token(context, TokenType_WORD);
}

bool Lexer_word_to_word_guard(void* context){
  Lexer* self = context;
  return Lexer_is_word(*self->str) || *self->str == ' ';
}

void Lexer_word_to_word_effect(void* context){
  Lexer_same_token(context);
}

bool Lexer_word_to_unknown_guard(void* context){
  Lexer* self = context;
  return !Lexer_is_word(*self->str) && *self->str != ' ';
}

void Lexer_word_to_unknown_effect(void* context){
  Lexer_finalize_token(context);
}

bool Lexer_unknown_to_number_guard(void* context){
  Lexer* self = context;
  return Lexer_is_number(*self->str);
}

void Lexer_unknown_to_number_effect(void* context){
  Lexer_new_token(context, TokenType_NUMBER);
}

bool Lexer_number_to_number_guard(void* context){
  Lexer* self = context;
  return Lexer_is_number(*self->str);
}

void Lexer_number_to_number_effect(void* context){
  Lexer_same_token(context);
}

bool Lexer_number_to_unkown_guard(void* context){
  Lexer* self = context;
  return !Lexer_is_number(*self->str);
}

void Lexer_number_to_unkown_effect(void* context){
  Lexer_finalize_token(context);
}

bool Lexer_unknown_to_seperator_guard(void* context){
  Lexer* self = context;
  return *self->str == self->seperator;
}

void Lexer_unknown_to_seperator_effect(void* context){
  Lexer_new_token(context, TokenType_SEPERATOR);
}

void Lexer_seperator_to_unknown_effect(void* context){
  Lexer_finalize_token(context);
}

bool Lexer_unknown_to_eof_guard(void* context){
  Lexer* self = context;
  return *self->str == '\0';
}

void Lexer_unknown_to_eof_effect(void* context){
  Lexer_new_token(context, TokenType_EOF);  
}

void Lexer_eof_to_final_effect(void* context){
  Lexer_finalize_token(context);
}

void Lexer_unknown_to_unknown_effect(void* context){
  Lexer* self = context;
  self->str++;
}

SM_def(sm);

void Lexer_init(Lexer* self, void(*token_handler)(void* context, Token* token), void* token_handler_context){
  self->token_handler = token_handler;
  self->token_handler_context = token_handler_context;
  self->seperator = ',';

  SM_Context_init(&self->sm_context, self);

  if(!sm->init){
    SM_State_create(unknown);
    SM_State_create(newline);
    SM_State_create(word);
    SM_State_create(number);
    SM_State_create(seperator);
    SM_State_create(eof);

    // initial transition
    SM_Transition_create(sm, initial_to_unknown, SM_INITIAL_STATE, unknown);

    // unknown to unknown
    SM_Transition_create(sm,unknown_to_unknown, unknown, unknown);
    SM_Transition_set_effect(unknown_to_unknown, Lexer_unknown_to_unknown_effect);

    // newline transitions
    SM_Transition_create(sm, unknown_to_newline, unknown, newline);
    SM_Transition_set_guard(unknown_to_newline, Lexer_unknown_to_newline_guard);
    SM_Transition_set_effect(unknown_to_newline, Lexer_unknown_to_newline_effect);

    SM_Transition_create(sm, newline_to_unknown, newline, unknown);
    SM_Transition_set_guard(newline_to_unknown, Lexer_newline_to_unknown_guard);
    SM_Transition_set_effect(newline_to_unknown, Lexer_newline_to_unknown_effect);

    // word transitions
    SM_Transition_create(sm, unknown_to_word, unknown, word);
    SM_Transition_set_guard(unknown_to_word, Lexer_unknown_to_word_guard);
    SM_Transition_set_effect(unknown_to_word, Lexer_unknown_to_word_effect);

    SM_Transition_create(sm, word_to_word, word, word);
    SM_Transition_set_guard(word_to_word, Lexer_word_to_word_guard);
    SM_Transition_set_effect(word_to_word, Lexer_word_to_word_effect);
    
    SM_Transition_create(sm, word_to_unknown, word, unknown);
    SM_Transition_set_guard(word_to_unknown, Lexer_word_to_unknown_guard);
    SM_Transition_set_effect(word_to_unknown, Lexer_word_to_unknown_effect);
    
    // number transitions
    SM_Transition_create(sm, unknown_to_number, unknown, number);
    SM_Transition_set_guard(unknown_to_number, Lexer_unknown_to_number_guard);
    SM_Transition_set_effect(unknown_to_number, Lexer_unknown_to_number_effect);
    
    SM_Transition_create(sm, number_to_number, number, number);
    SM_Transition_set_guard(number_to_number, Lexer_number_to_number_guard);
    SM_Transition_set_effect(number_to_number, Lexer_number_to_number_effect);
    
    SM_Transition_create(sm, number_to_unknown, number, unknown);
    SM_Transition_set_guard(number_to_unknown, Lexer_number_to_unkown_guard);
    SM_Transition_set_effect(number_to_unknown, Lexer_number_to_unkown_effect);

    // seperator transitions
    SM_Transition_create(sm, unknown_to_seperator, unknown, seperator);
    SM_Transition_set_guard(unknown_to_seperator, Lexer_unknown_to_seperator_guard);
    SM_Transition_set_effect(unknown_to_seperator, Lexer_unknown_to_seperator_effect);
    
    SM_Transition_create(sm, seperator_to_unknown, seperator, unknown);
    SM_Transition_set_effect(seperator_to_unknown, Lexer_seperator_to_unknown_effect);
    
    // eof transition
    SM_Transition_create(sm, unknown_to_eof, unknown, eof);
    SM_Transition_set_guard(unknown_to_eof, Lexer_unknown_to_eof_guard);
    SM_Transition_set_effect(unknown_to_eof, Lexer_unknown_to_eof_effect);

    // final transition
    SM_Transition_create(sm, eof_to_final, eof, SM_FINAL_STATE);
    SM_Transition_set_effect(eof_to_final, Lexer_eof_to_final_effect);
  }
}

void Lexer_lex(Lexer* self, const char* str){
  self->str = str;
  SM_run(sm, &self->sm_context);
}

void print_token(void* ctx, Token* token){
  printf("token: %s, '%.*s'\n", TokenType_to_str(token->type), (int)token->len, token->str);
}

int main(void){

  const char* text = "test, 123, end\n";
  printf("Lexing the following text: '%s'\n", text);
  Lexer lexer = {0};
  Lexer_init(&lexer, print_token, NULL);
  Lexer_lex(&lexer, text);

  return 0;
}
