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
            '<!(xcrun --show-sdk-path)/System/Library/Frameworks/CoreFoundation.framework/Headers'
          ],
          'ldflags': [
            "-L<!(pwd)/build/Release",
            "-I<!(xcrun --show-sdk-path)/System/Library/Frameworks/CoreFoundation.framework/Headers"
          ],
          'libraries': [
            '-L<!(pwd)/build/Release',
            '-llaunchctl'
          ]
        }]
      ]
    }
  ]
}
