/* https://bucklescript.github.io/docs/en/function.html#trick-2-polymorphic-variant-bsunwrap */

module Arr = Belt.Array;

module Error = {
   [@bs.deriving abstract] type t = {
    [@bs.as "type"]  type_: string,
    code: string,
    message: string,
    row: int
  }
};

module Meta = {
  [@bs.deriving abstract] type t = {
    delimiter: string,
    linebreak: string,
    aborted: bool,
    fields: array(string),
    truncated: bool
  }
};


module Results = {
 [@bs.deriving abstract] type t = {
      data: array(array(string)),
      error: array(Error.t),
      meta: Meta.t
  };
};

[@bs.val] [@bs.module "papaparse"] external parse :
  (string) => Results.t = "parse";

let arraySplitAt = (n:int, items: array('a)) : (array('a), array('a)) => {
  (Js.Array.slice(~start=0, ~end_=n, items),
    Js.Array.slice(~start=n, ~end_=Js.Array.length(items), items))
};

let escapeHTML = (s: string) : string => {
  s |. Js.String.replaceByRe([%re "/\\&/g"], "&amp;", _) |.
  Js.String.replaceByRe([%re "/</g"], "&lt;", _) |.
  Js.String.replaceByRe([%re "/>/g"], "&gt;", _);
};

/*
 * Make each \n in the string a new <div>. Also escape HTML.
 */
let processText = (s: string) : string => {
  let lines = Js.String.split("\n", s);
  Arr.map(lines, escapeHTML) |.
    Js.Array.joinWith("</div><div>", _);
};

/* Given an array of headers and an array of data,
 * create an HTML <dl> for that line
 */
let createDefnList = (headers: array(string), cells: array(string)) : string => {
  let rec helper = (acc: string, n: int) : string => {
    if (n == Arr.length(headers)) {
      acc
    } else {
      helper(acc ++ "<dt>" ++ headers[n] ++ "</dt>\n<dd><div>"
        ++ processText(cells[n]) ++ "</div></dd>\n", n + 1) ;
    }
  };
  "<dl>" ++ helper("", 0) ++ "</dl>\n\n";
};

let processRows = (headers: array(string), rows: array(array(string))) : string => {
  /* Partially apply createDefnList so it needs only one argument
   * that way, it can be used with map
   */
  let helper = createDefnList(headers);
  Arr.map(rows, helper) |.
    Js.Array.joinWith("<hr />\n", _);
};

let args = Node.Process.argv;
let outFile = Arr.getUnsafe(args, Arr.length(args) - 1);
let inFile = Arr.getUnsafe(args, Arr.length(args) - 2);

/* Read the entire CSV file as one string */
let allLines = Node.Fs.readFileAsUtf8Sync(inFile);

/* Parse the CSV  string  */
let parseData = Results.data(parse(allLines));

/* Split it into two array(array(string)) */

let (headersNested, commentary) = arraySplitAt(1, parseData);
let headers = Arr.getUnsafe(headersNested,0);

let htmlHeader = {js|
<!DOCTYPE html>
<html>
<head>
  <title>Feedback from European Dojo</title>
  <meta http-equiv="Content-Type" content="text/html; charset=utf-8" />
  <style type="text/css">
  body {font-family: helvetica, arial, sans-serif; }
  dl {
    margin: 0.5em 0;
  }
  dt { color: #666; }
  dd { margin-bottom: 0.5em; }
  </style>
</head>
<body>
|js};

let htmlString = (htmlHeader ++ processRows(headers, commentary) ++
  {js|</body>\n</html>|js});

let _ = Node.Fs.writeFileAsUtf8Sync(outFile, htmlString);
