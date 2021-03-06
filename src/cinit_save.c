/* -*- C -*- */
/* I hate this bloody country. Smash. */
/* This file is part of Metaresc project */

#include <stdio.h>
#include <stdarg.h>
#include <limits.h>

#include <metaresc.h>
#include <mr_stringify.h>

TYPEDEF_STRUCT (mr_save_type_data_t, ATTRIBUTES ( , "serialization of the node"),
		(char *, named_field_template, , "statically allocated string"),
		(char *, prefix, , "statically allocated string"),
		(char *, content, , "dynamically formed string. Need to be freed."),
		(char *, suffix, , "statically allocated string"),
		)

TYPEDEF_FUNC (int, cinit_json_save_handler_t, (int, mr_ra_mr_ptrdes_t *, mr_save_type_data_t *))

#define MR_CINIT_NULL "NULL"
#define MR_CINIT_INDENT_SPACES (2)
#define MR_CINIT_INDENT_TEMPLATE "%*s"
#define MR_CINIT_INDENT "  "

#define MR_CINIT_TYPE_NAME_TEMPLATE "%s"
#define MR_JSON_TYPE_NAME_TEMPLATE MR_CINIT_TYPE_NAME_TEMPLATE
#define MR_CINIT_FIELDS_DELIMITER ",\n"
#define MR_CINIT_NAMED_FIELD_TEMPLATE ".%s = "
#define MR_JSON_NAMED_FIELD_TEMPLATE "\"%s\" : "
#define MR_CINIT_ATTR_INT "/* %s = %" SCNd32 " */ "

#define MR_CINIT_UNNAMED_FIELDS (0)
#define MR_CINIT_NAMED_FIELDS (!MR_CINIT_UNNAMED_FIELDS)

static int cinit_named_node[MR_TYPE_LAST] = {
  [0 ... MR_TYPE_LAST - 1] = MR_CINIT_UNNAMED_FIELDS,
  [MR_TYPE_STRUCT] = MR_CINIT_NAMED_FIELDS,
  [MR_TYPE_UNION] = MR_CINIT_NAMED_FIELDS,
  [MR_TYPE_ANON_UNION] = MR_CINIT_NAMED_FIELDS,
  [MR_TYPE_NAMED_ANON_UNION] = MR_CINIT_NAMED_FIELDS,
};

/**
 * MR_NONE type saving handler.
 * @param idx an index of node in ptrs
 * @param ptrs resizeable array with pointers descriptors
 * @param data structure that argregates evrything required for saving
 * @return status
 */
static int
cinit_save_none (int idx, mr_ra_mr_ptrdes_t * ptrs, mr_save_type_data_t * data)
{
  return (!0);
}

/**
 * MR_XXX type saving handler. Make a string from *(XXX_t*)data.
 * \@param idx an index of node in ptrs
 * \@param ptrs resizeable array with pointers descriptors
 * \@param data structure that argregates evrything required for saving
 * \@return status
 */
#define CINIT_SAVE_TYPE(TYPE, EXT...) static int cinit_save_ ## TYPE (int idx, mr_ra_mr_ptrdes_t * ptrs, mr_save_type_data_t * data) { data->content = mr_stringify_ ## TYPE (&ptrs->ra.data[idx] EXT); return (0); }

CINIT_SAVE_TYPE (bool);
CINIT_SAVE_TYPE (int8_t);
CINIT_SAVE_TYPE (uint8_t);
CINIT_SAVE_TYPE (int16_t);
CINIT_SAVE_TYPE (uint16_t);
CINIT_SAVE_TYPE (int32_t);
CINIT_SAVE_TYPE (uint32_t);
CINIT_SAVE_TYPE (int64_t);
CINIT_SAVE_TYPE (uint64_t);
CINIT_SAVE_TYPE (enum);
CINIT_SAVE_TYPE (float);
CINIT_SAVE_TYPE (complex_float);
CINIT_SAVE_TYPE (double);
CINIT_SAVE_TYPE (complex_double);
CINIT_SAVE_TYPE (long_double_t);
CINIT_SAVE_TYPE (complex_long_double_t);
CINIT_SAVE_TYPE (bitfield);
CINIT_SAVE_TYPE (bitmask, , MR_BITMASK_OR_DELIMITER);

