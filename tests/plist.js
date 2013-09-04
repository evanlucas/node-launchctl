var should = require('should')
  , Plist = require('../lib/index').Plist

function getArg(a) {
  if ('string' === typeof a) return "'"+a+"'"
  if (Array.isArray(a)) return '['+a.join(', ')+']'
  if ('object' === typeof a) return JSON.stringify(a)
  return a
}
describe('Plist', function() {
  describe('type specifics', function(){
    describe('Booleans', function() {
      var plist = new Plist()
      var tests = [
        { k: '1', v: false, err: false },
        { k: '2', v: 'YES', err: true },
        { k: '3', v: 2, err: true },
        { k: '4', v: true, err: false },
        { k: '5', v: ['a', 'b'], err: true },
        { k: '6', v: {a: '1', b: '2'}, err: true }
      ]
      plist.reset()
      tests.forEach(function(test, idx) {
        describe(test.k+':addBoolean('+getArg(test.v)+')', function() {
          if (test.err === true) {
            it('Should throw an error', function() {
              (function() {
                plist.addBoolean(test.k, test.v)
              }).should.throw(test.k+' must be a boolean')
            })
          } else {
            it('Should not throw an error', function() {
              plist.addBoolean(test.k, test.v)
              plist.obj.should.have.property(test.k, test.v)
            })
          }
        })
      })
    })
    
    describe('Strings', function() {
      var plist = new Plist()
        , mand = ['Aqua', 'LoginWindow', 'Background', 'StandardIO', 'System']
      var tests = [
        { k: '1', v: 'YES', err: false, mand: false },
        { k: '2', v: 1, err: false, mand: false },
        { k: '3', v: false, err: true, mand: false },
        { k: '4', v: 'Aqua', err: false, mand: mand },
        { k: '5', v: 'Test', err: true, mand: mand }
      ]
      plist.reset()
      tests.forEach(function(test, idx) {
        describe(test.k+':addString('+getArg(test.v)+')', function() {
          if (test.err === true) {
            it('Should throw an error', function() {
              (function() {
                plist.addString(test.k, test.v, test.mand)
              }).should.throw(/must be/)
            })
          } else {
            it('Should not throw error', function() {
              plist.addString(test.k, test.v, test.mand)
              plist.obj.should.have.property(test.k, String(test.v))
            })
          }
        })
      })
    })
    
    describe('Arrays', function() {
      var plist = new Plist()
      var tests = [
        { k: '1', v: ['This', 'is', 'a', 'test'], err: false },
        { k: '2', v: 'This is a test', err: false },
        { k: '3', v: { test: 'This is a test' }, err: false },
        { k: '4', v: 4, err: false },
        { k: '5', v: false, err: false },
        { k: '6', v: '', err: false },
        { k: '7', v: null, err: false }
      ]
      plist.reset()
      tests.forEach(function(test, idx) {
        describe(test.k+':addArray('+getArg(test.v)+')', function() {
          if (test.err === true) {
            it('Should throw an error', function() {
              (function() {
                plist.addArray(test.k, test.v)
              }).should.throw()
            })
          } else {
            it('Should not throw an error', function() {
              plist.addArray(test.k, test.v)
              plist.obj[test.k].should.be.instanceOf(Array)
            })
          }
        })
      })
    })
    
    describe('Numbers', function() {
      var plist = new Plist()
      var tests = [
        { k: '1', v: 1, err: false },
        { k: '2', v: '1', err: false },
        { k: '3', v: '1.1', err: false },
        { k: '4', v: 1.12344, err: false },
        { k: '5', v: -13.24234523, err: false },
        { k: '6', v: 'This is a test', err: true },
        { k: '7', v: ['This', 'is', 'a', 'test'], err: true },
        { k: '8', v: {a: 'This', b: 'Is', c: 'test'}, err: true },
        { k: '9', v: false, err: false }
      ]
      plist.reset()
      tests.forEach(function(test, idx) {
        describe(test.k+':addNumber('+getArg(test.v)+')', function() {
          if (test.err === true) {
            it('Should throw an error', function() {
              (function() {
                plist.addNumber(test.k, test.v)
              }).should.throw(test.k+' must be a valid number')
            })
          } else {
            it('Should not throw an error', function() {
              plist.addNumber(test.k, test.v)
              plist.obj.should.have.property(test.k, Number(test.v))
            })
          }
        })
      })
    })
    
    describe('Objects', function() {
      var plist = new Plist()
      var tests = [
        { k: '1', v: ['This', 'is', 'a', 'test'], err: true },
        { k: '2', v: 'This is a test', err: true },
        { k: '3', v: { test: 'This is a test' }, err: false },
        { k: '4', v: 14, err: true },
        { k: '5', v: false, err: true }
      ]
      
      plist.reset()
      tests.forEach(function(test, idx) {
        describe(test.k+':addObject('+getArg(test.v)+')', function() {
          if (test.err === true) {
            it('Should throw an error', function() {
              (function() {
                plist.addObject(test.k, test.v)
              }).should.throw(test.k+' must be an object')
            })
          } else {
            it('Should not throw an error', function() {
              plist.addObject(test.k, test.v)
              plist.obj.should.have.property(test.k)
              plist.obj[test.k].should.be.a('object')
            })
          }
        })
      })
    })
  })
  
  describe('launchctl specifics', function() {
    var plist = new Plist()
    
    var theBools = [
      { title: 'setEnableTransactions()', func: 'setEnableTransactions', name: 'EnableTransactions'},
      { title: 'setDisabled()', func: 'setDisabled', name: 'Disabled'},
      { title: 'setEnableGlobbing()', func: 'setEnableGlobbing', name: 'EnableGlobbing' },
      { title: 'setOnDemand()', func: 'setOnDemand', name: 'OnDemand' },
      { title: 'setRunAtLoad()', func: 'setRunAtLoad', name: 'RunAtLoad' },
      { title: 'setInitGroups()', func: 'setInitGroups', name: 'InitGroups' },
      { title: 'setStartOnMount()', func: 'setStartOnMount', name: 'StartOnMount' },
      { title: 'setDebug()', func: 'setDebug', name: 'Debug' },
      { title: 'setWaitForDebugger()', func: 'setWaitForDebugger', name: 'WaitForDebugger' },
      { title: 'setAbandomProcessGroup()', func: 'setAbandonProcessGroup', name: 'AbandonProcessGroup' },
      { title: 'setLowPriorityIO()', func: 'setLowPriorityIO', name: 'LowPriorityIO' },
      { title: 'setLaunchOnlyOnce()', func: 'setLaunchOnlyOnce', name: 'LaunchOnlyOnce' }
    ]
    
    theBools.forEach(function(b, idx) {
      var vals = [
        { val: false, err: false },
        { val: true, err: false },
        { val: 'true', err: true },
        { val: 'false', err: true },
        { val: 1, err: true }
      ]
      describe(b.title, function() {
        vals.forEach(function(val) {
          describe(b.func+'('+getArg(val.val)+')', function() {
            if (val.err === true) {
              it('Should throw an error', function() {
                (function() {
                  plist[b.func].call(plist, val.val)
                }).should.throw(b.name+' must be a boolean')
              })
            } else {
              it('Should not throw an error', function() {
                plist[b.func].call(plist, val.val)
                plist.obj.should.have.property(b.name, Boolean(val.val))
              })
            }
          })
        })
      })
    })
    
    var mand = ['Aqua', 'LoginWindow', 'Background', 'StandardIO', 'System']
    
    
    var theStrings = [
      { title: 'setStdErrPath()', func: 'setStdErrPath', name: 'StandardErrorPath', mand: false },
      { title: 'setStdOutPath()', func: 'setStdOutPath', name: 'StandardOutPath', mand: false },
      { title: 'setStdInPath()', func: 'setStdInPath', name: 'StandardInPath', mand: false },
      { title: 'setRootDir()', func: 'setRootDir', name: 'RootDirectory', mand: false },
      { title: 'setWorkingDir()', func: 'setWorkingDir', name: 'WorkingDirectory', mand: false },
      { title: 'setGroupName()', func: 'setGroupName', name: 'GroupName', mand: false },
      { title: 'setUserName()', func: 'setUserName', name: 'UserName', mand: false },
      { title: 'setProgram()', func: 'setProgram', name: 'Program', mand: false },
      { title: 'setLabel()', func: 'setLabel', name: 'Label', mand: false }
    ]
    
    theStrings.forEach(function(s, idx) {
      var vals = [
        { val: false, err: true },
        { val: 'This is a test', err: false },
        { val: 1, err: false },
        { val: ['This', 'is', 'a', 'test'], err: true },
        { val: {test: 'This is a test'}, err: true }
      ]
      describe(s.title, function() {
        vals.forEach(function(val) {
          describe(s.func+'('+getArg(val.val)+')', function() {
            if (val.err === true) {
              it('Should throw an error', function() {
                (function() {
                  plist[s.func].call(plist, val.val)
                }).should.throw(s.name+' must be a string')
              })
            } else {
              it('Should not throw an error', function() {
                plist[s.func].call(plist, val.val)
                plist.obj.should.have.property(s.name, String(val.val))
              })
            }
          })
        })
      })
    })
    
    var theNums = [
      { title: 'setUmask()', func: 'setUmask', name: 'Umask' },
      { title: 'setTimeOut()', func: 'setTimeOut', name: 'TimeOut' },
      { title: 'setExitTimeOut()', func: 'setExitTimeOut', name: 'ExitTimeOut' },
      { title: 'setThrottleInterval()', func: 'setThrottleInterval', name: 'ThrottleInterval' },
      { title: 'setStartInterval()', func: 'setStartInterval', name: 'StartInterval' },
      { title: 'setNice()', func: 'setNice', name: 'Nice' }
    ]
    
    
    theNums.forEach(function(s, idx) {
      var vals = [
        { val: false, err: false },
        { val: 'This is a test', err: true },
        { val: '1', err: false },
        { val: 1, err: false },
        { val: ['This', 'is', 'a', 'test'], err: true },
        { val: {test: 'This is a test'}, err: true }
      ]
      describe(s.title, function() {
        vals.forEach(function(val) {
          describe(s.func+'('+getArg(val.val)+')', function() {
            if (val.err === true) {
              it('Should throw an error', function() {
                (function() {
                  plist[s.func].call(plist, val.val)
                }).should.throw(s.name+' must be a valid number')
              })
            } else {
              it('Should not throw an error', function() {
                plist[s.func].call(plist, val.val)
                plist.obj.should.have.property(s.name, Number(val.val)).and.be.a('number')
              })
            }
          })
        })
      })
    })
    
    
    
    describe('setEnvVar()', function() {
      var vals = [
        { val: false, err: true },
        { val: 'This is a test', err: true },
        { val: '1', err: true },
        { val: 1, err: true },
        { val: ['This', 'is', 'a', 'test'], err: true },
        { val: {test: 'This is a test'}, err: false }
      ]
      vals.forEach(function(val) {
        describe('setEnvVar('+getArg(val.val)+')', function() {
          if (val.err === true) {
            it('Should throw an error', function() {
              (function() {
                plist.setEnvVar(val.val)
              }).should.throw('EnvironmentVariables must be an object')
            })
          } else {
            plist.setEnvVar(val.val)
            plist.obj.should.have.property('EnvironmentVariables')
            Object.keys(val.val).forEach(function(k) {
              plist.obj.EnvironmentVariables.should.have.property(k, val.val[k])
            })
          }
        })
      })
    })
    
    
    describe('setProgramArgs()', function() {
      var vals = [
        { val: false, err: false },
        { val: 'This is a test', err: false },
        { val: '1', err: false },
        { val: 1, err: false },
        { val: ['This', 'is', 'a', 'test'], err: false },
        { val: {test: 'This is a test'}, err: false }
      ]
      vals.forEach(function(val) {
        describe('setProgramArgs('+getArg(val.val)+')', function() {
          if (val.err === true) {
            it('Should throw an error', function() {
              (function() {
                plist.setProgramArgs(val.val)
              }).should.throw()
            })
          } else {
            it('Should not throw an error', function() {
              plist.setProgramArgs(val.val)
              plist.obj.should.have.property('ProgramArguments')
            })
          }
        })
      })
    })
    
    describe('setInetdCompatibilityWait()', function() {
      var vals = [
        { val: false, err: false },
        { val: 'This is a test', err: true },
        { val: '1', err: true },
        { val: 1, err: true },
        { val: ['This', 'is', 'a', 'test'], err: true },
        { val: {test: 'This is a test'}, err: true }
      ]
      vals.forEach(function(val) {
        describe('setInetdCompatibilityWait('+getArg(val.val)+')', function() {
          if (val.err === true) {
            it('Should throw an error', function() {
              (function() {
                plist.setInetdCompatibilityWait(val.val)
              }).should.throw('The wait flag must be a boolean')
            })
          } else {
            it('Should not throw an error', function() {
              plist.setInetdCompatibilityWait(val.val)
              plist.obj.should.have.property('inetdCompatibility')
              plist.obj.inetdCompatibility.should.have.property('Wait', val.val)
            })
          }
        })
      })
    })
    
    describe('setLimitLoadToHosts()', function() {
      var vals = [
        { val: ['localhost', 'www.localhost.localdomain'], err: false },
        { val: false, err: false },
        { val: 'This is a test', err: false },
        { val: '1', err: false },
        { val: 1, err: false },
        { val: {test: 'This is a test'}, err: false }
      ]
      vals.forEach(function(val) {
        describe('setLimitLoadToHosts('+getArg(val.val)+')', function() {
          if (val.err === true) {
            it('Should throw an error', function() {
              (function() {
                plist.setLimitLoadToHosts(val.val)
              }).should.throw()
            })
          } else {
            it('Should not throw an error', function() {
              plist.setLimitLoadToHosts(val.val)
              plist.obj.should.have.property('LimitLoadToHosts')
              if (Array.isArray(val.val)) {
                plist.obj.should.have.property('LimitLoadToHosts').eql(val.val)
              } else {
                var a = (!val.val) ? [] : [val.val]
                plist.obj.should.have.property('LimitLoadToHosts').eql(a)
              }
            })
          }
        })
      })
    })
    describe('setLimitLoadFromHosts()', function() {
      var vals = [
        { val: ['localhost', 'www.localhost.localdomain'], err: false },
        { val: false, err: false },
        { val: 'This is a test', err: false },
        { val: '1', err: false },
        { val: 1, err: false },
        { val: {test: 'This is a test'}, err: false }
      ]
      vals.forEach(function(val) {
        describe('setLimitLoadFromHosts('+getArg(val.val)+')', function() {
          if (val.err === true) {
            it('Should throw an error', function() {
              (function() {
                plist.setLimitLoadFromHosts(val.val)
              }).should.throw()
            })
          } else {
            it('Should not throw an error', function() {
              plist.setLimitLoadFromHosts(val.val)
              plist.obj.should.have.property('LimitLoadFromHosts')
              if (Array.isArray(val.val)) {
                plist.obj.should.have.property('LimitLoadFromHosts').eql(val.val)
              } else {
                var a = (!val.val) ? [] : [val.val]
                plist.obj.should.have.property('LimitLoadFromHosts').eql(a)
              }
            })
          }
        })
      })
    })
    
    describe('setLimitLoadToSessionType()', function() {
      var vals = [
        { val: 'Aqua', err: false },
        { val: 'LoginWindow', err: false },
        { val: 'Background', err: false },
        { val: 'StandardIO', err: false },
        { val: 'System', err: false },
        { val: 'Test', err: true },
        { val: 1, err: true },
        { val: false, err: true },
        { val: ['This', 'is', 'a', 'test'], err: true },
        { val: {test: 'This is a test'}, err: true}
      ]
      
      vals.forEach(function(val) {
        describe('setLimitLoadToSessionType('+getArg(val.val)+')', function() {
          if (val.err === true) {
            it('Should throw an error', function() {
              (function() {
                plist.setLimitLoadToSessionType(val.val)
              }).should.throw()
            })
          } else {
            it('Should not throw an error', function() {
              plist.setLimitLoadToSessionType(val.val)
              plist.obj.should.have.property('LimitLoadToSessionType', val.val)
            })
          }
        })
      })
    })
    
    describe('setKeepAlive()', function() {
      var vals = [
        { val: 1, err: false },
        { val: '1', err: false },
        { val: {test: 'This is a test'}, err: false },
        { val: ['This', 'is', 'a', 'test'], err: false },
        { val: false, err: false}
      ]
      vals.forEach(function(val) {
        describe('setKeepAlive('+getArg(val.val)+')', function() {
          if (val.err === true) {
            it('Should throw an error', function() {
              (function() {
                plist.setKeepAlive(val.val)
              }).should.throw()
            })
          } else {
            it('Should not throw an error', function() {
              plist.setKeepAlive(val.val)
              plist.obj.should.have.property('KeepAlive').eql(val.val)
            })
          }
        })
      })
    })
    
    describe('setWatchPaths()', function() {
      var vals = [
        { val: ['localhost', 'www.localhost.localdomain'], err: false },
        { val: false, err: false },
        { val: 'This is a test', err: false },
        { val: '1', err: false },
        { val: 1, err: false },
        { val: {test: 'This is a test'}, err: false }
      ]
      vals.forEach(function(val) {
        describe('setWatchPaths('+getArg(val.val)+')', function() {
          if (val.err === true) {
            it('Should throw an error', function() {
              (function() {
                plist.setWatchPaths(val.val)
              }).should.throw()
            })
          } else {
            it('Should not throw an error', function() {
              plist.setWatchPaths(val.val)
              plist.obj.should.have.property('WatchPaths')
              if (Array.isArray(val.val)) {
                plist.obj.should.have.property('WatchPaths').eql(val.val)
              } else {
                var a = (!val.val) ? [] : [val.val]
                plist.obj.should.have.property('WatchPaths').eql(a)
              }
            })
          }
        })
      })
    })
    
    describe('setQueueDirectories()', function() {
      var vals = [
        { val: ['localhost', 'www.localhost.localdomain'], err: false },
        { val: false, err: false },
        { val: 'This is a test', err: false },
        { val: '1', err: false },
        { val: 1, err: false },
        { val: {test: 'This is a test'}, err: false }
      ]
      vals.forEach(function(val) {
        describe('setQueueDirectories('+getArg(val.val)+')', function() {
          if (val.err === true) {
            it('Should throw an error', function() {
              (function() {
                plist.setQueueDirectories(val.val)
              }).should.throw()
            })
          } else {
            it('Should not throw an error', function() {
              plist.setQueueDirectories(val.val)
              plist.obj.should.have.property('QueueDirectories')
              if (Array.isArray(val.val)) {
                plist.obj.should.have.property('QueueDirectories').eql(val.val)
              } else {
                var a = (!val.val) ? [] : [val.val]
                plist.obj.should.have.property('QueueDirectories').eql(a)
              }
            })
          }
        })
      })
    })
    
    describe('setProcessType()', function() {
      var mand = ['Background', 'Standard', 'Adaptive', 'Interactive']
      var vals = [
        { val: 'Background', err: false },
        { val: 'Standard', err: false },
        { val: 'Adaptive', err: false },
        { val: 'Interactive', err: false },
        { val: 'Aqua', err: true },
        { val: 'LoginWindow', err: true },
        { val: 'StandardIO', err: true },
        { val: 'System', err: true },
        { val: 'Test', err: true },
        { val: 1, err: true },
        { val: false, err: true },
        { val: ['This', 'is', 'a', 'test'], err: true },
        { val: {test: 'This is a test'}, err: true}
      ]
      vals.forEach(function(val) {
        describe('setProcessType('+getArg(val.val)+')', function() {
          if (val.err === true) {
            it('Should throw an error', function() {
              (function() {
                plist.setProcessType(val.val)
              }).should.throw()
            })
          } else {
            it('Should not throw an error', function() {
              plist.setProcessType(val.val)
              plist.obj.should.have.property('ProcessType', val.val)
            })
          }
        })
      })
    })
  })
})