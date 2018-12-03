#!/bin/sh
size_opt="-w3000 -h3000"
verbose=""
while test $# -ne 0; do
        case $1 in
        --verbose|-v)
                # TODO: Enable this in code
                verbose="-v"
                ;;
        --size)
                size_opt="-w${2} -h${2}"
                shift
                ;;
        *)
                echo "Invalid option \"$1\"">&2
                exit 1
                ;;
        esac
        shift
done
common_args="${size_opt} ${verbose}"
./julia1 -o julia1-1a.bmp $common_args -p2 -R-0.701760000 -I-0.3842000 -b32768 -d1
./julia1 -o julia1-1b.bmp $common_args -p1 -R-0.400000000 -I-0.6000000 -b32768 -d1
./julia1 -o julia1-1c.bmp $common_args -p1 -R-0.209600000 -I 0.7904000 -b32768 -d1 -z0.01 -x0.1
./julia1 -o julia1-1cd.bmp $common_args -D --equalize=0.8 -p1 -R-0.209600000 -I 0.7904000 -b32768 -d1 -z0.01 -x0.1
./julia1 -o julia1-1d.bmp $common_args -p2 -R-0.200000000 -I 0.8000000 -b32768 -d1
./julia1 -o julia1-1e.bmp $common_args -p2 -R-0.200000000 -I 0.8000000 -b32768 -d1 -z0.1
./julia1 -o julia1-1f.bmp $common_args -p2 -R 0.138565244 -I-0.6493599 -z1.0e-3 -x0.1206000 -y0.5230000
./julia1 -o julia1-1g.bmp $common_args -p2 -R 0.138565244 -I-0.6493599 -z5.0e-5 -x0.1205994 -y0.5231718
./julia1 -o julia1-1h.bmp $common_args -p2 -R-0.701760000 -I-0.3842000 -b32768 -d1 --negate -n1500
