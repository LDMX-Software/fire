# Design Principles

## Modern C++
I think of "modern" as relying more heavily on the Standard Template
Library (STL) than has been done previously. Especially with fire's
requirement of a C++17-compatible compiler, there are many features
we can use that make our development work easier and runtime safer
(e.g. smart pointers like `std::unique_ptr` and variable container
classes like `std::variant` and `std::any`).

## Modular
Not only allowing keeping fire modular, allowing it to be compiled
more quickly across multiple cores, we also focus on making sure
any components plugging in to fire can be modular. For example,
a processor does not have to be configurable (unless the user so wishes)
and users do not even need to define any conditions unless they 
need to.

## Analysis-Focused
Doing more work on the C++-side to prepare the data-on-disk
to be easier for the analyzer will save time when integrated
over the entire life-cycle of the data.
