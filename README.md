# SeSQLite

This repository contains the complete source code for the SeSQLite project.
SeSQLite integrated the Mandatory Access Control checks in the SQLite database.

SeSQLite is still in an alpha version, but the core functionalities of SQL
are provided. You can find additional information about the project at:

<http://unibg-seclab.github.io>


## Compiling

* Download the repository, then just run `make` from the repository directory.

* This creates a `build` directory. If SELinux is installed, the SeSQLite
  extension will be compiled and the checks integrated inside the SQLite
  database.

See the makefile for additional targets.

## License

SQLite is distributed under public domain.

The SeSQLite extension is distributed under Apache 2.0.
