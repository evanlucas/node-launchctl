{
	"targets": [
		{
			"target_name": "launchctl",
			"sources": ["src/launchctl.cc"],
			"conditions": [
				['OS=="mac"', {
					"defines": [ '__MACOSX_CORE__' ],
					'ldflags': [ '-liblaunch', '-framework CoreFoundation' ]
				}]
			]
		}
	]
}
