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
            'deps/liblaunchctl/include'
          ],
          'libraries': [
            '-L<!(pwd)/deps/liblaunchctl/build',
            '-llaunchctl'
          ]
        }]
      ]
    }
  ]
}
