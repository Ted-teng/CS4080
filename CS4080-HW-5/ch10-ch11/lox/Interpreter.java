package com.craftinginterpreters.lox;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

class Interpreter implements Expr.Visitor<Object>, Stmt.Visitor<Void> {
  final Environment globals = new Environment();
  private Environment environment = globals;
  // Chapter 11, Challenge 4: Store lexical depth and slot index.
  private static class Local {
    final int depth;
    final int slot;

    Local(int depth, int slot) {
      this.depth = depth;
      this.slot = slot;
    }
  }

  private final Map<Expr, Local> locals = new HashMap<>();

  // Chapter 8, Challenge 2: Sentinel for variables declared without an initializer.
  private static final Object UNINITIALIZED = new Object();

  // Chapter 9, Challenge 3: Control-flow exception to unwind stack on break
  private static class BreakException extends RuntimeException {}

  Interpreter() {
    globals.define("clock", new LoxCallable() {
      @Override
      public int arity() { return 0; }

      @Override
      public Object call(Interpreter interpreter, List<Object> arguments) {
        return (double)System.currentTimeMillis() / 1000.0;
      }

      @Override
      public String toString() { return "<native fn>"; }
    });
  }

  void interpret(List<Stmt> statements) {
    try {
      for (Stmt statement : statements) {
        execute(statement);
      }
    } catch (RuntimeError error) {
      Lox.runtimeError(error);
    }
  }

  // Chapter 8, Challenge 1: Evaluate a standalone REPL expression and return its value.
  String interpret(Expr expression) {
    try {
      Object value = evaluate(expression);
      return stringify(value);
    } catch (RuntimeError error) {
      Lox.runtimeError(error);
      return null;
    }
  }

  private Object evaluate(Expr expr) {
    return expr.accept(this);
  }

  private void execute(Stmt stmt) {
    stmt.accept(this);
  }

  void resolve(Expr expr, int depth, int slot) {
    locals.put(expr, new Local(depth, slot));
  }

  void executeBlock(List<Stmt> statements, Environment environment) {
    Environment previous = this.environment;
    try {
      this.environment = environment;
      for (Stmt statement : statements) {
        execute(statement);
      }
    } finally {
      this.environment = previous;
    }
  }

  @Override
  public Void visitBlockStmt(Stmt.Block stmt) {
    executeBlock(stmt.statements, new Environment(environment));
    return null;
  }

  @Override
  public Void visitClassStmt(Stmt.Class stmt) {
    Object superclass = null;
    if (stmt.superclass != null) {
      superclass = evaluate(stmt.superclass);
      if (!(superclass instanceof LoxClass)) {
        throw new RuntimeError(stmt.superclass.name, "Superclass must be a class.");
      }
    }

    environment.define(stmt.name.lexeme, null);

    if (stmt.superclass != null) {
      environment = new Environment(environment);
      environment.define("super", superclass);
    }

    Map<String, LoxFunction> methods = new HashMap<>();
    for (Stmt.Function method : stmt.methods) {
      LoxFunction function = new LoxFunction(method, environment,
          method.name.lexeme.equals("init"));
      methods.put(method.name.lexeme, function);
    }

    LoxClass klass = new LoxClass(
        stmt.name.lexeme,
        (LoxClass)superclass,
        methods);

    if (superclass != null) {
      environment = environment.enclosing;
    }

    environment.assign(stmt.name, klass);
    return null;
  }

  @Override
  public Void visitExpressionStmt(Stmt.Expression stmt) {
    evaluate(stmt.expression);
    return null;
  }

  @Override
  public Void visitFunctionStmt(Stmt.Function stmt) {
    LoxFunction function = new LoxFunction(stmt, environment, false);
    environment.define(stmt.name.lexeme, function);
    return null;
  }

  // Chapter 10, Challenge 2: Evaluate anonymous function expression
  @Override
  public Object visitFunctionExpr(Expr.Function expr) {
    return new LoxFunction(expr, environment);
  }

  @Override
  public Void visitIfStmt(Stmt.If stmt) {
    if (isTruthy(evaluate(stmt.condition))) {
      execute(thenBranch(stmt));
    } else if (stmt.elseBranch != null) {
      execute(stmt.elseBranch);
    }
    return null;
  }

