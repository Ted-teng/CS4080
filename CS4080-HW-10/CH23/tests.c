//ch 23 challenge 1 (verify switch evaluation order, branches, and stack balance)
//ch 23 challenge 2 (verify continue targets, scope cleanup, and compile errors)
//ch 23 challenge 3 (verify guarded continuation on both paths)
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "compiler.h"
#include "object.h"
#include "table.h"
#include "vm.h"

static int checks = 0;
static void run(const char* source) {
  assert(interpret(source) == INTERPRET_OK);
  assert(vm.stackTop == vm.stack);
  checks++;
}
static void number(const char* name, double expected) {
  Value value;
  assert(tableGet(&vm.globals, copyString(name, (int)strlen(name)), &value));
  assert(IS_NUMBER(value) && AS_NUMBER(value) == expected);
  checks++;
}
static void reject(const char* source) {
  Chunk chunk;
  initChunk(&chunk);
  assert(!compile(source, &chunk));
  freeChunk(&chunk);
  checks++;
}

int main(void) {
  initVM();
  run("var selected=0; var probes=0; var result=0;"
      "switch (selected=selected+1) {"
      "case (probes=probes+1): result=10;"
      "case unknown: result=20; default: result=30; }");
  number("selected", 1); number("probes", 1); number("result", 10);
  run("probes=0; switch (2) {"
      "case (probes=probes+1): result=10;"
      "case (probes=probes+1): result=20;"
      "case unknown: result=30; default: result=40; }");
  number("probes", 2); number("result", 20);
  run("var changed=1; switch(changed) {"
      "case (changed=2): result=0; case 1: result=99; }");
  number("changed", 2); number("result", 99);
  run("switch (0) {case 1: result=0; default: result=7;}");
  number("result", 7);
  run("switch (0) {case 1: result=0;} switch (0) {} ");
  number("result", 7);
  run("switch (selected=selected+1) {} switch(0) {default: result=8;}");
  number("selected", 2); number("result", 8);
  run("switch (1) {case 1: case 2: result=0; default: result=0;}");
  number("result", 8);
  run("switch (nil) {case false: result=0; case nil: result=1;}"
      "switch (true) {case 1: result=0; case true: result=result+1;}"
      "switch (\"a\"+\"b\") {case \"ab\": result=result+1;}");
  number("result", 3);
  run("{var outer=5; switch(2) {case 2: {var inner=outer+1; result=inner;} }"
      "result=result+outer; }");
  number("result", 11);
  run("switch(1) {case 1: switch(2) {case 2: result=12; default: result=0;}"
      "default: result=0;}");
  number("result", 12);

  run("var i=0; var sum=0; while(i<5) {i=i+1; if(i<3) continue; sum=sum+i;}");
  number("i", 5); number("sum", 12);
  run("sum=0; for(var j=0;j<5;j=j+1) {if(j<2) continue; sum=sum+j;}");
  number("sum", 9);
  run("sum=0; for(var j=0;j<3;) {j=j+1; sum=sum+1; continue;} ");
  number("sum", 3);
  run("for (;false;) continue;");
  run("sum=0; for(var a=0;a<3;a=a+1) {"
      "for(var b=0;b<3;b=b+1) {continue;} sum=sum+1;}");
  number("sum", 3);
  run("sum=0; for(var j=0;j<1000;j=j+1) {var first=j; {var second=first;"
      "switch(second) {case j: {var third=second;"
      "switch(third) {default: continue;} } default: sum=9999;} } sum=9999;}");
  number("sum", 0);
  run("sum=0; switch(1) {case 1: {var keep=7;"
      "for(var j=0;j<3;j=j+1) {continue;} sum=keep;} }");
  number("sum", 7);
  run("sum=0; for(var j=0;j<4;j=j+1) {var keep=j;"
      "if(j<2) continue; sum=sum+keep;}");
  number("sum", 5);
  run("sum=0; i=0; while(i<1000) {var first=i; i=i+1; {var second=first;"
      "continue;} sum=9999;}");
  number("i", 1000); number("sum", 0);

  run("sum=0; for(var j=0;j<5;j=j+1) {continue if (j<2); sum=sum+j;}");
  number("sum", 9);
  run("sum=0; i=0; while(i<5) {i=i+1; continue if (i<3); sum=sum+i;}");
  number("sum", 12);
  run("sum=0; probes=0; for(var j=0;j<4;j=j+1) {var keep=j;"
      "continue if ((probes=probes+1)<3); sum=sum+keep;}");
  number("probes", 4); number("sum", 5);
  run("sum=0; for(var j=0;j<3;j=j+1) {continue if (nil); continue if (false);"
      "sum=sum+1; continue if (0); sum=9999;}");
  number("sum", 3);
  run("sum=0; for(var j=0;j<1000;j=j+1) {var keep=j; switch(j) {"
      "case j: {var inner=keep; continue if (j<500); sum=sum+1;} } }");
  number("sum", 500);
  run("sum=0; for(var a=0;a<3;a=a+1) {for(var b=0;b<3;b=b+1) {"
      "continue if (b<2); sum=sum+1;} }");
  number("sum", 3);
  run("var switcheroo=1; var cases=2; var defaults=3; var continued=4;"
      "result=switcheroo+cases+defaults+continued;");
  number("result", 10);

  reject("continue;");
  reject("if(false) continue;");
  reject("switch(1) {default: continue;}");
  reject("continue if (false);");
  reject("while(false) {} continue;");
  reject("for(;false;) {} continue;");
  reject("while(false) {continue}");
  reject("while(false) {continue if true;}");
  reject("while(false) {continue if ();}");
  reject("while(false) {continue if (true)}");
  reject("switch(1) {case 1 print 1;}");
  reject("switch(1) {default: default:}");
  reject("switch(1) {default: case 1:}");
  reject("switch(1) {print 1;}");
  reject("switch(1) {case 1: var x=2;}");
  reject("switch(1) {case 1: print 1;");
  reject("case 1: print 1;");
  reject("default: print 1;");

  //ch 23 challenge 1 (grow the case-end list beyond the reference's 256 entries)
  char manyCases[12000]="switch(false) {";
  for(int n=0;n<300;n++) strcat(manyCases,"case true:");
  strcat(manyCases,"default: result=300;}");
  run(manyCases); number("result",300);
  //ch 23 challenge 1 (report a full local table before adding a hidden selector)
  char tooManyLocals[14000]="{";
  for(int n=0;n<256;n++) {
    char declaration[40]; sprintf(declaration,"var local%d=nil;",n);
    strcat(tooManyLocals,declaration);
  }
  strcat(tooManyLocals,"switch(nil) {} }");
  reject(tooManyLocals);
  //ch 23 challenge 2 (a rejected compilation must not leave a stale loop context)
  run("result=42;"); number("result",42);
  freeVM();
  printf("All %d checks passed.\n", checks);
  return 0;
}
