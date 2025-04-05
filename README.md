## xml.h - Header-only C/C++ library to parse XML.

Features:

- One tiny header file (~400 lines of code)
- Parses regular (`<tag>Tag Text</tag>`) and self-closing tags (`<tag/>`)
- Parses tags attributes (`<tag attribute="value" />`)
- Ignores comments
- Very easy to use
- No bloat

### Usage

It's one [stb-style](https://github.com/nothings/stb) `xml.h` header file, you can put it directly in your project.
It's very simple to use. Basically when you parse file with `xml_parse_file()` or string with `xml_parse_string()` you getting a root `XMLNode` which is gonna have children (sub-tags).
And you will be getting deeper into the tree of tags with `xml_node_child_at()`, by using loops or by using functions like `xml_node_find_tag()` and `xml_node_find_by_path()`.

Here's an example of how to use it:

```c
#define XML_H_IMPLEMENTATION // Must be defined before including xml.h in ONE source file
#include "xml.h"

const char test_xml[] = "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>"
                        "<!-- Test comment -->"
                        "<library>"
                        "    <book id=\"1\">"
                        "        <title>The Great Gatsby</title>"
                        "        <author>F. Scott Fitzgerald</author>"
                        "        <rating value=\"4.5\" />"
                        "    </book>"
                        "    <book id=\"2\">"
                        "        <title>1984</title>"
                        "        <author>George Orwell</author>"
                        "        <rating value=\"4.9\" />"
                        "        <bestseller />"
                        "    </book>"
                        "</library>";

int main() {
  printf("Test XML:\n%s\n", test_xml);
  // Parse XML string and get root node
  XMLNode *root = xml_parse_string(test_xml);
  // Get first child of the root node
  XMLNode *library = xml_node_child_at(root, 0);
  // Print ratings of all books
  for (size_t i = 0; i < library->children->len; i++) {
    // Get book
    XMLNode *book = xml_node_child_at(library, i);
    for (size_t j = 0; j < book->children->len; j++) {
      // Get book child tag
      XMLNode *sub_tag = xml_node_child_at(book, j);
      // If tag name is "rating" - print its value
      if (!strcmp(sub_tag->tag, "rating")) {
        // Get rating value
        const char *rating = xml_node_attr(sub_tag, "value");
        printf("Rating is %s\n", rating);
      }
    }
  }
  // Find 1st matching tag
  XMLNode *title_tag = xml_node_find_tag(root, "title", true);
  printf("Matching tag for 'title' = '%s'\n", title_tag->tag);
  // Find 1st matching tag by path
  XMLNode *author_tag = xml_node_find_by_path(root, "library/book/author", true);
  printf("Matching tag for 'author' = '%s'\n", author_tag->tag);
  // Cleanup and exit
  xml_node_free(root);
  return 0;
}
```

### Documentation

Look inside `xml.h`. All functions have comments.

## Contributing

Pull requests are welcome!
