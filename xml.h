/*

MIT License

Copyright (c) 2024-2025 Vlad Krupinskii <mrvladus@yandex.ru>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.

------------------------------------------------------------------------------

xml.h - Header-only C/C++ library to parse XML.

------------------------------------------------------------------------------

Features:

- One tiny header file (~400 lines of code)
- Parses regular (`<tag>Tag Text</tag>`) and self-closing tags (`<tag/>`)
- Parses tags attributes (`<tag attribute="value" />`)
- Ignores comments and processing instructions

------------------------------------------------------------------------------

Usage:

Define XML_H_IMPLEMENTATION in ONE file before including "xml.h" to include functions implementations.
In other files just include "xml.h".

#define XML_H_IMPLEMENTATION
#include "xml.h"

------------------------------------------------------------------------------

*/

#ifndef XML_H
#define XML_H

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include <stdbool.h>
#include <stddef.h>

// ---------- REDEFINE FUNCTIONS VISIBILITY  ---------- //

#ifndef XML_H_API
#define XML_H_API extern
#endif // XML_H_API

// ---------- REDEFINE ALLOC FUNCTIONS ---------- //

#ifndef XML_CALLOC_FUNC
#include <stdlib.h>
#define XML_CALLOC_FUNC calloc
#endif // XML_CALLOC_FUNC

#ifndef XML_REALLOC_FUNC
#include <stdlib.h>
#define XML_REALLOC_FUNC realloc
#endif // XML_REALLOC_FUNC

#ifndef XML_FREE_FUNC
#include <stdlib.h>
#define XML_FREE_FUNC free
#endif // XML_FREE_FUNC

// ---------- XMLString ---------- //

// NULL-terminated dynamically-growing string.
typedef struct {
  size_t len;  // Length of the string.
  size_t size; // Capacity of the string in bytes.
  char *str;   // Pointer to the null-terminated string.
} XMLString;

// Create a new `XMLString`.
// Returns `NULL` for error.
// Free with `xml_string_free()`.
XML_H_API XMLString *xml_string_new();
// Free `XMLString`.
XML_H_API void xml_string_free(XMLString *str);
// Append string to the end of the `XMLString`.
XML_H_API void xml_string_append(XMLString *str, const char *append);
// Steal the string pointer from `XMLString` and free the `XMLString`.
// Caller is responsible for freeing the returned string.
XML_H_API char *xml_string_steal(XMLString *str);

// ---------- XMLList ---------- //

// Simple dynamic list that only can grow in size.
// Used in XMLNode for list of children and list of tag attributes.
typedef struct {
  size_t len;  // Length of the list.
  size_t size; // Size of the list in bytes.
  void **data; // List of pointers to list items.
} XMLList;

// Create new dynamic array
XML_H_API XMLList *xml_list_new();
// Add element to the end of the array. Grow if needed.
XML_H_API void xml_list_add(XMLList *list, void *data);

// ---------- XMLNode ---------- //

// Tags attribute containing key and value.
// Like: <tag foo_key="foo_value" bar_key="bar_value" />
typedef struct XMLAttr {
  char *key;
  char *value;
} XMLAttr;

// The main object to interact with parsed XML nodes. Represents single XML tag.
typedef struct XMLNode XMLNode;
struct XMLNode {
  char *tag;         // Tag string.
  char *text;        // Inner text of the tag. NULL if tag has no inner text.
  XMLList *attrs;    // List of tag attributes. Check "node->attrs->len" if it has items.
  XMLNode *parent;   // Node's parent node. NULL for the root node.
  XMLList *children; // List of tag's sub-tags. Check "node->children->len" if it has items.
};

