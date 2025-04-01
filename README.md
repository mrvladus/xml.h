## xml.h - Header-only C/C++ library to parse XML.

Features:

- One tiny header file (~300 lines of code)
- Parses regular (`<tag>Tag Text</tag>`) and self-closing tags (`<tag/>`)
- Parses tags attributes (`<tag attribute="value" />`)
- Ignores comments
- Very easy to use
- No bloat

### Installation

It's one `xml.h` header file, you can put it directly in your project.

### Documentation

Look inside `xml.h`. All functions have comments.

### Usage

It's very simple to use. Basically when you parse file with `xml_parse_file()` or string with `xml_parse_string()` you getting a root `XMLNode` which is gonna have children (sub-tags).
And you will be getting deeper into the tree of tags with `xml_node_child_at()` or by using loops.
See `test.xml` and `test.c` files for complete example.

## Contributing

Pull requests are welcome!

Use `make build` or `make run` to test changes to this library.
