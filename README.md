# node-launchctl

Provides native bindings to launchctl commands

## Roadmap

We are currently working toward the [1.0 release](https://github.com/evanlucas/node-launchctl/tree/1.0-wip). There are a few considerations for that release of which I am not yet sure. They include:

- Possible removal of asynchronous functions due to trouble with getting correct results from launchd. This has not been an issue for the synchronous functions.
- On the fence for the plist portion of the module.  Its aim is to provide an easier way to create launchd plists, but is it out of the scope of the project?
- Do things like rusage, setenv, and unsetenv need asynchronous functions?

## Install

    npm install launchctl

## Test

    make test

## API

 [Documentation](http://evanlucas.github.io/node-launchctl)

## Docs

- Documentation is built using [doxx](https://github.com/FGRibreau/doxx)
- template.jade is the template used to generate the documentation
  - It a slightly modified version of the default template that comes with doxx
- To generate docs:

        make docs


## TODO

Make API more complete

- submit


## Contributions
- Please feel free to fork/contribute :]

