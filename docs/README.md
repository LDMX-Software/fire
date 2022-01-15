# fire docs
The fire documentation is generated with [Doxygen](https://www.doxygen.nl/index.html)
using the fancy [doxygen-awesome](https://github.com/jothepro/doxygen-awesome-css) theme.

You can generate a local copy of the documentation after installing doxygen.
We assume that doxygen is run from the root directory of the fire repository.
```
doxygen docs/doxyfile
```
The output of the documentation then is put into the `docs` folder.

Removing this large amount of files can be a pain, you can remove these files with `find`
if it is available.
```
find docs/ ! -name doxyfile ! -name doxygen-awesome-css -delete
```
