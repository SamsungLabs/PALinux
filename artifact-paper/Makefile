MAIN ?= ae
LTEX := --latex-args="-synctex=1 -shell-escape"
BTEX := --bibtex-args="-min-crossrefs=99"
SHELL:= $(shell echo $$SHELL)

all: $(DEPS) ## generate a pdf
	@TEXINPUTS="sty:" ./latexrun $(LTEX) $(BTEX) $(MAIN)
	cp latex.out/$(MAIN).synctex.gz .

clean:
	rm -f *.gz
	rm -rf latex.out/
	rm -f ae.pdf

.PHONY: all clean
