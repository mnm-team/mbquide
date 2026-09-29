# Minimal makefile for Sphinx documentation, extended to run Doxygen first.

# You can set these variables from the command line, and also from the
# environment, for the first two.
SPHINXOPTS    ?=
SPHINXBUILD   ?= sphinx-build
SOURCEDIR     = .
BUILDDIR      = _build

.PHONY: help Makefile doxygen clean

# Put it first so that "make" without argument is like "make help".
help:
	@$(SPHINXBUILD) -M help "$(SOURCEDIR)" "$(BUILDDIR)" $(SPHINXOPTS) $(O)

# Regenerate the Doxygen XML that Breathe reads. Every Sphinx target below
# depends on this so the API reference is always up to date with the C++
# headers.
doxygen:
	doxygen Doxyfile

clean:
	rm -rf "$(BUILDDIR)" doxygen

# Catch-all target: route all unknown targets to Sphinx using the new
# "make mode" option, after regenerating the Doxygen XML.
%: Makefile doxygen
	@$(SPHINXBUILD) -M $@ "$(SOURCEDIR)" "$(BUILDDIR)" $(SPHINXOPTS) $(O)
