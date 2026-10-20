#!/bin/bash

# the first argument is the path of sd2-trackgen
trackgen="sd2-trackgen"
if [ $# -eq 1 ]; then
    trackgen=$1
fi

temp_dir=$(mktemp -d -t trackgen.XXXXXXXX)
pwd=`pwd`

check-acc () {
	$trackgen -c $1 -n $2 -r -A -i $pwd/../../../data/tracks/$1/$2 -o $temp_dir >& $temp_dir/$2.txt
	status=$?
	if [ $status -eq 0 ]; then
		diff $pwd/../../../data/tracks/$1/$2/$2.acc $temp_dir/$2.acc > $temp_dir/$2.acc.diff
		[ $? -eq 0 ] && echo "$1/$2.acc GOOD" || echo "$1/$2.acc BAD"
	fi
}

check () {
	$trackgen -c $1 -n $2 $3 -i $pwd/../../../data/tracks/$1/$2 -o $temp_dir -a >& $temp_dir/$2.txt
	status=$?
	if [ $status -eq 0 ]; then
		if test -f "$pwd/../../../data/tracks/$1/$2/$2.ac"; then
			diff $pwd/../../../data/tracks/$1/$2/$2.ac $temp_dir/$2.ac > $temp_dir/$2.ac.diff
			[ $? -eq 0 ] && echo "$1/$2.ac GOOD" || echo "$1/$2.ac BAD"
			if test -f "$pwd/../../../data/tracks/$1/$2/$2-trk.ac"; then
				diff $pwd/../../../data/tracks/$1/$2/$2-trk.ac $temp_dir/$2-trk.ac > $temp_dir/$2.ac.diff
				[ $? -eq 0 ] && echo "$1/$2-trk.ac GOOD" || echo "$1/$2-trk.ac BAD"
			fi
			if test -f "$pwd/../../../data/tracks/$1/$2/$2-msh.ac"; then
				diff $pwd/../../../data/tracks/$1/$2/$2-msh.ac $temp_dir/$2-msh.ac > $temp_dir/$2.ac.diff
				[ $? -eq 0 ] && echo "$1/$2-msh.ac GOOD" || echo "$1/$2-msh.ac BAD"
			fi
			if test -f "$pwd/../../../data/tracks/$1/$2/$2-obj-1.ac"; then
				diff $pwd/../../../data/tracks/$1/$2/$2-obj-1.ac $temp_dir/$2-obj-1.ac > $temp_dir/$2.ac.diff
				[ $? -eq 0 ] && echo "$1/$2-obj-1.ac GOOD" || echo "$1/$2-obj-1.ac BAD"
			fi
			if test -f "$pwd/../../../data/tracks/$1/$2/$2-obj-2.ac"; then
				diff $pwd/../../../data/tracks/$1/$2/$2-obj-2.ac $temp_dir/$2-obj-2.ac > $temp_dir/$2.ac.diff
				[ $? -eq 0 ] && echo "$1/$2-obj-2.ac GOOD" || echo "$1/$2-obj-2.ac BAD"
			fi
		elif test -f "$pwd/../../../data/tracks/$1/$2/$2-src.ac"; then
			diff $pwd/../../../data/tracks/$1/$2/$2-src.ac $temp_dir/$2.ac > $temp_dir/$2.ac.diff
			[ $? -eq 0 ] && echo "$1/$2.ac GOOD" || echo "$1/$2.ac BAD"
			if test -f "$pwd/../../../data/tracks/$1/$2/$2-trk.ac"; then
				diff $pwd/../../../data/tracks/$1/$2/$2-trk.ac $temp_dir/$2-trk.ac > $temp_dir/$2.ac.diff
				[ $? -eq 0 ] && echo "$1/$2-trk.ac GOOD" || echo "$1/$2-trk.ac BAD"
			elif test -f "$pwd/../../../data/tracks/$1/$2/$2-trk-src.ac"; then
				diff $pwd/../../../data/tracks/$1/$2/$2-trk-src.ac $temp_dir/$2-trk.ac > $temp_dir/$2.ac.diff
				[ $? -eq 0 ] && echo "$1/$2-trk.ac GOOD" || echo "$1/$2-trk.ac BAD"
			fi
			if test -f "$pwd/../../../data/tracks/$1/$2/$2-msh.ac"; then
				diff $pwd/../../../data/tracks/$1/$2/$2-msh.ac $temp_dir/$2-msh.ac > $temp_dir/$2.ac.diff
				[ $? -eq 0 ] && echo "$1/$2-msh.ac GOOD" || echo "$1/$2-msh.ac BAD"
			elif test -f "$pwd/../../../data/tracks/$1/$2/$2-msh-src.ac"; then
				diff $pwd/../../../data/tracks/$1/$2/$2-msh-src.ac $temp_dir/$2-msh.ac > $temp_dir/$2.ac.diff
				[ $? -eq 0 ] && echo "$1/$2-msh.ac GOOD" || echo "$1/$2-msh.ac BAD"
			fi
			if test -f "$pwd/../../../data/tracks/$1/$2/$2-obj-1.ac"; then
				diff $pwd/../../../data/tracks/$1/$2/$2-obj-1.ac $temp_dir/$2-obj-1.ac > $temp_dir/$2.ac.diff
				[ $? -eq 0 ] && echo "$1/$2-obj-1.ac GOOD" || echo "$1/$2-obj-1.ac BAD"
			elif test -f "$pwd/../../../data/tracks/$1/$2/$2-obj-1-src.ac"; then
				diff $pwd/../../../data/tracks/$1/$2/$2-obj-1-src.ac $temp_dir/$2-obj-1.ac > $temp_dir/$2.ac.diff
				[ $? -eq 0 ] && echo "$1/$2-obj-1.ac GOOD" || echo "$1/$2-obj-1.ac BAD"
			fi
			if test -f "$pwd/../../../data/tracks/$1/$2/$2-obj-2.ac"; then
				diff $pwd/../../../data/tracks/$1/$2/$2-obj-2.ac $temp_dir/$2-obj-2.ac > $temp_dir/$2.ac.diff
				[ $? -eq 0 ] && echo "$1/$2-obj-2.ac GOOD" || echo "    $1/$2-obj-2.ac BAD"
			elif test -f "$pwd/../../../data/tracks/$1/$2/$2-obj-2-src.ac"; then
				diff $pwd/../../../data/tracks/$1/$2/$2-obj-2-src.ac $temp_dir/$2-obj-2.ac > $temp_dir/$2.ac.diff
				[ $? -eq 0 ] && echo "$1/$2-obj-2.ac GOOD" || echo "$1/$2-obj-2.ac BAD"
			fi	
		else
			echo "$1 $2 no $2.ac file"
		fi
	elif [ $status -eq 3 ]; then
		echo "$1 $2 CRASHED $status"		
		lines=()
		while IFS= read -r line
		do 
			lines+=("$line") 
		done < <(tail -n 3 $temp_dir/$2.txt)
		numLines=${#lines[@]}
		for (( i=0; i<${numLines}; i++ ));
		do
			printf "    %s\n" "${lines[$i]}"
		done
	else
		echo "$1 $2 FAILED $status"
		lines=()
		while IFS= read -r line
		do 
			lines+=("$line") 
		done < <(tail -n 3 $temp_dir/$2.txt)
		numLines=${#lines[@]}
		for (( i=0; i<${numLines}; i++ ));
		do
			printf "    %s\n" "${lines[$i]}"
		done
	fi
	
	if  test -f "$pwd/../../../data/tracks/$1/$2/$2-trk-raceline.ac"; then
		$trackgen -c $1 -n $2 -i $pwd/../../../data/tracks/$1/$2 -o $temp_dir -r >& $temp_dir/$2.txt
		status=$?
		if [ $status -eq 0 ]; then
			diff $pwd/../../../data/tracks/$1/$2/$2-trk-raceline.ac $temp_dir/$2-trk-raceline.ac > $temp_dir/$2.ac.txt
			[ $? -eq 0 ] && echo "$1/$2-trk-raceline.ac GOOD" || echo "$1/$2-trk-raceline.ac BAD"
		elif [ $status -eq 3 ]; then
			echo "    trackgen CRASHED $status"		
			lines=()
			while IFS= read -r line
			do 
				lines+=("$line") 
			done < <(tail -n 3 $temp_dir/$2.txt)
			numLines=${#lines[@]}
			for (( i=0; i<${numLines}; i++ ));
			do
				printf "        %s\n" "${lines[$i]}"
			done
		else
			echo "    trackgen FAILED $status"
			lines=()
			while IFS= read -r line
			do 
				lines+=("$line") 
			done < <(tail -n 3 $temp_dir/$2.txt)
			numLines=${#lines[@]}
			for (( i=0; i<${numLines}; i++ ));
			do
				printf "        %s\n" "${lines[$i]}"
			done
		fi
	elif  test -f "$pwd/../../../data/tracks/$1/$2/$2-trk-raceline-src.ac"; then
		$trackgen -c $1 -n $2 -i $pwd/../../../data/tracks/$1/$2 -o $temp_dir -r >& $temp_dir/$2.txt
		status=$?
		if [ $status -eq 0 ]; then
			diff $pwd/../../../data/tracks/$1/$2/$2-trk-raceline-src.ac $temp_dir/$2-trk-raceline.ac > $temp_dir/$2.ac.txt
			[ $? -eq 0 ] && echo "$1/$2-trk-raceline.ac GOOD" || echo "$1/$2-trk-raceline.ac BAD"
		elif [ $status -eq 3 ]; then
			echo "    trackgen CRASHED $status"		
			lines=()
			while IFS= read -r line
			do 
				lines+=("$line") 
			done < <(tail -n 3 $temp_dir/$2.txt)
			numLines=${#lines[@]}
			for (( i=0; i<${numLines}; i++ ));
			do
				printf "        %s\n" "${lines[$i]}"
			done
		else
			echo "    trackgen FAILED $status"
			lines=()
			while IFS= read -r line
			do 
				lines+=("$line") 
			done < <(tail -n 3 $temp_dir/$2.txt)
			numLines=${#lines[@]}
			for (( i=0; i<${numLines}; i++ ));
			do
				printf "        %s\n" "${lines[$i]}"
			done
		fi
	fi
}

# add tracks here as they are updated
#check circuit braga
#check circuit dijon
#check circuit jarama
check circuit tuna
check development barrier-testtrack
check development border-testtrack
check development no-barrier-testtrack
check development showroom
check development straight-10
check development testtrack
check dirt dirt-5
check dirt mixed-2
check road e-track-4
check speedway a-speedway
check speedway e-track-5

check-acc circuit tuna
check-acc development barrier-testtrack
check-acc development border-testtrack
check-acc development no-barrier-testtrack
check-acc development showroom
check-acc development straight-10
check-acc development testtrack
check-acc dirt dirt-5
check-acc dirt mixed-2
check-acc road e-track-4
check-acc speedway a-speedway
check-acc speedway e-track-5

rm -rf temp_dir
