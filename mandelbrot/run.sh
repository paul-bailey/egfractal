#! /usr/bin/env bash
# shell script for generating some of my favorites

generate_readmes=n
case $1 in
    readme)
                generate_readmes=y
                ;;
         *)
                ;;
esac

if test $generate_readmes = y
then
    ./mandelbrot -b16 -w300 -h300 -o readme-10.bmp -z0.0001  -d1 -p1 -x 1.2090000 -y0.2385000
    ./mandelbrot -b16 -w300 -h300 -o readme-15.bmp -z1.0e-7  -d1 -p2 -x 1.2089925 -y0.2385097 -n1200
    ./mandelbrot -b16 -w300 -h300 -o readme-16.bmp -z5.0e-8  -d1 -p6 -x-0.1550495 -y-0.65059865
    ./mandelbrot -b16 -w300 -h300 -o readme-17.bmp -z4.0e-12 -d1 -p3 -x-0.14000524460488 -y-0.64935985788190
    ./mandelbrot -b16 -w300 -h300 -o readme-18.bmp -z1.0e-4 -D  -x 0.7699100 -y 0.10949000
    ./mandelbrot -b16 -w300 -h300 -o readme-19.bmp -z1.0e-6 -D  -x-0.25204350 -y 0.00014850 --negate
    ./mandelbrot -b16 -w300 -h300 -o readme-20.bmp -z1.0e-6 -D  -x-0.25205250 -y 0.00014590 -n100000
    for i in `seq 10 20`
    do
        fil=readme-${i}
        test -f ${fil}.bmp && convert ${fil}.bmp ${fil}.png
        test -f ${fil}.bmp && rm ${fil}.bmp
    done
    exit 0
fi

./mandelbrot -b32768 -w6000 -h6000 -o mandelbrot-favorites-01.bmp -z1.0e-4  -d1 -p1 -x 1.2090000 -y0.2385000
./mandelbrot -b32768 -w6000 -h6000 -o mandelbrot-favorites-02.bmp -z1.0e-7  -d1 -p2 -x 1.20899252 -y0.2385098 -n1400
./mandelbrot -b32768 -w6000 -h6000 -o mandelbrot-favorites-03.bmp -z5.0e-8  -d1 -p6 -x-0.1550495 -y-0.65059865
./mandelbrot -b65536 -w6000 -h6000 -o mandelbrot-favorites-04.bmp -z4.0e-12 -d3 -p3 -x-0.14000524460488 -y-0.64935985788190
./mandelbrot -b32768 -w6000 -h6000 -o mandelbrot-favorites-05.bmp -z1.0e-4 -D -x 0.76991000 -y 0.10949000
./mandelbrot -b32768 -w6000 -h6000 -o mandelbrot-favorites-06.bmp -z1.0e-6 -D -x-0.25204350 -y 0.00014850 -n3000 --negate
./mandelbrot -b32768 -w6000 -h6000 -o mandelbrot-favorites-07.bmp -z1.0e-6 -D -x-0.25205250 -y 0.00014590 -n100000
./mandelbrot -b32768 -w6000 -h6000 -o mandelbrot-favorites-08.bmp -z5.0e-7 -D -x-0.25205185 -y 0.00014800 -n100000
./mandelbrot -b32768 -w6000 -h6000 -o mandelbrot-favorites-09.bmp -z5.0e-6 -D -x-0.25205000 -y 0.00014850 -n10000
./mandelbrot -b32768 -w6000 -h6000 -o mandelbrot-favorites-10.bmp -z1.0e-3 -D -x 0.77000000 -y 0.11000000 --distance-root 6
./mandelbrot -b65536 -w3000 -h3000 -o mandelbrot-favorites-11.bmp -z4 -D --formula sin --distance-root 8


# Old favorites, some were picked out and added to above
# test -d mdb || mkdir mdb
# echo "main"
# ./mandelbrot -b32768.0 -d1 -p1 -z0.0002 -x1.209 -y0.2385 -w600 -h600 -o mdb/mandelbrot-1a.bmp
# ./mandelbrot -b32768.0 -d1 -p1 -z0.00001 -x1.209 -y0.2385 -w600 -h600 -o mdb/mandelbrot-1c.bmp
# ./mandelbrot -b32768.0 -d1 -p2 -z1.0e-6 -x1.2089925 -y0.2385097 -w600 -h600 -o mdb/mandelbrot-1d.bmp
# ./mandelbrot -b32768.0 -d1 -p3 -z1.0e-8 -x1.208992495 -y0.238509705 -w600 -h600 -o mdb/mandelbrot-1f.bmp
# ./mandelbrot -b32768.0 -d3 -p3 -x-0.14000524460488 -y-0.64935985788190 -z4.0e-12 -w600 -h600 -o mdb/mandelbrot-1g.bmp
# ./mandelbrot -b32768.0 -d1 -p1 -z1.0e-6 -x1.2089925 -y0.2385097 -w600 -h600 -o mdb/mandelbrot-1p.bmp
# ./mandelbrot -D -z1.0e-5 -x-0.25200000 -y0.0001500 -b65536.0 -h600 -w600 -o mdb/mandelbrot-1q.bmp

# These coordinates are from superliminal.com/fractals/mbrot/mbrot.htm.
#
# This takes like two computer-years to compute.
# So for about a thousand dollars and a few years' patience you could
# have this running on AWS or whatnot to get a picture that honestly
# looks no more interesting than any of the others.
#
# The X-coordinate is reverse here because my algo is backwards :(
#
# ./mandelbrot \
# 	-x0.13856524454488 \
# 	-y-0.64935990748190 \
# 	-z4.5e-10 \
# 	-n5000000000 \
#       -w6000 -h6000 \
# 	-b2.0 -o mandelbrot-h.bmp
#
# Also check out paulbourke.net/fractals/mandelbrot/
# and www.cuug.ab.ca/dewara/mandelbrot/images.html
# for more locations.

