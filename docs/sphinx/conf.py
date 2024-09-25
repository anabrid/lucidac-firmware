# Configuration file for the Sphinx documentation builder.
#
# For the full list of built-in configuration values, see the documentation:
# https://www.sphinx-doc.org/en/master/usage/configuration.html

# -- Project information -----------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#project-information

project = 'REDAC-Firmware'
copyright = '2024, Anabrid GmbH'
author = 'Anabrid GmbH'

# -- General configuration ---------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#general-configuration

# pip install breathe
# https://breathe.readthedocs.io/ for Doxygen binding
extensions = [
    "breathe"
]

templates_path = ['_templates']
exclude_patterns = ['_build', 'Thumbs.db', '.DS_Store']


###### OPTIONS FOR BREATHE
breathe_projects = {"hybrid-controller": "../docs/xml/"}
breathe_default_project = "hybrid-controller"

#breathe_projects_source = {
#    "hc" : ( "../examples/specific", ["auto_function.h", "auto_class.h"] )
#}



# -- Options for HTML output -------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#options-for-html-output

#html_theme = 'alabaster' # = default
html_theme = 'sphinx_rtd_theme' # Read the docs theme, nicer.


html_static_path = ['_static']
