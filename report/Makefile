# Makefile for LaTeX Report Compilation
# University of Aveiro - BI Report 1

# Main document
MAIN = main
LATEX = pdflatex
BIBTEX = bibtex
VIEWER = xdg-open

# Compilation flags
LATEX_FLAGS = -interaction=nonstopmode -halt-on-error

# Output directory (optional)
BUILD_DIR = .

.PHONY: all pdf clean clean-all view help watch quick

# Default target
all: pdf

# Help target - shows available commands
help:
	@echo "Available targets:"
	@echo "  make          - Build the PDF (same as 'make pdf')"
	@echo "  make pdf      - Full build with bibliography"
	@echo "  make quick    - Quick build without bibliography"
	@echo "  make view     - Open the PDF with default viewer"
	@echo "  make clean    - Remove auxiliary files (keep PDF)"
	@echo "  make clean-all - Remove all generated files including PDF"
	@echo "  make watch    - Continuous compilation (requires inotify-tools)"
	@echo "  make help     - Show this help message"

# Quick compilation (no bibliography)
quick:
	@echo "==> Quick compilation (no bibliography)"
	$(LATEX) $(LATEX_FLAGS) $(MAIN).tex

# Full PDF build with bibliography
pdf:
	@echo "==> Building PDF with bibliography..."
	$(LATEX) $(LATEX_FLAGS) $(MAIN).tex
	@if grep -q "\\citation" $(MAIN).aux 2>/dev/null; then \
		echo "==> Running BibTeX..."; \
		$(BIBTEX) $(MAIN); \
		echo "==> Recompiling to integrate references..."; \
		$(LATEX) $(LATEX_FLAGS) $(MAIN).tex; \
	fi
	@echo "==> Final compilation..."
	$(LATEX) $(LATEX_FLAGS) $(MAIN).tex
	@echo "==> PDF generated: $(MAIN).pdf"

# Open PDF with default viewer
view: pdf
	@echo "==> Opening PDF..."
	$(VIEWER) $(MAIN).pdf &

# Clean auxiliary files (keep PDF)
clean:
	@echo "==> Cleaning auxiliary files..."
	@rm -f *.aux *.log *.out *.toc *.lof *.lot *.fls *.fdb_latexmk
	@rm -f *.bbl *.blg *.bcf *.run.xml *.synctex.gz
	@rm -f *.nav *.snm *.vrb *.dvi *.ps
	@rm -f sections/*/*.aux
	@echo "==> Clean complete (PDF preserved)"

# Clean everything including PDF
clean-all: clean
	@echo "==> Removing PDF..."
	@rm -f $(MAIN).pdf
	@echo "==> All generated files removed"

# Continuous compilation (requires inotify-tools)
watch:
	@echo "==> Watching for changes (Ctrl+C to stop)..."
	@echo "==> Install inotify-tools if this fails: sudo apt install inotify-tools"
	@while true; do \
		inotifywait -qe modify $(MAIN).tex sections/*/*.tex references.bib 2>/dev/null && \
		make quick; \
	done
