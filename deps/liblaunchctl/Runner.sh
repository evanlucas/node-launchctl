#!/bin/sh

#  Runner.sh
#  liblaunchctl
#
#  Created by Evan Lucas on 6/21/13.
#  Copyright (c) 2013 Hattiesburg Clinic. All rights reserved.

echo "$1"ing liblaunchctl

make "$@"
#if [[ "$1" == "clean" ]]; then
#    xcodebuild clean
#elif [[ "$1" == "test" ]]; then
#    xcodebuild -scheme make test
#else
#    make
#fi