# source file for workflow environments
# assume we are running from root directory of repo

# same behavior in local and remote setup
export GITHUB_WORKFLOWS_PATH="$(dirname ${BASH_SOURCE[0]})"
export FIRE_TEST_MODULE_PATH="$(realpath ${GITHUB_WORKFLOWS_PATH}/../../test/module)"

source ${GITHUB_WORKFLOWS_PATH}/container-env.sh

if ! hash ldmx &> /dev/null; then
  echo "Not inside ldmx environmnet!"
  return 1
fi

# shared variables for the action
export BENCH_OUTPUT_DIR=${GITHUB_WORKFLOWS_PATH}/output
mkdir -p ${BENCH_OUTPUT_DIR} || { rc=$?; echo "Unable to create output dir."; return $rc; }
export BENCH_DATA_FILE=${BENCH_OUTPUT_DIR}/data.csv

__group__() {
  echo "::group::$@"
}

__endgroup__() {
  echo "::endgroup::"
}

# GitHub workflow command to set an output key,val pair
__set_output__() {
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
    ldmx time -p fire ${@:2} || return $?
  done |& awk '/real/ { real = real + $2; nr++ }
    END { if (nr>0) printf("%f\n", real/nr); }'
}

__runner__() {
  [ -f /.dockerenv ] && { echo "docker"; return 0; }
  [ -f /singularity ] && { echo "singularity"; return 0; }
  echo "bare"
  return 0
}

# Print the five inputs into the five columns of a CSV line
__print_csv_line__() {
  printf "%s,%s,%s,%s,%s,%s\n" $@
}

# input trials per n_events run and then squence of n_events to run
#  e.g. <script> <name> 100 1 10 100 1000 10000 100000
# we get the size of the output file by assuming it matches the form
#   output/*_<num-events>.*
__run_bench__() {
  local tag=$1; shift
  local trials=$1; shift
  [ -f ${BENCH_DATA_FILE} ] || __print_csv_line__ runner serializer mode events time size | tee ${BENCH_DATA_FILE}
  local runner=$(__runner__)
  local n_events
  for n_events in $@; do
    echo "  benchmarking ${n_events} Events"
    local t=$(__time__ ${trials} ${FIRE_TEST_MODULE_PATH}/produce.py ${n_events})
    [[ "$?" != "0" ]] && { echo "fire produce.py Errored Out!"; return 1; }
    local produce_output="${FIRE_TEST_MODULE_PATH}/output/${n_events}.h5"
    local s=$(stat -c "%s" ${produce_output})
    __print_csv_line__ ${runner} ${tag} produce ${n_events} ${t} ${s} | tee -a ${BENCH_DATA_FILE}
    t=$(__time__ ${trials} ${FIRE_TEST_MODULE_PATH}/recon.py ${produce_output})
    [[ "$?" != "0" ]] && { echo "fire recon.py Errored Out!"; return 1; }
    s=$(stat -c "%s" output/recon_output_${n_events}) 
    __print_csv_line__ ${runner} ${tag} recon ${n_events} ${t} ${s} | tee -a ${BENCH_DATA_FILE}
  done
}

# compile a branch
#   <branch>
__compile__() {
  local _branch=$1

  __group__ Switch to ${_branch}
  git checkout ${_branch} || return $?
  __endgroup__

  __group__ Configure Build
  ldmx cmake -B build/${_branch} -S . || return $?
  __endgroup__

  __group__ Build and Install
  ldmx cmake --build build/${_branch} --target install || return $?
  __endgroup__
}

# bench a specific branch
#   <branch> <trials> <event points>
__bench__() {
  __compile__ $1

  __group__ Run Benchmark
  __run_bench__ ${_script_inputs[@]} || return $?
  __endgroup__

  __group__ Delete Output Files
  rm -vr output || return $?
  __endgroup__
  
  __group__ Post Outputs
  __set_output__ dir ${BENCH_OUTPUT_DIR}
  __set_output__ file ${BENCH_DATA_FILE}
  __endgroup__

  return 0
}

__bench_help__() {
  cat << HELP

 USAGE:
  bench <branch> <trials> <n_events_1> [n_events_2 ...]

  We assume this command is run from the root directory of the repository.
  This is necessary so we can deduce where the test module is.

 ARGUMENTS:
  branch   : non-trunk branch to benchmark relative to trunk
  trials   : Number of trials to run for each N_EVENTS
  n_events : Number of events to benchmark for both ROOT and HDF5

HELP
}

bench() {
  if [ -z $1 ]; then
    __bench_help__
    return
  fi
  if [ "$#" -lt "3" ]; then
    echo "ERROR: Arguments '$@' do not meet the required pattern."
    echo "        <branch> <trials> <n1> [n2 ... ]"
  fi

  local _branch=$1

  __group__ Init Environment
  ldmx use dev hdf5 || return $?
  __endgroup__

  __bench__ ${_branch} ${@:2} || return $?
  __bench__ trunk      ${@:2} || return $?

  return 0
}

