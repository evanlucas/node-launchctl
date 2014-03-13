#!/usr/bin/env bash

ELLOG_HEADING="launchctl"
source ~/bin/bash/ellog.bash

VERSIONS=("0.10.26" "0.11.9")

for vers in ${VERSIONS[*]}; do
  ellog_info "using" "$vers"
  n "$vers"
  ellog_info "install" "rebuilding"
  rm -rf node_modules
  npm install --silent
  if [[ $? != "0" ]]; then
    ellog_error "$vers" "failed to install dependencies"
    exit 1
  fi
  ellog_info "tests" "run"
  grunt
  if [[ $? != "0" ]]; then
    ellog_error "$vers" "tests failed"
    exit 1
  fi
done

for vers in ${VERSIONS[*]}; do
  ellog_info "test" "success" "$vers"
done