// Create new `XMLNode`.
// `parent` can be NULL if node is root. If parent is not NULL, node will be added to parent's children list.
// `tag` and `inner_text` can be NULL.
// Returns `NULL` for error.
// Free with `xml_node_free()`.
XML_H_API XMLNode *xml_node_new(XMLNode *parent, const char *tag, const char *inner_text);
// Parse XML string and return root XMLNode.
// Returns NULL for error.
// Free with `xml_node_free()`.
XML_H_API XMLNode *xml_parse_string(const char *xml);
// Parse XML file for given path and return root XMLNode.
// Returns NULL for error.
// Free with `xml_node_free()`.
XML_H_API XMLNode *xml_parse_file(const char *path);
// Get child of the node at index.
// Returns NULL if not found.
XML_H_API XMLNode *xml_node_child_at(XMLNode *node, size_t idx);
// Get first matching tag in the tree.
// It also can search tags by path in the format: `div/p/href`
// If `exact` is `true` - tag names will be matched exactly.
// If `exact` is `false` - tag names will be matched partially (containing sub-string).
XML_H_API XMLNode *xml_node_find_tag(XMLNode *node, const char *tag, bool exact);
// Get value of the tag attribute.
// Returns NULL if not found.
XML_H_API const char *xml_node_attr(XMLNode *node, const char *attr_key);
// Add attribute with `key` and `value` to the node's list of attributes.
XML_H_API void xml_node_add_attr(XMLNode *node, const char *key, const char *value);
// Serialize `XMLNode` into `XMLString`.
XML_H_API void xml_node_serialize(XMLNode *node, XMLString *str);
// Cleanup node and all it's children recursively.
XML_H_API void xml_node_free(XMLNode *node);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // XML_H

// -------------------- FUNCTIONS IMPLEMENTATION -------------------- //

#ifdef XML_H_IMPLEMENTATION

#include <ctype.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define XML_FREE(ptr)                                                                                                  \
  if (ptr) {                                                                                                           \
    XML_FREE_FUNC(ptr);                                                                                                \
    ptr = NULL;                                                                                                        \
  }

static inline void xml__skip_whitespace(const char *xml, size_t *idx) {
  while (xml[*idx] != '\0' && isspace((unsigned char)xml[*idx])) (*idx)++;
}

static inline char *xml__strndup(const char *str, size_t n) {
  void *dup = XML_CALLOC_FUNC(1, n + 1);
  memcpy(dup, str, n);
  return dup;
}

static inline char *xml__strdup(const char *str) { return xml__strndup(str, strlen(str)); }

static char *xml__decode_entities(const char *str, size_t len) {
  XMLString *decoded = xml_string_new();
  size_t i = 0;
  while (i < len) {
    if (str[i] == '&') {
      if (strncmp(&str[i], "&lt;", 4) == 0) {
        xml_string_append(decoded, "<");
        i += 4;
      } else if (strncmp(&str[i], "&gt;", 4) == 0) {
        xml_string_append(decoded, ">");
        i += 4;
      } else if (strncmp(&str[i], "&amp;", 5) == 0) {
        xml_string_append(decoded, "&");
        i += 5;
      } else if (strncmp(&str[i], "&apos;", 6) == 0) {
        xml_string_append(decoded, "'");
        i += 6;
      } else if (strncmp(&str[i], "&quot;", 6) == 0) {
        xml_string_append(decoded, "\"");
        i += 6;
      } else {
        // Copy as-is for unknown entities
        char temp[2] = {str[i], 0};
        xml_string_append(decoded, temp);
        i++;
      }
    } else {
      char temp[2] = {str[i], 0};
      xml_string_append(decoded, temp);
      i++;
    }
  }
  return xml_string_steal(decoded);
}

// ---------- XMLString ---------- //

XML_H_API XMLString *xml_string_new() {
  XMLString *str = (XMLString *)XML_CALLOC_FUNC(1, sizeof(XMLString));
  str->len = 0;
  str->size = 64;
  str->str = (char *)XML_CALLOC_FUNC(1, str->size);
  str->str[0] = '\0';
  return str;
}

XML_H_API void xml_string_append(XMLString *str, const char *append) {
  if (!str || !append) return;
  size_t append_len = strlen(append);
  if (str->len + append_len + 1 > str->size) {
    str->size = (str->len + append_len + 1) * 2;
    str->str = (char *)XML_REALLOC_FUNC(str->str, str->size);
  }
  strcpy(str->str + str->len, append);
  str->len += append_len;
}

XML_H_API void xml_string_free(XMLString *str) {
  if (!str) return;
  XML_FREE(str->str);
  XML_FREE(str);
}

XML_H_API char *xml_string_steal(XMLString *str) {
  if (!str) return NULL;
  char *stealed = str->str;
  XML_FREE(str);
  return stealed;
}

// ---------- XMLList ---------- //

// Create new dynamic array
XML_H_API XMLList *xml_list_new() {
  XMLList *list = (XMLList *)XML_CALLOC_FUNC(1, sizeof(XMLList));
  list->len = 0;
  list->size = 32;
  list->data = XML_CALLOC_FUNC(1, sizeof(void *) * list->size);
  return list;
}

