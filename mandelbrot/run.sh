# TODO: These are all good coordinates but their properties need
# tweaking to get the picture just right.

# Favorites
test -d mdb || mkdir mdb
./mandelbrot -b32768.0 -d1 -p1 -z0.0002 -x1.209 -y0.2385 -w6000 -h6000 -o mdb/mandelbrot-1a.bmp
./mandelbrot -b32768.0 -d1 -p1 -z0.0001 -x1.209 -y0.2385 -w6000 -h6000 -o mdb/mandelbrot-1b.bmp
./mandelbrot -b32768.0 -d1 -p1 -z0.00001 -x1.209 -y0.2385 -w6000 -h6000 -o mdb/mandelbrot-1c.bmp
./mandelbrot -b32768.0 -d1 -p2 -z1.0e-6 -x1.2089925 -y0.2385097 -w6000 -h6000 -o mdb/mandelbrot-1d.bmp
./mandelbrot -b32768.0 -d1 -p2 -z1.0e-7 -x1.2089925 -y0.2385097 -w6000 -h6000 -o mdb/mandelbrot-1e.bmp
./mandelbrot -b32768.0 -d1 -p3 -z1.0e-8 -x1.208992495 -y0.238509705 -w6000 -h6000 -o mdb/mandelbrot-1f.bmp
./mandelbrot -b32768.0 -d3 -p3 -x-0.14000524460488 -y-0.64935985788190 -z4.0e-12 -w6000 -h6000 -o mdb/mandelbrot-1g.bmp
./mandelbrot -b32768.0 -d1 -p1 -x-0.1550495 -y-0.65059865 -z5.0e-8 -w6000 -h6000 -o mdb/mandelbrot-1j.bmp
./mandelbrot -b32768.0 -d1 -p3 -x-0.1550495 -y-0.65059865 -z5.0e-8 -w6000 -h6000 -o mdb/mandelbrot-1k.bmp
./mandelbrot -b32768.0 -d1 -p4 -x-0.1550495 -y-0.65059865 -z5.0e-8 -w6000 -h6000 -o mdb/mandelbrot-1m.bmp
./mandelbrot -b32768.0 -d1 -p1 -x 0.7700000 -y 0.11000000 -z1.0e-3 -w6000 -h6000 -o mdb/mandelbrot-1n.bmp
./mandelbrot -b32768.0 -d1 -p1 -x 0.7699100 -y 0.10949000 -z1.0e-4 -w6000 -h6000 -o mdb/mandelbrot-1o.bmp
./mandelbrot -b32768.0 -d1 -p1 -z1.0e-6 -x1.2089925 -y0.2385097 -w6000 -h6000 -o mdb/mandelbrot-1p.bmp
./mandelbrot -D -z1.0e-5 -x-0.25200000 -y0.0001500 -b65536.0 -h6000 -w6000 -o mdb/mandelbrot-1q.bmp
./mandelbrot -D -z5.0e-6 -x-0.25205000 -y0.0001485 -b65536.0 -h6000 -w6000 -o mdb/mandelbrot-1r.bmp
./mandelbrot -D -z1.0e-6 -x-0.25204350 -y0.0001485 -b65536.0 -h6000 -w6000 -o mdb/mandelbrot-1s.bmp
./mandelbrot -D -z1.0e-6 -x-0.25205250 -y0.0001459 -b65536.0 -n100000 -h6000 -w6000 -o mdb/mandelbrot-1t.bmp
./mandelbrot -D -z5.0e-7 -x-0.25205185 -y0.0001480 -b65536.0 -n100000 -h6000 -w6000 -o mdb/mandelbrot-1u.bmp

