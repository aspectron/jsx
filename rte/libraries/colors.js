//
// Copyright (c) 2011 - 2015 ASPECTRON Inc.
// All Rights Reserved.
//
// This file is part of JSX (https://github.com/aspectron/jsx) project.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE
//

/**
@module colors - Colors
Colored output to console. Adds styles and colors to the String prototype.
Inspired by Node.js module [`colors`](https://www.npmjs.org/package/colors)
**/

/**
## Usage

``` js
var colors = require('colors');

console.log('hello'.green); // outputs green text
console.log('i like cake and pies'.underline.red) // outputs red underlined text
console.log('inverse the color'.inverse); // inverses the color
console.log('OMG Rainbows!'.rainbow); // rainbow (ignores spaces)
```

## Creating Custom themes

```js
var colors = require('colors');

colors.setTheme({
  silly: 'rainbow',
  input: 'grey',
  verbose: 'cyan',
  prompt: 'grey',
  info: 'green',
  data: 'grey',
  help: 'cyan',
  warn: 'yellow',
  debug: 'blue',
  error: 'red'
});

// outputs red text
console.log("this is an error".error);

// outputs yellow text
console.log("this is a warning".warn);
```
**/

/**
Allowed text styles:
  * `bold`
  * `italic`
  * `underline`
  * `inverse`
  * `strikethrough`
  * `rainbow`
  * `zebra`
  * `random`
  * `stripColors`

Allowed text colors:
  * `white`
  * `gray`
  * `black`
  * `blue`
  * `cyan`
  * `green`
  * `magenta`
  * `red`
  * `yellow`

Allowed background colors:
  * `whiteBG`
  * `grayBG`
  * `blackBG`
  * `blueBG`
  * `cyanBG`
  * `greenBG`
  * `magentaBG`
  * `redBG`
  * `yellowBG`

**/
var styles = {
    bold:      { console: ['\x1B[1m',  '\x1B[22m'], browser: ['<b>',  '</b>'] },
    italic:    { console: ['\x1B[3m',  '\x1B[23m'], browser: ['<i>',  '</i>'] },
    underline: { console: ['\x1B[4m',  '\x1B[24m'], browser: ['<u>',  '</u>'] },
    inverse:   { console: ['\x1B[7m',  '\x1B[27m'], browser: ['<span style="background-color:black;color:white;">',  '</span>'] },
    strikethrough: { console: ['\x1B[9m',  '\x1B[29m'], browser: ['<del>',  '</del>'] },

    black:     { console: ['\x1B[30m', '\x1B[39m'], browser: ['<span style="color:black;">', '</span>'] },
    red:       { console: ['\x1B[31m', '\x1B[39m'], browser: ['<span style="color:red;">', '</span>'] },
    green:     { console: ['\x1B[32m', '\x1B[39m'], browser: ['<span style="color:green;">', '</span>'] },
    yellow:    { console: ['\x1B[33m', '\x1B[39m'], browser: ['<span style="color:yellow;">', '</span>'] },
    blue:      { console: ['\x1B[34m', '\x1B[39m'], browser: ['<span style="color:blue;">', '</span>'] },
    magenta:   { console: ['\x1B[35m', '\x1B[39m'], browser: ['<span style="color:magenta;">', '</span>'] },
    cyan:      { console: ['\x1B[36m', '\x1B[39m'], browser: ['<span style="color:cyan;">', '</span>'] },
    white:     { console: ['\x1B[37m', '\x1B[39m'], browser: ['<span style="color:white;">', '</span>'] },
    gray:      { console: ['\x1B[90m', '\x1B[39m'], browser: ['<span style="color:gray;">', '</span>']  },

    blackBG:   { console: ['\x1B[40m', '\x1B[49m'], browser: ['<span style="background-color:black;">', '</span>'] },
    redBG:     { console: ['\x1B[41m', '\x1B[49m'], browser: ['<span style="background-color:red;">', '</span>'] },
    greenBg:   { console: ['\x1B[42m', '\x1B[49m'], browser: ['<span style="background-color:green;">', '</span>'] },
    yellowBG:  { console: ['\x1B[43m', '\x1B[49m'], browser: ['<span style="background-color:yellow;">', '</span>'] },
    blueBG:    { console: ['\x1B[44m', '\x1B[49m'], browser: ['<span style="background-color:blue;">', '</span>'] },
    magentaBG: { console: ['\x1B[45m', '\x1B[49m'], browser: ['<span style="background-color:magenta;">', '</span>'] },
    cyanBG:    { console: ['\x1B[46m', '\x1B[49m'], browser: ['<span style="background-color:cyan;">', '</span>'] },
    whiteBG:   { console: ['\x1B[47m', '\x1B[49m'], browser: ['<span style="background-color:white;">', '</span>'] },
    grayBG:    { console: ['\x1B[49;5;8m', '\x1B[49m'], browser: ['<span style="background-color:gray;">', '</span>']  },
};
styles.grey = styles.gray;
styles.greyBG = styles.grayBG;