// Add element to the end of the array. Grow if needed.
XML_H_API void xml_list_add(XMLList *list, void *data) {
  if (!list || !data) return;
  if (list->len >= list->size) {
    list->size *= 2;
    list->data = XML_REALLOC_FUNC(list->data, list->size * sizeof(void *));
  }
  list->data[list->len++] = data;
}

// ---------- XMLNode ---------- //

XML_H_API XMLNode *xml_node_new(XMLNode *parent, const char *tag, const char *inner_text) {
  XMLNode *node = XML_CALLOC_FUNC(1, sizeof(XMLNode));
  node->parent = parent;
  node->tag = tag ? xml__strdup(tag) : NULL;
  node->text = inner_text ? xml__strdup(inner_text) : NULL;
  node->children = xml_list_new();
  node->attrs = xml_list_new();
  if (parent) xml_list_add(parent->children, node);
  return node;
}

XML_H_API void xml_node_add_attr(XMLNode *node, const char *key, const char *value) {
  XMLAttr *attr = XML_CALLOC_FUNC(1, sizeof(XMLAttr));
  attr->key = xml__strdup(key);
  attr->value = xml__strdup(value);
  xml_list_add(node->attrs, attr);
}

XML_H_API XMLNode *xml_node_child_at(XMLNode *node, size_t index) {
  if (!node || !node->children) return NULL;
  if (index >= node->children->len) return NULL;
  return (XMLNode *)node->children->data[index];
}

XML_H_API XMLNode *xml_node_find_tag(XMLNode *node, const char *tag, bool exact) {
  if (!node || !tag) return NULL;
  // If tag doesn't contain any '/' then it's a single tag search
  if (!strchr(tag, '/')) {
    // Check if the current node matches the tag
    if (node->tag && ((exact && strcmp(node->tag, tag) == 0) || (!exact && strstr(node->tag, tag) != NULL)))
      return node;
    // Recursively search through the children of the node
    for (size_t i = 0; i < node->children->len; i++) {
      XMLNode *result = xml_node_find_tag(node->children->data[i], tag, exact);
      if (result) return result; // Return the first match found
    }
    return NULL;
  }
  // Path tag search
  char *tokenized_path = xml__strdup(tag);
  if (!tokenized_path) return NULL;
  char *segment = strtok(tokenized_path, "/");
  XMLNode *current = node;
  while (segment && current) {
    bool found = false;
    for (size_t i = 0; i < current->children->len; i++) {
      XMLNode *child = (XMLNode *)current->children->data[i];
      if (!child->tag) continue;
      if ((exact && strcmp(child->tag, segment) == 0) || (!exact && strstr(child->tag, segment) != NULL)) {
        current = child;
        found = true;
        break;
      }
    }
    if (!found) {
      XML_FREE(tokenized_path);
      return NULL;
    }
    segment = strtok(NULL, "/");
  }
  XML_FREE(tokenized_path);
  return current;
}

XML_H_API const char *xml_node_attr(XMLNode *node, const char *attr_key) {
  if (!node || !attr_key) return NULL;
  for (size_t i = 0; i < node->attrs->len; i++) {
    XMLAttr *attr = (XMLAttr *)node->attrs->data[i];
    if (attr->key && strcmp(attr->key, attr_key) == 0) return attr->value;
  }
  return NULL;
}

// Skip <!-- ... -->, <? ... ?>, <!DOCTYPE ... >
// Returns true if skipped.
static bool xml__skip_tags(const char *xml, size_t *idx) {
  if (xml[*idx] == '!' || xml[*idx] == '?') {
    size_t open_braket_count = 0;
    while (xml[*idx] != '>' || (xml[*idx] == '>' && open_braket_count > 0)) {
      if (xml[*idx] == '\0') return false;
      if (xml[*idx] == '<') open_braket_count++;
      if (xml[*idx] == '>') open_braket_count--;
      (*idx)++;
    }
    (*idx)++; // Skip '>'
    return true;
  }
  return false;
}

// Parse end tag </tag>.
static void xml__parse_end_tag(const char *xml, size_t *idx, XMLNode **curr_node) {
  (*idx)++; // Skip '/'
  xml__skip_whitespace(xml, idx);
  while (xml[*idx] != '>' && xml[*idx] != '\0') (*idx)++;
  if (xml[*idx] == '>') (*idx)++; // Skip '>'
  *curr_node = (*curr_node)->parent;
}

