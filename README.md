# node-launchctl

[![Build Status](https://travis-ci.org/evanlucas/node-launchctl.png?branch=master)](https://travis-ci.org/evanlucas/node-launchctl)

Provides native bindings to launchctl commands

[![NPM](https://nodei.co/npm/launchctl.png?downloads=true)](https://nodei.co/npm/launchctl/)

## Notice

Due to the fact that the source code of `launchd` for OS X 10.10 is no longer available, 
development on this project has been stopped. If someone else is interested in maintaining it,
please don't hesitate to let me know.

## It has been tested with the following versions:

- 0.8.26
- 0.10.x
- 0.11.13 (In preparation for 0.12.x and with the change in nan)

## Dependencies

- Tested on OS X 10.7.5 - OS X 10.9.x
- Requires Xcode 4.5+

## Install

```bash
$ npm install launchctl
```

## Usage

```js
var ctl = require('launchctl')
```

## Test

```bash
$ npm test
```

- To run tests for multiple versions

**WARNING: This will install n and will change your node version, so use with caution**

```bash
$ npm run test-versions
```

## API

 [Documentation](http://evanlucas.github.io/node-launchctl)

## TODO

- Make API more complete

## Contributions

- Please feel free to fork/contribute :]

