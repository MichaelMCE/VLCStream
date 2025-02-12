/*
 * "$Id: mxml.h 385 2009-03-19 05:38:52Z mike $"
 *
 * Header file for Mini-XML, a small XML-like file parsing library.
 *
 * Copyright 2003-2009 by Michael Sweet.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

// This release of Mini-XML has been modified for Process Hacker in the following ways:
// * Memory allocations are done through Process Hacker's PhAllocate*/PhFree* functions.
// * The file descriptor functions now use file handles.

/*
 * Prevent multiple inclusion...
 */

#ifndef _mxml_h_
#  define _mxml_h_

/*
 * Include necessary headers...
 */

#  include <stdio.h>
#  include <stdlib.h>
#  include <string.h>
#  include <ctype.h>
#  include <errno.h>
#  include <inttypes.h>
#  include <windows.h>


#define MXML_CAN_LOAD		1
#define MXML_CAN_SAX_LOAD	0
#define MXML_CAN_SAVE		0

/*
 * Constants...
 */

#  define MXML_TAB		8	/* Tabs every N columns */

#  define MXML_NO_CALLBACK	0	/* Don't use a type callback */
#  define MXML_INTEGER_CALLBACK	mxml_integer_cb
					/* Treat all data as integers */
#  define MXML_OPAQUE_CALLBACK	mxml_opaque_cb
					/* Treat all data as opaque */
#  define MXML_REAL_CALLBACK	mxml_real_cb
					/* Treat all data as real numbers */
#  define MXML_TEXT_CALLBACK	0	/* Treat all data as text */
#  define MXML_IGNORE_CALLBACK	mxml_ignore_cb
					/* Ignore all non-element content */

#  define MXML_NO_PARENT	0	/* No parent for the node */

#  define MXML_DESCEND		1	/* Descend when finding/walking */
#  define MXML_NO_DESCEND	0	/* Don't descend when finding/walking */
#  define MXML_DESCEND_FIRST	-1	/* Descend for first find */

#  define MXML_WS_BEFORE_OPEN	0	/* Callback for before open tag */
#  define MXML_WS_AFTER_OPEN	1	/* Callback for after open tag */
#  define MXML_WS_BEFORE_CLOSE	2	/* Callback for before close tag */
#  define MXML_WS_AFTER_CLOSE	3	/* Callback for after close tag */

#  define MXML_ADD_BEFORE	0	/* Add node before specified node */
#  define MXML_ADD_AFTER	1	/* Add node after specified node */
#  define MXML_ADD_TO_PARENT	NULL	/* Add node relative to parent */


/*
 * Data types...
 */

typedef enum mxml_sax_event_e		/**** SAX event type. ****/
{
  MXML_SAX_CDATA,			/* CDATA node */
  MXML_SAX_COMMENT,			/* Comment node */
  MXML_SAX_DATA,			/* Data node */
  MXML_SAX_DIRECTIVE,			/* Processing directive node */
  MXML_SAX_ELEMENT_CLOSE,		/* Element closed */
  MXML_SAX_ELEMENT_OPEN			/* Element opened */
} mxml_sax_event_t;

typedef enum mxml_type_e		/**** The XML node type. ****/
{
  MXML_IGNORE = -1,			/* Ignore/throw away node @since Mini-XML 2.3@ */
  MXML_ELEMENT,				/* XML element with attributes */
  MXML_INTEGER,				/* Integer value */
  MXML_OPAQUE,				/* Opaque string */
  MXML_REAL,				/* Real value */
  MXML_TEXT,				/* Text fragment */
  MXML_CUSTOM				/* Custom data @since Mini-XML 2.1@ */
} mxml_type_t;

typedef void (*mxml_custom_destroy_cb_t)(void *);
					/**** Custom data destructor ****/

typedef void (*mxml_error_cb_t)(const char *);  
					/**** Error callback function ****/

typedef struct mxml_attr_s		/**** An XML element attribute value. ****/
{
  char			*name;		/* Attribute name */
  char			*value;		/* Attribute value */
} mxml_attr_t;

typedef struct mxml_element_s		/**** An XML element value. ****/
{
  char			*name;		/* Name of element */
  int			num_attrs;	/* Number of attributes */
  mxml_attr_t		*attrs;		/* Attributes */
} mxml_element_t;

typedef struct mxml_text_s		/**** An XML text value. ****/
{
  int			whitespace;	/* Leading whitespace? */
  char			*string;	/* Fragment string */
} mxml_text_t;

typedef struct mxml_custom_s		/**** An XML custom value. @since Mini-XML 2.1@ ****/
{
  void			*data;		/* Pointer to (allocated) custom data */
  mxml_custom_destroy_cb_t destroy;	/* Pointer to destructor function */
} mxml_custom_t;

typedef union mxml_value_u		/**** An XML node value. ****/
{
  mxml_element_t	element;	/* Element */
  int64_t			integer;	/* Integer number */
  char				*opaque;	/* Opaque string */
  double			real;		/* Real number */
  mxml_text_t		text;		/* Text fragment */
  mxml_custom_t		custom;		/* Custom data @since Mini-XML 2.1@ */
} mxml_value_t;

