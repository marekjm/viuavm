all: text html pdf

.PHONY: watch

text:
	COLOR=no ./view.py > manual.txt

html:
	RENDERING_MODE=RENDERING_MODE_HTML_ASCII_ART ./view.py > manual.html
	COLOR=no RENDERING_MODE=RENDERING_MODE_HTML_ASCII_ART ./view.py > manual.nocolor.html

ps:
	paps --font 'Mononoki 9' manual.txt > manual.ps

pdf: ps
	ps2pdf manual.ps

watch:
	find ./sections ./opcodes ./introduction ./view.py -type f | entr make
