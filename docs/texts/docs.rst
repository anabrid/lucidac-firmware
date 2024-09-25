Documentation
=============

The firmware documentation is currently a mix of
`Sphinx <https://www.sphinx-doc.org/>`_ and `Doxygen <https://www.doxygen.nl/>`_.

This means:

* Regular Javadoc-style in-code comments.
* a number of restructured text (RST) files in the ``/docs/texts`` directory
* Doxygen API docs built at ``/docs/doxygen``
* Sphinx docs built at ``/docs/sphinx`` using `Breathe <https://breathe.readthedocs.io/>`_ to
  read in doxygen documentation.
  
Doxygen provides a very good *cross-reference* for C++ code which can replace an IDE and makes it
easy to dig into the code in particular for people not familiar with it. However, the generated
website is not very user friendly. In contrast, Sphinx provides a very good way of writing
documentation whereas the API overview quality for the C++ domain is not so good.

The guiding idea is as following:

* Doxygen is run without any supplementary documentation files or pictures
* Sphinx hosts the RST files, pictures, etc.
* Sphinx builds with the *breathe* plugin so it loads at least some of the doxygen
  information. However, we intend to host both the Doxygen as well as the Sphinx build
  artefacts.

Hosted output
-------------

The docs are build as a CI job to https://anabrid.dev/docs/lucidac-firmware/. This directory
holds both the sphinx and doxygen output.

How to build the doxygen locally
-----------------------------

Doxygen HTML output
...................

In order to build the doxygen locally,

1. install doxygen (for instance with your system's package manager)
2. run ``cd docs && make doxygen``
3. point your browser to the generated ``/docs/doxygen/html/index.html`` file

Note that `Graphviz <https://graphviz.org/>` is assumed on the path (``dot`` command). If
not available, change the line ``HAVE_DOT = YES`` in the ``Doxyfile`` to ``NO``. Doxygen then falls back
to internally drawn inheritance/collaboration graphs (which are not that beautiful, but still impressive).
Build time using ``dot`` for the first time is dramatically increased (like 120sec instead of 9sec)
but in subsequent builds dramatically decreased (like 1sec instead of 9sec).

We use `Doxygen Awesome <https://jothepro.github.io/doxygen-awesome-css/>`_ as a CSS
theme to modernize the look at least a bit. It is currently hosed within the firmware repository and
has no build-time-dependencies so it will just work.


Doxygen Latex/PDF output
........................

Doxygen also generates latex which "just compiles" with a contemporary standard
`texlive <https://tug.org/texlive/>`_ installation.

Note that doxygen uses a lot of EPS files on intermediate steps internally which require
``epstopdf`` and  `repstopdf`` binaries on the path. ``epspdf`` is not compatible to these
commands. However you can download the `epstopdf package from CTAN <https://ctan.org/pkg/epstopdf>`_
i.e. the following will work:

::

    cd ~/bin
    wget https://mirrors.ctan.org/support/epstopdf.zip
    unzip epstopdf.zip
    mv epstopdf epstopdf.ctan
    ln -s $PWD/epstopdf.ctan/epstopdf.pl  epstopdf
    ln -s $PWD/epstopdf.ctan/epstopdf.pl repstopdf


If your path doesn't include ``~/bin`` yet, add a line such as

::

    PATH="$PATH:$HOME/bin"


to your ``~/.bashrc`` file or similar. Afterwards you should be able to go to the ``/docs/doxygen/latex``
output directory and just run ``make`` in order to build the file ``refman.pdf`` from the input
file ``refman.tex`` (and it more then 1000 included tex files).

Doxygen: How to add figures
...........................

Unfortunately, adding figures requires to manually add them at ``HTML_EXTRA_FILES``, otherwise
they won't be available. This is one of the shortcomings of Doxygen. 

How to build the Sphinx locally
-------------------------------

1. Ensure you have the dependencies installed (there should be a file ``requirements.txt``
   which you can install with pip).
2. Ensure to run doxygen before sphinx (see above).
3. Then run something like ``cd docs/sphinx && make html``
4. Point your browser to ``docs/sphinx/_build/html/index.html``

