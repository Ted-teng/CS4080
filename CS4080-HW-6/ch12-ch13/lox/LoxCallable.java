//> Functions callable
package com.craftinginterpreters.lox;

import java.util.List;

interface LoxCallable {
//> callable-arity
  // Chapter 13, Challenge 2: A negative arity accepts any argument count for a missing inner method.
  int arity();
//< callable-arity
  Object call(Interpreter interpreter, List<Object> arguments);
}
