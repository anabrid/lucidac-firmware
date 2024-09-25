\page docs Firmware documentation

The firmware documentation is currently written with [Doxygen](https://www.doxygen.nl/). This means:

* Regular Javadoc-style in-code comments
* a number of markdown files in the `/docs` directory

Doxygen is a rather old tool which however produces a fantastic *cross-reference* for C++ code
which can replace an IDE and makes it easy to dig into the code in particular for people not
familiar with it.

## Hosted output

The docs are build as a CI job to https://anabrid.dev/docs/hybrid-controller/

## HTML output

Traditional Doxygen can be used to generate the HTML view.

In order to build the docs locally, install doxygen (for instance with your system's
package manager) and run `make docs`. Then point your browser to the generated `/docs/html/index.html`
file.

Note that [Graphviz](https://graphviz.org/) is assumed on the path (`dot` command). If
not available, change the line `HAVE_DOT = YEs` in the `Doxyfile` to `NO`. Doxygen then falls back
to internally drawn inheritance/collaboration graphs (which are not that beautiful, but still impressive).
Build time using `dot` for the first time is dramatically increased (like 120sec instead of 9sec)
but in subsequent builds dramatically decreased (like 1sec instead of 9sec).

We use [Doxygen Awesome](https://jothepro.github.io/doxygen-awesome-css/) as a CSS
theme to modernize the look. It is currently hosed within the firmware repository and
has no build-time-dependencies so it will just work.

Despite using Doxygen Awesome, the website still feels old and crumbled. We consider using
https://breathe.readthedocs.io/ in order to use [Sphinx](https://www.sphinx-doc.org) for
documentation instead.

## Latex/PDF output

Doxygen also generates latex which "just compiles" with a contemporary standard
[texlive](https://tug.org/texlive/) installation.

Note that doxygen uses a lot of EPS files on intermediate steps internally which require
`epstopdf` and `repstopdf` binaries on the path. `epspdf` is not compatible to these
commands. However you can download the [epstopdf package from CTAN](https://ctan.org/pkg/epstopdf),
i.e. the following will work:

```
cd ~/bin
wget https://mirrors.ctan.org/support/epstopdf.zip
unzip epstopdf.zip
mv epstopdf epstopdf.ctan
ln -s $PWD/epstopdf.ctan/epstopdf.pl  epstopdf
ln -s $PWD/epstopdf.ctan/epstopdf.pl repstopdf
```

If your path doesn't include `~/bin` yet, add a line such as

```
PATH="$PATH:$HOME/bin"
```

to your `~/.bashrc` file or similar. Afterwards you should be able to go to the `/docs/latex`
output directory and just run `make` in order to build the file `refman.pdf` from the input
file `refman.tex` (and it more then 1000 included tex files).

## How to add figures

Unfortunately, adding figures requires to manually add them at `HTML_EXTRA_FILES`, otherwise
they won't be available.
