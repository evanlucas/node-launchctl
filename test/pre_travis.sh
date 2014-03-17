#!/usr/bin/env bash

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
ELLOG_HEADING="launchctl"
source "$DIR"/ellog.bash

ellog_info "install" "stable node"
brew update
brew install node

N=$(which n)
if [[ $? != "0" ]]; then
  ellog_info "n" "not installed"
  ellog_info "n" "installing..."
  git clone https://github.com/visionmedia/n
  sudo make install
else
  ellog_info "n" "already installed"
fi