// Parse tag name <name ... >
static void xml__parse_tag_name(const char *xml, size_t *idx, XMLNode **curr_node) {
  size_t tag_start = *idx;
  while (!(isspace(xml[*idx]) || xml[*idx] == '>' || xml[*idx] == '/') && xml[*idx] != '\0') (*idx)++;
  (*curr_node)->tag = xml__strndup(xml + tag_start, *idx - tag_start);
}

// Parse tag attributes <tag attr="value" ... >
static void xml__parse_tag_attributes(const char *xml, size_t *idx, XMLNode **curr_node) {
  xml__skip_whitespace(xml, idx);
  while (xml[*idx] != '\0' && !(xml[*idx] == '>' || xml[*idx] == '/')) {
    size_t attr_start = *idx;
    while (xml[*idx] != '\0' && xml[*idx] != '=' && !isspace(xml[*idx])) (*idx)++;
    if (*idx == attr_start) {
      while (xml[*idx] != '\0' && xml[*idx] != '>' && xml[*idx] != '/') { (*idx)++; }
      break;
    }
    size_t attr_len = *idx - attr_start;
    char attr_key[attr_len + 1];
    strncpy(attr_key, xml + attr_start, attr_len);
    attr_key[attr_len] = '\0';
    xml__skip_whitespace(xml, idx);
    if (xml[*idx] != '=') break;
    (*idx)++;
    xml__skip_whitespace(xml, idx);
    char quote = xml[*idx];
    if (quote != '"' && quote != '\'') break;
    (*idx)++; // Skip opening quote
    size_t value_start = *idx;
    while (xml[*idx] != '\0') {
      if (xml[*idx] == quote)
        if (*idx == 0 || xml[*idx - 1] != '\\') break;
      (*idx)++;
    }
    if (xml[*idx] == '\0') break;
    size_t value_len = *idx - value_start;
    char attr_value[value_len + 1];
    strncpy(attr_value, xml + value_start, value_len);
    attr_value[value_len] = '\0';
    (*idx)++; // Skip closing quote
    xml_node_add_attr(*curr_node, attr_key, attr_value);
    xml__skip_whitespace(xml, idx);
  }
}

static void xml__parse_tag_inner_text(const char *xml, size_t *idx, XMLNode **curr_node) {
  size_t text_start = *idx;
  while (xml[*idx] != '<' && xml[*idx] != '\0') (*idx)++;
  if (*idx > text_start) (*curr_node)->text = xml__decode_entities(xml + text_start, *idx - text_start);
}

// Parse start tag.
// Returns false if the tag is self-closing like: <tag />
// Call continue if returns false.
static bool xml__parse_tag(const char *xml, size_t *idx, XMLNode **curr_node) {
  xml__skip_whitespace(xml, idx);
  if (xml__skip_tags(xml, idx)) return false;
  // End tag </tag>
  if (xml[*idx] == '/') {
    xml__parse_end_tag(xml, idx, curr_node);
    return false;
  }
  // Create new node with current curr_node as parent
  *curr_node = xml_node_new(*curr_node, NULL, NULL);
  // Start tag <tag...>
  xml__parse_tag_name(xml, idx, curr_node);
  // Parse attributes
  if (xml[*idx] != '\0' && isspace(xml[*idx])) xml__parse_tag_attributes(xml, idx, curr_node);
  // Self-closing tag <tag ... />
  if (xml[*idx] == '/') {
    (*idx)++; // Skip '/'
    while (xml[*idx] != '>' && xml[*idx] != '\0') (*idx)++;
    if (xml[*idx] == '\0') return false;
    (*idx)++; // Consume '>'
    *curr_node = (*curr_node)->parent;
    return false;
  }
  // Start tag <tag ... >
  else if (xml[*idx] == '>') {
    (*idx)++; // Consume '>'
    xml__skip_whitespace(xml, idx);
    xml__parse_tag_inner_text(xml, idx, curr_node);
    // If the next character is '<', parse the next tag
    if (xml[*idx] == '<') {
      (*idx)++; // Consume '<'
      return xml__parse_tag(xml, idx, curr_node);
    }
    return true;
  }
  xml__skip_whitespace(xml, idx);
  return true;
}

XML_H_API XMLNode *xml_parse_string(const char *xml) {
  XMLNode *root = xml_node_new(NULL, NULL, NULL);
  XMLNode *curr_node = root;
  size_t idx = 0;
  while (xml[idx] != '\0') {
    xml__skip_whitespace(xml, &idx);
    // Parse tag
    if (xml[idx] == '<') {
      idx++;
      xml__skip_whitespace(xml, &idx);
      if (xml__skip_tags(xml, &idx)) continue;
      if (!xml__parse_tag(xml, &idx, &curr_node)) continue;
    }
    idx++;
  }
  return root;
}

