var ctl = require('./lib');

ctl.start('hombrew.mxcl.redis', function(err, data) {
	if (err) {
		console.log(err);
	} else {
		console.log(data);
	}
});
