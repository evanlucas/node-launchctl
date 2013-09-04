module.exports = function(grunt) {
  grunt.initConfig({
    pkg: grunt.file.readJSON('package.json'),
    
    cafemocha: {
      launchctl: {
        src: 'tests/launchctl.js',
        options: {
          colors: true,
          reporter: grunt.option('reporter') || 'nyan',
          ui: 'bdd'
        }
      },
      plist: {
        src: 'tests/plist.js',
        options: {
          colors: true,
          reporter: grunt.option('reporter') || 'nyan',
          ui: 'bdd'
        }
      },
      utils: {
        src: 'tests/utils.js',
        options: {
          colors: true,
          reporter: grunt.option('reporter') || 'nyan',
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
  grunt.registerTask('plist', ['cafemocha:plist'])
  grunt.registerTask('utils', ['cafemocha:utils'])
  
  grunt.registerTask('test', ['launchctl', 'plist', 'utils'])
  grunt.registerTask('default', ['modverify', 'test'])
}