  private Stmt thenBranch(Stmt.If stmt) {
    return stmt.thenBranch;
  }

  @Override
  public Void visitPrintStmt(Stmt.Print stmt) {
    Object value = evaluate(stmt.expression);
    System.out.println(stringify(value));
    return null;
  }

  @Override
  public Void visitReturnStmt(Stmt.Return stmt) {
    Object value = null;
    if (stmt.value != null) value = evaluate(stmt.value);
    throw new Return(value);
  }

  // Chapter 8, Challenge 2: Keep an uninitialized variable distinct from Lox nil.
  @Override
  public Void visitVarStmt(Stmt.Var stmt) {
    Object value = UNINITIALIZED;

    if (stmt.initializer != null) {
      value = evaluate(stmt.initializer);
    }

    environment.define(stmt.name.lexeme, value);
    return null;
  }

  // Chapter 9, Challenge 3: Catch BreakException to terminate nearest loop
  @Override
  public Void visitWhileStmt(Stmt.While stmt) {
    try {
      while (isTruthy(evaluate(stmt.condition))) {
        execute(stmt.body);
      }
    } catch (BreakException ex) {
      // Exit the current loop
    }
    return null;
  }

  // Chapter 9, Challenge 3: Throw control-flow exception when break is executed
  @Override
  public Void visitBreakStmt(Stmt.Break stmt) {
    throw new BreakException();
  }

  @Override
  public Object visitAssignExpr(Expr.Assign expr) {
    Object value = evaluate(expr.value);
    Local local = locals.get(expr);

    if (local != null) {
      environment.assignAt(local.depth, local.slot, value);
    } else {
      globals.assign(expr.name, value);
    }

    return value;
  }

  // Chapter 6, Challenge 2: Evaluate conditional (ternary) expressions
  @Override
  public Object visitConditionalExpr(Expr.Conditional expr) {
    Object condition = evaluate(expr.condition);

    if (isTruthy(condition)) {
      return evaluate(expr.thenBranch);
    } else {
      return evaluate(expr.elseBranch);
    }
  }

  @Override
  public Object visitBinaryExpr(Expr.Binary expr) {
    Object left = evaluate(expr.left);
    Object right = evaluate(expr.right);

    switch (expr.operator.type) {
      // Chapter 6, Challenge 1: Evaluate comma expression
      case COMMA:
        return right;

      case BANG_EQUAL:
        return !isEqual(left, right);

      case EQUAL_EQUAL:
        return isEqual(left, right);

      case GREATER:
        checkNumberOperands(expr.operator, left, right);
        return (double)left > (double)right;

      case GREATER_EQUAL:
        checkNumberOperands(expr.operator, left, right);
        return (double)left >= (double)right;

      case LESS:
        checkNumberOperands(expr.operator, left, right);
        return (double)left < (double)right;

      case LESS_EQUAL:
        checkNumberOperands(expr.operator, left, right);
        return (double)left <= (double)right;

      case MINUS:
        checkNumberOperands(expr.operator, left, right);
        return (double)left - (double)right;

      // Chapter 7, Challenge 2: Auto-coercion concatenation
      case PLUS:
        if (left instanceof String || right instanceof String) {
          return stringify(left) + stringify(right);
        }

        if (left instanceof Double && right instanceof Double) {
          return (double)left + (double)right;
        }

        throw new RuntimeError(expr.operator,
            "Operands must be two numbers or at least one string.");

      // Chapter 7, Challenge 3: Division by zero runtime error
      case SLASH:
        checkNumberOperands(expr.operator, left, right);

        if ((double)right == 0.0) {
          throw new RuntimeError(expr.operator, "Division by zero.");
        }

        return (double)left / (double)right;

      case STAR:
        checkNumberOperands(expr.operator, left, right);
        return (double)left * (double)right;
    }

    return null;
  }

