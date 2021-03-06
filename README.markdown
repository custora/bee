# Bee

A Hive client for R, which is also compatible with Spark's ThriftServer.

Unlike other such libraries, Bee does not require rJava, a client-side JVM, or
any extra services to run. Bee interfaces with HiveServer via a C++ thrift
library, keeping client dependencies minimal, and processing fast.

## Installation

You will need:

* Thrift >= 0.9.2 (including development headers)
* R packages: Rcpp and dplyr
* Make

Then run:

    R CMD INSTALL .

## Usage

    library(bee)
    bee <- bee_connect('127.0.0.1', 10000)
    data_frame <- bee_run(bee, 'select * from honey')

The host and port default to `"127.0.0.1"` and `10000` respectively.

## Authentication

Bee currently supports the PLAIN SASL authentication mechanism only. You may
provide `user` and `pass` arguments to `bee_connect` -- they default to
`"anonymous"`.

## Installation not working?

### Custom Thrift path

As Thrift is not always available as a package (e.g. on Debian/Ubuntu), some
users have had trouble building/loading bee with Thrift installed in a custom
path. Here is a recipe to install it on Ubuntu:

* Install thrift from source as per Apache instructions. You need to build the
  cpp library (`--with-cpp=yes`), and for this example we'll install into
  `/opt/thrift`:

```
sudo apt-get install libboost-{test,program-options,system,filesystem}-dev \
    libboost-dev libevent-dev
wget http://apache.arvixe.com/thrift/0.9.3/thrift-0.9.3.tar.gz
tar xvzf thrift-0.9.3.tar.gz
cd thrift-0.9.3
./configure --prefix=/opt/thrift --with-cpp=yes
make
sudo make install
```

* Install the Rcpp and dplyr R packages.
* Add the following to /root/.R/Makevars:

```
CXXFLAGS += -I/opt/thrift/include
LDFLAGS += -L/opt/thrift/lib
```

* Run the following to install bee:

```
sudo su
LD_LIBRARY_PATH=/opt/thrift/lib:$LD_LIBRARY_PATH PATH=/opt/thrift/bin:$PATH make clean install
```

* Run R (as regular user) like this:

````
LD_LIBRARY_PATH=/opt/thrift/lib:$LD_LIBRARY_PATH R
````

## Contributing

 * [Bug reports](https://github.com/custora/bee/issues)
 * [Source](https://github.com/custora/bee)
 * Patches: Fork on Github, send pull request.
   * Include tests where practical.
   * Leave the version alone, or bump it in a separate commit.

## Copyright

Copyright (c) Custora. See LICENSE for details.
