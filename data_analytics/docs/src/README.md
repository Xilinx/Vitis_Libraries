Document Source Code
=========================

## Generating HTML Document

### Environment Variables

The following environment variables must be set before using the Makefile
to compile the document:

+ **HTML_DEST_DIR** points to the directory in which the HTML will be
  installed.
+ **PATH** should include the directory with `doxyrest` binary.

### Makefile Targets

The top Makefile contains the following targets, each depending on the previous
one.

+ **xml**: calls `doxygen` to extract the inline doc from C++ headers.
+ **rst**: calls `doxyrest` to yield reStructuredText files from XML files.
+ **html**: calls `sphinx` to generate HTML files from generated reStructuredText
  files and manually written ones.
+ **install**: copies the HTML files to `$HTML_DEST_DIR`.
