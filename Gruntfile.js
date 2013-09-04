module.exports = function(grunt) {
  grunt.initConfig({
    pkg: grunt.file.readJSON('package.json'),
    
    cafemocha: {
      main: {
        src: 'test.js',
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
  
  grunt.registerTask('default', ['cafemocha', 'modverify'])
}