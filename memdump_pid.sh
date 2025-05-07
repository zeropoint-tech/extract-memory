#!/bin/bash -
#===============================================================================
#
#          FILE: memdump_pid.sh
#
#         USAGE: ./memdump_pid.sh --output <DEST-PATH> --proc-id <PID> --period <PERIOD>
#         DEST-PATH: path/to/write/the/memdump.bin
#         PID: Process id of the running appl which we want to generate memdump for
#         PERIOD: How often the tool should extract a memdump
#
#
#   DESCRIPTION:
#
#       OPTIONS: ---
#  REQUIREMENTS: ---
#          BUGS: ---
#         NOTES: ---
#        AUTHOR: Angelos Arelakis (aarel), angelos.arelakis@zptcorp.com
#  ORGANIZATION: ZeroPoint Technologies AB
#       CREATED: 2019-10-28 15:29:59
#      REVISION:  ---
#===============================================================================

set -o nounset                              # Treat unset variables as an error

declare -a pids

excludenull=false
ignorelimit=false

#-------------------------------------------------------------------------------
# STATICALLY DEFINED PARAMS & PATHS
#-------------------------------------------------------------------------------
period="30s"
ORGPATH=`pwd`
DSTPATH=$ORGPATH
GITPATH=`git rev-parse --show-toplevel`
nullBlkEliminatorPath=$GITPATH/new_compression_util/nullBlkEliminator
nullBlkEliminator=$nullBlkEliminatorPath/nullBlkEliminator
samplerPath=$GITPATH/new_compression_util/random_sampler
sampler_standalone=$samplerPath/sampler_standalone

#Current size limit on the upload page
FIXED=1073741824
#-------------------------------------------------------------------------------
curpid=$$

echo "======================= Memdump extraction for process id(s) ======================="
echo "========================== Owned by ZeroPoint Technologies ========================="
echo " "
echo " "

#-------------------------------------------------------------------------------
# DYNAMICALLY DEFINED PARAMS & PATHS -- will override static ones
#-------------------------------------------------------------------------------
# Option parser, the order doesn't matter
while [ $# -gt 0 ]; do
    case "$1" in
        -o|--output)
            DSTPATH="$2"
            shift 2
            echo "Overwrote the default DSTPATH with provided $DSTPATH"
            ;;
        --proc-id)
            pids+=("$2")
            shift 2
            ;;
        --period)
            period="$2"
            shift 2
            ;;
        --excludenull)
            excludenull=true
            shift 1
            ;;
        --ignorelimit)
            ignorelimit=true
            shift 1
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

echo "Memory-dump extraction is selected to be executed every $period"
echo " "

#-------------------------------------------------------------------------------
# RUN for all pids provided in the cmd line
#-------------------------------------------------------------------------------
RUN=true

while ${RUN}
do
  echo " "
  echo " "
  echo "Waiting for $period"
  sleep $period
  for pid in ${pids[@]}; do
    if ps -p $pid > /dev/null; then
      kill -STOP $pid
    fi
  done

  DATE=`date +%F--%H-%M-%S`

  for pid in ${pids[@]}; do
    if ps -p $pid > /dev/null; then
      echo " "
      echo "[memdump] Extracting a memdump for pid $pid"
      ./memdump $pid $DSTPATH/$pid"."$DATE".bin" > $DSTPATH/$pid"."$DATE".map"
      if $excludenull; then
        echo "[nullBlkEliminator] Removing null pages from the memdump"
        if [ -f $nullBlkEliminator ]; then
          $nullBlkEliminator --snapshot-path $DSTPATH/$pid"."$DATE".bin" --output-path $DSTPATH/tmp.bin --block-size 4096
          rm $DSTPATH/$pid"."$DATE".bin"
        else
            echo "WARNING: nullBlkEliminator does not exist. You need to compile it first:"
            echo "cd {$nullBlkEliminatorPath}"
            echo "make"
        fi
      fi

      if $ignorelimit; then
        echo "Ignoring limit of memdump size"
        if $excludenull; then
          mv $DSTPATH/tmp.bin $DSTPATH/$pid"."$DATE".bin"
        fi
      else
        if ! $excludenull; then
          mv $DSTPATH/$pid"."$DATE".bin" $DSTPATH/tmp.bin
        fi
        while [ "$(ls -l $DSTPATH/tmp.bin | awk '{print $5}')" -gt "$FIXED" ]; do
          echo "The memdump size exceeds in size the upload limit"
          echo "[sampler] Will extract a representative subset out of it"
          if [ -f $sampler_standalone ]; then
            $sampler_standalone --snapshot-path $DSTPATH/tmp.bin --output-path $DSTPATH/tmp.bin --sampler 1 10.0
            mv $DSTPATH/tmp.bin.generated.bin $DSTPATH/tmp.bin
          else
            echo "WARNING: sampler_standalone does not exist. You need to compile it first:"
            echo "cd {$samplerPath}"
            echo "make"
          fi
        done
        mv $DSTPATH/tmp.bin $DSTPATH/$pid"."$DATE".bin"
      fi
    fi
  done

  for pid in ${pids[@]}; do
    if ps -p $pid > /dev/null; then
      kill -CONT $pid
    fi
  done

  RUN=false
  for pid in ${pids[@]}; do
    if ps -p $pid > /dev/null; then
      RUN=true
    fi
  done
done

echo "The running pid(s) is terminated."
echo "This tool will now terminate."
echo "Thank you for using the Memdump extraction tool of ZeroPoint Technologies!"
echo " "
echo "========================== ZeroPoint Technologies AB  ==============================="

