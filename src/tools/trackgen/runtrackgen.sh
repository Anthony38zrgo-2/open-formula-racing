#!/bin/bash

# the first argument is the path of sd2-trackgen
trackgen="sd2-trackgen"
if [ $# -eq 1 ]; then
    trackgen=$1
fi

pwd=`pwd`

run-acc () {
	$trackgen -c $1 -n $2 $3 -i $pwd/../../../data/tracks/$1/$2 -o $pwd/../../../data/tracks/$1/$2 -r -A
}

run () {
	$trackgen -c $1 -n $2 $3 -i $pwd/../../../data/tracks/$1/$2 -o $pwd/../../../data/tracks/$1/$2 -a
}

# add tracks here as they are updated
#run circuit braga
#run circuit dijon
#run circuit jarama
run circuit tuna
run development barrier-testtrack
run development border-testtrack
run development no-barrier-testtrack
run development showroom
run development straight-10
run development testtrack
run dirt dirt-5
run dirt mixed-2
run road e-track-4
run speedway a-speedway
run speedway e-track-5

run-acc circuit tuna
run-acc development barrier-testtrack
run-acc development border-testtrack
run-acc development no-barrier-testtrack
run-acc development showroom
run-acc development straight-10
run-acc development testtrack
run-acc dirt dirt-5
run-acc dirt mixed-2
run-acc road e-track-4
run-acc speedway a-speedway
run-acc speedway e-track-5

rm -rf temp_dir
