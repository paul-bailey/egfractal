./mandelbrot -b32768.0 -d1 -p1 -z0.0002 -x1.209 -y0.2385 -w6000 -h6000 -o mandelbrot-1a.bmp
./mandelbrot -b32768.0 -d1 -p1 -z0.0001 -x1.209 -y0.2385 -w6000 -h6000 -o mandelbrot-1b.bmp
./mandelbrot -b32768.0 -d1 -p1 -z0.00001 -x1.209 -y0.2385 -w6000 -h6000 -o mandelbrot-1c.bmp
./mandelbrot -b32768.0 -d1 -p2 -z1.0e-6 -x1.2089925 -y0.2385097 -w6000 -h6000 -o mandelbrot-1d.bmp
./mandelbrot -b32768.0 -d1 -p2 -z1.0e-7 -x1.2089925 -y0.2385097 -w6000 -h6000 -o mandelbrot-1e.bmp
./mandelbrot -b32768.0 -d1 -p3 -z1.0e-8 -x1.208992495 -y0.238509705 -w6000 -h6000 -o mandelbrot-1f.bmp
./mandelbrot -b32768.0 -d3 -p3 -x-0.14000524460488 -y-0.64935985788190 -z4.0e-12 -w6000 -h6000 -o mandelbrot-1g.bmp

# This takes like a week to compute
# but at superliminal.com/fractals/mbrot/mbrot.htm
# there is an awesome picture that the author says exists here.
# The X-coordinate is reverse here because my algo is backwards :(
#
# ./mandelbrot \
# 	-x0.13856524454488 \
# 	-y-0.64935990748190 \
# 	-z4.5e-10 \
# 	-n5000000000 \
# 	-b2.0 -o mandelbrot-h.bmp
#
# Also check out paulbourke.net/fractals/mandelbrot/
# and www.cuug.ab.ca/dewara/mandelbrot/images.html
# for more locations.

./mandelbrot -b32768.0 -d1 -x-0.1550495 -y-0.65059865 -z5.0e-8 -p1 -w6000 -h6000 -o mandelbrot-1j.bmp
./mandelbrot -b32768.0 -d1 -x-0.1550495 -y-0.65059865 -z5.0e-8 -p3 -w6000 -h6000 -o mandelbrot-1k.bmp
./mandelbrot -b32768.0 -d1 -x-0.1550495 -y-0.65059865 -z5.0e-8 -p4 -w6000 -h6000 -o mandelbrot-1m.bmp
./mandelbrot -b32768.0 -d1 -x 0.7700000 -y 0.11000000 -z1.0e-3 -p1 -w6000 -h6000 -o mandelbrot-1n.bmp
./mandelbrot -b32768.0 -d1 -x 0.7699100 -y 0.10949000 -z1.0e-4 -p1 -w6000 -h6000 -o mandelbrot-1o.bmp
./mandelbrot -b32768.0 -d1 -p1 -z1.0e-6 -x1.2089925 -y0.2385097 -w6000 -h6000 -o mandelbrot-1p.bmp
./mandelbrot -D -z1.0e-5 -x-0.25200000 -y0.0001500 -b65536.0 -h6000 -w6000 -o mandelbrot-1q.bmp
./mandelbrot -D -z5.0e-6 -x-0.25205000 -y0.0001485 -b65536.0 -h6000 -w6000 -o mandelbrot-1r.bmp
./mandelbrot -D -z1.0e-6 -x-0.25204350 -y0.0001485 -b65536.0 -h6000 -w6000 -o mandelbrot-1s.bmp
./mandelbrot -D -z1.0e-6 -x-0.25205250 -y0.0001459 -b65536.0 -n100000 -h6000 -w6000 -o mandelbrot-1t.bmp
./mandelbrot -D -z5.0e-7 -x-0.25205185 -y0.0001480 -b65536.0 -n100000 -h6000 -w6000 -o mandelbrot-1u.bmp
