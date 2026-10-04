//> Classes lox-class
package com.craftinginterpreters.lox;

import java.util.ArrayList;
import java.util.List;
import java.util.Map;

// Chapter 12, Challenge 1: A class object is also an instance that owns static methods.
class LoxClass extends LoxInstance implements LoxCallable {
  final String name;
  final LoxClass superclass;
  private final Map<String, LoxFunction> methods;
  private final Map<String, LoxFunction> staticMethods;

  LoxClass(String name, LoxClass superclass,
           Map<String, LoxFunction> methods,
           Map<String, LoxFunction> staticMethods) {
    super(null);
    this.name = name;
    this.superclass = superclass;
    this.methods = methods;
    this.staticMethods = staticMethods;
  }

  void addMethod(String methodName, LoxFunction function, boolean isStatic) {
    if (isStatic) {
      staticMethods.put(methodName, function);
    } else {
      methods.put(methodName, function);
    }
  }

  // Chapter 13, Challenge 1: Copy methods from a reusable mixin class.
  void includeMixin(LoxClass mixin) {
    for (Map.Entry<String, LoxFunction> entry : mixin.methods.entrySet()) {
      methods.putIfAbsent(entry.getKey(),
          entry.getValue().withOwnerClass(this));
    }
  }

  // Chapter 13, Challenge 2: Search from the root of the hierarchy downward.
  LoxFunction findMethod(String methodName) {
    if (superclass != null) {
      LoxFunction inherited = superclass.findMethod(methodName);
      if (inherited != null) return inherited;
    }

    return methods.get(methodName);
  }

  LoxFunction findStaticMethod(String methodName) {
    if (superclass != null) {
      LoxFunction inherited = superclass.findStaticMethod(methodName);
      if (inherited != null) return inherited;
    }

    return staticMethods.get(methodName);
  }

  // Chapter 13, Challenge 2: Find the next implementation below the current owner.
  LoxFunction findInnerMethod(LoxClass owner, String methodName) {
    List<LoxClass> chain = new ArrayList<>();
    for (LoxClass current = this; current != null;
         current = current.superclass) {
      chain.add(0, current);
    }

    int ownerIndex = chain.indexOf(owner);
    if (ownerIndex < 0) return null;

    for (int i = ownerIndex + 1; i < chain.size(); i++) {
      LoxFunction method = chain.get(i).methods.get(methodName);
      if (method != null) return method;
    }

    return null;
  }

  // Chapter 12, Challenge 1: Static methods are properties of the class object.
  @Override
  Object get(Token name, Interpreter interpreter) {
    // Chapter 12, Challenge 1: Class fields take precedence over static methods.
    if (fields.containsKey(name.lexeme)) {
      return fields.get(name.lexeme);
    }

    LoxFunction method = findStaticMethod(name.lexeme);
    if (method != null) {
      if (method.isGetter()) {
        return method.call(interpreter, java.util.Collections.emptyList());
      }
      return method;
    }

    throw new RuntimeError(name,
        "Undefined class property '" + name.lexeme + "'.");
  }

  @Override
  public String toString() {
    return name;
  }

  @Override
  public Object call(Interpreter interpreter, List<Object> arguments) {
    LoxInstance instance = new LoxInstance(this);
    LoxFunction initializer = findMethod("init");
    if (initializer != null) {
      initializer.bind(instance).call(interpreter, arguments);
    }
    return instance;
  }

  @Override
  public int arity() {
    LoxFunction initializer = findMethod("init");
    if (initializer == null) return 0;
    return initializer.arity();
  }
}
