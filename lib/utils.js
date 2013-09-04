/**
 * Module dependencies
 */
var utils = exports

/**
 * allows map day of week as a string to a number
 */
utils.dows = {
    'sunday': 0
  , 'monday': 1
  , 'tuesday': 2
  , 'wednesday': 3
  , 'thursday': 4
  , 'friday': 5
  , 'saturday': 6
};

/**
 * Maps either a string containing a day of the week
 * or a number between 0 and 7 to the correct day of the week
 *
 * @param {String|Number} d The day of the week as a string or number
 * @api public
 * @returns Number
 */
utils.getWeekday = function(d) {
  var day
  if ('string' === typeof d) {
    d = d.toLowerCase()
    if (utils.dows.hasOwnProperty(d)) {
      day = utils.dows[d]
    } else {
      throw new Error('Invalid day of week.')
    }
  } else if ('number' === typeof d) {
    day = d
  } else {
    throw new Error('Invalid day of week.')
  }
  return day
}

/**
 * Gets the number value of the given `n` or false
 *
 * @param {*} n The input number
 * @api public
 * @returns Number or false
 */
utils.getNumber = function(n) {
  if ('number' !== typeof n) {
    n = Number(n)
  }
  if (isNaN(n)) return false
  return n
}

/**
 * Determines whether the input is an empty object
 *
 * @param {*} o The input object
 * @api public
 * @returns Boolean
 */
utils.emptyObject = function(o) {
  if (Array.isArray(o)) return true
  if ('object' === typeof o) {
    var keys = Object.keys(o)
    return !(keys && keys.length > 0)
  }
  return true
}