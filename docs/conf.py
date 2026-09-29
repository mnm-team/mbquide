# Configuration file for the Sphinx documentation builder.
#
# Full list of options: https://www.sphinx-doc.org/en/master/usage/configuration.html

import os
import subprocess
import sys

# -- Path setup --------------------------------------------------------------

DOCS_DIR = os.path.abspath(os.path.dirname(__file__))
DOXYGEN_XML_DIR = os.path.join(DOCS_DIR, "doxygen", "xml")

# -- Project information ------------------------------------------------------

project = "MBQuIDE"
copyright = "2026, MNM Team"
author = "MNM Team"
release = "0.1.0"

# -- General configuration ----------------------------------------------------

extensions = [
    "breathe",
    "sphinx_rtd_theme",
    "sphinx.ext.todo",
]

templates_path = ["_templates"]
exclude_patterns = [
    "_build",
    "Thumbs.db",
    ".DS_Store",
    "doxygen",
    ".venv",
    "venv",
]

primary_domain = "cpp"
highlight_language = "cpp"

# -- Breathe (Doxygen -> Sphinx bridge) ---------------------------------------

breathe_projects = {"mbquide": DOXYGEN_XML_DIR}
breathe_default_project = "mbquide"
breathe_default_members = ("members", "undoc-members")

# On Read the Docs, .readthedocs.yaml runs `doxygen Doxyfile` in a pre_build
# job before Sphinx runs, so the XML is already there. For a plain local
# `sphinx-build` (bypassing the Makefile/build.sh helpers), regenerate it
# here too so `make html` always "just works".
if not os.path.isdir(DOXYGEN_XML_DIR):
    try:
        subprocess.run(["doxygen", "Doxyfile"], cwd=DOCS_DIR, check=True)
    except (OSError, subprocess.CalledProcessError) as exc:
        sys.stderr.write(
            f"warning: could not run Doxygen automatically ({exc}); "
            "the API reference page will be empty. Install Doxygen and run "
            "'doxygen Doxyfile' from docs/ manually.\n"
        )

# -- Options for HTML output --------------------------------------------------

html_theme = "sphinx_rtd_theme"
html_static_path = ["_static"]
html_logo = "_static/mbquide-logo.png"
html_theme_options = {
    "collapse_navigation": False,
    "navigation_depth": 4,
    "style_external_links": True,
    "logo_only": True,
}
html_show_sourcelink = True

# -- Options for todo extension -----------------------------------------------

todo_include_todos = True
