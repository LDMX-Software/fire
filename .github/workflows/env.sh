# source file for workflow environments
# assume we are running from root directory of repo

# same behavior in local and remote setup
export GITHUB_WORKFLOWS_PATH="$(dirname ${BASH_SOURCE[0]})"
export FIRE_TEST_MODULE_PATH="$(realpath ${GITHUB_WORKFLOWS_PATH}/../../test/module)"
export FIRE_INSTALL_PREFIX="$(realpath ${GITHUB_WORKFLOWS_PATH}/../../install)"

export PATH=${PATH}:${FIRE_INSTALL_PREFIX}/bin
export LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:${FIRE_INSTALL_PREFIX}/lib
export PYTHONPATH=${PYTHONPATH}:${FIRE_INSTALL_PREFIX}/python
export CMAKE_PREFIX_PATH=${FIRE_INSTALL_PREFIX}

if [ ! -z $BENCH_OUTPUT_DIR ]; then
  mkdir -p ${BENCH_OUTPUT_DIR} || { rc=$?; echo "Unable to create output dir."; return $rc; }
fi

group() {
  echo "::group::$@"
}

group Deduced Environment
env

# GitHub workflow command to set an output key,val pair
set_output() {
  local _key="$1"
  local _val="$2"
  echo "${_key} = ${_val}"
  echo "::set-output name=${_key}::${_val}"
}

# define helpful avg timeing function
#   source: https://stackoverflow.com/a/54920339/17617632
# usage: time <trials> <fire-args>
#   removed tracking of user and sys times
#   just prints out the avg real time in seconds
#   assumes that the string 'real' does not appear in
#   output of 'fire'
__time__() {
  local n_trials=$1
  for ((i = 0; i < n_trials; i++)); do
    time -p fire ${@:2} || return $?
  done |& awk '/real/ { real = real + $2; nr++ }
    END { if (nr>0) printf("%f\n", real/nr); }'
}

# Print the five inputs into the five columns of a CSV line
__print_csv_line__() {
  printf "%s,%s,%s,%s,%s\n" $@
}

# input trials per n_events run and then squence of n_events to run
#  e.g. <script> <name> 100 1 10 100 1000 10000 100000
# we get the size of the output file by assuming it matches the form
#   output/*_<num-events>.*
run_bench() {
  local tag=$1; shift
  local trials=$1; shift
  [ -f ${BENCH_DATA_FILE} ] || __print_csv_line__ branch  mode events time size | tee ${BENCH_DATA_FILE}
  local n_events
  for n_events in $@; do
    echo "  benchmarking ${n_events} Events"
    local t=$(__time__ ${trials} ${FIRE_TEST_MODULE_PATH}/produce.py ${n_events})
    [[ "$?" != "0" ]] && { echo "fire produce.py Errored Out!"; return 1; }
    local produce_output="${FIRE_TEST_MODULE_PATH}/output_${n_events}.h5"
    local s=$(stat -c "%s" ${produce_output})
    [[ "$?" != "0" ]] && return 1
    __print_csv_line__ ${tag} produce ${n_events} ${t} ${s} | tee -a ${BENCH_DATA_FILE}
    t=$(__time__ ${trials} ${FIRE_TEST_MODULE_PATH}/recon.py ${produce_output})
    [[ "$?" != "0" ]] && { echo "fire recon.py Errored Out!"; return 1; }
    s=$(stat -c "%s" ${FIRE_TEST_MODULE_PATH}/recon_output_${n_events}.h5) 
    [[ "$?" != "0" ]] && return 1
    __print_csv_line__ ${tag} recon ${n_events} ${t} ${s} | tee -a ${BENCH_DATA_FILE}
  done
}