  @Override
  public Object visitCallExpr(Expr.Call expr) {
    Object callee = evaluate(expr.callee);

    List<Object> arguments = new ArrayList<>();
    for (Expr argument : expr.arguments) {
      arguments.add(evaluate(argument));
    }

    if (!(callee instanceof LoxCallable)) {
      throw new RuntimeError(expr.paren,
          "Can only call functions and classes.");
    }

    LoxCallable function = (LoxCallable)callee;

    if (arguments.size() != function.arity()) {
      throw new RuntimeError(expr.paren,
          "Expected " + function.arity() +
          " arguments but got " + arguments.size() + ".");
    }

    return function.call(this, arguments);
  }

  @Override
  public Object visitGetExpr(Expr.Get expr) {
    Object object = evaluate(expr.object);

    if (object instanceof LoxInstance) {
      return ((LoxInstance) object).get(expr.name);
    }

    throw new RuntimeError(
        expr.name,
        "Only instances have properties.");
  }

  @Override
  public Object visitGroupingExpr(Expr.Grouping expr) {
    return evaluate(expr.expression);
  }

  @Override
  public Object visitLiteralExpr(Expr.Literal expr) {
    return expr.value;
  }

  @Override
  public Object visitLogicalExpr(Expr.Logical expr) {
    Object left = evaluate(expr.left);

    if (expr.operator.type == TokenType.OR) {
      if (isTruthy(left)) return left;
    } else {
      if (!isTruthy(left)) return left;
    }

    return evaluate(expr.right);
  }

  @Override
  public Object visitSetExpr(Expr.Set expr) {
    Object object = evaluate(expr.object);

    if (!(object instanceof LoxInstance)) {
      throw new RuntimeError(
          expr.name,
          "Only instances have fields.");
    }

    Object value = evaluate(expr.value);
    ((LoxInstance)object).set(expr.name, value);
    return value;
  }

  @Override
  public Object visitSuperExpr(Expr.Super expr) {
    Local local = locals.get(expr);
    LoxClass superclass =
        (LoxClass)environment.getAt(local.depth, local.slot);

    LoxInstance object =
        (LoxInstance)environment.getAt(local.depth - 1, 0);

    LoxFunction method =
        superclass.findMethod(expr.method.lexeme);

    if (method == null) {
      throw new RuntimeError(
          expr.method,
          "Undefined property '" + expr.method.lexeme + "'.");
    }

    return method.bind(object);
  }

  @Override
  public Object visitThisExpr(Expr.This expr) {
    return lookUpVariable(expr.keyword, expr);
  }

  @Override
  public Object visitUnaryExpr(Expr.Unary expr) {
    Object right = evaluate(expr.right);

    switch (expr.operator.type) {
      case BANG:
        return !isTruthy(right);

      case MINUS:
        checkNumberOperand(expr.operator, right);
        return -(double)right;
    }

    return null;
  }

  @Override
  public Object visitVariableExpr(Expr.Variable expr) {
    // Chapter 8, Challenge 2: Reject use of a variable before it is initialized.
    Object value = lookUpVariable(expr.name, expr);

    if (value == UNINITIALIZED) {
      throw new RuntimeError(
          expr.name,
          "Variable must be initialized before use.");
    }

    return value;
  }

  private Object lookUpVariable(Token name, Expr expr) {
    Local local = locals.get(expr);

    if (local != null) {
      return environment.getAt(local.depth, local.slot);
    } else {
      return globals.get(name);
    }
  }

  private void checkNumberOperand(Token operator, Object operand) {
    if (operand instanceof Double) return;

    throw new RuntimeError(
        operator,
        "Operand must be a number.");
  }

  private void checkNumberOperands(
      Token operator,
      Object left,
      Object right) {

    if (left instanceof Double && right instanceof Double) return;

    throw new RuntimeError(
        operator,
        "Operands must be numbers.");
  }

  private boolean isTruthy(Object object) {
    if (object == null) return false;
    if (object instanceof Boolean) return (boolean)object;
    return true;
  }

  private boolean isEqual(Object a, Object b) {
    if (a == null && b == null) return true;
    if (a == null) return false;

    return a.equals(b);
  }

  private String stringify(Object object) {
    if (object == null) return "nil";

    if (object instanceof Double) {
      String text = object.toString();

      if (text.endsWith(".0")) {
        text = text.substring(0, text.length() - 2);
      }

      return text;
    }

    return object.toString();
  }
}
