{
  "targets": [
    {
      "target_name": "bindings",
      "sources": ['src/bindings.cc', "src/launchctl.cc"],
      "conditions": [
        ['OS=="mac"', {
          "defines": [ '__MACOSX_CORE__' ],
          "dependencies": [
            'deps/liblaunchctl/binding.gyp:launchctl'
          ],
          "include_dirs": [
            'deps/liblaunchctl/liblaunchctl',
            "<!(node -e \"require('nan')\")"
          ],
          'ldflags': [
            "-framework CoreFoundation",
            "-L<!(pwd)/build/Release"
          ],
          'libraries': [
            '-llaunchctl'
          ],
          'xcode_settings': {
            'OTHER_LDFLAGS': [
              '-framework CoreFoundation'
            ]
          }
        }]
      ]
    }
  ]
}