#define ESC_CHAR_MAP_SIZE (1 << CHAR_BIT)
static int map[ESC_CHAR_MAP_SIZE] = {
  [0 ... ESC_CHAR_MAP_SIZE - 1] = -1,
  [(unsigned char)'\f'] = (unsigned char)'f',
  [(unsigned char)'\n'] = (unsigned char)'n',
  [(unsigned char)'\r'] = (unsigned char)'r',
  [(unsigned char)'\t'] = (unsigned char)'t',
  [(unsigned char)'\v'] = (unsigned char)'v',
  [(unsigned char)'\\'] = (unsigned char)'\\',
};

/**
 * Quote string.
 * @param str string pointer
 * @param quote character for quote
 * @return quoted string
 */
static char *
cinit_quote_string (char * str, char quote)
{
  int length = 0;
  char * str_;
  char * ptr;

  for (ptr = str; *ptr; ++ptr)
    {
      if ((quote == *ptr) || (map[(unsigned char)*ptr] >= 0))
	length += 2;
      else if (isprint (*ptr))
	length += 1;
      else
	length += 4;
    }
  str_ = MR_MALLOC (length + 3);
  if (NULL == str_)
    {
      MR_MESSAGE (MR_LL_FATAL, MR_MESSAGE_OUT_OF_MEMORY);
      return (NULL);
    }
  length = 0;
  str_[length++] = quote;
  for (ptr = str; *ptr; ++ptr)
    {
      if (quote == *ptr)
	{
	  str_[length++] = '\\';
	  str_[length++] = *ptr;
	}
      else if (map[(unsigned char)*ptr] >= 0)
	{
	  str_[length++] = '\\';
	  str_[length++] = map[(unsigned char)*ptr];
	}
      else if (isprint (*ptr))
	str_[length++] = *ptr;
      else
	{
	  sprintf (&str_[length], CINIT_CHAR_QUOTE, (int)(unsigned char)*ptr);
	  length += strlen (&str_[length]);
	}
    }
  str_[length++] = quote;
  str_[length++] = 0;
  return (str_);
}

/**
 * MR_CHAR type saving handler. Stringify char.
 * @param idx an index of node in ptrs
 * @param ptrs resizeable array with pointers descriptors
 * @param data structure that argregates evrything required for saving
 * @return status
 */
static int
cinit_save_char (int idx, mr_ra_mr_ptrdes_t * ptrs, mr_save_type_data_t * data)
{
  char str[] = " ";
  str[0] = *(char*)ptrs->ra.data[idx].data;
  if (0 == str[0])
    data->content = MR_STRDUP ("'\\000'");
  else
    data->content = cinit_quote_string (str, '\'');
  return (0);
}

/**
 * MR_CHAR_ARRAY type saving handler. Saves char array.
 * @param idx an index of node in ptrs
 * @param ptrs resizeable array with pointers descriptors
 * @param data structure that argregates evrything required for saving
 * @return status
 */
static int
cinit_save_char_array (int idx, mr_ra_mr_ptrdes_t * ptrs, mr_save_type_data_t * data)
{
  data->content = cinit_quote_string (ptrs->ra.data[idx].data, '"');
  return (0);
}

/**
 * MR_STRING type saving handler. Saves string.
 * @param idx an index of node in ptrs
 * @param ptrs resizeable array with pointers descriptors
 * @param data structure that argregates evrything required for saving
 * @return status
 */
static int
cinit_save_string (int idx, mr_ra_mr_ptrdes_t * ptrs, mr_save_type_data_t * data)
{
  char * str = *(char**)ptrs->ra.data[idx].data;
  if ((ptrs->ra.data[idx].flags.is_null) || (ptrs->ra.data[idx].ref_idx >= 0))
    data->content = MR_STRDUP (MR_CINIT_NULL);
  else
    data->content = cinit_quote_string (str, '"');
  return (0);
}

/**
 * MR_STRING type saving handler. Saves string.
 * @param idx an index of node in ptrs
 * @param ptrs resizeable array with pointers descriptors
 * @param data structure that argregates evrything required for saving
 * @return status
 */
static int
cinit_save_func (int idx, mr_ra_mr_ptrdes_t * ptrs, mr_save_type_data_t * data)
{
  char * func_str = mr_stringify_func (&ptrs->ra.data[idx]);
  if (func_str)
    {
      if (MR_TRUE == ptrs->ra.data[idx].flags.is_null)
	{
	  MR_FREE (func_str);
	  data->content = MR_STRDUP (MR_CINIT_NULL);
	}
      else if (isdigit (func_str[0])) /* pointer serialized as int */
	{
	  char * type = (MR_TYPE_FUNC == ptrs->ra.data[idx].fd.mr_type) ? MR_VOIDP_T_STR : ptrs->ra.data[idx].fd.type;
	  char buf[strlen (type) + strlen (func_str) + sizeof ("()")];
	  sprintf (buf, "(%s)%s", type, func_str);
	  MR_FREE (func_str);
	  data->content = MR_STRDUP (buf);
	}
      else
	data->content = func_str;
    }
  return (data->content == NULL);
}

