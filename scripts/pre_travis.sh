#!/usr/bin/env bash

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
ELLOG_HEADING="launchctl"
source "$DIR"/ellog.bash

ellog_info "install" "stable node"
wget http://nodejs.org/dist/v0.10.26/node-v0.10.26.pkg -O node.pkg
sudo installer -pkg node.pkg -target /
hash -r

ellog_info "install" "n" "global"
npm install -g n
ellog_info "install" "tap" "global"
npm install -g tap
