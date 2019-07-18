VIUAVM_ASM=./build/bin/vm/asm
VIUAVM_NEW_ASM=./build/tooling/exec/assembler.bin

echo $(pwd)

set -x

rm -f ./up/*.module ./up/*.bc

$VIUAVM_ASM -c -o ./up/modfoo.module ./up/modfoo.asm
$VIUAVM_ASM -c -o ./up/modbar.module ./up/modbar.asm
$VIUAVM_ASM -c -o ./up/modbaz.module ./up/modbaz.asm
VIUA_LIBRARY_PATH=./up $VIUAVM_NEW_ASM -o ./up/hello.bc ./up/hello.asm ./up/modbar.module

set +x

touch ./up/hello.bc
