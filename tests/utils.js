var should = require('should')
  , utils = require('../lib/utils')

function getArg(a) {
  if ('string' === typeof a) return "'"+a+"'"
  if (Array.isArray(a)) return '['+a.join(', ')+']'
  if ('object' === typeof a) return JSON.stringify(a)
  return a
}

describe('utils', function() {
  describe('utils.getWeekday()', function() {
    var tests = [
      { val: 'Monday', res: 1, err: false },
      { val: 'monday', res: 1, err: false },
      { val: 'MONDAY', res: 1, err: false },
      { val: 1, res: 1, err: false },
      { val: 'Tuesday', res: 2, err: false },
      { val: 'tuesday', res: 2, err: false },
      { val: 'TUESDAY', res: 2, err: false },
      { val: 2, res: 2, err: false },
      { val: 'Wednesday', res: 3, err: false },
      { val: 'wednesday', res: 3, err: false },
      { val: 'WEDNESDAY', res: 3, err: false },
      { val: 3, res: 3, err: false },
      { val: 'Thursday', res: 4, err: false },
      { val: 'thursday', res: 4, err: false },
      { val: 'THURSDAY', res: 4, err: false },
      { val: 4, res: 4, err: false },
      { val: 'Friday', res: 5, err: false },
      { val: 'friday', res: 5, err: false },
      { val: 'FRIDAY', res: 5, err: false },
      { val: 5, res: 5, err: false },
      { val: 'Saturday', res: 6, err: false },
      { val: 'saturday', res: 6, err: false },
      { val: 'SATURDAY', res: 6, err: false },
      { val: 6, res: 6, err: false },
      { val: 'Sunday', res: 0, err: false },
      { val: 'sunday', res: 0, err: false },
      { val: 'SUNDAY', res: 0, err: false },
      { val: 0, res: 0, err: false },
      { val: 7, res: 7, err: false },
      { val: 'test', res: false, err: true },
      { val: false, res: false, err: true },
      { val: ['This', 'is', 'a', 'test'], res: false, err: true }
    ]
    tests.forEach(function(test, idx) {
      describe('utils.getWeekday('+getArg(test.val)+')', function() {
        if (test.err === true) {
          it('Should throw an error', function() {
            (function() {
              utils.getWeekday(test.val)
            }).should.throw('Invalid day of week.')
          })
        } else {
          it('Should not throw an error', function() {
            var dow = utils.getWeekday(test.val)
            dow.should.be.a('number').eql(test.res)            
          })
        }
      })
    })
  })
  
  describe('utils.getNumber()', function() {
    var tests = [
      { val: 0, res: 0, err: false },
      { val: '1', res: 1, err: false },
      { val: false, res: 0, err: false },
      { val: 'th', res: false, err: true },
      { val: ['fdf','asdf'], res: false, err: true }
    ]
    tests.forEach(function(test, idx) {
      describe('utils.getNumber('+getArg(test.val)+')', function() {
        if (test.err === true) {
          it('Should return false', function() {
            var res = utils.getNumber(test.val)
            res.should.be.a('boolean').eql(false)
          })
        } else {
          it('Should return the number value', function() {
            var res = utils.getNumber(test.val)
            res.should.be.a('number').eql(test.res)
          })
        }
      })
    })
  })
  
  describe('utils.emptyObject()', function() {
    var tests = [
      { val: {}, err: true },
      { val: { t: 'this' }, err: false },
      { val: '1', err: true },
      { val: 1, err: true },
      { val: ['ddd', 'eee'], err: true },
      { val: false, err: true }
    ]
    tests.forEach(function(test, idx) {
      describe('utils.emptyObject('+getArg(test.val)+')', function() {
        if (test.err === true) {
          it('Should return true', function() {
            var res = utils.emptyObject(test.val)
            res.should.eql(true)
          })
        } else {
          it('Should return false', function() {
            var res = utils.emptyObject(test.val)
            res.should.eql(false)
          })
        }
      })
    })
  })
})