# SeSQLite

This repository contains the complete source code for the SeSQLite project.
SeSQLite integrated the Mandatory Access Control checks in the SQLite database.

## Compiling

* Download the repository, then just run `make` from the repository directory.

* This creates a `build` directory. If SELinux is installed, the SeSQLite
  extension will be compiled and the checks integrated inside the SQLite
  database.

See the makefile for additional targets.

## Documentation

You can find additional information about the project at:

<http://unibg-seclab.github.io>

## License

SQLite is distributed under public domain.
The SeSQLite extension is distributed under Apache 2.0.

## Akcnowledgements

This work was partially supported by a Google Award (Winter 2014).
