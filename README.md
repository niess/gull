# GULL
( **G**eomagnetic **U**ti**L**ities **L**ibrary )

## Description

GULL provides the instantaneous geomagnetic field in local **E**ast,
**N**orth, **U**pward coordinates. It is optimised for time independent
problems. The supported data sets are:

* [IGRF 12](http://www.ngdc.noaa.gov/IAGA/vmod/igrf.html)
* [WWM 2015](https://www.ngdc.noaa.gov/geomag/WMM/DoDWMM.shtml)

Both have worldwide coverage. The library is written in C99 with the Standard
Library as sole dependency.

## Installation

Currently there is no automatic build procedure. If on a Linux box you might
try and adapt the provided `setup.sh` and `Makefile`. The source code conforms
to C99 and its Standard Library.

## Documentation

The API documentation can be found [here](https://niess.github.io/gull/docs/index.html#HEAD),
as well as some basic examples of usage.

## License
The GULL library is  under the **GPL-3.0** license. See the provided
`LICENSE` file.