typedef struct mxml_node_s		/**** An XML node. ****/
{
  mxml_type_t		type;		/* Node type */
  struct mxml_node_s	*next;		/* Next node under same parent */
  struct mxml_node_s	*prev;		/* Previous node under same parent */
  struct mxml_node_s	*parent;	/* Parent node */
  struct mxml_node_s	*child;		/* First child node */
  struct mxml_node_s	*last_child;	/* Last child node */
  mxml_value_t		value;		/* Node value */
  int			ref_count;	/* Use count */
  void			*user_data;	/* User data */
} mxml_node_t;

typedef struct mxml_index_s		/**** An XML node index. ****/
{
  char			*attr;		/* Attribute used for indexing or NULL */
  int			num_nodes;	/* Number of nodes in index */
  int			alloc_nodes;	/* Allocated nodes in index */
  int			cur_node;	/* Current node */
  mxml_node_t		**nodes;	/* Node array */
} mxml_index_t;

typedef int (*mxml_custom_load_cb_t)(mxml_node_t *, const char *);
					/**** Custom data load callback function ****/

typedef char *(*mxml_custom_save_cb_t)(mxml_node_t *);  
					/**** Custom data save callback function ****/

typedef int (*mxml_entity_cb_t)(const char *);
					/**** Entity callback function */

typedef  mxml_type_t (*mxml_load_cb_t)(mxml_node_t *);
					/**** Load callback function ****/

typedef const char *(*mxml_save_cb_t)(mxml_node_t *, int);
					/**** Save callback function ****/

typedef void (*mxml_sax_cb_t)(mxml_node_t *, mxml_sax_event_t, void *);  
					/**** SAX callback function ****/


/*
 * C++ support...
 */

