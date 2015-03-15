XDEBUG=--debug --Eall

.PHONY: test clean

test: link.out
	wudoo-cpu link.out

clean:
	touch dummy.out dummy.wlib
	rm -v *.out
	rm -v *.wlib

link.out: link.asm jumplib.asm.wlib
	wudoo-asm ${DEBUG} link.asm link.out

jumplib.asm.wlib: jumplib.asm
	wudoo-asm ${DEBUG} --lib jumplib.asm
