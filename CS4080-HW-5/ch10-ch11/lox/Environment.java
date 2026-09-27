//> Statements and State environment-class
package com.craftinginterpreters.lox;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

class Environment {
//> enclosing-field
  final Environment enclosing;
//< enclosing-field
  // Chapter 11, Challenge 4: Store values in indexed slots.
  private final List<Object> values = new ArrayList<>();
  private final Map<String, Integer> slots = new HashMap<>();
//> environment-constructors
  Environment() {
    enclosing = null;
  }

  Environment(Environment enclosing) {
    this.enclosing = enclosing;
  }
//< environment-constructors
//> environment-get

  Object get(Token name) {
    Integer slot = slots.get(name.lexeme);
    if (slot != null) {
      return values.get(slot);
    }
//> environment-get-enclosing

    if (enclosing != null) return enclosing.get(name);
//< environment-get-enclosing

    throw new RuntimeError(name,
        "Undefined variable '" + name.lexeme + "'.");
  }

//< environment-get
//> environment-assign
  void assign(Token name, Object value) {
    Integer slot = slots.get(name.lexeme);
    if (slot != null) {
      values.set(slot, value);
      return;
    }

//> environment-assign-enclosing
    if (enclosing != null) {
      enclosing.assign(name, value);
      return;
    }

//< environment-assign-enclosing
    throw new RuntimeError(name,
        "Undefined variable '" + name.lexeme + "'.");
  }
//< environment-assign
//> environment-define
  void define(String name, Object value) {
    slots.put(name, values.size());
    values.add(value);
  }
//< environment-define
//> Resolving and Binding ancestor
  Environment ancestor(int distance) {
    Environment environment = this;
    for (int i = 0; i < distance; i++) {
      environment = environment.enclosing; // [coupled]
    }

    return environment;
  }
//< Resolving and Binding ancestor
//> Resolving and Binding get-at
  Object getAt(int distance, int slot) {
    return ancestor(distance).values.get(slot);
  }
//< Resolving and Binding get-at
//> Resolving and Binding assign-at
  void assignAt(int distance, int slot, Object value) {
    ancestor(distance).values.set(slot, value);
  }
//< Resolving and Binding assign-at
//> omit
  @Override
  public String toString() {
    String result = values.toString();
    if (enclosing != null) {
      result += " -> " + enclosing.toString();
    }

    return result;
  }
//< omit
}
