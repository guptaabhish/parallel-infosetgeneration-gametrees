all:
	pdflatex paper.tex
	bibtex paper | ls
	pdflatex paper.tex
	pdflatex paper.tex
	pdflatex paper.tex

bib:
	bibtexpaper

go:
	pdflatex paper.tex

clean:
	rm *.bbl *.blg *.aux *.log
