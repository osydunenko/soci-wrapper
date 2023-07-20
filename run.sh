#!/usr/bin/env bash

set +x

usage() {
cat <<-EOD
Usage: $(basename $0) [options]

Options:
    --clear                          Clear build dir
    --compile_container              Compile the docker container
    --compile_debug                  Compile in debug mode w/ tests
EOD
}

test "$#" -eq 0 && usage && exit 1

until [ -z "$1" ]; do
    case "$1" in
        --clear)
            CLEAR=true
            shift
            ;;
        --compile_container)
            COMPILE_CONTAINER=true
            shift
            ;;
        --compile_debug)
            COMPILE_DEBUG=true
            shift
            ;;
        *)
            usage
            exit 1
            ;;
    esac
done

SRC_DIR="$(realpath $(dirname $0))"
BUILD_DIR="${SRC_DIR}/build"
CONT_TAG="soci_wrapper"
mkdir -p "${BUILD_DIR}"

: ${CLEAR:=false}
: ${COMPILE_CONTAINER:=false}
: ${COMPILE_DEBUG:=false}

test $CLEAR = true && cd $SRC_DIR && rm -fr $BUILD_DIR && mkdir $BUILD_DIR
test $COMPILE_CONTAINER = true &&  docker build -f Dockerfile . --no-cache -t "${CONT_TAG}"
test $COMPILE_DEBUG = true && docker run --rm \
    -v "${SRC_DIR}:/src" \
    "${CONT_TAG}" \
    bash -c 'cmake -GNinja -DSW_SQLITE=ON -DSOCI_SHARED=ON -DCMAKE_BUILD_TYPE=Debug -DSW_BUILD_TESTS=ON ../ && ninja && ninja test'
