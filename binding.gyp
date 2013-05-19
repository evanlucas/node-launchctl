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
            'deps/liblaunchctl-master/include',
            'deps/liblaunchctl-xcodeproj/liblaunchctl'
          ],
          'libraries': [
            '-L<!(pwd)/deps/liblaunchctl-master/build',
            '-L<!(pwd)',
            '-llaunchctl'
          ]
        }]
      ]
    }
  ]
}
