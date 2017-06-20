;
; modulo function
;
; Author: vktgz <vktgz@rocday.pl>
;
; License: GPLv3
;

.function: main/0
  ;%2 = %3 mod %2
  fstore %2 local 4
  fstore %3 local 7
  frame ^[(param %0 %3 local) (param %1 %2 local)]
  call %2 mod/2
  echo (strstore %1 local "7 mod 4 = 3 : ")
  print %2 local
  fstore %2 local 4
  fstore %3 local -7
  frame ^[(param %0 %3 local) (param %1 %2 local)]
  call %2 mod/2
  echo (strstore %1 local "-7 mod 4 = 1 : ")
  print %2 local
  fstore %2 local -4
  fstore %3 local 7
  frame ^[(param %0 %3 local) (param %1 %2 local)]
  call %2 mod/2
  echo (strstore %1 local "7 mod -4 = -1 : ")
  print %2 local
  fstore %2 local -4
  fstore %3 local -7
  frame ^[(param %0 %3 local) (param %1 %2 local)]
  call %2 mod/2
  echo (strstore %1 local "-7 mod -4 = -3 : ")
  print %2 local
  fstore %2 local 4.2
  fstore %3 local -7
  frame ^[(param %0 %3 local) (param %1 %2 local)]
  call %2 mod/2
  echo (strstore %1 local "-7 mod 4.2 = 1.4 : ")
  print %2 local
  fstore %2 local 4
  fstore %3 local -7.6
  frame ^[(param %0 %3 local) (param %1 %2 local)]
  call %2 mod/2
  echo (strstore %1 local "-7.6 mod 4 = 0.4 : ")
  print %2 local
  fstore %2 local 4.2
  fstore %3 local -7.6
  frame ^[(param %0 %3 local) (param %1 %2 local)]
  call %2 mod/2
  echo (strstore %1 local "-7.6 mod 4.2 = 0.8 : ")
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
  arg %arg0 %0
  arg %arg1 %1
  ; arg1 <> 0
  if (not (eq %tmp_bool %arg1 (fstore %tmp_float 0))) mod_not_zero
  throw (strstore %tmp_str "modulo by zero")
  .mark: mod_not_zero
  ; if (arg1 > 0) then result in (0, arg1)
  ; if (arg1 < 0) then result in (arg1, 0)
  if (lt %tmp_bool %arg1 (fstore %tmp_float 0)) mod_negative
  fstore %min_res 0
  copy %max_res %arg1
  jump mod_prepare
  .mark: mod_negative
  copy %min_res %arg1
  fstore %max_res 0
  .mark: mod_prepare
  ; result = arg0
  ; step = arg1
  ; if (arg0 > 0) then step < 0
  ; if (arg0 < 0) then step > 0
  copy %result %arg0
  copy %step %arg1
  if (lt %tmp_bool %arg0 (fstore %tmp_float 0)) mod_check_step
  if (gt %tmp_bool %step (fstore %tmp_float 0)) mod_negate_step mod_check
  .mark: mod_check_step
  if (gt %tmp_bool %step (fstore %tmp_float 0)) mod_check
  .mark: mod_negate_step
  mul %step %step (fstore %tmp_float -1)
  .mark: mod_check
  ; while (result not in (min_res, max_res)) do result = result + step
  if (not (gte %tmp_bool %result %min_res)) mod_add
  if (lte %tmp_bool %result %max_res) mod_done
  .mark: mod_add
  add %result %result %step
  jump mod_check
  .mark: mod_done
  return
.end
