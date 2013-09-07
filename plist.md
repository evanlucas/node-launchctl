  - [Plist()](#plist)
  - [Plist.addBoolean()](#plistaddbooleankstringvboolean)
  - [Plist.addString()](#plistaddstringkstringvstringmandarray)
  - [Plist.addArray()](#plistaddarraykstringvarraystringnumberobject)
  - [Plist.addNumber()](#plistaddnumberkstringvnumber)
  - [Plist.addObject()](#plistaddobjectkstringvobject)
  - [Plist.setDisabled()](#plistsetdisableddisabledboolean)
  - [Plist.setEnableGlobbing()](#plistsetenableglobbingglobbingboolean)
  - [Plist.setEnableTransactions()](#plistsetenabletransactionstransactionsboolean)
  - [Plist.setOnDemand()](#plistsetondemandondemandboolean)
  - [Plist.setRunAtLoad()](#plistsetrunatloadrunatloadboolean)
  - [Plist.setInitGroups()](#plistsetinitgroupsgroupsboolean)
  - [Plist.setInetdCompatibilityWait()](#plistsetinetdcompatibilitywaitwaitboolean)
  - [Plist.setStartOnMount()](#plistsetstartonmountsboolean)
  - [Plist.setDebug()](#plistsetdebugdboolean)
  - [Plist.setWaitForDebugger()](#plistsetwaitfordebuggerdboolean)
  - [Plist.setAbandonProcessGroup()](#plistsetabandonprocessgroupdboolean)
  - [Plist.setLowPriorityIO()](#plistsetlowpriorityiodboolean)
  - [Plist.setLaunchOnlyOnce()](#plistsetlaunchonlyoncedboolean)
  - [Plist.setLabel()](#plistsetlabellabelstring)
  - [Plist.setProgram()](#plistsetprogramprogramstring)
  - [Plist.setUserName()](#plistsetusernameusernamestring)
  - [Plist.setGroupName()](#plistsetgroupnamegroupnamestring)
  - [Plist.setLimitLoadToSessionType()](#plistsetlimitloadtosessiontypesessiontypestring)
  - [Plist.setStdErrPath()](#plistsetstderrpathpstring)
  - [Plist.setStdOutPath()](#plistsetstdoutpathpstring)
  - [Plist.setStdInPath()](#plistsetstdinpathsstring)
  - [Plist.setRootDir()](#plistsetrootdirdirstring)
  - [Plist.setWorkingDir()](#plistsetworkingdirdirstring)
  - [Plist.setProcessType()](#plistsetprocesstypesstring)
  - [Plist.setUmask()](#plistsetumaskumasknumber)
  - [Plist.setTimeOut()](#plistsettimeouttonumber)
  - [Plist.setExitTimeOut()](#plistsetexittimeouttonumber)
  - [Plist.setThrottleInterval()](#plistsetthrottleintervalthrottlenumber)
  - [Plist.setStartInterval()](#plistsetstartintervalinumber)
  - [Plist.setNice()](#plistsetniceinumber)
  - [Plist.setProgramArgs()](#plistsetprogramargsargsarraystringnumberobject)
  - [Plist.setLimitLoadToHosts()](#plistsetlimitloadtohostshostsarraystringnumber)
  - [Plist.setLimitLoadFromHosts()](#plistsetlimitloadfromhostshostsarraystringnumber)
  - [Plist.setWatchPaths()](#plistsetwatchpathsparraystringnumberobject)
  - [Plist.setQueueDirectories()](#plistsetqueuedirectoriesdarraystringnumberobject)
  - [Plist.setEnvVar()](#plistsetenvvarobjobject)
  - [Plist.setSoftResourceLimits()](#plistsetsoftresourcelimits)
  - [Plist.setHardResourceLimits()](#plistsethardresourcelimits)
  - [Plist.setKeepAlive()](#plistsetkeepalivekeepalivebooleanstringnumberobject)
  - [Plist.addMachService()](#plistaddmachservice)
  - [Plist.addSocket()](#plistaddsocket)
  - [Plist.deleteStartCalendarInterval()](#plistdeletestartcalendarinterval)
  - [Plist.addCalendarInterval()](#plistaddcalendarintervaloobject)
  - [Plist.build()](#plistbuildobjobject)
  - [Plist.reset()](#plistreset)

## Plist()

  Constructor

## Plist.addBoolean(k:String, v:Boolean)

  Adds a boolean to the plist object for the given `k`

## Plist.addString(k:String, v:String, mand:Array)

  Adds a string to the plist object for the given `k`
  
  If `mand` is not an array, then no restrictions are placed

## Plist.addArray(k:String, v:Array|String|Number|Object)

  Adds an array to the plist object for the given `k`
  
  NOTE:
  
```js
 **Any value that is not an array will be wrapped in an array**
```

## Plist.addNumber(k:String, v:Number)

  Adds a number to the plist object for the given `k`

## Plist.addObject(k:String, v:Object)

  Adds an object to the plist object for the given `k`

## Plist.setDisabled(disabled:Boolean)

  Sets the `Disabled` bool

## Plist.setEnableGlobbing(globbing:Boolean)

  Sets the `EnableGlobbing` bool

## Plist.setEnableTransactions(transactions:Boolean)

  Sets the `EnableTransactions` bool

## Plist.setOnDemand(ondemand:Boolean)

  Sets the `OnDemand` bool

## Plist.setRunAtLoad(runatload:Boolean)

  Sets the `RunAtLoad` bool

## Plist.setInitGroups(groups:Boolean)

  Sets the `InitGroups` bool

## Plist.setInetdCompatibilityWait(wait:Boolean)

  Sets the `inetdCompatibility.Wait` bool

## Plist.setStartOnMount(s:Boolean)

  Sets the `StartOnMount` bool

## Plist.setDebug(d:Boolean)

  Sets the `Debug` bool

## Plist.setWaitForDebugger(d:Boolean)

  Sets the `WaitForDebugger` bool

## Plist.setAbandonProcessGroup(d:Boolean)

  Sets the `AbandonProcessGroup` bool

## Plist.setLowPriorityIO(d:Boolean)

  Sets the `LowPriorityIO` bool

## Plist.setLaunchOnlyOnce(d:Boolean)

  Sets the `LaunchOnlyOnce` bool

## Plist.setLabel(label:String)

  Sets the `Label` field

## Plist.setProgram(program:String)

  Sets the `Program` string

## Plist.setUserName(username:String)

  Sets the `UserName` string

## Plist.setGroupName(groupname:String)

  Sets the `GroupName` string

## Plist.setLimitLoadToSessionType(sessiontype:String)

  Sets the `LimitLoadToSessionType` string

## Plist.setStdErrPath(p:String)

  Sets the `StandardErrorPath` string

## Plist.setStdOutPath(p:String)

  Sets the `StandardOutPath` string

## Plist.setStdInPath(s:String)

  Sets the `StandardInPath` string

## Plist.setRootDir(dir:String)

  Sets the `RootDirectory` string

## Plist.setWorkingDir(dir:String)

  Sets the `WorkingDirectory` string

## Plist.setProcessType(s:String)

  Sets the `ProcessType` string

## Plist.setUmask(umask:Number)

  Sets the `Umask` number

## Plist.setTimeOut(to:Number)

  Sets the `TimeOut` number

## Plist.setExitTimeOut(to:Number)

  Sets the `ExitTimeOut` number

## Plist.setThrottleInterval(throttle:Number)

  Sets the `ThrottleInterval` number

## Plist.setStartInterval(i:Number)

  Sets the `StartInterval` number

## Plist.setNice(i:Number)

  Sets the `Nice` number

## Plist.setProgramArgs(args:Array|String|Number|Object)

  Sets the `ProgramArguments` array

## Plist.setLimitLoadToHosts(hosts:Array|String|Number)

  Sets the `LimitLoadToHosts` array

## Plist.setLimitLoadFromHosts(hosts:Array|String|Number)

  Sets the `LimitLoadFromHosts` array

## Plist.setWatchPaths(p:Array|String|Number|Object)

  Sets the `WatchPaths` array

## Plist.setQueueDirectories(d:Array|String|Number|Object)

  Sets the `QueueDirectories` array

## Plist.setEnvVar(obj:Object)

  Sets the `EnvironmentVariables` object

## Plist.setSoftResourceLimits()

  Sets the `SoftResourceLimits` object
  
  **WARNING: Not yet implemented**

## Plist.setHardResourceLimits()

  Sets the `HardResourceLimits` object
  
  **WARNING: Not yet implemented**

## Plist.setKeepAlive(keepalive:Boolean|String|Number|Object)

  Sets the `KeepAlive` value

## Plist.addMachService()

  Adds an object to the `MachServices` key
  
  **WARNING: Not yet implemented**

## Plist.addSocket()

  Adds an object to the `Sockets` key
  
  **WARNING: Not yet implemented**

## Plist.deleteStartCalendarInterval()

  Removes the `StartCalendarInterval` object/array

## Plist.addCalendarInterval(o:Object)

  Adds a Dictionary for the `StartCalendarInterval`

## Plist.build(obj:Object)

  Builds the actual plist object into a string

## Plist.reset()

  Wipes the slate clean
