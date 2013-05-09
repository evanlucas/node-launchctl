var LaunchCTLError = function() {
  this.code = '';
  this.msg = '';
  this.errno = 0;
}

var createError = exports.createError = function() {
  return new LaunchCTLError();
}

LaunchCTLError.prototype.getError = function(errno) {
  var self = this;
  switch (errno) {
    case -1:
      self.code = 'EUNKNO';
      self.msg = 'Launchctl returned unexpected response';
      self.errno = -1;
      break;
    case 1:
      self.code = 'EPERM';
      self.msg = 'Operation not permitted';
      self.errno = 1;
      break;
    case 2:
      self.code = 'ENOENT';
      self.msg = 'No such file or directory';
      self.errno = 2;
      break;
    case 3:
      self.code = 'ESRCH';
      self.msg = 'No such process';
      self.errno = 3;
  }
  
  if (self.errno == 0) {
    return false;
  } 
  var e = new Error();
  e.code = self.code;
  e.msg = self.msg;
  e.errno = self.errno;
  return e;
}

exports.LaunchCTLError = LaunchCTLError;