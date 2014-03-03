module.exports = function(grunt) {
  grunt.initConfig({
    pkg: grunt.file.readJSON('package.json'),

    cafemocha: {
      launchctl: {
        src: 'tests/launchctl.js',
        options: {
          colors: true,
          reporter: grunt.option('reporter') || 'spec',
          ui: 'bdd'
        }
      }
    },

    modverify: {
      main: {
        options: {

        }
      }
    }
  })

  grunt.loadNpmTasks('grunt-cafe-mocha')
  grunt.loadNpmTasks('grunt-modverify')

  grunt.registerTask('launchctl', ['cafemocha:launchctl'])

  grunt.registerTask('test', ['launchctl'])
  grunt.registerTask('default', ['modverify', 'test'])
}