Object.keys(styles).forEach(function (style)
{
    addProperty(style, function() { return stylize(this, style); });
});
addProperty('stripColors', function() { return this.toString().replace(/\x1B\[\d+m/g, ''); });

/**
@property mode {String} - Colors mode
Allowed values are:
  * `console` - Output colors as VT100 terminal sequenses
  * `browser` - Output colors as HTML tags
  * `none`    - Do not use colors
**/
if (typeof exports !== 'undefined')
{
    var isHeadless = true;
    exports.mode = 'console';
}
else
{
    var isHeadless = false;
    var exports = {};
    var module = {};
    var colors = exports;
    exports.mode = 'browser';
}

function stylize(str, style)
{
    return (exports.mode === 'none')? str :
        styles[style][exports.mode][0] + str + styles[style][exports.mode][1];
}

function addProperty(style, func)
{
    exports[style] = function(str) { return func.apply(str); };
    String.prototype.__defineGetter__(style, func);
}

function sequencer(map)
{
    return function()
    {
        if (!isHeadless) return this.replace(/( )/, '$1');
        var exploded = this.split('');
        exploded = exploded.map(map);
        return exploded.join('');
    };
}

exports.addSequencer = function(name, map) { addProperty(name, sequencer(map)); }

var rainbowColors = ['red', 'yellow', 'green', 'blue', 'magenta']; //RoY G BiV

exports.addSequencer('rainbow', function(letter, i)
{
    return letter === ' '? letter :
        stylize(letter, rainbowColors[i++ % rainbowColors.length]);
});

exports.addSequencer('zebra', function(letter, i)
{
    return i % 2 === 0 ? letter : letter.inverse;
});

/**
@property themes {Object} - Loaded themes
**/
exports.themes = {};

/**
@function setTheme(theme)
@param theme {Object|String}
Set styles from `theme` object.
If theme is a `String` load theme object from file as `require(theme)`
**/
exports.setTheme = function(theme)
{
    if (typeof theme === 'string')
    {
        var themeObject = require(theme);
        applyTheme(themeObject);
        exports.themes[theme] = themeObject;
    }
    else
    {
        applyTheme(theme);
    }
}

function applyTheme(theme)
{
    //TODO: get String.prototype keys before modification
    var stringPrototypeKeys = [
        '__defineGetter__', '__defineSetter__', '__lookupGetter__', '__lookupSetter__', 'charAt', 'constructor',
        'hasOwnProperty', 'isPrototypeOf', 'propertyIsEnumerable', 'toLocaleString', 'toString', 'valueOf', 'charCodeAt',
        'indexOf', 'lastIndexof', 'length', 'localeCompare', 'match', 'replace', 'search', 'slice', 'split', 'substring',
        'toLocaleLowerCase', 'toLocaleUpperCase', 'toLowerCase', 'toUpperCase', 'trim', 'trimLeft', 'trimRight'
    ];

    Object.keys(theme).forEach(function(key)
    {
        if (stringPrototypeKeys.indexOf(key) !== -1)
        {
            console.warning('Style %s conflicts with property in String.prototype, ignoring it');
            return;
        }

        var style = theme[key]
        var func;
        if (typeof style === 'string')
        {
            func = function() { return exports[style](this); }
        }
        else
        {
            func = function()
            {
                var ret = this;
                for (var t = 0; t < style.length; ++t)
                {
                    ret = exports[style[t]](ret);
                }
                return ret;
            }
        }
        addProperty(key, func);
    });
}
