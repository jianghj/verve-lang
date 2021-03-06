#pragma once

#define VERVE_FUNCTION(FN_NAME) \
  Value FN_NAME( \
      __attribute__((unused)) unsigned argc, \
      __attribute__((unused)) Value *argv, \
      __attribute__((unused)) VM *vm)

namespace Verve {
  class VM;
  struct Value;

  VERVE_FUNCTION(print_string);
  VERVE_FUNCTION(concat_string);

  VERVE_FUNCTION(head);
  VERVE_FUNCTION(tail);
  VERVE_FUNCTION(length);

  // type conversion
  VERVE_FUNCTION(int_to_string);
  VERVE_FUNCTION(float_to_string);

  VERVE_FUNCTION(add);
  VERVE_FUNCTION(sub);
  VERVE_FUNCTION(mul);
  VERVE_FUNCTION(div);
  VERVE_FUNCTION(mod);
  VERVE_FUNCTION(lt);
  VERVE_FUNCTION(gt);
  VERVE_FUNCTION(lte);
  VERVE_FUNCTION(gte);
  VERVE_FUNCTION(equals);
  VERVE_FUNCTION(not_equal);
  VERVE_FUNCTION(_and);
  VERVE_FUNCTION(_or);
  VERVE_FUNCTION(_not);
  VERVE_FUNCTION(minus);
  VERVE_FUNCTION(at);
  VERVE_FUNCTION(substr);
  VERVE_FUNCTION(count);
  VERVE_FUNCTION(heapSize);

  void registerBuiltins(VM &);

}
