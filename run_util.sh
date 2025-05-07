#!/bin/bash - 
#===============================================================================
#
#          FILE: run_util.sh
# 
#         USAGE: ./run_util.sh --input /path/to/memdumps --output /path/to/zpt-compliant-memdumps
# 
#   DESCRIPTION: 
# 
#       OPTIONS: ---
#  REQUIREMENTS: ---
#          BUGS: ---
#         NOTES: ---
#        AUTHOR: Angelos Arelakis (aarel), angelos.arelakis@zptcorp.com
#  ORGANIZATION: ZeroPoint Technologies AB
#       CREATED: 11/18/2019 02:19:14 PM
#      REVISION:  ---
#===============================================================================

set -o nounset                              # Treat unset variables as an error


#-------------------------------------------------------------------------------
# STATICALLY DEFINED PARAMS & PATHS
#-------------------------------------------------------------------------------
ORGPATH=`pwd`
SRCPATH=$ORGPATH
DSTPATH=$ORGPATH

#Current size limit on the upload page
FIXED=1073741824

#-------------------------------------------------------------------------------
curpid=$$

echo "================================ Memdump extraction== ============================"
echo "========================== Owned by ZeroPoint Technologies ========================="
echo " "
echo " "

#-------------------------------------------------------------------------------
# DYNAMICALLY DEFINED PARAMS & PATHS -- will override static ones
#-------------------------------------------------------------------------------
# Option parser, the order doesn't matter
while [ $# -gt 0 ]; do
    case "$1" in
        -i|--input)
            SRCPATH="$2"
            shift 2
            echo "Overwrote the default SRCPATH with provided $SRCPATH"
            ;;
        -o|--output)
            DSTPATH="$2"
            shift 2
            echo "Overwrote the default DSTPATH with provided $DSTPATH"
            ;;
        *)
            break
            ;;
    esac
done

# Some simple argument checks
wrong_arg() {
    echo "Error: invalid value for $1" >&2
    exit 2
}


cd $SRCPATH
files=*

for i in $files
do
  echo "[eliminateNullPages] Removing null pages from the memdump $i"
  $ORGPATH/util/eliminateNullPages --snapshot-path $SRCPATH/$i --output-path $DSTPATH/tmp.bin

  while [ "$(ls -l $DSTPATH/tmp.bin | awk '{print $5}')" -gt "$FIXED" ]; do 
    echo "The memdump size exceeds in size the upload limit"
    echo "[minimizeMemDump] Will extract a representative subset out of it"
    $ORGPATH/util/minimizeMemDump --snapshot-path $DSTPATH/tmp.bin --output-path $DSTPATH/tmp.bin --sampler 1 10.0
    mv $DSTPATH/tmp.bin.generated.bin $DSTPATH/tmp.bin
  done
  mv $DSTPATH/tmp.bin $DSTPATH/$i
done

echo "Thank you for using the Memdump extraction tool of ZeroPoint Technologies!"
echo " "
echo "========================== ZeroPoint Technologies AB  ==============================="

