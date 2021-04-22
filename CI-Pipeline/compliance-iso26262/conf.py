# Configuration file for the Sphinx documentation builder.
#
# This file only contains a selection of the most common options. For a full
# list see the documentation:
# https://www.sphinx-doc.org/en/master/usage/configuration.html

# -- Path setup --------------------------------------------------------------

# If extensions (or modules to document with autodoc) are in another directory,
# add these directories to sys.path here. If the directory is relative to the
# documentation root, use os.path.abspath to make it absolute, like shown here.
#
# import os
# import sys
# sys.path.insert(0, os.path.abspath('.'))


# -- Project information -----------------------------------------------------

project = 'iceoryx'
copyright = '2021, Neil Langmead'
author = 'Neil Langmead'


# -- General configuration ---------------------------------------------------

# Add any Sphinx extension module names here, as strings. They can be
# extensions coming with Sphinx (named 'sphinx.ext.*') or your custom
# ones.
# Support svg image files with:
# .. image:: image.svg
extensions = ['sphinxcontrib.inkscapeconverter']

# Add any paths that contain templates here, relative to this directory.
templates_path = ['_templates']

# List of patterns, relative to source directory, that match files and
# directories to ignore when looking for source files.
# This pattern also affects html_static_path and html_extra_path.
exclude_patterns = ['_build', 'Thumbs.db', '.DS_Store']


# |project_name|
rst_epilog = '.. |project_name| replace:: %s' % project

latex_elements = {
    # A4 paper size
    'papersize': 'a4paper',
	
    # Select single-side output to remove blank pages from output
    'extraclassoptions': 'openany,oneside',

    # Ensure figures are placed inline with the text (no float option)
    # See https://stackoverflow.com/questions/24285445/sphinx-to-latex-figure-placement
    'figure_align': 'H',

    # Include the underscore package so that Latex can correctly hyphenate words with underscores
    # Note that this changes the behaviour of underscores so included files may not have underscores in their name
    'extrapackages': r'\usepackage{underscore}',
}

