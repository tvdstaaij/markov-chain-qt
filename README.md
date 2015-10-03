# Markov chain text generator

Qt frontend for generating Markov chain texts using libmarkov (see
[tvdstaaij/libmarkov][1] for details). A simple application, but it can handle
quite large text inputs (tested with over a hundred million words).

## Building

Make sure to check out the libmarkov submodule (for example by using
`git clone --recursive` when cloning this repository). Then, build
`MarkovChain.pro` using qmake. The easiest way to do this is with Qt Creator,
which is included in a [Qt download][2].

The application has been tested with GCC/MingW only.

[1]: https://github.com/tvdstaaij/libmarkov
[2]: https://www.qt.io/download
