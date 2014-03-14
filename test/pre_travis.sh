#!/usr/bin/env bash

N=$(which n)
if [[ $? != "0" ]]; then
  ellog_info "n" "not installed"
  ellog_info "n" "installing..."
  git clone https://github.com/visionmedia/n
  cd n
  make install
  cd ..
fi