XML_H_API XMLNode *xml_parse_file(const char *path) {
  FILE *file = fopen(path, "rb");
  if (!file) return NULL;
  fseek(file, 0, SEEK_END);
  size_t file_size = ftell(file);
  fseek(file, 0, SEEK_SET);
  char *buffer = (char *)XML_CALLOC_FUNC(1, file_size + 1);
  if (!buffer) {
    fclose(file);
    return NULL;
  }
  size_t bytes_read = fread(buffer, 1, file_size, file);
  if (bytes_read != file_size) {
    XML_FREE(buffer);
    fclose(file);
    return NULL;
  }
  buffer[file_size] = '\0';
  fclose(file);
  XMLNode *node = xml_parse_string(buffer);
  XML_FREE(buffer);
  return node;
}

XML_H_API void xml_node_serialize(XMLNode *node, XMLString *str) {
  if (!node || !str) return;

  if (node->tag) {
    xml_string_append(str, "<");
    xml_string_append(str, node->tag);
    // Attributes
    for (size_t i = 0; i < node->attrs->len; ++i) {
      XMLAttr *attr = node->attrs->data[i];
      xml_string_append(str, " ");
      xml_string_append(str, attr->key);
      xml_string_append(str, "=\"");
      xml_string_append(str, attr->value);
      xml_string_append(str, "\"");
    }
    // Self-closing case
    if (node->children->len == 0 && !node->text) {
      xml_string_append(str, "/>");
      return;
    }
    xml_string_append(str, ">");
  }
  // Text
  if (node->text) xml_string_append(str, node->text);
  // Children
  for (size_t i = 0; i < node->children->len; ++i) {
    XMLNode *child = node->children->data[i];
    xml_node_serialize(child, str);
  }
  // Closing tag
  if (node->tag) {
    xml_string_append(str, "</");
    xml_string_append(str, node->tag);
    xml_string_append(str, ">");
  }
}

XML_H_API void xml_node_free(XMLNode *node) {
  if (!node) return;
  // Free the text
  XML_FREE(node->text);
  // Free the attributes
  for (size_t i = 0; i < node->attrs->len; i++) {
    XMLAttr *attr = (XMLAttr *)node->attrs->data[i];
    XML_FREE(attr->key);
    XML_FREE(attr->value);
    XML_FREE(attr);
  }
  XML_FREE(node->attrs->data);
  XML_FREE(node->attrs);
  // Recursively free the children
  for (size_t i = 0; i < node->children->len; i++) xml_node_free((XMLNode *)node->children->data[i]);
  XML_FREE(node->children->data);
  XML_FREE(node->children);
  // Free the tag
  XML_FREE(node->tag);
  // Free the node itself
  XML_FREE(node);
}

#endif // XML_H_IMPLEMENTATION

/*

CHANGELOG:

2.1:
    Removed:
        - XML_STRDUP_FUNC (POSIX function. Replaced with xml__strdup() implementation)
        - XML_STRNDUP_FUNC (POSIX function. Replaced with xml__strndup() implementation)
        - Private trim_text() function.

    Added:
        - xml_string_steal()

    Added namespacing to internal functions:
        - SKIP_WHITESPACE -> xml__skip_whitespace()
        - skip_tags() -> xml__skip_tags()
        - parse_end_tag() -> xml__parse_end_tag()
        - parse_tag_name() -> xml__parse_tag_name()
        - parse_tag_attributes() -> xml__parse_tag_attributes()
        - parse_tag_inner_text() -> xml__parse_tag_inner_text()
        - parse_tag() -> xml__parse_tag()

    Fixed:
        - <!DOCTYPE ... > is ignored now too
        - Multiple OOB-read and NULL-dereference bugs

2.0:
    Breaking API changes:
        `xml_node_find_by_path()` functionality merged into `xml_node_find_tag()`.
        Use `xml_node_find_tag()` everywhere `xml_node_find_by_path()` is used.

    Removed:
        - xml_node_find_by_path()

    Added:
        - Redefinable macros to change allocation functions to your own:
            - XML_CALLOC_FUNC
            - XML_REALLOC_FUNC
            - XML_STRDUP_FUNC
            - XML_STRNDUP_FUNC
            - XML_FREE_FUNC

1.0: First release

*/
