#!/bin/bash

NODE=$(which node)

if [[ "$?" != "0" ]]; then
  echo "Fetching node.js"
  wget http://nodejs.org/dist/v0.10.22/node-v0.10.22.pkg -O node.pkg
  sudo installer -pkg node.pkg -target /
  hash -r
fi

NODE=$(which node)

VERS=$($NODE --version)

echo "Using node $VERS"

GRUNT=$(which grunt)

if [[ "$?" != "0" ]]; then
  sudo npm install -g grunt-cli
fi

