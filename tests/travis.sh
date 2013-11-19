#!/bin/bash

NODE=$(which node)

if [[ "$?" != "0" ]]; then
  echo "Fetching node.js"
  wget http://nodejs.org/dist/v0.10.21/node-v0.10.21.pkg -O node.pkg
  sudo installer -pkg node.pkg -target /
  hash -r
fi

NODE=$(which node)