/**
 * MR_STRUCT type saving handler. Saves struct.
 * @param idx an index of node in ptrs
 * @param ptrs resizeable array with pointers descriptors
 * @param data structure that argregates evrything required for saving
 * @return status
 */
static int
cinit_save_struct (int idx, mr_ra_mr_ptrdes_t * ptrs, mr_save_type_data_t * data)
{
  data->prefix = "{\n";
  data->suffix = "}";
  return (0);
}

/**
 * MR_ANON_UNION type saving handler. Saves anonymous unions.
 * @param idx an index of node in ptrs
 * @param ptrs resizeable array with pointers descriptors
 * @param data structure that argregates evrything required for saving
 * @return status
 */
static int
cinit_save_anon_union (int idx, mr_ra_mr_ptrdes_t * ptrs, mr_save_type_data_t * data)
{
  data->prefix = "\"\", {\n";
  data->suffix = "}";
  return (0);
}

/**
 * MR_TYPE_EXT_RARRAY_DATA type saving handler. Saves resizeable array content.
 * @param idx an index of node in ptrs
 * @param ptrs resizeable array with pointers descriptors
 * @param data structure that argregates evrything required for saving
 * @return status
 */
static int
cinit_save_rarray_data (int idx, mr_ra_mr_ptrdes_t * ptrs, mr_save_type_data_t * data)
{
  if ((ptrs->ra.data[idx].flags.is_null) || (ptrs->ra.data[idx].ref_idx >= 0))
    data->content = MR_STRDUP (MR_CINIT_NULL);
  else
    {
      data->prefix = "(" MR_CINIT_TYPE_NAME_TEMPLATE "[]){\n";
      data->suffix = "}";
    }
  return (0);
}

/**
 * MR_TYPE_EXT_POINTER type saving handler. Save pointer as a string.
 * @param idx an index of node in ptrs
 * @param ptrs resizeable array with pointers descriptors
 * @param data structure that argregates evrything required for saving
 * @return status
 */
static int
cinit_save_pointer (int idx, mr_ra_mr_ptrdes_t * ptrs, mr_save_type_data_t * data)
{
  if (ptrs->ra.data[idx].first_child < 0)
    data->content = MR_STRDUP (MR_CINIT_NULL);
  else if ((MR_TYPE_CHAR_ARRAY == ptrs->ra.data[idx].fd.mr_type) && (0 == strcmp ("string_t", ptrs->ra.data[idx].fd.type)))
    {
      data->prefix = "(" MR_CINIT_TYPE_NAME_TEMPLATE "){\n";
      data->suffix = "}";
    }
  else
    {
      data->prefix = "(" MR_CINIT_TYPE_NAME_TEMPLATE "[]){\n";
      data->suffix = "}";
    }
  return (0);
}

/**
 * MR_TYPE_EXT_ARRAY type saving handler. Saves arrays content in JSON.
 * @param idx an index of node in ptrs
 * @param ptrs resizeable array with pointers descriptors
 * @param data structure that argregates evrything required for saving
 * @return status
 */
static int
json_save_array (int idx, mr_ra_mr_ptrdes_t * ptrs, mr_save_type_data_t * data)
{
  data->prefix = "[\n";
  data->suffix = "]";
  return (0);
}

/**
 * MR_TYPE_EXT_RARRAY_DATA type saving handler. Saves resizeable array content in JSON.
 * @param idx an index of node in ptrs
 * @param ptrs resizeable array with pointers descriptors
 * @param data structure that argregates evrything required for saving
 * @return status
 */
static int
json_save_rarray_data (int idx, mr_ra_mr_ptrdes_t * ptrs, mr_save_type_data_t * data)
{
  if ((ptrs->ra.data[idx].flags.is_null) || (ptrs->ra.data[idx].ref_idx >= 0))
    data->content = MR_STRDUP (MR_CINIT_NULL);
  else
    {
      data->prefix = "[\n";
      data->suffix = "]";
    }
  return (0);
}

