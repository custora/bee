# Bee

A Hive client for R, which is also compatible with Spark's ThriftServer.

Unlike other such libraries, Bee does not require rJava, a client-side JVM, or
any extra services to run. Bee interfaces with HiveServer via a C++ thrift
library, keeping client dependencies minimal, and processing fast.

## Installation

From source:

    make install

## Usage

    library(bee)
    bee <- bee_connect('127.0.0.1', 10000)
    data_frame <- bee_run('select * from honey')

The host and port default to `"127.0.0.1"` and `10000` respectively.

## Authentication

Bee currently supports the PLAIN SASL authentication mechanism only. You may
provide `user` and `pass` arguments to `bee_connect` -- they default to
`"anonymous"`.

## Contributing

 * [Bug reports](https://github.com/custora/bee/issues)
 * [Source](https://github.com/custora/bee)
 * Patches: Fork on Github, send pull request.
   * Include tests where practical.
   * Leave the version alone, or bump it in a separate commit.

## Copyright

Copyright (c) Custora. See LICENSE for details.
