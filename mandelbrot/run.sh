./mandelbrot.exe -b32768.0 -d1 -p1 -z0.0002 -x1.209 -y0.2385 -w3000 -h3000 -o mandelbrot-a.bmp
./mandelbrot.exe -b32768.0 -d1 -p1 -z0.0001 -x1.209 -y0.2385 -w3000 -h3000 -o mandelbrot-b.bmp
./mandelbrot.exe -b32768.0 -d1 -p1 -z0.00001 -x1.209 -y0.2385 -w3000 -h3000 -o mandelbrot-c.bmp
./mandelbrot.exe -b32768.0 -d1 -p2 -z1.0e-6 -x1.2089925 -y0.2385097 -w3000 -h3000 -o mandelbrot-d.bmp
./mandelbrot.exe -b32768.0 -d1 -p2 -z1.0e-7 -x1.2089925 -y0.2385097 -w3000 -h3000 -o mandelbrot-e.bmp
./mandelbrot.exe -b32768.0 -d1 -p3 -z1.0e-8 -x1.208992495 -y0.238509705 -w3000 -h3000 -o mandelbrot-f.bmp
./mandelbrot.exe -b32768.0 -d3 -p3 -x-0.14000524460488 -y-0.64935985788190 -z4.0e-12 -w3000 -h3000 -o mandelbrot-g.bmp
# This one doesn't work (apparently the entire image is outside the set)
# but at superliminal.com/fractals/mbrot/mbrot.htm
# there is an awesome picture that the author says exists here.
# ./mandelbrot.exe \
# 	-x-0.13856524454488 \
# 	-y-0.64935990748190 \
# 	-z4.5e-10 \
# 	-n5000000000 \
# 	-b65536.0 -o mandelbrot-h.bmp
./mandelbrot.exe -b32768.0 -d1 -x-0.1550495 -y-0.65059865 -z5.0e-8 -p1 -w3000 -h3000 -o mandelbrot-j.bmp
./mandelbrot.exe -b32768.0 -d1 -x-0.1550495 -y-0.65059865 -z5.0e-8 -p3 -w3000 -h3000 -o mandelbrot-k.bmp
./mandelbrot.exe -b32768.0 -d1 -x-0.1550495 -y-0.65059865 -z5.0e-8 -p4 -w3000 -h3000 -o mandelbrot-m.bmp
