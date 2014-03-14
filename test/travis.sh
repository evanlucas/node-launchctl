#!/usr/bin/env bash
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
ELLOG_HEADING="launchctl"
source "$DIR"/ellog.bash

N=$(which n)
if [[ $? != "0" ]]; then
  ellog_info "n" "not installed"
  ellog_info "n" "installing..."
  git clone https://github.com/visionmedia/n
  cd n
  make install
  cd ..
fi

VERSIONS=("0.8.26" "0.10.20" "0.10.21" "0.10.22" "0.10.23" "0.10.24" "0.10.25" "0.10.26" "0.11.6" "0.11.7" "0.11.8" "0.11.9" "0.11.10")

# 0.11.11 botched addons
# 0.11.12 messed up MakeCallback so async addon functions fail

for vers in ${VERSIONS[*]}; do
  n "$vers"
  ellog_info "$vers" "installing dependencies"
  npm install --silent >/dev/null
  if [[ $? != "0" ]]; then
    ellog_error "$vers" "failed to install dependencies"
    exit 1
  fi
  ellog_info "$vers" "dependencies installed"

  ellog_info "$vers" "run tests"
  tap test/*.js
  if [[ $? != "0" ]]; then
    ellog_error "$vers" "run tests failed"
    exit 1
  fi
  ellog_info "$vers" "tests run successfully"
done

for vers in ${VERSIONS[*]}; do
  ellog_info "$vers" "test" "success"
done

