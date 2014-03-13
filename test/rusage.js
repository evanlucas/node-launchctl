var test = require('tap').test
  , ctl = require('../lib')

test('getRUsage', function(t) {
  var keys = ['user_time_used', 'system_time_used', 'max_resident_set_size',
    'shared_text_memory_size', 'unshared_data_size', 'unshared_stack_size',
    'page_reclaims', 'page_faults', 'swaps', 'block_input_operations',
    'block_output_operations', 'messages_sent', 'messages_received',
    'signals_received', 'voluntary_context_switches',
    'involuntary_context_switches']
  var usage = ctl.getRUsage('self')
  t.type(usage, 'object', 'should be an object')
  keys.forEach(function(key) {
    t.notEqual(usage[key], undefined, 'should have key '+key)
  })
  t.end()
})
