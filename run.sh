#!/bin/bash

folder="$1"
target="$2"
shift 2

docker_debug() {
    docker compose up --build -d
}

docker_run () {
    docker compose run --build csapp3e /bin/bash -c "cd $1 && $2"
}

case $folder in
    datalab)
        case $target in 
            "") docker_run "datalab-handout" "make && ./dlc bits.c && chmod +x ./btest && ./btest";;
            debug) docker_debug;;
            clean)
                cd datalab-handout
                make clean;;
        esac;;
    bomb)
        case $target in
            "") docker_run "bomb-handout" "./bomb < psol.txt";;
            debug) docker_debug;;
        esac;;
    target1)
        case $target in
            ctarget.1) docker_run "target1" "./hex2raw < ./inputs/ctarget.1.txt | ./ctarget -q";;
            ctarget.2) docker_run "target1" "./hex2raw < ./inputs/ctarget.2.txt | ./ctarget -q";;
            ctarget.3) docker_run "target1" "/hex2raw < ./inputs/ctarget.3.txt | ./ctarget -q";;
            rtarget.2) docker_run "target1" "./hex2raw < ./inputs/rtarget.2.txt | ./rtarget -q";;
            rtarget.3) docker_run "target1" "./hex2raw < ./inputs/rtarget.3.txt | ./rtarget -q";;
            debug) docker_debug;;
        esac;;
    archlab)
        case $target in
            prereq) docker_run "archlab-handout/sim" "make";;
            sum) docker_run "archlab-handout/sim/misc" "make sum.yo && ./yis sum.yo";;
            rsum) docker_run "archlab-handout/sim/misc" "make rsum.yo && ./yis rsum.yo";;
            copy) docker_run "archlab-handout/sim/misc" "make copy.yo && ./yis copy.yo";;
            iaddq) docker_run "archlab-handout/sim/seq" "make VERSION=full && cd ../ptest && make SIM=../seq/ssim TFLAGS=-i";;
            ncopy-len) docker_run "archlab-handout/sim/pipe" "make ncopy.yo && ./check-len.pl < ncopy.yo";;
            ncopy-driver) docker_run "archlab-handout/sim/pipe" "make drivers && ./correctness.pl";;
            ncopy-psim) docker_run "archlab-handout/sim/pipe" "make psim VERSION=full && cd ../ptest && make SIM=../pipe/psim TFLAGS=-i";;
            ncopy-reg) docker_run "archlab-handout/sim/pipe" "make VERSION=full && ./correctness.pl -p";;
            ncopy-bm) docker_run "archlab-handout/sim/pipe" "make VERSION=full && ./benchmark.pl";;
            clean)
                cd archlab-handout
                make clean
                cd sim
                make clean;;
            debug) docker_debug;;
        esac;;
    cachelab)
        case $target in
            "") docker_run "cachelab-handout" "make && ./driver.py";;
            csim) docker_run "cachelab-handout" "make && ./test-csim";;
            csim-ref) docker_run "cachelab-handout" "./csim-ref -h";;
            trans) docker_run "cachelab-handout" "make && ./test-trans -M 64 -N 64 && ./test-trans -M 32 -N 32 && ./test-trans -M 61 -N 67";;
            clean)
                cd cachelab-handout
                make clean;;
            debug) docker_debug;;
        esac;;
    shlab)
        case $target in
            tshref) docker_run "shlab-handout" "make && ./tshref";;
            tsh) docker_run "shlab-handout" "make && ./tsh";;
            test) docker_run "shlab-handout" "make && for num in {01..16}; do make test\$num; done | tee tsh-test.out";;
            testref) docker_run "shlab-handout" "make && for num in {01..16}; do make rtest\$num; done | tee tshref-test.out";;
            clean)
                cd shlab-handout
                make clean;;
            debug) docker_debug;;
        esac;;
    malloclab)
        case $target in
            "") docker_run "malloclab-handout" "make mdriver && ./mdriver -V -t \$(pwd)/traces";;
            clean)
                cd malloclab-handout
                make clean;;
            debug) docker_debug;;
        esac;;
    proxylab)
        case $target in
            "") docker_run "proxylab-handout" "make && ./driver.sh";;
            clean)
                cd proxylab-handout
                make clean;;
            debug) docker_debug;;
        esac;;
esac