/**
 * MR_TYPE_EXT_POINTER type saving handler. Saves pointer in JSON.
 * @param idx an index of node in ptrs
 * @param ptrs resizeable array with pointers descriptors
 * @param data structure that argregates evrything required for saving
 * @return status
 */
static int
json_save_pointer (int idx, mr_ra_mr_ptrdes_t * ptrs, mr_save_type_data_t * data)
{
  if (ptrs->ra.data[idx].first_child < 0)
    data->content = MR_STRDUP (MR_CINIT_NULL);
  else
    {
      data->prefix = "/* (" MR_JSON_TYPE_NAME_TEMPLATE "[]) */ [\n";
      data->suffix = "]";
    }
  return (0);
}

/**
 * Init IO handlers Table
 */
static cinit_json_save_handler_t cinit_save_handler[] =
  {
    [MR_TYPE_NONE] = cinit_save_none,
    [MR_TYPE_VOID] = cinit_save_none,
    [MR_TYPE_ENUM] = cinit_save_enum,
    [MR_TYPE_BITFIELD] = cinit_save_bitfield,
    [MR_TYPE_BITMASK] = cinit_save_bitmask,
    [MR_TYPE_BOOL] = cinit_save_bool,
    [MR_TYPE_INT8] = cinit_save_int8_t,
    [MR_TYPE_UINT8] = cinit_save_uint8_t,
    [MR_TYPE_INT16] = cinit_save_int16_t,
    [MR_TYPE_UINT16] = cinit_save_uint16_t,
    [MR_TYPE_INT32] = cinit_save_int32_t,
    [MR_TYPE_UINT32] = cinit_save_uint32_t,
    [MR_TYPE_INT64] = cinit_save_int64_t,
    [MR_TYPE_UINT64] = cinit_save_uint64_t,
    [MR_TYPE_FLOAT] = cinit_save_float,
    [MR_TYPE_COMPLEX_FLOAT] = cinit_save_complex_float,
    [MR_TYPE_DOUBLE] = cinit_save_double,
    [MR_TYPE_COMPLEX_DOUBLE] = cinit_save_complex_double,
    [MR_TYPE_LONG_DOUBLE] = cinit_save_long_double_t,
    [MR_TYPE_COMPLEX_LONG_DOUBLE] = cinit_save_complex_long_double_t,
    [MR_TYPE_CHAR] = cinit_save_char,
    [MR_TYPE_CHAR_ARRAY] = cinit_save_char_array,
    [MR_TYPE_STRING] = cinit_save_string,
    [MR_TYPE_STRUCT] = cinit_save_struct,
    [MR_TYPE_FUNC] = cinit_save_func,
    [MR_TYPE_FUNC_TYPE] = cinit_save_func,
    [MR_TYPE_UNION] = cinit_save_struct,
    [MR_TYPE_ANON_UNION] = cinit_save_struct,
    [MR_TYPE_NAMED_ANON_UNION] = cinit_save_anon_union,
  };

static cinit_json_save_handler_t ext_cinit_save_handler[] =
  {
    [MR_TYPE_EXT_ARRAY] = cinit_save_struct,
    [MR_TYPE_EXT_RARRAY_DATA] = cinit_save_rarray_data,
    [MR_TYPE_EXT_POINTER] = cinit_save_pointer,
  };

static cinit_json_save_handler_t json_save_handler[] =
  {
    [MR_TYPE_NONE] = cinit_save_none,
    [MR_TYPE_VOID] = cinit_save_none,
    [MR_TYPE_ENUM] = cinit_save_enum,
    [MR_TYPE_BITFIELD] = cinit_save_bitfield,
    [MR_TYPE_BITMASK] = cinit_save_bitmask,
    [MR_TYPE_BOOL] = cinit_save_bool,
    [MR_TYPE_INT8] = cinit_save_int8_t,
    [MR_TYPE_UINT8] = cinit_save_uint8_t,
    [MR_TYPE_INT16] = cinit_save_int16_t,
    [MR_TYPE_UINT16] = cinit_save_uint16_t,
    [MR_TYPE_INT32] = cinit_save_int32_t,
    [MR_TYPE_UINT32] = cinit_save_uint32_t,
    [MR_TYPE_INT64] = cinit_save_int64_t,
    [MR_TYPE_UINT64] = cinit_save_uint64_t,
    [MR_TYPE_FLOAT] = cinit_save_float,
    [MR_TYPE_COMPLEX_FLOAT] = cinit_save_complex_float,
    [MR_TYPE_DOUBLE] = cinit_save_double,
    [MR_TYPE_COMPLEX_DOUBLE] = cinit_save_complex_double,
    [MR_TYPE_LONG_DOUBLE] = cinit_save_long_double_t,
    [MR_TYPE_COMPLEX_LONG_DOUBLE] = cinit_save_complex_long_double_t,
    [MR_TYPE_CHAR] = cinit_save_char,
    [MR_TYPE_CHAR_ARRAY] = cinit_save_char_array,
    [MR_TYPE_STRING] = cinit_save_string,
    [MR_TYPE_STRUCT] = cinit_save_struct,
    [MR_TYPE_FUNC] = cinit_save_func,
    [MR_TYPE_FUNC_TYPE] = cinit_save_func,
    [MR_TYPE_UNION] = cinit_save_struct,
    [MR_TYPE_ANON_UNION] = cinit_save_struct,
    [MR_TYPE_NAMED_ANON_UNION] = cinit_save_struct,
  };

