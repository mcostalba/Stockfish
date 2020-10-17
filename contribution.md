#!/bin/bash
# check for errors under valgrind or sanitizers.

error()
{
  echo "instrumented testing failed on line $1"
  exit 1
}
trap 'error ${LINENO}' ERR

# define suitable post and prefixes for testing options
case $1 in
  --valgrind)
    echo "valgrind testing started"
    prefix=''
    exeprefix='valgrind --error-exitcode=42'
    postfix='1>/dev/null'
    threads="1"
  ;;
  --valgrind-thread)
    echo "valgrind-thread testing started"
    prefix=''
    exeprefix='valgrind --error-exitcode=42'
    postfix='1>/dev/null'
    threads="2"
  ;;
  --sanitizer-undefined)
    echo "sanitizer-undefined testing started"
    prefix='!'
    exeprefix=''
    postfix='2>&1 | grep -A50 "runtime error:"'
    threads="1"
  ;;
  --sanitizer-thread)
    echo "sanitizer-thread testing started"
    prefix='!'
    exeprefix=''
    postfix='2>&1 | grep -A50 "WARNING: ThreadSanitizer:"'
    threads="2"

cat << EOF > tsan.supp
race:TTEntry::move
race:TTEntry::depth
race:TTEntry::bound
race:TTEntry::save
race:TTEntry::value
race:TTEntry::eval
race:TTEntry::is_pv
