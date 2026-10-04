//> Appendix II stmt
package com.craftinginterpreters.lox;

import java.util.List;

abstract class Stmt {
  interface Visitor<R> {
    R visitBlockStmt(Block stmt);
    // Chapter 13, Challenge 3: Visitor for assert statements.
    R visitAssertStmt(Assert stmt);
    R visitClassStmt(Class stmt);
    R visitExpressionStmt(Expression stmt);
    R visitFunctionStmt(Function stmt);
    R visitIfStmt(If stmt);
    R visitPrintStmt(Print stmt);
    R visitReturnStmt(Return stmt);
    R visitVarStmt(Var stmt);
    R visitWhileStmt(While stmt);
    // Chapter 9, Challenge 3: Visitor interface for break statement
    R visitBreakStmt(Break stmt);
  }

  // Nested Stmt classes here...
//> stmt-block
  static class Block extends Stmt {
    Block(List<Stmt> statements) {
      this.statements = statements;
    }

    @Override
    <R> R accept(Visitor<R> visitor) {
      return visitor.visitBlockStmt(this);
    }

    final List<Stmt> statements;
  }
//< stmt-block
//> stmt-class
  static class Class extends Stmt {
    Class(Token name,
          Expr.Variable superclass,
          List<Expr.Variable> mixins,
          List<Stmt.Function> methods) {
      this.name = name;
      this.superclass = superclass;
      this.mixins = mixins;
      this.methods = methods;
    }

    @Override
    <R> R accept(Visitor<R> visitor) {
      return visitor.visitClassStmt(this);
    }

    final Token name;
    final Expr.Variable superclass;
    // Chapter 13, Challenge 1: Classes may include reusable mixin classes.
    final List<Expr.Variable> mixins;
    final List<Stmt.Function> methods;
  }
//< stmt-class
//> stmt-expression
  static class Expression extends Stmt {
    Expression(Expr expression) {
      this.expression = expression;
    }

    @Override
    <R> R accept(Visitor<R> visitor) {
      return visitor.visitExpressionStmt(this);
    }

    final Expr expression;
  }

  // Chapter 13, Challenge 3: AST node for assert statements.
  static class Assert extends Stmt {
    Assert(Token keyword, Expr condition) {
      this.keyword = keyword;
      this.condition = condition;
    }

    @Override
    <R> R accept(Visitor<R> visitor) {
      return visitor.visitAssertStmt(this);
    }

    final Token keyword;
    final Expr condition;
  }
//< stmt-expression
//> stmt-function
  static class Function extends Stmt {
    Function(Token name, List<Token> params, List<Stmt> body,
             boolean isStatic, boolean isGetter) {
      this.name = name;
      this.params = params;
      this.body = body;
      this.isStatic = isStatic;
      this.isGetter = isGetter;
    }

    @Override
    <R> R accept(Visitor<R> visitor) {
      return visitor.visitFunctionStmt(this);
    }

    final Token name;
    final List<Token> params;
    final List<Stmt> body;
    // Chapter 12, Challenge 1: Mark class methods that belong to the class object.
    final boolean isStatic;
    // Chapter 12, Challenge 2: Mark methods whose body is run on property access.
    final boolean isGetter;
  }
//< stmt-function
//> stmt-if
  static class If extends Stmt {
    If(Expr condition, Stmt thenBranch, Stmt elseBranch) {
      this.condition = condition;
      this.thenBranch = thenBranch;
      this.elseBranch = elseBranch;
    }

    @Override
    <R> R accept(Visitor<R> visitor) {
      return visitor.visitIfStmt(this);
    }

    final Expr condition;
    final Stmt thenBranch;
    final Stmt elseBranch;
  }
//< stmt-if
//> stmt-print
  static class Print extends Stmt {
    Print(Expr expression) {
      this.expression = expression;
    }

    @Override
    <R> R accept(Visitor<R> visitor) {
      return visitor.visitPrintStmt(this);
    }

    final Expr expression;
  }
//< stmt-print
//> stmt-return
  static class Return extends Stmt {
    Return(Token keyword, Expr value) {
      this.keyword = keyword;
      this.value = value;
    }

    @Override
    <R> R accept(Visitor<R> visitor) {
      return visitor.visitReturnStmt(this);
    }

    final Token keyword;
    final Expr value;
  }
//< stmt-return
//> stmt-var
  static class Var extends Stmt {
    Var(Token name, Expr initializer) {
      this.name = name;
      this.initializer = initializer;
    }

    @Override
    <R> R accept(Visitor<R> visitor) {
      return visitor.visitVarStmt(this);
    }

    final Token name;
    final Expr initializer;
  }
//< stmt-var
//> stmt-while
  static class While extends Stmt {
    While(Expr condition, Stmt body) {
      this.condition = condition;
      this.body = body;
    }

    @Override
    <R> R accept(Visitor<R> visitor) {
      return visitor.visitWhileStmt(this);
    }

    final Expr condition;
    final Stmt body;
  }
//< stmt-while

  abstract <R> R accept(Visitor<R> visitor);

// Chapter 9, Challenge 3: AST node for break statement
static class Break extends Stmt {
  Break() {}

  @Override
  <R> R accept(Visitor<R> visitor) {
    return visitor.visitBreakStmt(this);
  }
}



}
//< Appendix II stmt
