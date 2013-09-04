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
    }
  })
  
  grunt.loadNpmTasks('grunt-cafe-mocha')
  grunt.registerTask('default', ['cafemocha'])
}