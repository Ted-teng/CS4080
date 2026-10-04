//> Functions lox-function
package com.craftinginterpreters.lox;

import java.util.List;

class LoxFunction implements LoxCallable {
  // Chapter 10, Challenge 2: Store parameters, body, and optional name explicitly for lambdas
  private final String name;
  private final List<Token> params;
  private final List<Stmt> body;
//> closure-field
  private final Environment closure;
  private final LoxClass ownerClass;
  private final LoxInstance boundInstance;
  private final boolean isGetter;
//< closure-field
/* Functions lox-function < Functions closure-constructor
  LoxFunction(Stmt.Function declaration) {
*/
/* Functions closure-constructor < Classes is-initializer-field
  LoxFunction(Stmt.Function declaration, Environment closure) {
*/
//> Classes is-initializer-field
  private final boolean isInitializer;

  LoxFunction(Stmt.Function declaration, Environment closure,
              boolean isInitializer) {
    this(declaration, closure, isInitializer, null);
  }

  LoxFunction(Stmt.Function declaration, Environment closure,
              boolean isInitializer, LoxClass ownerClass) {
    this(declaration.name.lexeme, declaration.params, declaration.body,
        closure, isInitializer, ownerClass, null, declaration.isGetter);
  }

  // Chapter 10, Challenge 2: Constructor for anonymous lambda functions
  LoxFunction(Expr.Function declaration, Environment closure) {
    this(null, declaration.params, declaration.body, closure, false,
        null, null, false);
  }

  LoxFunction(String name, List<Token> params, List<Stmt> body,
              Environment closure, boolean isInitializer) {
    this(name, params, body, closure, isInitializer, null, null, false);
  }

  LoxFunction(String name, List<Token> params, List<Stmt> body,
              Environment closure, boolean isInitializer,
              LoxClass ownerClass, LoxInstance boundInstance,
              boolean isGetter) {
    this.name = name;
    this.params = params;
    this.body = body;
    this.isInitializer = isInitializer;
    this.ownerClass = ownerClass;
    this.boundInstance = boundInstance;
    this.isGetter = isGetter;
//< Classes is-initializer-field
//> closure-constructor
    this.closure = closure;
//< closure-constructor
  }
//> Classes bind-instance
  LoxFunction bind(LoxInstance instance) {
    Environment environment = new Environment(closure);
    environment.define("this", instance);
/* Classes bind-instance < Classes lox-function-bind-with-initializer
    return new LoxFunction(declaration, environment);
*/
//> lox-function-bind-with-initializer
    return new LoxFunction(name, params, body, environment, isInitializer,
        ownerClass, instance, isGetter);
//< lox-function-bind-with-initializer
  }
//< Classes bind-instance
//> function-to-string
  @Override
  public String toString() {
    // Chapter 10, Challenge 2: Print <fn> if anonymous
    if (name == null) return "<fn>";
    return "<fn " + name + ">";
  }
//< function-to-string
//> function-arity
  @Override
  public int arity() {
    return params.size();
  }

  boolean isGetter() {
    return isGetter;
  }

  // Chapter 13, Challenge 1: Re-own a mixin method when it is included.
  LoxFunction withOwnerClass(LoxClass owner) {
    return new LoxFunction(name, params, body, closure, isInitializer,
        owner, null, isGetter);
  }
//< function-arity
//> function-call
  @Override
  public Object call(Interpreter interpreter,
                     List<Object> arguments) {
/* Functions function-call < Functions call-closure
    Environment environment = new Environment(interpreter.globals);
*/
//> call-closure
    Environment environment = new Environment(closure);
//< call-closure
    for (int i = 0; i < params.size(); i++) {
      environment.define(params.get(i).lexeme,
          arguments.get(i));
    }

    if (ownerClass != null && boundInstance != null) {
      environment.defineInner(
          new InnerCallable(ownerClass, boundInstance, name));
    }

/* Functions function-call < Functions catch-return
    interpreter.executeBlock(declaration.body, environment);
*/
//> catch-return
    try {
      interpreter.executeBlock(body, environment);
    } catch (Return returnValue) {
//> Classes early-return-this
      if (isInitializer) return closure.getAt(0, 0);

//< Classes early-return-this
      return returnValue.value;
    }
//< catch-return
//> Classes return-this

    if (isInitializer) return closure.getAt(0, 0);
//< Classes return-this
    return null;
  }

  // Chapter 13, Challenge 2: Callable used by the BETA-style inner keyword.
  private static class InnerCallable implements LoxCallable {
    private final LoxClass ownerClass;
    private final LoxInstance receiver;
    private final String methodName;

    InnerCallable(LoxClass ownerClass, LoxInstance receiver,
                  String methodName) {
      this.ownerClass = ownerClass;
      this.receiver = receiver;
      this.methodName = methodName;
    }

    @Override
    public int arity() {
      // Chapter 13, Challenge 2: Validate arguments against the next method.
      LoxFunction method = receiver.klass().findInnerMethod(
          ownerClass, methodName);
      return method == null ? -1 : method.arity();
    }

    @Override
    public Object call(Interpreter interpreter, List<Object> arguments) {
      LoxFunction method = receiver.klass().findInnerMethod(
          ownerClass, methodName);
      if (method == null) return null;
      return method.bind(receiver).call(interpreter, arguments);
    }

    @Override
    public String toString() {
      return "<inner>";
    }
  }
//< function-call
}