static cinit_json_save_handler_t ext_json_save_handler[] =
  {
    [MR_TYPE_EXT_ARRAY] = json_save_array,
    [MR_TYPE_EXT_RARRAY_DATA] = json_save_rarray_data,
    [MR_TYPE_EXT_POINTER] = json_save_pointer,
  };

/**
 * Public function. Save scheduler. Saves any object as a string.
 * Code common for c-init and json.
 * @param ptrs resizeable array with pointers descriptors
 * @param named_field_template template for named field (c-init or json)
 * @param node_handler hook for node serialization
 * @return stringified representation of the object
 */
static char *
cinit_json_save (mr_ra_mr_ptrdes_t * ptrs, int (*node_handler) (mr_fd_t*, int, mr_ra_mr_ptrdes_t*, mr_save_type_data_t*))
{
  mr_rarray_t mr_ra_str = { .data = MR_STRDUP (""), .size = sizeof (""), .alloc_size = sizeof (""), .ext = { NULL }, };
  int idx = 0;

  if (NULL == mr_ra_str.data)
    return (NULL);

  while (idx >= 0)
    {
      int level = 0;
      int skip_node;
      mr_fd_t * fdp = &ptrs->ra.data[idx].fd;
      mr_save_type_data_t save_data = { .named_field_template = NULL, .prefix = NULL, .content = NULL, .suffix = NULL, };

      /* route saving handler */
      skip_node = node_handler (fdp, idx, ptrs, &save_data);
      if (!skip_node)
	{
	  int named_node = MR_CINIT_UNNAMED_FIELDS;
	  level = MR_LIMIT_LEVEL (ptrs->ra.data[idx].level);

	  if (ptrs->ra.data[idx].level > 0)
	    if (mr_ra_printf (&mr_ra_str, MR_CINIT_INDENT_TEMPLATE, level * MR_CINIT_INDENT_SPACES, "") < 0)
	      return (NULL);

	  if ((ptrs->ra.data[idx].parent >= 0) && (MR_TYPE_EXT_NONE == ptrs->ra.data[ptrs->ra.data[idx].parent].fd.mr_type_ext))
	    named_node = cinit_named_node[ptrs->ra.data[ptrs->ra.data[idx].parent].fd.mr_type];

	  if ((MR_CINIT_NAMED_FIELDS == named_node) && (save_data.named_field_template))
	    if (mr_ra_printf (&mr_ra_str, save_data.named_field_template, ptrs->ra.data[idx].fd.name.str) < 0)
	      return (NULL);

	  if (ptrs->ra.data[idx].ref_idx >= 0)
	    if (mr_ra_printf (&mr_ra_str, MR_CINIT_ATTR_INT,
			      (ptrs->ra.data[idx].flags.is_content_reference) ? MR_REF_CONTENT : MR_REF,
			      ptrs->ra.data[ptrs->ra.data[idx].ref_idx].idx) < 0)
	      return (NULL);

	  if (ptrs->ra.data[idx].flags.is_referenced)
	    if (mr_ra_printf (&mr_ra_str, MR_CINIT_ATTR_INT, MR_REF_IDX, ptrs->ra.data[idx].idx) < 0)
	      return (NULL);

	  if (save_data.prefix)
	    if (mr_ra_printf (&mr_ra_str, save_data.prefix, ptrs->ra.data[idx].fd.type) < 0)
	      return (NULL);

	  if (save_data.content)
	    {
	      if (mr_ra_printf (&mr_ra_str, "%s", save_data.content) < 0)
		return (NULL);
	      MR_FREE (save_data.content);
	    }

	  ptrs->ra.data[idx].ext.ptr = save_data.suffix;
	  ptrs->ra.data[idx].ptr_type = "string_t";
	}

      if (ptrs->ra.data[idx].first_child >= 0)
	idx = ptrs->ra.data[idx].first_child;
      else
	{
	  if (save_data.suffix)
	    {
	      level = MR_LIMIT_LEVEL (ptrs->ra.data[idx].level);
	      if (mr_ra_printf (&mr_ra_str, MR_CINIT_INDENT_TEMPLATE, level * MR_CINIT_INDENT_SPACES, "") < 0)
		return (NULL);
	      if (mr_ra_printf (&mr_ra_str, "%s", save_data.suffix) < 0)
		return (NULL);
	    }
	  if (!skip_node && (idx != 0))
	    if (mr_ra_printf (&mr_ra_str, MR_CINIT_FIELDS_DELIMITER) < 0)
	      return (NULL);
	  while ((ptrs->ra.data[idx].next < 0) && (ptrs->ra.data[idx].parent >= 0))
	    {
	      idx = ptrs->ra.data[idx].parent;
	      if (ptrs->ra.data[idx].ext.ptr)
		{
		  level = MR_LIMIT_LEVEL (ptrs->ra.data[idx].level);
		  if (mr_ra_printf (&mr_ra_str, MR_CINIT_INDENT_TEMPLATE, level * MR_CINIT_INDENT_SPACES, "") < 0)
		    return (NULL);
		  if (mr_ra_printf (&mr_ra_str, "%s", (char*)ptrs->ra.data[idx].ext.ptr) < 0)
		    return (NULL);
		  if (idx != 0)
		    if (mr_ra_printf (&mr_ra_str, MR_CINIT_FIELDS_DELIMITER) < 0)
		      return (NULL);
		}
	    }
	  idx = ptrs->ra.data[idx].next;
	}
    }

  return (mr_ra_str.data);
}

