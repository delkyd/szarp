#!/bin/bash 

fail() {

	echo
	echo " FATAL ERROR  -  building terminated "
	echo
	exit $1
}

do_build() {
	dist=$1
	ver=$2
	echo
	echo " ====== Building: $dist $ver ====== "
	echo
	git commit -m "building $dist" --allow-empty || fail $?
	git tag $ver || fail $?
}

do_push() {
	git push origin HEAD || fail $?
	git push origin $ver || fail $?
}

do_usage() {
	echo "Usage:"
	echo "  $0 [-p] <distribution> <version>"
	echo
	echo -e "  -p\tpush to origin"
	echo 
	echo "  use distribution 'stable' for building master or 'unstable' for building rc"
	echo "  $0 will create tag and build SZARP only for the current git branch."
	exit 0
}

push=0

if [ $# -eq 3 ]
then
	if [ $1 == '-p' ]
	then
		push=1
		shift
	else
		do_usage
	fi
elif [ $# -ne 2 ]
then 
	do_usage
fi

dists=$1

version=$2

for d in $dists 
do
	if [ "$d" != "stable" ] && [ "$d" != "unstable" ]
	then
		echo "distribution must be 'stable' (for master) or 'unstable' (for rc)"
		fail
	fi
	do_build $d $version
	if [ $push -eq 1 ]
	then
		do_push $d $version
	fi
done