#  ifdef __cplusplus
extern "C" {
#  endif /* __cplusplus */


#if BUILD_DLL
#define MXMLAPI_EXPORT extern 
#else
#define MXMLAPI_EXPORT 
#endif

/*
 * Prototypes...
 */
MXMLAPI_EXPORT void			mxmlAdd(mxml_node_t *parent, int where, mxml_node_t *child, mxml_node_t *node);
MXMLAPI_EXPORT void			mxmlDelete(mxml_node_t *node);
MXMLAPI_EXPORT void			mxmlElementDeleteAttr(mxml_node_t *node, const char *name);
MXMLAPI_EXPORT const char	*mxmlElementGetAttr(mxml_node_t *node, const char *name);
MXMLAPI_EXPORT void			mxmlElementSetAttr(mxml_node_t *node, const char *name, const char *value);
MXMLAPI_EXPORT void			mxmlElementSetAttrf(mxml_node_t *node, const char *name,const char *format, ...)
#    ifdef __GNUC__
__attribute__ ((__format__ (__printf__, 3, 4)))
#    endif /* __GNUC__ */
;


MXMLAPI_EXPORT int			 mxmlEntityAddCallback(mxml_entity_cb_t cb);
MXMLAPI_EXPORT const char	*mxmlEntityGetName(int val);
MXMLAPI_EXPORT int			 mxmlEntityGetValue(const char *name);
MXMLAPI_EXPORT void			 mxmlEntityRemoveCallback(mxml_entity_cb_t cb);
MXMLAPI_EXPORT mxml_node_t	*mxmlFindElement(mxml_node_t *node, mxml_node_t *top, const char *name, const char *attr, const char *value, int descend);
MXMLAPI_EXPORT void			 mxmlIndexDelete(mxml_index_t *ind);
MXMLAPI_EXPORT mxml_node_t	*mxmlIndexEnum(mxml_index_t *ind);
MXMLAPI_EXPORT mxml_node_t	*mxmlIndexFind(mxml_index_t *ind, const char *element, const char *value);
MXMLAPI_EXPORT mxml_index_t	*mxmlIndexNew(mxml_node_t *node, const char *element, const char *attr);
MXMLAPI_EXPORT mxml_node_t	*mxmlIndexReset(mxml_index_t *ind);
MXMLAPI_EXPORT mxml_node_t	*mxmlLoadFd(mxml_node_t *top, HANDLE fd, mxml_type_t (*cb)(mxml_node_t *));
MXMLAPI_EXPORT mxml_node_t	*mxmlLoadFile(mxml_node_t *top, FILE *fp, mxml_type_t (*cb)(mxml_node_t *));
MXMLAPI_EXPORT mxml_node_t	*mxmlLoadString(mxml_node_t *top, const char *s, mxml_type_t (*cb)(mxml_node_t *));
MXMLAPI_EXPORT mxml_node_t	*mxmlNewCDATA(mxml_node_t *parent, const char *string);
MXMLAPI_EXPORT mxml_node_t	*mxmlNewCustom(mxml_node_t *parent, void *data, mxml_custom_destroy_cb_t destroy);
MXMLAPI_EXPORT mxml_node_t	*mxmlNewElement(mxml_node_t *parent, const char *name);
MXMLAPI_EXPORT mxml_node_t	*mxmlNewInteger(mxml_node_t *parent, int64_t integer);
MXMLAPI_EXPORT mxml_node_t	*mxmlNewOpaque(mxml_node_t *parent, const char *opaque);
MXMLAPI_EXPORT mxml_node_t	*mxmlNewReal(mxml_node_t *parent, double real);
MXMLAPI_EXPORT mxml_node_t	*mxmlNewText(mxml_node_t *parent, int whitespace, const char *string);
MXMLAPI_EXPORT mxml_node_t	*mxmlNewTextf(mxml_node_t *parent, int whitespace, const char *format, ...)
#    ifdef __GNUC__
__attribute__ ((__format__ (__printf__, 3, 4)))
#    endif /* __GNUC__ */
;

MXMLAPI_EXPORT mxml_node_t	*mxmlNewXML(const char *version);
MXMLAPI_EXPORT int			mxmlRelease(mxml_node_t *node);
MXMLAPI_EXPORT void			mxmlRemove(mxml_node_t *node);
MXMLAPI_EXPORT int			mxmlRetain(mxml_node_t *node);
MXMLAPI_EXPORT char			*mxmlSaveAllocString(mxml_node_t *node, mxml_save_cb_t cb);
MXMLAPI_EXPORT int			mxmlSaveFd(mxml_node_t *node, HANDLE fd, mxml_save_cb_t cb);
MXMLAPI_EXPORT int			mxmlSaveFile(mxml_node_t *node, FILE *fp, mxml_save_cb_t cb);
MXMLAPI_EXPORT int			mxmlSaveString(mxml_node_t *node, char *buffer, int bufsize, mxml_save_cb_t cb);
MXMLAPI_EXPORT mxml_node_t	*mxmlSAXLoadFd(mxml_node_t *top, HANDLE fd, mxml_type_t (*cb)(mxml_node_t *), mxml_sax_cb_t sax, void *sax_data);
MXMLAPI_EXPORT mxml_node_t	*mxmlSAXLoadFile(mxml_node_t *top, FILE *fp, mxml_type_t (*cb)(mxml_node_t *), mxml_sax_cb_t sax, void *sax_data);
MXMLAPI_EXPORT mxml_node_t	*mxmlSAXLoadString(mxml_node_t *top, const char *s, mxml_type_t (*cb)(mxml_node_t *), mxml_sax_cb_t sax, void *sax_data);
MXMLAPI_EXPORT int			mxmlSetCDATA(mxml_node_t *node, const char *data);
MXMLAPI_EXPORT int			mxmlSetCustom(mxml_node_t *node, void *data, mxml_custom_destroy_cb_t destroy);
MXMLAPI_EXPORT void			mxmlSetCustomHandlers(mxml_custom_load_cb_t load, mxml_custom_save_cb_t save);
MXMLAPI_EXPORT int			mxmlSetElement(mxml_node_t *node, const char *name);
MXMLAPI_EXPORT void			mxmlSetErrorCallback(mxml_error_cb_t cb);
MXMLAPI_EXPORT int			mxmlSetInteger(mxml_node_t *node, int64_t integer);
MXMLAPI_EXPORT int			mxmlSetOpaque(mxml_node_t *node, const char *opaque);
MXMLAPI_EXPORT int			mxmlSetReal(mxml_node_t *node, double real);
MXMLAPI_EXPORT int			mxmlSetText(mxml_node_t *node, int whitespace, const char *string);
MXMLAPI_EXPORT int			mxmlSetTextf(mxml_node_t *node, int whitespace, const char *format, ...)
#    ifdef __GNUC__
__attribute__ ((__format__ (__printf__, 3, 4)))
#    endif /* __GNUC__ */
;

MXMLAPI_EXPORT void			mxmlSetWrapMargin(int column);
MXMLAPI_EXPORT mxml_node_t	*mxmlWalkNext(mxml_node_t *node, mxml_node_t *top, int descend);
MXMLAPI_EXPORT mxml_node_t	*mxmlWalkPrev(mxml_node_t *node, mxml_node_t *top, int descend);


/*
 * Semi-private functions...
 */

MXMLAPI_EXPORT void			mxml_error(const char *format, ...);
MXMLAPI_EXPORT mxml_type_t	mxml_ignore_cb(mxml_node_t *node);
MXMLAPI_EXPORT mxml_type_t	mxml_integer_cb(mxml_node_t *node);
MXMLAPI_EXPORT mxml_type_t	mxml_opaque_cb(mxml_node_t *node);
MXMLAPI_EXPORT mxml_type_t	mxml_real_cb(mxml_node_t *node);


/*
 * C++ support...
 */

#  ifdef __cplusplus
}
#  endif /* __cplusplus */
#endif /* !_mxml_h_ */


/*
 * End of "$Id: mxml.h 385 2009-03-19 05:38:52Z mike $".
 */
