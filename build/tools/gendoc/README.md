# gendoc

Yet another documentation generator from source code comments in Markdown format.

Source code documentation should be placed into `/ ** ** /`
comment block. Comment block should be opened and closed on a separate line.

Documentation text may be formatted in Markdown.
Supported tags:
<pre>
  @module name [- Brief description]
  Module description. A separate Markdown document would be created for each
  module with name `module.md`.

  @class name  [- Brief]
  Long class description.

  @function functionName(param1, [param2], [param3=default_value])  [- Brief]
  @param param1 [- Parameter descripton]
  @param [param2] {Type} [- Optional parameter description with type]
  @param [param3=default_value]
  @param args*  - Argument list args1 .. argN
  @param [args]* - Optional argument list
  @return {Type} - function return descripton

  @property qulified.name {Type} [- Brief]
  Property description. Name may be qualified in scope of module.
</pre>

JavaDoc autobrief is working - if no inline brief with tag, the first sentence
from long description would be used.

For @class and @function additionaly @emit tags may be supplied:

<pre>
  @event eventName(param1, ... paramN) - Describe event, similar to function
  @param param1 {Type} - Parameter description
  @param paramN
</pre>

For @function tag @emit tags describe which events are fired from that function

<pre>
  @emit eventNameX
  @emit eventNameY
</pre>

## gendoc(config)

  * config `Object`
  * Return: `Object` Generated documentation object

Generate documentation from source code in Markdown format.

Configuration object `config` should have follwing options:
  * `input` - array of input directories with source code to scan
  * `output` - result output directory name, would be created on necessary
  * `[cleanup=false]` - cleanup output directory before start
  * `[reporter=console.log]` - function to report work progress
  * `[extension='.md']` - result documents extension
  * `[result]` - write result in JSON format into the output directory, if supplied
  * `[single]` - additionaly generate whole documentation as a single document in the specified filename
  * `[index]` - additionaly generate index document in the specified filename
  * `[htmlOutput]` - convert generated Markdown to HTML pages in specified output directory
  * `[htmlExtension='.html']` - extension for output HTML files
  * `[htmlTemplate='template.html']` - HTML template filename
  * `[htmlStyle='style.css']` - HTML stylesheet filename
  * `[htmlHighliteStyle='highlight.js/styles/default.css']` - HTML stylesheet from highlight.js

Input directory may be a String or a RegEx object. For recurse directory scanning
specify `*` as a last but one path element. Examples:

```
    // all files in `dir1` subdirectory
    var input1 = './dir1';

    // files with .js extension in `dir2` subdirectory
    var input2 = './dir2/\\.+[.]js$';

    // files with .hpp or .cpp extension in `dir3` and all its subdirectories
    var input3 = './dir3/* /\\.+[.][hc]pp$';

    gendoc({ output: './doc/out', input: [input1, input2, input3] });
```

