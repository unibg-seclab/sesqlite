# SeSQLite
----------

SQLite is the most widely deployed in-process library that implements a SQL
database engine. It offers high storage efficiency, fast query operation and
small memory needs. Due to the fact that a complete SQLite database is stored
in a single cross-platform disk file and SQLite does not support multiple
users, anyone who has direct access to the file can read the whole database
content.

SELinux was originally developed as a Mandatory Access Control (MAC) mechanism
for Linux to demonstrate how to overcome DAC limitations. However, SELinux
provides per-file protection, thus the database file is treated as an atomic
unit, impeding the definition of a fine-grained mandatory access control (MAC)
policy for database objects.

SeSQLite is a SQLite extension that integrates SELinux access controls into
SQLite with minimal performance and storage overhead. SeSQLite implements
labeling and access control at both schema level (for tables and columns)
and row level. This permits the management of a fine-grained access policy
for database objects.

## Source Code

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

* SQLite is distributed under public domain.
* The SeSQLite extension is distributed under Apache 2.0.
