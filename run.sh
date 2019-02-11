#! /usr/bin/env bash
# shell script for generating some of my favorites

size_opt="-w6000 -h6000"
verbose=""
convert=n
convert_type=png
delete_current=n
palettes_only=n
while test $# -ne 0
do
        case $1 in
        --verbose|-v)
                verbose="-v"
                ;;
        --size)
                size_opt="-w${2} -h${2}"
                shift;
                ;;
        --convert)
                case $2 in
                png) convert_type=png ;;
                jpg) convert_type=jpg ;;
                *)
                        echo "Invalid conversion type \"$2\"">&2
                        exit 1
                        ;;
                esac
                convert=y
                shift
                ;;
        --delete)
                delete_current=y
                ;;
        --palette)
                palettes_only=y
                ;;
        *)
                echo "Invalid option \"$1\"">&2
                exit 1
                ;;
        esac
        shift
done

outdir=output_examples
if test ${delete_current} = y
then
        test -d ${outdir} && rm -r ${outdir}
fi

test -d ${outdir} || mkdir ${outdir}

do_mbrot_palette () {
        ./mandelbrot/mandelbrot --print-palette -p${1} -o ${common_orgs} ${outdir}/mbrot-pallete-${1}.bmp
}

if test ${palettes_only} = y
then
  for i in `seq 1 8`
  do
          do_mbrot_palette $i
  done
  exit 0
fi

common_args="${size_opt} ${verbose}"

mbrot="./mandelbrot/mandelbrot ${common_args} -o ${outdir}/mandelbrot"
${mbrot}-01.bmp -b32768 -z1.0e-4  -d1 -p1 -x 1.2090000 -y0.2385000
${mbrot}-02.bmp -b32768 -z1.0e-7  -d1 -p2 -x 1.20899252 -y0.2385098 -n1400
${mbrot}-03.bmp -b32768 -z5.0e-8  -d1 -p6 -x-0.1550495 -y-0.65059865
${mbrot}-04.bmp -b65536 -z4.0e-12 -d3 -p3 -x-0.14000524460488 -y-0.64935985788190
${mbrot}-05.bmp -b32768 -z1.0e-4 -D -x 0.76991000 -y 0.10949000
${mbrot}-06.bmp -b32768 -z1.0e-6 -D -x-0.25204350 -y 0.00014850 -n3000 --negate
${mbrot}-07.bmp -b32768 -z1.0e-6 -D -x-0.25205250 -y 0.00014590 -n100000
${mbrot}-08.bmp -b32768 -z5.0e-7 -D -x-0.25205185 -y 0.00014800 -n100000
${mbrot}-09.bmp -b32768 -z5.0e-6 -D -x-0.25205000 -y 0.00014850 -n10000
${mbrot}-10.bmp -b32768 -z1.0e-3 -x 0.77000000 -y 0.11000000 --distance=6
${mbrot}-11.bmp -b65536 -z1.0e-3 -D -x 0.77000000 -y 0.11000000 --color-distance -p2 --negate
${mbrot}-12.bmp -b65536 -z4 --formula sin --distance=8 --color-distance -p4
${mbrot}-13.bmp -b32768 -z 0.0010 --formula burnship -p1 -x 1.625 -y-0.00200
${mbrot}-14.bmp -b32768 -z 0.0010 --formula burnship -p1 -x 1.620 -y 0.00199
${mbrot}-15.bmp -b32768 -z 0.0005 --formula burnship -p1 -x 1.624 -y-0.00100
${mbrot}-16.bmp -b32768 -z 1.0e-7 --formula burnship -p2 -x 1.600 -y 0.00000 --negate --distance=6 --color-distance
${mbrot}-17.bmp -b32768 -z 1.0e-3 --formula burnship -p5 -x-0.952 -y 1.25000 --distance=16 --color-distance
${mbrot}-18.bmp -b32768 -z 2.0e-8 -p2 -x 0.7210050 -y 0.3557445 -n 100000 --negate --distance=10 --color-distance
${mbrot}-19.bmp -b32768 -z 1.0e-4 --formula sin -x 3.141592654 -y0.1000 --distance=3 --color-distance -p6 --negate
${mbrot}-20.bmp -b32768 -z 1.0e-4 --formula sin -x 3.141592654 -y0.0747 --distance=8 --color-distance -p6
${mbrot}-21.bmp -b32768 -z 1.0e-3 --formula sin -x 6.050185307 -y0.5000 --distance=8 --color-distance -p6
${mbrot}-22.bmp -b32768 -z 1.0e-4 --formula sin -x 3.141592654 -y0.1000 --distance=4 --color-distance -p5 --negate
${mbrot}-23.bmp -b32768 -z 1.0e-4 --formula sin -x 3.141592654 -y0.1000 --distance=16 --color-distance -p6
${mbrot}-24.bmp -b32768 --formula cos -x 1.4000 -y 1.300 -z4.0e-1 --distance=16 --negate # TODO: needs work
${mbrot}-25.bmp -b32768 --formula cos -x 1.7000 -y 1.700 -z2.0e-1 --distance=8 #ditto
${mbrot}-26.bmp -b32768 --formula cos -x 1.7005 -y 1.743 -z1.0e-4 --distance=8 #...
${mbrot}-27.bmp -b32768 --formula cos -x 1.7005 -y 1.743 -z1.0e-5 --distance=8 --color-distance -p5
${mbrot}-28.bmp -b32768 --formula cos -x 1.7005 -y 1.743 -z1.0e-5 --distance=16 --negate
${mbrot}-29.bmp -b32768 --formula cos -x 1.850 -y 1.980 -z1.0e-1 --distance=8 --color-distance -p6
${mbrot}-30.bmp -b32768 --formula cos -x 1.8275 -y 1.9792105 -z1.0e-6 --distance=16 -n3000 # TODO: needs work
${mbrot}-31.bmp -b32768 --formula cos -x 1.8275 -y 1.9792105 -z1.0e-7 --distance=4 -n3000 --color-distance -p6
${mbrot}-32.bmp -b32768 --formula poly2 -x 1.0000 -y 0.0100 -z1.0e-1 --distance=8
${mbrot}-33.bmp -b32768 --formula poly2 -x 0.6000 -y 0.6302 -z1.0e-4 --distance=16 -n10000 --negate
${mbrot}-34.bmp -b32768 --formula poly2 -x 0.6000 -y 0.6302 -z1.0e-4 -n10000 --distance=16 --color-distance -p2 --negate
${mbrot}-35.bmp -b32768 --formula poly5 -x-0.00513 -y 0.00047 -z2.0e-5 --distance=4 -n800 --color-distance -p5
${mbrot}-36.bmp -b32768 --formula poly5 -x-0.00513 -y 0.00047 -z2.0e-5 --distance=4 -n800 --color-distance -p6
${mbrot}-37.bmp -b32768 -x 0.761574 -y-0.0847596 -z1.6e-3 --distance=4 --color-distance --negate
${mbrot}-38.bmp -b32768 -x 0.761574 -y-0.0847596 -z64e-6 --distance=4 --negate
${mbrot}-39.bmp -b32768 -x 0.761574 -y-0.0847596 -z12.8e-6 -d1 --color-distance -p8
${mbrot}-40.bmp -b32768 -x 0.790000 -y-0.1500000 -z1.0e-2 --distance=4 --color-distance -p2 --negate
${mbrot}-41.bmp -b32768 -x 0.746300 -y-0.1102000 -z5.0e-3 --distance=4 --color-distance -p2 --negate
${mbrot}-42.bmp -b32768 -x 0.745290 -y 0.1103075 -z1.5e-4 --distance=4 --color-distance -p2 --negate
${mbrot}-43.bmp -b32768 -x 1.250660 -y 0.0201200 -z1.7e-4 --distance=4 --color-distance -p2 --negate
${mbrot}-44.bmp -b32768 -x 0.748000 -y-0.1000000 -z0.0014 --distance=4 --color-distance -p2 --negate

