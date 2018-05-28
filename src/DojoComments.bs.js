// Generated by BUCKLESCRIPT VERSION 3.1.4, PLEASE EDIT WITH CARE
'use strict';

var Fs = require("fs");
var Process = require("process");
var Papaparse = require("papaparse");
var Belt_Array = require("bs-platform/lib/js/belt_Array.js");
var Caml_array = require("bs-platform/lib/js/caml_array.js");

var $$Error = /* module */[];

var Meta = /* module */[];

var Results = /* module */[];

function arraySplitAt(n, items) {
  return /* tuple */[
          items.slice(0, n),
          items.slice(n, items.length)
        ];
}

function escapeHTML(s) {
  var __x = s.replace((/\&/g), "&amp;");
  var __x$1 = __x.replace((/</g), "&lt;");
  return __x$1.replace((/>/g), "&gt;");
}

function processText(s) {
  var lines = s.split("\n");
  return Belt_Array.map(lines, escapeHTML).join("</div><div>");
}

function createDefnList(headers, cells) {
  var helper = function (_acc, _n) {
    while(true) {
      var n = _n;
      var acc = _acc;
      if (n === headers.length) {
        return acc;
      } else {
        _n = n + 1 | 0;
        _acc = acc + ("<dt>" + (Caml_array.caml_array_get(headers, n) + ("</dt>\n<dd><div>" + (processText(Caml_array.caml_array_get(cells, n)) + "</div></dd>\n"))));
        continue ;
      }
    };
  };
  return "<dl>" + (helper("", 0) + "</dl>\n\n");
}

function processRows(headers, rows) {
  var helper = function (param) {
    return createDefnList(headers, param);
  };
  return Belt_Array.map(rows, helper).join("<hr />\n");
}

var args = Process.argv;

var outFile = args[args.length - 1 | 0];

var inFile = args[args.length - 2 | 0];

var allLines = Fs.readFileSync(inFile, "utf8");

var parseData = Papaparse.parse(allLines).data;

var match = arraySplitAt(1, parseData);

var commentary = match[1];

var headersNested = match[0];

var headers = headersNested[0];

var htmlHeader = "\n<!DOCTYPE html>\n<html>\n<head>\n  <title>Feedback from European Dojo</title>\n  <meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\" />\n  <style type=\"text/css\">\n  body {font-family: helvetica, arial, sans-serif; }\n  dl {\n    margin: 0.5em 0;\n  }\n  dt { color: #666; }\n  dd { margin-bottom: 0.5em; }\n  </style>\n</head>\n<body>\n";

var htmlString = htmlHeader + (processRows(headers, commentary) + "</body>\n</html>");

Fs.writeFileSync(outFile, htmlString, "utf8");

var Arr = 0;

exports.Arr = Arr;
exports.$$Error = $$Error;
exports.Meta = Meta;
exports.Results = Results;
exports.arraySplitAt = arraySplitAt;
exports.escapeHTML = escapeHTML;
exports.processText = processText;
exports.createDefnList = createDefnList;
exports.processRows = processRows;
exports.args = args;
exports.outFile = outFile;
exports.inFile = inFile;
exports.allLines = allLines;
exports.parseData = parseData;
exports.headersNested = headersNested;
exports.commentary = commentary;
exports.headers = headers;
exports.htmlHeader = htmlHeader;
exports.htmlString = htmlString;
/* args Not a pure module */