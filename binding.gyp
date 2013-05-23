{
  "targets": [
    {
      "target_name": "launchctl",
      "sources": ["src/launchctl.cc"],
      "conditions": [
        ['OS=="mac"', {
          "defines": [ '__MACOSX_CORE__' ],
          'ldflags': [ '-liblaunch', '-framework CoreFoundation', '-framework System' ],
          'include_dirs': [
            'deps/liblaunchctl-master/liblaunchctl',
            'deps/liblaunchctl-xcodeproj/liblaunchctl'
          ],
          'ldflags': [
            '-L<!(pwd)'
          ],
          'libraries': [
            '-L<!(pwd)',
            '-L<!(pwd)/deps/liblaunchctl-master/build',
            '-llaunchctl'
          ]
        }]
      ]
    }
  ]
}
