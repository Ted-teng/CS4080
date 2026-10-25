#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "compiler.h"
#include "memory.h"
#include "localmap.h"
#include "vm.h"
#include "scanner.h"

#ifdef DEBUG_PRINT_CODE
#include "debug.h"
#endif

typedef struct {
  Token current;
  Token previous;
  bool hadError;
  bool panicMode;
} Parser;

typedef enum {
  PREC_NONE,
  PREC_ASSIGNMENT,  // =
  PREC_OR,          // or
  PREC_AND,         // and
  PREC_EQUALITY,    // == !=
  PREC_COMPARISON,  // < > <= >=
  PREC_TERM,        // + -
  PREC_FACTOR,      // * /
  PREC_UNARY,       // ! -
  PREC_CALL,        // . ()
  PREC_PRIMARY
} Precedence;

typedef void (*ParseFn)(bool canAssign);

typedef struct {
  ParseFn prefix;
  ParseFn infix;
  Precedence precedence;
} ParseRule;

typedef struct {
  Token name;
  int depth;
  bool isConst;
#ifndef CLOX_LINEAR_LOCALS
  int previous;
#endif
} Local;

typedef struct {
  Local* locals;
  int localCapacity;
  int localCount;
  int scopeDepth;
#ifndef CLOX_LINEAR_LOCALS
  LocalMap bindings;
#endif
} Compiler;

Parser parser;
Compiler* current = NULL;
Chunk* compilingChunk;

// Chapter 22, Challenge 3: Collect global constants before compilation.
static Table sourceConstants;
static bool initializingGlobalConst;
static Token globalConstName;

static Chunk* currentChunk() {
  return compilingChunk;
}

static void errorAt(Token* token, const char* message) {
  if (parser.panicMode) return;
  parser.panicMode = true;
  fprintf(stderr, "[line %d] Error", token->line);

  if (token->type == TOKEN_EOF) {
    fprintf(stderr, " at end");
  } else if (token->type == TOKEN_ERROR) {
    // Nothing.
  } else {
    fprintf(stderr, " at '%.*s'", token->length, token->start);
  }

  fprintf(stderr, ": %s\n", message);
  parser.hadError = true;
}
static void error(const char* message) {
  errorAt(&parser.previous, message);
}
static void errorAtCurrent(const char* message) {
  errorAt(&parser.current, message);
}

static void advance() {
  parser.previous = parser.current;

  for (;;) {
    parser.current = scanToken();
    if (parser.current.type != TOKEN_ERROR) break;

    errorAtCurrent(parser.current.start);
  }
}
static void consume(TokenType type, const char* message) {
  if (parser.current.type == type) {
    advance();
    return;
  }

  errorAtCurrent(message);
}
static bool check(TokenType type) {
  return parser.current.type == type;
}
static bool match(TokenType type) {
  if (!check(type)) return false;
  advance();
  return true;
}
static void emitByte(uint8_t byte) {
  writeChunk(currentChunk(), byte, parser.previous.line);
}
static void emitBytes(uint8_t byte1, uint8_t byte2) {
  emitByte(byte1);
  emitByte(byte2);
}
// Chapter 22, Challenge 4: Local operands use two bytes, high byte first.
static void emitLocal(uint8_t instruction, int slot) {
  emitByte(instruction);
  emitByte((uint8_t)(slot >> 8));
  emitByte((uint8_t)slot);
}
static void emitReturn() {
  emitByte(OP_RETURN);
}
static uint8_t makeConstant(Value value) {
  int constant = addConstant(currentChunk(), value);
  if (constant > UINT8_MAX) {
    error("Too many constants in one chunk.");
    return 0;
  }

  return (uint8_t)constant;
}
static void emitConstant(Value value) {
  emitBytes(OP_CONSTANT, makeConstant(value));
}
static void initCompiler(Compiler* compiler) {
  compiler->locals = NULL;
  compiler->localCapacity = 0;
  compiler->localCount = 0;
  compiler->scopeDepth = 0;
#ifndef CLOX_LINEAR_LOCALS
  initLocalMap(&compiler->bindings);
#endif
  current = compiler;
}
static void endCompiler() {
  emitReturn();
#ifdef DEBUG_PRINT_CODE
  if (!parser.hadError) {
    disassembleChunk(currentChunk(), "code");
  }
#endif
}
static void beginScope() {
  current->scopeDepth++;
}
static void endScope() {
  current->scopeDepth--;

  while (current->localCount > 0 &&
         current->locals[current->localCount - 1].depth >
            current->scopeDepth) {
    emitByte(OP_POP);
#ifndef CLOX_LINEAR_LOCALS
    // Restore the outer binding before removing the inner local.
    Local* local = &current->locals[current->localCount - 1];
    if (local->previous == -1) {
      deleteLocalMap(&current->bindings, &local->name);
    } else {
      setLocalMap(&current->bindings, local->name, local->previous);
    }
#endif
    current->localCount--;
  }
}

