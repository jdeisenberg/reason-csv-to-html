# Converting CSV to HTML

Recently, [@bsansouci](https://github.com/bsansouci) organized a ReasonML dojo that was held simultaneously in several locations in Europe. He provided a feedback form for attendees, and put the results in a Google document, which looks like this:

![screenshot showing spreadsheet with horizontal scrolling](google_doc_screenshot.png)

The problem is that you have to scroll horizontally to read the spreadsheet, and it’s not pleasant to read in any event. I decided to [export the spreadsheet as CSV](./feedback.csv), then write a program in ReasonML to convert the CSV to a [somewhat more readable HTML form](./europe_dojo_feedback.html). You can see the repository at [https://github.com/jdeisenberg/dojo-comments](https://github.com/jdeisenberg/dojo-comments). I learned a lot of things writing this, and I’ll share them with you here.

## The Big Picture

Here’s my plan for the program, which I run from the command line as follows:

```
node src/DojoComments.bs.js inputfile.csv outputfile.html
```

* Get the input and output file names from the command line
* Read the input CSV file as one large string
* Parse the CSV
* Separate the first (header) row from the remaining rows
* For each row, create a string that has a definition list `<dl>` with the heading for each column as the `<dt>` and the contents of the cell as the `<dt>`.
* Join these strings with `<hr />` elements (to make them easier to read when displayed).
* Sandwich the resulting string between an `<html>...` and `</html>`
* Write to the output file

## Reading Command Line Arguments

To read command line arguments, you need access to Node’s `process` global. ReasonML provides an [interface to `process`](https://bucklescript.github.io/bucklescript/api/Node_process.html) via BuckleScript. The `argv` variable gives me an array of command line arguments. For this program, the last element in the array is the output file name, and the next to last is the input file name. Current best practice in ReasonML is to use the `Belt.Array` module to manipulate arrays. I suspected I would be doing a lot of calls to functions from that module, and I didn’t want to type `Belt.Array` all the time, so I used a module alias at the top of my code:

```reason;shared(arr)
module Arr = Belt.Array;
```
and later on in the code:

```reason;use(arr);shared(args);
let args = Node.Process.argv;
let outFile = Arr.getUnsafe(args, Arr.length(args) - 1);
let inFile = Arr.getUnsafe(args, Arr.length(args) - 2);
```

This is where (not for the first time during this program) I got lazy. `Belt.Array.get()` returns `option` type, and I really didn’t want to deal with that, so I went with `getUnsafe()`, which means that if you don’t provide enough arguments to the program, it will crash.

## Reading the Input File

ReasonML also has an [interface to Node’s file system](https://bucklescript.github.io/bucklescript/api/Node.Fs.html). I was able to find the `readFileAsUtf8Sync()` function, which reads the entire file in as a single string. This would not be a good thing to use if I had an enormous, multi-gigabyte CSV file, but for a file the size that I was dealing with, it will do. It’s a synchronous call, so I don’t have to deal with callbacks. When the program finishes, I’ll use the corresponding `writeFileAsUtf8Sync()` to do my output.

```reason;use(args);shared(allLines);
let allLines = Node.Fs.readFileAsUtf8Sync(inFile);
```

## Parsing the CSV

At this point, I *could* have written functions to parse CSV, but this is not a well-behaved CSV file; there are newlines and quote marks and commas within the cells. Writing code to handle these cases correctly would have been a nightmare. I decided to avoid the nightmare and the needless duplication of effort, as there are plenty of excellent CSV parsing libraries out there for Node. The one I decided to use was [Papa Parse](https://www.papaparse.com/), which I installed with:

```txt
npm install --save papaparse
```

Papa Parse can [parse a string that is in CSV format](https://www.papaparse.com/docs#strings), and it will return a [parse results object](https://www.papaparse.com/docs#results). Using this required me to write an interface between ReasonML and JavaScript. My first task was to represent the results object(s) in ReasonML. ReasonML has records, which look a lot like JavaScript objects, but aren’t. Instead, you have to create data types that correspond to JavaScript objects, as described [in the documentation](https://bucklescript.github.io/docs/en/object.html#record-mode).

It is current practice to put each of these data types in a separate [module](https://reasonml.github.io/docs/en/module.html); this helps avoid name collisions among field names.

When you parse a CSV string with Papa Parse, you get back an object that contains an array of array of strings, an error object, and a meta-information object. Here is the error object in JavaScript:

```js
{
	type: "",     // A generalization of the error
	code: "",     // Standardized error code
	message: "",  // Human-readable details
	row: 0,       // Row index of parsed data where error is
	
}
```

and here it is in ReasonML:

```reason;shared(mod1)
module Error = {
   [@bs.deriving abstract] type t = {
    [@bs.as "type"]  type_: string,
    code: string,
    message: string,
    row: int
  }
};
```

First, the convention for the type name is `t`. I could have called it `errorType`, but then every time I wanted to access a field I would have to type `Error.errorType....`. This way, I refer to `Error.t...`, which you can mentally read as `error type`. Second, this object has a field named `type`, which happens to be a reserved word in ReasonML. To get around this problem, I call the field `type_`, and the `[@bs.as "type"]` directive tells ReasonML to compile that field to use the name `type` when it emits JavaScript. Here are the other two objects that Papa Parse gives you:

```reason;use(mod1);shared(mod2)
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
```

...and look! I am using types containing other types, as `Results.t` has a field of type `array(Error.t)` and another of type `Meta.t`.

The `parse()` function in Papa Parse takes one required parameter: the string to be parsed, and an optional configuration object. It turns out that the defaults for Papa Parse are exactly what I needed, so I did not have to create a new data type for the configuration object. Instead, I wrote this:

```reason;use(mod2);shared(parseExt);
[@bs.val] [@bs.module "papaparse"] external parse :
  (string) => Results.t = "parse";
```

The `[@bs.val]` directive says that I am binding to a global value in the modules specified by the `[@bs.module]` directive. The name I will use for the function is `parse`, and its signature follows the colon: it has a `string` parameter and returns a `Results.t`; this binds to the symbol `"parse"` in the module.

Now I used this to parse the CSV file and get the `data` object from the result (I didn’t worry about `error` or `meta`; laziness again):

```reason;use(parseExt);use(allLines);shared(parseData)
let parseData = Results.data(parse(allLines));
```

Note the function call `Results.data(...)` in the last line. When you created the `Results.t` data type, ReasonML created *getter* and *setter* functions to allow you to read and write fields in the JavaScript object, so `Results.data(...)` gives back the `data` field in the result of `parse(allLines)`.

```

`parseData` is an array of array of string. For example, the first part of CSV file might parse into something like this (just a segment to give you the idea):

```txt
[|
  [|"Timestamp", "Which dojo did you attend?", "Any previous programming experience"...|],
  [|"5/27/2018 19:46:19", "Austria, Vienna", "Yes"...|],
  [|"5/27/2018 23:08:53","Belgium, Brussels", "Yes"...|]
|]
```

I used `Belt.Array.slice()` to separate it into two arrays of arrays; the first one containing only the headers, and the second containing the remaining rows:

```reason;use(parseData);shared(splitArr)
let headers = Arr.slice(parseData, ~offset=0, ~len=1) |. Arr.getUnsafe(_,0);
let contentRows = Arr.slice(parseData, ~offset=1, ~len=Arr.length(parseData) - 1);
```

Take a closer look at the first line. First, `slice()` has *named parameters*. Instead of me having to remember the order of the parameters, and which number comes first—the offset or the length—named parameters let me specify the parameters in any order I please, and the names tell me who’s who and what’s what. The first call splits off the first array in `parseData`, returning an array of arrays that happens to have only one entry:

```txt
[|
  [|"Timestamp", "Which dojo did you attend?", "Any previous programming experience"...|]
|]
```

but I want only a simple array, so I had to pass that result to `getUnsafe()` to extract the first (and only) sub-array. The *fast pipe* operator `|.` sends the value on the left to fill in the `_` in the next call. You will often see people using fast pipe instead of nested function calls like this:

```txt
let headers = Arr.getUnsafe(Arr.slice(parseData, ~offset=0, ~len=1), 0);
```

The second line that slices off the `contentRows` is fine as it is; I *want* an array of arrays here.

## Processing the Rows

As mentioned before, each row will become a definition list, each column as `<dt>header</dt>` and the cell data as `<dd><div>content</div></dd>`. Why the `<div>`? That will let me handle cell contents with newlines; I will translate each newline into a `</div><div>\n`, and they will fit nicely between the beginning and ending tags already in the `<dd>`. Before I do that, I need to escape entities like `<`, `>`, and `&` within the text. Here’s a function that does that by fast piping a string through a series of global string replacements, courtesy of `Js.String.replaceByRe()`, where `Re` stands for *regular expression*:

```reason;shared(processCell)
let processCell = (s: string) : string => {
  s |.
  Js.String.replaceByRe([%re "/\\&/g"], "&amp;", _) |.
  Js.String.replaceByRe([%re "/</g"], "&lt;", _) |.
  Js.String.replaceByRe([%re "/>/g"], "&gt;", _) |.
  Js.String.replaceByRe([%re "/\\n/g"], "</div><div>", _);
};
```

> I have fully annotated the parameter and return types for `processCell()`. You will normally not see ReasonML programmers doing this; they will let ReasonML’s type inference engine do the work for them. So why am I doing this extra work?
> I have, for many years, taught beginning programming courses. Sometimes, I am in the middle of explaining some program that I am live coding, and I stop and tell the class, “I am not explaining how I am writing this to convince you. I’m doing it to convince *me*. And that’s why I am fully annotating the types for all my functions: to convince myself that I know exactly what kind of data is coming into and going out of my functions. Your Mileage May Vary.  

Now I can create a definition list from an array of headers and a row’s worth of cells. I could use `Belt.Array.Reduce`, but it seemed easier to use a recursive helper function to accumulate a string `acc` from cell index `n`:

```reason;use(processCell);use(arr);shared(createDL)
let createDefnList = (headers: array(string), cells: array(string)) : string => {

  let rec helper = (acc: string, n: int) : string => {
    if (n == Arr.length(headers)) {
      acc
    } else {
      helper(acc ++ "<dt>" ++ headers[n] ++ "</dt>\n<dd><div>"
        ++ processCell(cells[n]) ++ "</div></dd>\n", n + 1) ;
    }
  };
  
  "<dl>" ++ helper("", 0) ++ "</dl>\n\n";
};
```

The keyword `rec` allows me to do recursive call. If the index `n` is at the end of the array, the function returns the accumulator. Otherwise, it calls the helper function recursively with a new accumulator (the old value plus a new `<dt>..</dt>` and `<dd>..</dd>`) and the next index value (`n + 1`).

Notice that ReasonML has syntactic sugar that lets me write `headers[n]` instead of having to say `Array.get(headers, n)`. That’s the built-in `Array` module, not `Belt.Array`, by the way.

Now I can (finally) process all the rows in the array. My plan is to use `Belt.Array.map()` to apply `createDefnList()` to each of the rows in `contentRows`, which will give me an array of `<dl>...</dl>` strings that I can join with `<hr/>` to visually separate them.  There’s just one problem: the function you give to `map()` can only have one parameter: the item from the array being processed, and `createDefnList()` has two parameters: The headers and the contents. There is a solution: *currying*. All functions in ReasonML are automatically curried—if you call them with fewer parameters than they specify, instead of giving you an error, the function returns a new function with the parameters you supplied “filled in.”  Look at this code:

```reason;use(createDL);shared(processRows)
let processRows = (headers: array(string), rows: array(array(string))) : string => {
  let helperFcn = createDefnList(headers);
  Arr.map(rows, helperFcn) |.
    Js.Array.joinWith("<hr />\n", _);
};
```

The right hand side of `let helper = createDefnList(headers);` calls the `createDefnList()` function with just the first parameters. The return value, which will be bound to the value `helperFcn`, is a function that has one argument already supplied, so it needs only one parameter to do its job—and that fulfill’s `map()`’s requirement for a function that has only one parameter.

When `processRows()` is finished, it will return one very long string that consists of `<dl>..</dl><hr/>`s, one for every row in the CSV file.

## Finishing the Task

To wrap everything up, I sandwiched the `processRows()` call between strings that provide the HTML header and ending tags, and then used `writeFileAsUtf8Sync()` to send that to the output file:

```txt
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

let htmlString = (htmlHeader ++ processRows(headers, contentRows) ++
  {js|</body>\n</html>|js});

let _ = Node.Fs.writeFileAsUtf8Sync(outFile, htmlString);
```

## Conclusion

There you have it, a modest but useful program. Here are the concepts I needed:

* Reading command line arguments
* Reading and writing files
* Creating data types and binding to functions in an existing JavaScript library
* Using the `Belt.Array` module
* Using fast pipe (`|.`) to make code more readable
* Using recursion to accumulate a value
* Using currying to partially apply a function

I learned a lot in getting this program to work, and I hope you enjoyed going through the process with me.
The repository is at [https://github.com/jdeisenberg/dojo-comments](https://github.com/jdeisenberg/dojo-comments).


