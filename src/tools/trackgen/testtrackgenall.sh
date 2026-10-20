#!/bin/bash

# the first argument is the path of sd2-trackgen
trackgen="sd2-trackgen"
if [ $# -eq 1 ]; then
    trackgen=$1
fi

temp_dir=$(mktemp -d -t trackgen.XXXXXXXX)

echo "tmp dir: $temp_dir"

pwd=`pwd`

check () {
	$trackgen -c $1 -n $2 -i $pwd/../../../data/tracks/$1/$2 -o $temp_dir -a >& $temp_dir/$2.txt
	status=$?
	if [ $status -eq 0 ]; then
		if  test -f "$pwd/../../../data/tracks/$1/$2/$2.ac"; then
			diff $pwd/../../../data/tracks/$1/$2/$2.ac $temp_dir/$2.ac > $temp_dir/$2.ac.txt
			[ $? -eq 0 ] && echo "    $1/$2.ac GOOD" || echo "    $1/$2.ac BAD"
			if test -f "$pwd/../../../data/tracks/$1/$2/$2-trk.ac"; then
				diff $pwd/../../../data/tracks/$1/$2/$2-trk.ac $temp_dir/$2-trk.ac > $temp_dir/$2.ac.diff
				[ $? -eq 0 ] && echo "    $1/$2-trk.ac GOOD" || echo "    $1/$2-trk.ac BAD"
			fi
			if test -f "$pwd/../../../data/tracks/$1/$2/$2-msh.ac"; then
				diff $pwd/../../../data/tracks/$1/$2/$2-msh.ac $temp_dir/$2-msh.ac > $temp_dir/$2.ac.diff
				[ $? -eq 0 ] && echo "    $1/$2-msh.ac GOOD" || echo "    $1/$2-msh.ac BAD"
			fi
			if test -f "$pwd/../../../data/tracks/$1/$2/$2-obj-1.ac"; then
				diff $pwd/../../../data/tracks/$1/$2/$2-obj-1.ac $temp_dir/$2-obj-1.ac > $temp_dir/$2.ac.diff
				[ $? -eq 0 ] && echo "    $1/$2-obj-1.ac GOOD" || echo "    $1/$2-obj-1.ac BAD"
			fi
			if test -f "$pwd/../../../data/tracks/$1/$2/$2-obj-2.ac"; then
				diff $pwd/../../../data/tracks/$1/$2/$2-obj-2.ac $temp_dir/$2-obj-2.ac > $temp_dir/$2.ac.diff
				[ $? -eq 0 ] && echo "    $1/$2-obj-2.ac GOOD" || echo "    $1/$2-obj-2.ac BAD"
			fi
		elif  test -f "$pwd/../../../data/tracks/$1/$2/$2-src.ac"; then
			echo "    FOUND $2-src.ac"
			diff $pwd/../../../data/tracks/$1/$2/$2-src.ac $temp_dir/$2.ac > $temp_dir/$2.ac.diff
			[ $? -eq 0 ] && echo "    $1/$2.ac GOOD" || echo "    $1/$2.ac BAD"
			if test -f "$pwd/../../../data/tracks/$1/$2/$2-trk.ac"; then
				diff $pwd/../../../data/tracks/$1/$2/$2-trk.ac $temp_dir/$2-trk.ac > $temp_dir/$2.ac.diff
				[ $? -eq 0 ] && echo "    $1/$2-trk.ac GOOD" || echo "    $1/$2-trk.ac BAD"
			elif test -f "$pwd/../../../data/tracks/$1/$2/$2-trk-src.ac"; then
				echo "    FOUND $2-trk-src.ac"
				diff $pwd/../../../data/tracks/$1/$2/$2-trk-src.ac $temp_dir/$2-trk.ac > $temp_dir/$2.ac.diff
				[ $? -eq 0 ] && echo "    $1/$2-trk.ac GOOD" || echo "    $1/$2-trk.ac BAD"
			fi
			if test -f "$pwd/../../../data/tracks/$1/$2/$2-msh.ac"; then
				diff $pwd/../../../data/tracks/$1/$2/$2-msh.ac $temp_dir/$2-msh.ac > $temp_dir/$2.ac.diff
				[ $? -eq 0 ] && echo "    $1/$2-msh.ac GOOD" || echo "    $1/$2-msh.ac BAD"
			elif test -f "$pwd/../../../data/tracks/$1/$2/$2-msh-src.ac"; then
				echo "    FOUND $2-msh-src.ac"
				diff $pwd/../../../data/tracks/$1/$2/$2-msh-src.ac $temp_dir/$2-msh.ac > $temp_dir/$2.ac.diff
				[ $? -eq 0 ] && echo "    $1/$2-msh.ac GOOD" || echo "    $1/$2-msh.ac BAD"
			fi
			if test -f "$pwd/../../../data/tracks/$1/$2/$2-obj-1.ac"; then
				diff $pwd/../../../data/tracks/$1/$2/$2-obj-1.ac $temp_dir/$2-obj-1.ac > $temp_dir/$2.ac.diff
				[ $? -eq 0 ] && echo "    $1/$2-obj-1.ac GOOD" || echo "    $1/$2-obj-1.ac BAD"
			elif test -f "$pwd/../../../data/tracks/$1/$2/$2-obj-1-src.ac"; then
				echo "    FOUND $2-obj-1-src.ac"
				diff $pwd/../../../data/tracks/$1/$2/$2-obj-1-src.ac $temp_dir/$2-obj-1.ac > $temp_dir/$2.ac.diff
				[ $? -eq 0 ] && echo "    $1/$2-obj-1.ac GOOD" || echo "    $1/$2-obj-1.ac BAD"
			fi
			if test -f "$pwd/../../../data/tracks/$1/$2/$2-obj-2.ac"; then
				diff $pwd/../../../data/tracks/$1/$2/$2-obj-2.ac $temp_dir/$2-obj-2.ac > $temp_dir/$2.ac.diff
				[ $? -eq 0 ] && echo "    $1/$2-obj-2.ac GOOD" || echo "    $1/$2-obj-2.ac BAD"
			elif test -f "$pwd/../../../data/tracks/$1/$2/$2-obj-2-src.ac"; then
				echo "    FOUND $2-obj-2-src.ac"
				diff $pwd/../../../data/tracks/$1/$2/$2-obj-2-src.ac $temp_dir/$2-obj-2.ac > $temp_dir/$2.ac.diff
				[ $? -eq 0 ] && echo "    $1/$2-obj-2.ac GOOD" || echo "    $1/$2-obj-2.ac BAD"
			fi	
		else
			echo "    SKIPPED no $2.ac file"
		fi
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
	
	if  test -f "$pwd/../../../data/tracks/$1/$2/$2-trk-raceline.ac"; then
		$trackgen -c $1 -n $2 -i $pwd/../../../data/tracks/$1/$2 -o $temp_dir -r >& $temp_dir/$2.txt
		status=$?
		if [ $status -eq 0 ]; then
			diff $pwd/../../../data/tracks/$1/$2/$2-trk-raceline.ac $temp_dir/$2-trk-raceline.ac > $temp_dir/$2.ac.txt
			[ $? -eq 0 ] && echo "    $1/$2-trk-raceline.ac GOOD" || echo "    $1/$2-trk-raceline.ac BAD"
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
			echo "    FOUND $2-trk-raceline-src.ac"
			diff $pwd/../../../data/tracks/$1/$2/$2-trk-raceline-src.ac $temp_dir/$2-trk-raceline.ac > $temp_dir/$2.ac.txt
			[ $? -eq 0 ] && echo "    $1/$2-trk-raceline.ac GOOD" || echo "    $1/$2-trk-raceline.ac BAD"
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

array=(`find $pwd/../../../data/tracks -name "*.xml"`)

for path in "${array[@]}"
do :
	file="${path##*/}"
	dir=${path%/*}
	track="${dir##*/}"
	xml=$track
	xml+=".xml"
	if [ $file = $xml ]; then
		dir1=${dir%/*}
		category=${dir1##*/}
		echo "FOUND" $category/$track/$file
		check $category $track
	fi
done
