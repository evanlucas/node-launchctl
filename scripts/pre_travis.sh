#!/usr/bin/env bash

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
ELLOG_HEADING="launchctl"
source "$DIR"/ellog.bash

ellog_info "install" "stable node"
mkdir -p node
cd node
curl -L# http://nodejs.org/dist/v0.10.26/node-v0.10.26-darwin-x64.tar.gz | tar -zx --strip 1
cp -fR * /usr/local/
cd ..
hash -r

ellog_info "install" "n" "global"
npm install -g n
ellog_info "install" "tap" "global"
npm install -g tap
