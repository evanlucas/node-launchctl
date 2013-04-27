var ctl = require('./launchctl.node');

var s = ctl.getAllJobs();
console.log(s);

var d = ctl.getJob("com.apple.mdworker.mail");
console.log(d);