static void expression();
static void statement();
static void declaration();
static ParseRule* getRule(TokenType type);
static void parsePrecedence(Precedence precedence);

static uint8_t identifierConstant(Token* name) {
  return makeConstant(OBJ_VAL(copyString(name->start,
                                         name->length)));
}
static bool identifiersEqual(Token* a, Token* b) {
  if (a->length != b->length) return false;
  return memcmp(a->start, b->start, a->length) == 0;
}
// Both compiler variants use this entry point; the benchmark times it too.
static int findLocalSlot(Compiler* compiler, Token* name) {
#ifdef CLOX_LINEAR_LOCALS
  for (int i = compiler->localCount - 1; i >= 0; i--) {
    if (identifiersEqual(name, &compiler->locals[i].name)) return i;
  }
  return -1;
#else
  return getLocalMap(&compiler->bindings, name);
#endif
}
static int resolveLocal(Compiler* compiler, Token* name) {
  int slot = findLocalSlot(compiler, name);
  if (slot != -1 && compiler->locals[slot].depth == -1) {
    error("Can't read local variable in its own initializer.");
  }
  return slot;
}
static void addLocal(Token name, bool isConst) {
  if (current->localCount > UINT16_MAX) {
    error("Too many local variables in function.");
    return;
  }

  if (current->localCapacity < current->localCount + 1) {
    int oldCapacity = current->localCapacity;
    current->localCapacity = GROW_CAPACITY(oldCapacity);
    current->locals = GROW_ARRAY(Local, current->locals,
                                 oldCapacity, current->localCapacity);
  }
  Local* local = &current->locals[current->localCount++];
  local->name = name;
  local->depth = -1;
  local->isConst = isConst;
#ifndef CLOX_LINEAR_LOCALS
  local->previous = getLocalMap(&current->bindings, &name);
  setLocalMap(&current->bindings, name, current->localCount - 1);
#endif
}
static void declareVariable(bool isConst) {
  if (current->scopeDepth == 0) return;

  Token* name = &parser.previous;
#ifdef CLOX_LINEAR_LOCALS
  for (int i = current->localCount - 1; i >= 0; i--) {
    Local* local = &current->locals[i];
    if (local->depth != -1 && local->depth < current->scopeDepth) {
      break; // [negative]
    }

    if (identifiersEqual(name, &local->name)) {
      error("Already a variable with this name in this scope.");
    }
  }

#else
  int slot = findLocalSlot(current, name);
  if (slot != -1 && (current->locals[slot].depth == -1 ||
                    current->locals[slot].depth == current->scopeDepth)) {
    error("Already a variable with this name in this scope.");
  }
#endif
  addLocal(*name, isConst);
}
static uint8_t parseVariable(const char* errorMessage, bool isConst) {
  consume(TOKEN_IDENTIFIER, errorMessage);

  declareVariable(isConst);
  if (current->scopeDepth > 0) return 0;

  return identifierConstant(&parser.previous);
}
static void markInitialized() {
  current->locals[current->localCount - 1].depth =
      current->scopeDepth;
}
static void defineVariable(uint8_t global, bool isConst) {
  if (current->scopeDepth > 0) {
    markInitialized();
    return;
  }

  emitBytes(isConst ? OP_DEFINE_CONST : OP_DEFINE_GLOBAL, global);
}
static void binary(bool canAssign) {
  TokenType operatorType = parser.previous.type;
  ParseRule* rule = getRule(operatorType);
  parsePrecedence((Precedence)(rule->precedence + 1));

  switch (operatorType) {
    case TOKEN_BANG_EQUAL:    emitBytes(OP_EQUAL, OP_NOT); break;
    case TOKEN_EQUAL_EQUAL:   emitByte(OP_EQUAL); break;
    case TOKEN_GREATER:       emitByte(OP_GREATER); break;
    case TOKEN_GREATER_EQUAL: emitBytes(OP_LESS, OP_NOT); break;
    case TOKEN_LESS:          emitByte(OP_LESS); break;
    case TOKEN_LESS_EQUAL:    emitBytes(OP_GREATER, OP_NOT); break;
    case TOKEN_PLUS:          emitByte(OP_ADD); break;
    case TOKEN_MINUS:         emitByte(OP_SUBTRACT); break;
    case TOKEN_STAR:          emitByte(OP_MULTIPLY); break;
    case TOKEN_SLASH:         emitByte(OP_DIVIDE); break;
    default: return; // Unreachable.
  }
}
static void literal(bool canAssign) {
  switch (parser.previous.type) {
    case TOKEN_FALSE: emitByte(OP_FALSE); break;
    case TOKEN_NIL: emitByte(OP_NIL); break;
    case TOKEN_TRUE: emitByte(OP_TRUE); break;
    default: return; // Unreachable.
  }
}
static void grouping(bool canAssign) {
  expression();
  consume(TOKEN_RIGHT_PAREN, "Expect ')' after expression.");
}
static void number(bool canAssign) {
  double value = strtod(parser.previous.start, NULL);
  emitConstant(NUMBER_VAL(value));
}
static void string(bool canAssign) {
  emitConstant(OBJ_VAL(copyString(parser.previous.start + 1,
                                  parser.previous.length - 2)));
}
static void namedVariable(Token name, bool canAssign) {
  int arg = resolveLocal(current, &name);
  bool isLocal = arg != -1;
  bool isConst;
  if (isLocal) {
    isConst = current->locals[arg].isConst;
  } else {
    ObjString* key = copyString(name.start, name.length);
    Value ignored;
    isConst = tableGet(&sourceConstants, key, &ignored) ||
              tableGet(&vm.globalConstants, key, &ignored);
    if (initializingGlobalConst && identifiersEqual(&name, &globalConstName)) {
      error("Can't read constant in its own initializer.");
    }
    arg = identifierConstant(&name);
  }

  bool assigning = canAssign && match(TOKEN_EQUAL);
  if (assigning) {
    if (isConst) errorAt(&name, "Can't assign to a constant.");
    expression();
  }
  if (isLocal) {
    emitLocal(assigning ? OP_SET_LOCAL : OP_GET_LOCAL, arg);
  } else {
    emitBytes(assigning ? OP_SET_GLOBAL : OP_GET_GLOBAL, (uint8_t)arg);
  }
}
static void variable(bool canAssign) {
  namedVariable(parser.previous, canAssign);
}
static void unary(bool canAssign) {
  TokenType operatorType = parser.previous.type;

  // Compile the operand.
  parsePrecedence(PREC_UNARY);

  // Emit the operator instruction.
  switch (operatorType) {
    case TOKEN_BANG: emitByte(OP_NOT); break;
    case TOKEN_MINUS: emitByte(OP_NEGATE); break;
    default: return; // Unreachable.
  }
}
ParseRule rules[] = {
  [TOKEN_LEFT_PAREN]    = {grouping, NULL,   PREC_NONE},
  [TOKEN_RIGHT_PAREN]   = {NULL,     NULL,   PREC_NONE},
  [TOKEN_LEFT_BRACE]    = {NULL,     NULL,   PREC_NONE}, // [big]
  [TOKEN_RIGHT_BRACE]   = {NULL,     NULL,   PREC_NONE},
  [TOKEN_COMMA]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_DOT]           = {NULL,     NULL,   PREC_NONE},
  [TOKEN_MINUS]         = {unary,    binary, PREC_TERM},
  [TOKEN_PLUS]          = {NULL,     binary, PREC_TERM},
  [TOKEN_SEMICOLON]     = {NULL,     NULL,   PREC_NONE},
  [TOKEN_SLASH]         = {NULL,     binary, PREC_FACTOR},
  [TOKEN_STAR]          = {NULL,     binary, PREC_FACTOR},
  [TOKEN_BANG]          = {unary,    NULL,   PREC_NONE},
  [TOKEN_BANG_EQUAL]    = {NULL,     binary, PREC_EQUALITY},
  [TOKEN_EQUAL]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_EQUAL_EQUAL]   = {NULL,     binary, PREC_EQUALITY},
  [TOKEN_GREATER]       = {NULL,     binary, PREC_COMPARISON},
  [TOKEN_GREATER_EQUAL] = {NULL,     binary, PREC_COMPARISON},
  [TOKEN_LESS]          = {NULL,     binary, PREC_COMPARISON},
  [TOKEN_LESS_EQUAL]    = {NULL,     binary, PREC_COMPARISON},
  [TOKEN_IDENTIFIER]    = {variable, NULL,   PREC_NONE},
  [TOKEN_STRING]        = {string,   NULL,   PREC_NONE},
  [TOKEN_NUMBER]        = {number,   NULL,   PREC_NONE},
  [TOKEN_AND]           = {NULL,     NULL,   PREC_NONE},
  [TOKEN_CLASS]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_CONST]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_ELSE]          = {NULL,     NULL,   PREC_NONE},
  [TOKEN_FALSE]         = {literal,  NULL,   PREC_NONE},
  [TOKEN_FOR]           = {NULL,     NULL,   PREC_NONE},
  [TOKEN_FUN]           = {NULL,     NULL,   PREC_NONE},
  [TOKEN_IF]            = {NULL,     NULL,   PREC_NONE},
  [TOKEN_NIL]           = {literal,  NULL,   PREC_NONE},
  [TOKEN_OR]            = {NULL,     NULL,   PREC_NONE},
  [TOKEN_PRINT]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_RETURN]        = {NULL,     NULL,   PREC_NONE},
  [TOKEN_SUPER]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_THIS]          = {NULL,     NULL,   PREC_NONE},
  [TOKEN_TRUE]          = {literal,  NULL,   PREC_NONE},
  [TOKEN_VAR]           = {NULL,     NULL,   PREC_NONE},
  [TOKEN_WHILE]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_ERROR]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_EOF]           = {NULL,     NULL,   PREC_NONE},
};
static void parsePrecedence(Precedence precedence) {
  advance();
  ParseFn prefixRule = getRule(parser.previous.type)->prefix;
  if (prefixRule == NULL) {
    error("Expect expression.");
    return;
  }

  bool canAssign = precedence <= PREC_ASSIGNMENT;
  prefixRule(canAssign);

  while (precedence <= getRule(parser.current.type)->precedence) {
    advance();
    ParseFn infixRule = getRule(parser.previous.type)->infix;
    infixRule(canAssign);
  }

  if (canAssign && match(TOKEN_EQUAL)) {
    error("Invalid assignment target.");
  }
}
static ParseRule* getRule(TokenType type) {
  return &rules[type];
}
static void expression() {
  parsePrecedence(PREC_ASSIGNMENT);
}
static void block() {
  while (!check(TOKEN_RIGHT_BRACE) && !check(TOKEN_EOF)) {
    declaration();
  }

  consume(TOKEN_RIGHT_BRACE, "Expect '}' after block.");
}
static void varDeclaration(bool isConst) {
  uint8_t global = parseVariable("Expect variable name.", isConst);
  initializingGlobalConst = isConst && current->scopeDepth == 0;
  globalConstName = parser.previous;

  if (match(TOKEN_EQUAL)) {
    expression();
  } else {
    if (isConst) error("Expect initializer for constant.");
    emitByte(OP_NIL);
  }
  initializingGlobalConst = false;
  consume(TOKEN_SEMICOLON,
          "Expect ';' after variable declaration.");

  defineVariable(global, isConst);
}
static void expressionStatement() {
  expression();
  consume(TOKEN_SEMICOLON, "Expect ';' after expression.");
  emitByte(OP_POP);
}
static void printStatement() {
  expression();
  consume(TOKEN_SEMICOLON, "Expect ';' after value.");
  emitByte(OP_PRINT);
}
static void synchronize() {
  parser.panicMode = false;

  while (parser.current.type != TOKEN_EOF) {
    if (parser.previous.type == TOKEN_SEMICOLON) return;
    switch (parser.current.type) {
      case TOKEN_CLASS:
      case TOKEN_FUN:
      case TOKEN_VAR:
      case TOKEN_CONST:
      case TOKEN_FOR:
      case TOKEN_IF:
      case TOKEN_WHILE:
      case TOKEN_PRINT:
      case TOKEN_RETURN:
        return;

      default:
        ; // Do nothing.
    }

    advance();
  }
}
static void declaration() {
  if (match(TOKEN_VAR)) {
    varDeclaration(false);
  } else if (match(TOKEN_CONST)) {
    varDeclaration(true);
  } else {
    statement();
  }

  if (parser.panicMode) synchronize();
}
static void statement() {
  if (match(TOKEN_PRINT)) {
    printStatement();
  } else if (match(TOKEN_LEFT_BRACE)) {
    beginScope();
    block();
    endScope();
  } else {
    expressionStatement();
  }
}

