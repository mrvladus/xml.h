#define XML_H_DEBUG
#include "xml.h"

int main() {
  // Parse XML file and get root node
  XMLNode *root = xml_parse_file("test.xml");

  // Check for errors
  if (!root)
    return 1;

  // Get first child of the root node
  XMLNode *library = xml_node_child_at(root, 0);

  // Check for errors
  if (!library)
    return 1;

  // Get second child
  XMLNode *book2 = xml_node_child_at(library, 1);

  // Check for errors
  if (!book2)
    return 1;

  // Get book title
  XMLNode *title = xml_node_child_at(book2, 0);

  // Check for errors
  if (!title)
    return 1;

  // Print book title
  printf("Book 2 title: %s\n", title->text);

  // Example of looping nodes children.
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
  XMLNode *matching_tag = xml_node_find_tag(root, "title", true);
  printf("Found tag: %s\n", matching_tag->tag);

  // Cleanup and exit
  xml_node_free(root);
  return 0;
}
