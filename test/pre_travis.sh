#!/usr/bin/env bash

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
ELLOG_HEADING="launchctl"
source "$DIR"/ellog.bash

N=$(which n)
if [[ $? != "0" ]]; then
  ellog_info "n" "not installed"
  ellog_info "n" "installing..."
  git clone https://github.com/visionmedia/n
  cp n/bin/n .
else
  ellog_info "n" "already installed"
fi
