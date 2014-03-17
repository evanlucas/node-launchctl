#!/usr/bin/env bash

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
ELLOG_HEADING="launchctl"
source "$DIR"/ellog.bash

ellog_info "install" "stable node"
brew update
brew install node

ellog_info "install" "n" "global"
npm install -g n
ellog_info "install" "tap" "global"
npm install -g tap
