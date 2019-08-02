#!/bin/sh

make -B

fn="./test/simple2"

rm -f "$fn.bb"

#time ./rlebwt -m $fn ./index ana
./rlebwt -m $fn ./index ana

#time valgrind --tool=massif --pages-as-heap=yes  ./rlebwt  -m $fn  ./index  "in"

 xxd -b "$fn.b"

# xxd -b "$fn.bb"

# xxd -b "$fn.bb1"

diff "$fn.bb" "$fn.bb1"