# Red, white, blue
test -d rwb || mkdir rwb
./mandelbrot -b32768.0 -d1 -p6 -z0.0002 -x1.209 -y0.2385 -w6000 -h6000 -o rwb/mandelbrot-1a.rwb.bmp
./mandelbrot -b32768.0 -d1 -p6 -z0.0001 -x1.209 -y0.2385 -w6000 -h6000 -o rwb/mandelbrot-1b.rwb.bmp
./mandelbrot -b32768.0 -d1 -p6 -z0.00001 -x1.209 -y0.2385 -w6000 -h6000 -o rwb/mandelbrot-1c.rwb.bmp
./mandelbrot -b32768.0 -d1 -p6 -z1.0e-6 -x1.2089925 -y0.2385097 -w6000 -h6000 -o rwb/mandelbrot-1d.rwb.bmp
./mandelbrot -b32768.0 -d1 -p6 -z1.0e-7 -x1.2089925 -y0.2385097 -w6000 -h6000 -o rwb/mandelbrot-1e.rwb.bmp
./mandelbrot -b32768.0 -d1 -p6 -z1.0e-8 -x1.208992495 -y0.238509705 -w6000 -h6000 -o rwb/mandelbrot-1f.rwb.bmp
./mandelbrot -b32768.0 -d3 -p6 -x-0.14000524460488 -y-0.64935985788190 -z4.0e-12 -w6000 -h6000 -o rwb/mandelbrot-1g.rwb.bmp
./mandelbrot -b32768.0 -d1 -p6 -z5.0e-8 -x-0.1550495 -y-0.65059865 -w6000 -h6000 -o rwb/mandelbrot-1j.rwb.bmp
./mandelbrot -b32768.0 -d1 -p6 -z1.0e-3 -x 0.7700000 -y 0.11000000 -w6000 -h6000 -o rwb/mandelbrot-1n.rwb.bmp
./mandelbrot -b32768.0 -d1 -p6 -z1.0e-4 -x 0.7699100 -y 0.10949000 -w6000 -h6000 -o rwb/mandelbrot-1o.rwb.bmp
./mandelbrot -b32768.0 -d1 -p6 -z1.0e-6 -x 1.2089925 -y 0.2385097 -w6000 -h6000 -o rwb/mandelbrot-1p.rwb.bmp
./mandelbrot -b65536.0 -d1 -p6 -z1.0e-5 -x-0.25200000 -y0.0001500 -h6000 -w6000 -o rwb/mandelbrot-1q.rwb.bmp
./mandelbrot -b65536.0 -d1 -p6 -z5.0e-6 -x-0.25205000 -y0.0001485 -h6000 -w6000 -o rwb/mandelbrot-1r.rwb.bmp
./mandelbrot -b65536.0 -d1 -p6 -z1.0e-6 -x-0.25204350 -y0.0001485 -h6000 -w6000 -o rwb/mandelbrot-1s.rwb.bmp
./mandelbrot -b65536.0 -d1 -p6 -z1.0e-6 -x-0.25205250 -y0.0001459 -n100000 -h6000 -w6000 -o rwb/mandelbrot-1t.rwb.bmp
./mandelbrot -b65536.0 -d1 -p6 -z5.0e-7 -x-0.25205185 -y0.0001480 -n100000 -h6000 -w6000 -o rwb/mandelbrot-1u.rwb.bmp

#  Black and white
test -d bw || mkdir bw
./mandelbrot -D -b32768.0 -z0.0002 -x1.209 -y0.2385 -w6000 -h6000 -o bw/mandelbrot-1a.bw.bmp
./mandelbrot -D -b32768.0 -z0.0001 -x1.209 -y0.2385 -w6000 -h6000 -o bw/mandelbrot-1b.bw.bmp
./mandelbrot -D -b32768.0 -z0.00001 -x1.209 -y0.2385 -w6000 -h6000 -o bw/mandelbrot-1c.bw.bmp
./mandelbrot -D -b32768.0 -z1.0e-6 -x1.2089925 -y0.2385097 -w6000 -h6000 -o bw/mandelbrot-1d.bw.bmp
./mandelbrot -D -b32768.0 -z1.0e-7 -x1.2089925 -y0.2385097 -w6000 -h6000 -o bw/mandelbrot-1e.bw.bmp
./mandelbrot -D -b32768.0 -z1.0e-8 -x1.208992495 -y0.238509705 -w6000 -h6000 -o bw/mandelbrot-1f.bw.bmp
./mandelbrot -D -b32768.0 -x-0.14000524460488 -y-0.64935985788190 -z4.0e-12 -w6000 -h6000 -o bw/mandelbrot-1g.bw.bmp
./mandelbrot -D -b32768.0 -z5.0e-8 -x-0.15504950 -y-0.65059865 -w6000 -h6000 -o bw/mandelbrot-1j.bw.bmp
./mandelbrot -D -b32768.0 -z1.0e-3 -x 0.77000000 -y 0.11000000 -w6000 -h6000 -o bw/mandelbrot-1n.bw.bmp
./mandelbrot -D -b32768.0 -z1.0e-4 -x 0.76991000 -y 0.10949000 -w6000 -h6000 -o bw/mandelbrot-1o.bw.bmp
./mandelbrot -D -b32768.0 -z1.0e-6 -x 1.20899250 -y 0.23850970 -w6000 -h6000 -o bw/mandelbrot-1p.bw.bmp
./mandelbrot -D -b65536.0 -z1.0e-5 -x-0.25200000 -y 0.00015000 -h6000 -w6000 -o bw/mandelbrot-1q.bw.bmp
./mandelbrot -D -b65536.0 -z5.0e-6 -x-0.25205000 -y 0.00014850 -h6000 -w6000 -o bw/mandelbrot-1r.bw.bmp
./mandelbrot -D -b65536.0 -z1.0e-6 -x-0.25204350 -y 0.00014850 -h6000 -w6000 -o bw/mandelbrot-1s.bw.bmp
./mandelbrot -D -b65536.0 -z1.0e-6 -x-0.25205250 -y 0.00014590 -n100000 -h6000 -w6000 -o bw/mandelbrot-1t.bw.bmp
./mandelbrot -D -b65536.0 -z5.0e-7 -x-0.25205185 -y 0.00014800 -n100000 -h6000 -w6000 -o bw/mandelbrot-1u.bw.bmp