// A token pass catches writes before a later global const declaration.
// Only globals are collected; local names are still resolved by scope.
static void collectGlobalConstants(const char* source) {
  Table declarations;
  initTable(&declarations);
  initScanner(source);
  int depth = 0;
  for (;;) {
    Token token = scanToken();
    if (token.type == TOKEN_EOF) break;
    if (token.type == TOKEN_LEFT_BRACE) depth++;
    if (token.type == TOKEN_RIGHT_BRACE) depth--;
    if (depth != 0 ||
        (token.type != TOKEN_VAR && token.type != TOKEN_CONST)) continue;

    bool isConst = token.type == TOKEN_CONST;
    Token name = scanToken();
    if (name.type != TOKEN_IDENTIFIER) continue;
    ObjString* key = copyString(name.start, name.length);
    Value previous;
    if (tableGet(&vm.globalConstants, key, &previous) ||
        (isConst && tableGet(&vm.globals, key, &previous)) ||
        (tableGet(&declarations, key, &previous) &&
         (isConst || AS_BOOL(previous)))) {
      errorAt(&name, "Can't redeclare a global constant or make an existing global constant.");
    }
    tableSet(&declarations, key, BOOL_VAL(isConst));
    if (isConst) tableSet(&sourceConstants, key, BOOL_VAL(true));
  }
  freeTable(&declarations);
}

bool compile(const char* source, Chunk* chunk) {
  initScanner(source);
  Compiler compiler;
  initCompiler(&compiler);
  compilingChunk = chunk;

  parser.hadError = false;
  parser.panicMode = false;
  initializingGlobalConst = false;
  initTable(&sourceConstants);
  collectGlobalConstants(source);
  initScanner(source);
  parser.panicMode = false;

  advance();

  while (!match(TOKEN_EOF)) {
    declaration();
  }

  endCompiler();
#ifndef CLOX_LINEAR_LOCALS
  freeLocalMap(&compiler.bindings);
#endif
  FREE_ARRAY(Local, compiler.locals, compiler.localCapacity);
  freeTable(&sourceConstants);
  current = NULL;
  return !parser.hadError;
}
