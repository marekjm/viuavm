;
; modulo function
;
; Author: vktgz <vktgz@rocday.pl>
;
; License: GPLv3
;

.function: main/0
  ;%2 = %3 mod %2
  float %2 local 4
  float %3 local 7
  frame ^[(param %0 %3 local) (param %1 %2 local)]
  call %2 local mod/2
  echo (string %1 local "7 mod 4 = 3 : ") local
  print %2 local
  float %2 local 4
  float %3 local -7
  frame ^[(param %0 %3 local) (param %1 %2 local)]
  call %2 local mod/2
  echo (string %1 local "-7 mod 4 = 1 : ") local
  print %2 local
  float %2 local -4
  float %3 local 7
  frame ^[(param %0 %3 local) (param %1 %2 local)]
  call %2 local mod/2
  echo (string %1 local "7 mod -4 = -1 : ") local
  print %2 local
  float %2 local -4
  float %3 local -7
  frame ^[(param %0 %3 local) (param %1 %2 local)]
  call %2 local mod/2
  echo (string %1 local "-7 mod -4 = -3 : ") local
  print %2 local
  float %2 local 4.2
  float %3 local -7
  frame ^[(param %0 %3 local) (param %1 %2 local)]
  call %2 local mod/2
  echo (string %1 local "-7 mod 4.2 = 1.4 : ") local
  print %2 local
  float %2 local 4
  float %3 local -7.6
  frame ^[(param %0 %3 local) (param %1 %2 local)]
  call %2 local mod/2
  echo (string %1 local "-7.6 mod 4 = 0.4 : ") local
  print %2 local
  float %2 local 4.2
  float %3 local -7.6
  frame ^[(param %0 %3 local) (param %1 %2 local)]
  call %2 local mod/2
  echo (string %1 local "-7.6 mod 4.2 = 0.8 : ") local
  print %2 local
  izero %0 local
  return
.end

.function: mod/2
  ; result = arg0 mod arg1
  .name: %0 result
  .name: %1 tmp_str
  .name: %2 arg1
  .name: %3 arg0
  .name: %4 tmp_float
  .name: %5 tmp_bool
  .name: %6 min_res
  .name: %7 max_res
  .name: %8 step
  arg %arg0 local %0
  arg %arg1 local %1
  ; arg1 <> 0
  if (not (eq %tmp_bool local %arg1 local (float %tmp_float local 0) local) local) local mod_not_zero
  throw (string %tmp_str local "modulo by zero") local
  .mark: mod_not_zero
  ; if (arg1 > 0) then result in (0, arg1)
  ; if (arg1 < 0) then result in (arg1, 0)
  if (lt %tmp_bool local %arg1 local (float %tmp_float local 0) local) local mod_negative
  float %min_res local 0
  copy %max_res local %arg1 local
  jump mod_prepare
  .mark: mod_negative
  copy %min_res local %arg1 local
  float %max_res local 0
  .mark: mod_prepare
  ; result = arg0
  ; step = arg1
  ; if (arg0 > 0) then step < 0
  ; if (arg0 < 0) then step > 0
  copy %result local %arg0 local
  copy %step local %arg1 local
  if (lt %tmp_bool local %arg0 local (float %tmp_float local 0) local) local mod_check_step
  if (gt %tmp_bool local %step local (float %tmp_float local 0) local) local mod_negate_step mod_check
  .mark: mod_check_step
  if (gt %tmp_bool local %step local (float %tmp_float local 0) local) local mod_check
  .mark: mod_negate_step
  mul %step local %step local (float %tmp_float local -1) local
  .mark: mod_check
  ; while (result not in (min_res, max_res)) do result = result + step
  if (not (gte %tmp_bool local %result local %min_res local) local) local mod_add
  if (lte %tmp_bool local %result local %max_res local) local mod_done
  .mark: mod_add
  add %result local %result local %step local
  jump mod_check
  .mark: mod_done
  return
.end
