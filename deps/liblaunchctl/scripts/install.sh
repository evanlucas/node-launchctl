#!/bin/bash
#
# Installs dependencies

RED_FG=$(tput setaf 9)
GREEN_FG=$(tput setaf 10)
BOLD=$(tput bold)
RESET=$(tput sgr0)

print_error() {
  echo -ne "${RED_FG}${BOLD}error: ${RESET} "
  echo -n "$@"
  echo ""
}

print_info() {
  echo -ne "${GREEN_FG}${BOLD}info: ${RESET}  "
  echo -n "$@"
  echo ""
}

NPM=$(which npm 2>/dev/null)
print_info "Checking for npm..."
if [[ ! "$NPM" ]]; then
  print_error "npm is not installed..."
  print_error "please install npm before continuing"
  exit 1
else
  print_info "npm is already installed :]"
fi

GYP=$(which node-gyp 2>/dev/null)
if [[ ! "$GYP" ]]; then
  print_info "node-gyp is not installed...installing..."
  "$NPM" install -g node-gyp
else
  print_info "node-gyp is already installed"
fi