# Because I wrote all this during the Christmas season
test -d rg || mkdir rg
./mandelbrot -b32768.0 -d1 -p7 -z0.0002 -x1.209 -y0.2385 -w6000 -h6000 -o rg/mandelbrot-1a.rg.bmp
./mandelbrot -b32768.0 -d1 -p7 -z0.0001 -x1.209 -y0.2385 -w6000 -h6000 -o rg/mandelbrot-1b.rg.bmp
./mandelbrot -b32768.0 -d1 -p7 -z0.00001 -x1.209 -y0.2385 -w6000 -h6000 -o rg/mandelbrot-1c.rg.bmp
./mandelbrot -b32768.0 -d1 -p7 -z1.0e-6 -x1.2089925 -y0.2385097 -w6000 -h6000 -o rg/mandelbrot-1d.rg.bmp
./mandelbrot -b32768.0 -d1 -p7 -z1.0e-7 -x1.2089925 -y0.2385097 -w6000 -h6000 -o rg/mandelbrot-1e.rg.bmp
./mandelbrot -b32768.0 -d1 -p7 -z1.0e-8 -x1.208992495 -y0.238509705 -w6000 -h6000 -o rg/mandelbrot-1f.rg.bmp
./mandelbrot -b32768.0 -d3 -p7 -z4.0e-12 -x-0.14000524460488 -y-0.64935985788190 -w6000 -h6000 -o rg/mandelbrot-1g.rg.bmp
./mandelbrot -b32768.0 -d1 -p7 -z5.0e-8 -x-0.1550495 -y-0.65059865 -w6000 -h6000 -o rg/mandelbrot-1j.rg.bmp
./mandelbrot -b32768.0 -d1 -p7 -z1.0e-3 -x 0.7700000 -y 0.11000000 -w6000 -h6000 -o rg/mandelbrot-1n.rg.bmp
./mandelbrot -b32768.0 -d1 -p7 -z1.0e-4 -x 0.7699100 -y 0.10949000 -w6000 -h6000 -o rg/mandelbrot-1o.rg.bmp
./mandelbrot -b32768.0 -d1 -p7 -z1.0e-6 -x 1.2089925 -y 0.2385097 -w6000 -h6000 -o rg/mandelbrot-1p.rg.bmp
./mandelbrot -b65536.0 -d1 -p7 -z1.0e-5 -x-0.25200000 -y0.0001500 -h6000 -w6000 -o rg/mandelbrot-1q.rg.bmp
./mandelbrot -b65536.0 -d1 -p7 -z5.0e-6 -x-0.25205000 -y0.0001485 -h6000 -w6000 -o rg/mandelbrot-1r.rg.bmp
./mandelbrot -b65536.0 -d1 -p7 -z1.0e-6 -x-0.25204350 -y0.0001485 -h6000 -w6000 -o rg/mandelbrot-1s.rg.bmp
./mandelbrot -b65536.0 -d1 -p7 -z1.0e-6 -x-0.25205250 -y0.0001459 -n100000 -h6000 -w6000 -o rg/mandelbrot-1t.rg.bmp
./mandelbrot -b65536.0 -d1 -p7 -z5.0e-7 -x-0.25205185 -y0.0001480 -n100000 -h6000 -w6000 -o rg/mandelbrot-1u.rg.bmp


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


