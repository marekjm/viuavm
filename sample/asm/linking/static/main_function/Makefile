XDEBUG=--debug --Eall

.PHONY: test clean

test: link.out
	wudoo-cpu link.out

clean:
	touch dummy.out dummy.wlib
	rm -v *.out
	rm -v *.wlib

link.out: libb.asm.wlib liba.asm.wlib main.asm.wlib
	wudoo-asm ${DEBUG} link.asm link.out

libb.asm.wlib: libb.asm
	wudoo-asm ${DEBUG} --lib libb.asm

liba.asm.wlib: liba.asm
	wudoo-asm ${DEBUG} --lib liba.asm

main.asm.wlib: main.asm
	wudoo-asm ${DEBUG} --lib main.asm
