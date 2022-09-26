# File Specification

The files produced by `fire` follow a specification laid on top of
[the HDF5 file format](https://www.hdfgroup.org/solutions/hdf5/).
This page detailing this specification is intended for users developing
a library that reads (or perhaps writes) this format (e.g. fire or its
python analysis package) that _already_ has an HDF5 library. Refer 
directly to the HDF5 documentation if the language you are using does
not already have an HDF5 API. fire-files use the following features
of the HDF5 API.
- Groups
- DataSets of atomic types
- Variable length string DataSets
- Definition of an enum-type and DataSets of it

Following HDF5's convention, the hierarchy within a file is denoted
with Unix-style directory slashes.

The fire specification is relatively simple on the surface; however,
it quickly becomes complicated as the recusive-nature of the type system
allows for exceedingly complex objects.

In-File Path | Description
---|---
`/` | the file itself
`/runs/` | group holding the runs within this file
`/events/` | group holding the event objects within this file

## Objects

## std::vector