static int
cinit_node_handler (mr_fd_t * fdp, int idx, mr_ra_mr_ptrdes_t * ptrs, mr_save_type_data_t * save_data)
{
  int skip_node = 0;
  if ((fdp->mr_type != MR_TYPE_ANON_UNION) && /* anonymous unions should be unnamed for CINIT */
      ((ptrs->ra.data[idx].parent >= 0) && strcmp ("mr_ptr_t", ptrs->ra.data[ptrs->ra.data[idx].parent].fd.type))) /* ugly hack for synthetic type. mr_ptr_t members should be unnamed */
    save_data->named_field_template = MR_CINIT_NAMED_FIELD_TEMPLATE;

  if ((fdp->mr_type_ext < MR_TYPE_EXT_LAST) && ext_cinit_save_handler[fdp->mr_type_ext])
    skip_node = ext_cinit_save_handler[fdp->mr_type_ext] (idx, ptrs, save_data);
  else if ((fdp->mr_type < MR_TYPE_LAST) && cinit_save_handler[fdp->mr_type])
    skip_node = cinit_save_handler[fdp->mr_type] (idx, ptrs, save_data);
  else
    MR_MESSAGE_UNSUPPORTED_NODE_TYPE_ (fdp);
  return (skip_node);
}

char *
cinit_save (mr_ra_mr_ptrdes_t * _ptrs_)
{
  return (cinit_json_save (_ptrs_, cinit_node_handler));
}

static int
json_node_handler (mr_fd_t * fdp, int idx, mr_ra_mr_ptrdes_t * ptrs, mr_save_type_data_t * save_data)
{
  int skip_node = 0;
  save_data->named_field_template = MR_JSON_NAMED_FIELD_TEMPLATE;
  if ((fdp->mr_type_ext < MR_TYPE_EXT_LAST) && ext_json_save_handler[fdp->mr_type_ext])
    skip_node = ext_json_save_handler[fdp->mr_type_ext] (idx, ptrs, save_data);
  else if ((fdp->mr_type < MR_TYPE_LAST) && json_save_handler[fdp->mr_type])
    skip_node = json_save_handler[fdp->mr_type] (idx, ptrs, save_data);
  else
    MR_MESSAGE_UNSUPPORTED_NODE_TYPE_ (fdp);
  return (skip_node);
}

char *
json_save (mr_ra_mr_ptrdes_t * _ptrs_)
{
  return (cinit_json_save (_ptrs_, json_node_handler));
}
