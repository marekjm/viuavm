all: text html pdf

RENDER_COLUMNS=100

.PHONY: watch clean

text:
	RENDER_COLUMNS=$(RENDER_COLUMNS) COLOR=no ./view.py > manual.txt

html:
	RENDER_COLUMNS=$(RENDER_COLUMNS) RENDERING_MODE=RENDERING_MODE_HTML_ASCII_ART ./view.py > manual.html
	RENDER_COLUMNS=$(RENDER_COLUMNS) COLOR=no RENDERING_MODE=RENDERING_MODE_HTML_ASCII_ART ./view.py > manual.nocolor.html

ps: text
	paps --font 'Mononoki 9' manual.txt > manual.ps

pdf: ps
	ps2pdf manual.ps

watch:
	find ./sections ./opcodes ./introduction ./view.py -type f | entr make

clean:
	$(RM) *.ps
	$(RM) *.pdf
	$(RM) *.json
	$(RM) *.html
	$(RM) *.txt
