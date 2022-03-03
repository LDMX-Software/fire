
###############################################################################
# container-env.sh
#   This script is intended to define all the container aliases required
#   to test fire with ROOT enabled on GitHub actions.
#     1. Has docker engine installed
#     2. Can run docker as a non-root user
###############################################################################

export LDMX_BASE=${GITHUB_WORKSPACE}

# Run the container
ldmx() {
  docker run --rm -e LDMX_BASE \
    -v ${GITHUB_WORKSPACE}:${GITHUB_WORKSPACE} \
    -u $(id -u ${USER}):$(id -g ${USER}) \
    $LDMX_DOCKER_TAG $(pwd -P) "$@"
  return $?
}