julia="./julia1/julia1 ${common_args} -o ${outdir}/julia1"
${julia}-01.bmp -p2 -R-0.701760000 -I-0.3842000 -b32768 -d1
${julia}-02.bmp -p1 -R-0.400000000 -I-0.6000000 -b32768 -d1
${julia}-03.bmp -p1 -R-0.209600000 -I 0.7904000 -b32768 -d1 -z0.01 -x0.1
${julia}-04.bmp -D --equalize=0.8 -p1 -R-0.209600000 -I 0.7904000 -b32768 -d1 -z0.01 -x0.1
${julia}-05.bmp -p2 -R-0.200000000 -I 0.8000000 -b32768 -d1
${julia}-06.bmp -p2 -R-0.200000000 -I 0.8000000 -b32768 -d1 -z0.1
${julia}-07.bmp -p2 -R 0.138565244 -I-0.6493599 -z1.0e-3 -x0.1206000 -y0.5230000
${julia}-08.bmp -p2 -R 0.138565244 -I-0.6493599 -z5.0e-5 -x0.1205994 -y0.5231718
${julia}-09.bmp -p2 -R-0.701760000 -I-0.3842000 -b32768 -d1 --negate -n1500
${julia}-10.bmp -p1 -R 1.625000000 -I-0.0020000 -b32768 --formula burnship -D --color-distance
${julia}-11.bmp -p2 -R 6.050185307 -I 0.5000000 -b32768 --formula cos --distance=8 --negate
${julia}-12.bmp -p2 -R-0.790000000 -I 0.1500000 -b32768 --distance=8 --negate
${julia}-13.bmp -p2 -R 0.280000000 -I 0.0080000 -b32768 --distance=8 --negate

if test $convert = y
        then
        for bmp in ${outdir}/*.bmp
        do
                # could be $jpg instead of $png, but what's in a name?
                png=`echo $bmp | sed "s/bmp/${convert_type}/"`
                convert $bmp $png
                rm $bmp
        done
fi


# Old favorites, some were picked out and added to above
# test -d mdb || mkdir mdb
# echo "main"
# ./mandelbrot -b32768.0 -d1 -p1 -z0.0002 -x1.209 -y0.2385 -o mdb/mandelbrot-1a.bmp
# ./mandelbrot -b32768.0 -d1 -p1 -z0.00001 -x1.209 -y0.2385 -o mdb/mandelbrot-1c.bmp
# ./mandelbrot -b32768.0 -d1 -p2 -z1.0e-6 -x1.2089925 -y0.2385097 -o mdb/mandelbrot-1d.bmp
# ./mandelbrot -b32768.0 -d1 -p3 -z1.0e-8 -x1.208992495 -y0.238509705 -o mdb/mandelbrot-1f.bmp
# ./mandelbrot -b32768.0 -d3 -p3 -x-0.14000524460488 -y-0.64935985788190 -z4.0e-12 -o mdb/mandelbrot-1g.bmp
# ./mandelbrot -b32768.0 -d1 -p1 -z1.0e-6 -x1.2089925 -y0.2385097 -o mdb/mandelbrot-1p.bmp
# ./mandelbrot -D -z1.0e-5 -x-0.25200000 -y0.0001500 -b65536.0 -o mdb/mandelbrot-1q.bmp
# ./mandelbrot -d1 -n 1000 -p1 -z 0.005 -x 1.115 -y 1.156

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

