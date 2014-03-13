#!/bin/bash

N=$(which n)
if [[ $? != "0" ]]; then
  git clone https://github.com/evanlucas/n
  cd n
  make install
  cd ..
fi

VERSIONS=("stable" "latest")

for vers in ${VERSIONS[*]}; do
  n "$vers"
  rm -rf node_modules
  npm install --silent
  if [[ $? != "0" ]]; then
    echo "$vers failed to install dependencies"
    exit 1
  fi

  echo "$vers" "run tests"
  tap test/*.js
  if [[ $? != "0" ]]; then
    echo "$vers" "run tests failed"
    exit 1
  fi
done

for vers in ${VERSIONS[*]}; do
  echo "$vers" "test" "success"
done
