/* -*- C -*- */
/* I hate this bloody country. Smash. */
/* This file is part of Metaresc project */

#ifdef HAVE_CONFIG_H
#include <mr_config.h>
#endif /* HAVE_CONFIG_H */
#include <stdio.h>
#include <string.h>
#ifdef HAVE_DLFCN_H
#define __USE_GNU
#include <dlfcn.h>
#endif /* HAVE_DLFCN_H */

#include <metaresc.h>
#include <mr_stringify.h>
#include <mr_ic.h>
#include <mr_load.h>
#include <mr_value.h>

TYPEDEF_FUNC (mr_status_t, mr_load_handler_t, (int /* idx */, mr_load_data_t * /* mr_load_data */))

/**
 * Post load references setting. If node was marked as references
 * it should be substitude with actual pointer. This substition
 * can't be made during structure loading because of forward references.
 * @param mr_load_data structures that holds context of loading
 * @return Status.
 */
static mr_status_t
mr_set_crossrefs (mr_load_data_t * mr_load_data)
{
  int i;
  int count = mr_load_data->ptrs.ra.size / sizeof (mr_load_data->ptrs.ra.data[0]);
  int max_idx = -1;
  int * table;

  for (i = 0; i < count; ++i)
    if (mr_load_data->ptrs.ra.data[i].idx > max_idx)
      max_idx = mr_load_data->ptrs.ra.data[i].idx;

  if (max_idx < 0)
    return (MR_SUCCESS);

  table = MR_MALLOC (sizeof (table[0]) * (max_idx + 1));
  if (NULL == table)
    {
      MR_MESSAGE (MR_LL_FATAL, MR_MESSAGE_OUT_OF_MEMORY);
      return (MR_FAILURE);
    }

  for (i = 0; i <= max_idx; ++i)
    table[i] = -1;
  for (i = 0; i < count; ++i)
    if (mr_load_data->ptrs.ra.data[i].idx >= 0)
      {
	/* checking indexes collision */
	if (table[mr_load_data->ptrs.ra.data[i].idx] >= 0)
	  MR_MESSAGE (MR_LL_WARN, MR_MESSAGE_IDS_COLLISION, MR_REF_IDX, mr_load_data->ptrs.ra.data[i].idx);
	table[mr_load_data->ptrs.ra.data[i].idx] = i;
      }

  /* set all cross refernces */
  for (i = 0; i < count; ++i)
    if (mr_load_data->ptrs.ra.data[i].ref_idx >= 0)
      {
	int idx = table[mr_load_data->ptrs.ra.data[i].ref_idx];
	if (idx < 0)
	  MR_MESSAGE (MR_LL_WARN, MR_MESSAGE_UNDEFINED_REF_IDX, MR_REF_IDX, mr_load_data->ptrs.ra.data[i].ref_idx);
	else
	  {
	    void * data;
	    if (mr_load_data->ptrs.ra.data[i].flags.is_content_reference)
	      data = *(void**)(mr_load_data->ptrs.ra.data[idx].data);
	    else
	      data = mr_load_data->ptrs.ra.data[idx].data;

	    if ((MR_TYPE_EXT_POINTER == mr_load_data->ptrs.ra.data[i].fd.mr_type_ext) ||
		(MR_TYPE_EXT_RARRAY_DATA == mr_load_data->ptrs.ra.data[i].fd.mr_type_ext) ||
		(MR_TYPE_STRING == mr_load_data->ptrs.ra.data[i].fd.mr_type))
	      *(void**)mr_load_data->ptrs.ra.data[i].data = data;
	  }
      }
  MR_FREE (table);
  return (MR_SUCCESS);
}

/**
 * MR_NONE load handler (dummy)
 * @param idx node index
 * @param mr_load_data structures that holds context of loading
 * @return Status of read
 */
static mr_status_t
mr_load_none (int idx, mr_load_data_t * mr_load_data)
{
  return (MR_SUCCESS);
}

/**
 * MR_INTEGER load handler
 * @param idx node index
 * @param mr_load_data structures that holds context of loading
 * @return Status of read (0 - failure, !0 - success)
 */
mr_status_t
mr_load_integer (int idx, mr_load_data_t * mr_load_data)
{
  mr_ptrdes_t * ptrdes = &mr_load_data->ptrs.ra.data[idx];

  if (MR_SUCCESS != mr_value_cast (MR_VT_INT, &ptrdes->mr_value))
    return (MR_FAILURE);

  switch (ptrdes->fd.mr_type)
    {
    case MR_TYPE_BOOL:
      *(bool*)ptrdes->data = ptrdes->mr_value.vt_int;
      break;
    case MR_TYPE_INT8:
      *(int8_t*)ptrdes->data = ptrdes->mr_value.vt_int;
      break;
    case MR_TYPE_UINT8:
      *(uint8_t*)ptrdes->data = ptrdes->mr_value.vt_int;
      break;
    case MR_TYPE_INT16:
      *(int16_t*)ptrdes->data = ptrdes->mr_value.vt_int;
      break;
    case MR_TYPE_UINT16:
      *(uint16_t*)ptrdes->data = ptrdes->mr_value.vt_int;
      break;
    case MR_TYPE_INT32:
      *(int32_t*)ptrdes->data = ptrdes->mr_value.vt_int;
      break;
    case MR_TYPE_UINT32:
      *(uint32_t*)ptrdes->data = ptrdes->mr_value.vt_int;
      break;
    case MR_TYPE_INT64:
      *(int64_t*)ptrdes->data = ptrdes->mr_value.vt_int;
      break;
    case MR_TYPE_UINT64:
      *(uint64_t*)ptrdes->data = ptrdes->mr_value.vt_int;
      break;

    case MR_TYPE_ENUM:
    case MR_TYPE_BITMASK:
      switch (ptrdes->fd.size)
	{
	case sizeof (uint8_t):
	  *(uint8_t*)ptrdes->data = ptrdes->mr_value.vt_int;
	  break;
	case sizeof (uint16_t):
	  *(uint16_t*)ptrdes->data = ptrdes->mr_value.vt_int;
	  break;
	case sizeof (uint32_t):
	  *(uint32_t*)ptrdes->data = ptrdes->mr_value.vt_int;
	  break;
	case sizeof (uint64_t):
	  *(uint64_t*)ptrdes->data = ptrdes->mr_value.vt_int;
	  break;
	default:
	  memcpy (ptrdes->data, &ptrdes->mr_value.vt_int,
		  MR_MIN (ptrdes->fd.size, sizeof (ptrdes->mr_value.vt_int)));
	  break;
	}
      break;
    default:
      MR_MESSAGE (MR_LL_WARN, MR_MESSAGE_UNEXPECTED_TARGET_TYPE, ptrdes->fd.mr_type);
      return (MR_FAILURE);
    }
  return (MR_SUCCESS);
}

/**
 * MR_FUNC load handler
 * @param idx node index
 * @param mr_load_data structures that holds context of loading
 * @return Status of read (0 - failure, !0 - success)
 */
mr_status_t
mr_load_func (int idx, mr_load_data_t * mr_load_data)
{
  mr_ptrdes_t * ptrdes = &mr_load_data->ptrs.ra.data[idx];
  *(void**)ptrdes->data = NULL;
  switch (ptrdes->mr_value.value_type)
    {
    case MR_VT_INT:
      *(void**)ptrdes->data = (void*)(long)ptrdes->mr_value.vt_int;
      break;
    case MR_VT_STRING:
      *(void**)ptrdes->data = ptrdes->mr_value.vt_string;
      ptrdes->mr_value.vt_string = NULL;
      break;
    case MR_VT_ID:
    case MR_VT_UNKNOWN:
      if (NULL == ptrdes->mr_value.vt_string)
	MR_MESSAGE (MR_LL_WARN, MR_MESSAGE_UNEXPECTED_NULL_POINTER);
      else
	{
	  if (isdigit (ptrdes->mr_value.vt_string[0]))
	    {
	      if (MR_SUCCESS != mr_value_cast (MR_VT_INT, &ptrdes->mr_value))
		return (MR_FAILURE);
	      *(void**)ptrdes->data = (void*)(long)ptrdes->mr_value.vt_int;
	    }
#ifdef HAVE_LIBDL
	  else
	    *(void**)ptrdes->data = dlsym (RTLD_DEFAULT, ptrdes->mr_value.vt_string);
#endif /* HAVE_LIBDL */
	}
      break;
    default:
      MR_MESSAGE (MR_LL_ERROR, MR_MESSAGE_UNEXPECTED_TARGET_TYPE, ptrdes->mr_value.value_type);
      return (MR_FAILURE);
    }
  return (MR_SUCCESS);
}

/**
 * MR_BITFIELD load handler. Load int from string and save it to
 * bit shifted field.
 * @param idx node index
 * @param mr_load_data structures that holds context of loading
 * @return Status of read
 */
static mr_status_t
mr_load_bitfield (int idx, mr_load_data_t * mr_load_data)
{
  mr_ptrdes_t * ptrdes = &mr_load_data->ptrs.ra.data[idx];
  uint64_t value;

  if (MR_SUCCESS != mr_value_cast (MR_VT_INT, &ptrdes->mr_value))
    return (MR_FAILURE);

  value = ptrdes->mr_value.vt_int;
  return (mr_load_bitfield_value (ptrdes, &value));
}

static mr_status_t
mr_load_float (int idx, mr_load_data_t * mr_load_data)
{
  mr_ptrdes_t * ptrdes = &mr_load_data->ptrs.ra.data[idx];
  if (MR_SUCCESS != mr_value_cast (MR_VT_FLOAT, &ptrdes->mr_value))
    return (MR_FAILURE);

  switch (ptrdes->fd.mr_type)
    {
    case MR_TYPE_FLOAT:
      *(float*)ptrdes->data = ptrdes->mr_value.vt_float;
      break;
    case MR_TYPE_DOUBLE:
      *(double*)ptrdes->data = ptrdes->mr_value.vt_float;
      break;
    case MR_TYPE_LONG_DOUBLE:
      *(long double*)ptrdes->data = ptrdes->mr_value.vt_float;
      break;
    default:
      MR_MESSAGE (MR_LL_WARN, MR_MESSAGE_UNEXPECTED_NULL_POINTER);
      return (MR_FAILURE);
    }
  return (MR_SUCCESS);
}

static mr_status_t
mr_load_complex (int idx, mr_load_data_t * mr_load_data)
{
  mr_ptrdes_t * ptrdes = &mr_load_data->ptrs.ra.data[idx];
  if (MR_SUCCESS != mr_value_cast (MR_VT_COMPLEX, &ptrdes->mr_value))
    return (MR_FAILURE);

  switch (ptrdes->fd.mr_type)
    {
    case MR_TYPE_COMPLEX_FLOAT:
      *(complex float*)ptrdes->data = ptrdes->mr_value.vt_complex;
      break;
    case MR_TYPE_COMPLEX_DOUBLE:
      *(complex double*)ptrdes->data = ptrdes->mr_value.vt_complex;
      break;
    case MR_TYPE_COMPLEX_LONG_DOUBLE:
      *(complex long double*)ptrdes->data = ptrdes->mr_value.vt_complex;
      break;
    default:
      MR_MESSAGE (MR_LL_WARN, MR_MESSAGE_UNEXPECTED_NULL_POINTER);
      return (MR_FAILURE);
    }
  return (MR_SUCCESS);
}

/**
 * MR_CHAR load handler. Handles nonprint characters in octal format.
 * @param idx node index
 * @param mr_load_data structures that holds context of loading
 * @return Status of read
 */
static mr_status_t
mr_load_char (int idx, mr_load_data_t * mr_load_data)
{
  mr_status_t status = MR_SUCCESS;
  mr_ptrdes_t * ptrdes = &mr_load_data->ptrs.ra.data[idx];
  
  switch (ptrdes->mr_value.value_type)
    {
    case MR_VT_UNKNOWN:
      if (NULL == ptrdes->mr_value.vt_string)
	status = MR_FAILURE;
      else if (strlen (ptrdes->mr_value.vt_string) <= 1)
	*(char*)ptrdes->data = ptrdes->mr_value.vt_string[0];
      else
	{
	  int32_t code = 0;
	  int size = 0;
	  if (1 != sscanf (ptrdes->mr_value.vt_string, CINIT_CHAR_QUOTE "%n", &code, &size))
	    status = MR_FAILURE;
	  else
	    {
	      while (isspace (ptrdes->mr_value.vt_string[size]))
		++size;
	      if (0 == ptrdes->mr_value.vt_string[size])
		*(char*)ptrdes->data = code;
	      else
		status = MR_FAILURE;
	    }
	}
      
      if (MR_FAILURE == status)
	MR_MESSAGE (MR_LL_ERROR, MR_MESSAGE_READ_CHAR, ptrdes->mr_value.vt_string);
      
      break;
      
    case MR_VT_CHAR:
      *(char*)ptrdes->data = ptrdes->mr_value.vt_char;
      break;
      
    default:
      if (MR_SUCCESS == mr_value_cast (MR_VT_INT, &ptrdes->mr_value))
	*(char*)ptrdes->data = ptrdes->mr_value.vt_int;
      else
	status = MR_FAILURE;
      break;
    }

  return (status);
}

/**
 * MR_STRING load handler. Allocate memory for a string.
 * @param idx node index
 * @param mr_load_data structures that holds context of loading
 * @return Status of read
 */
static mr_status_t
mr_load_string (int idx, mr_load_data_t * mr_load_data)
{
  mr_ptrdes_t * ptrdes = &mr_load_data->ptrs.ra.data[idx];
  if (ptrdes->flags.is_null || (ptrdes->ref_idx >= 0))
    *(char**)ptrdes->data = NULL;
  else
    {
      switch (ptrdes->mr_value.value_type)
	{
	case MR_VT_STRING:
	case MR_VT_UNKNOWN:
	  *(char**)ptrdes->data = ptrdes->mr_value.vt_string;
	  ptrdes->mr_value.vt_string = NULL;
	  break;
	case MR_VT_INT:
	  *(char**)ptrdes->data = (void*)(long)ptrdes->mr_value.vt_int;
	  break;
	default:
	  return (MR_FAILURE);
	}
    }
  return (MR_SUCCESS);
}

/**
 * MR_CHAR_ARRAY load handler.
 * Save string in place (truncate string if needed).
 * @param idx node index
 * @param mr_load_data structures that holds context of loading
 * @return Status of read
 */
static mr_status_t
mr_load_char_array (int idx, mr_load_data_t * mr_load_data)
{
  mr_ptrdes_t * ptrdes = &mr_load_data->ptrs.ra.data[idx];
  int max_size = ptrdes->fd.param.array_param.count * ptrdes->fd.size;
  mr_status_t status = MR_FAILURE;
  
  memset (ptrdes->data, 0, max_size);
  if ((MR_VT_STRING == ptrdes->mr_value.value_type) ||
      (MR_VT_UNKNOWN == ptrdes->mr_value.value_type))
    {
      char * str = ptrdes->mr_value.vt_string;
      if (NULL != str)
	{
	  int str_len = strlen (str);
	  status = MR_SUCCESS;
	  if ((0 == strcmp (ptrdes->fd.type, "string_t")) &&
	      (ptrdes->parent >= 0) &&
	      (MR_TYPE_EXT_POINTER == mr_load_data->ptrs.ra.data[ptrdes->parent].fd.mr_type_ext))
	    {
	      ptrdes->data = MR_REALLOC (ptrdes->data, str_len + 1);
	      *(void**)mr_load_data->ptrs.ra.data[ptrdes->parent].data = ptrdes->data;
	      if (NULL == ptrdes->data)
		{
		  MR_MESSAGE (MR_LL_FATAL, MR_MESSAGE_OUT_OF_MEMORY);
		  status = MR_FAILURE;
		}
	    }
	  else if (str_len >= max_size)
	    {
	      str[max_size - 1] = 0;
	      MR_MESSAGE (MR_LL_WARN, MR_MESSAGE_STRING_TRUNCATED);
	    }
	  if (ptrdes->data)
	    strcpy (ptrdes->data, str);
	}
    }
  else
    MR_MESSAGE (MR_LL_ERROR, MR_MESSAGE_WRONG_RESULT_TYPE);
  return (status);
}

static mr_fd_t *
mr_load_struct_next_field (mr_td_t * tdp, mr_fd_t * fdp)
{
  int i, count = tdp->fields.size / sizeof (tdp->fields.data[0]);

  for (i = 0; i < count; ++i)
    if (NULL == fdp)
      return (tdp->fields.data[i].fdp);
    else if (tdp->fields.data[i].fdp == fdp)
      fdp = NULL;

  return (NULL);
}

/**
 * MR_STRUCT load handler.
 * Save content of subnodes to structure fileds.
 * @param idx node index
 * @param mr_load_data structures that holds context of loading
 * @param tdp type descriptor
 * @return Status of read
 */
static mr_status_t
mr_load_struct_inner (int idx, mr_load_data_t * mr_load_data, mr_td_t * tdp)
{
  char * data = mr_load_data->ptrs.ra.data[idx].data;
  int first_child = mr_load_data->ptrs.ra.data[idx].first_child;
  mr_fd_t * fdp = NULL;

  /* get pointer on structure descriptor */
  if (NULL == tdp)
    {
      MR_MESSAGE (MR_LL_ERROR, MR_MESSAGE_NO_TYPE_DESCRIPTOR, mr_load_data->ptrs.ra.data[idx].fd.type);
      return (MR_FAILURE);
    }

  /* for C init style we can get union descriptor only from type cast */
  if ((0 == strcmp (tdp->type.str, "mr_ptr_t")) && (first_child >= 0) &&
      mr_load_data->ptrs.ra.data[first_child].fd.type && (NULL == mr_load_data->ptrs.ra.data[first_child].fd.name.str))
    {
      mr_load_data->ptrs.ra.data[first_child].fd.name.str = mr_load_data->ptrs.ra.data[first_child].fd.type;
      mr_load_data->ptrs.ra.data[first_child].fd.type = NULL;
    }

  /* loop on all subnodes */
  for (idx = first_child; idx >= 0; idx = mr_load_data->ptrs.ra.data[idx].next)
    {
      if (mr_load_data->ptrs.ra.data[idx].fd.name.str)
	fdp = mr_get_fd_by_name (tdp, mr_load_data->ptrs.ra.data[idx].fd.name.str);
      else
	fdp = mr_load_struct_next_field (tdp, fdp);

      if (NULL == fdp)
	{
	  MR_MESSAGE (MR_LL_ERROR, MR_MESSAGE_UNKNOWN_SUBNODE, tdp->type.str, mr_load_data->ptrs.ra.data[idx].fd.name.str);
	  return (MR_FAILURE);
	}

      /* recursively load subnode */
      if (MR_SUCCESS != mr_load (&data[fdp->offset], fdp, idx, mr_load_data))
	return (MR_FAILURE);
    }
  return (MR_SUCCESS);
}

/**
 * MR_STRUCT load handler. Wrapper over mr_load_struct_inner.
 * @param idx node index
 * @param mr_load_data structures that holds context of loading
 * @return Status of read
 */
static mr_status_t
mr_load_struct (int idx, mr_load_data_t * mr_load_data)
{
  return (mr_load_struct_inner (idx, mr_load_data, mr_get_td_by_name (mr_load_data->ptrs.ra.data[idx].fd.type)));
}

/**
 * MR_ARRAY load handler.
 * Save content of subnodes to array elements.
 * @param idx node index
 * @param mr_load_data structures that holds context of loading
 * @return Status of read
 */
static mr_status_t
mr_load_array (int idx, mr_load_data_t * mr_load_data)
{
  char * data = mr_load_data->ptrs.ra.data[idx].data;
  mr_fd_t fd_ = mr_load_data->ptrs.ra.data[idx].fd;
  int row_count = fd_.param.array_param.row_count;
  int count = fd_.param.array_param.count;
  int i = 0;

  if (1 == fd_.param.array_param.row_count)
    fd_.mr_type_ext = MR_TYPE_EXT_NONE; /* prepare copy of filed descriptor for array elements loading */
  else
    {
      fd_.param.array_param.count = row_count;
      fd_.param.array_param.row_count = 1;
    }

  /* loop on subnodes */
  for (idx = mr_load_data->ptrs.ra.data[idx].first_child; idx >= 0; idx = mr_load_data->ptrs.ra.data[idx].next)
    {
      /* check if array index is in range */
      if ((i < 0) || (i >= count))
	{
	  MR_MESSAGE (MR_LL_ERROR, MR_MESSAGE_RANGE_CHECK, fd_.name.str);
	  return (MR_FAILURE);
	}
      /* load recursively */
      if (MR_SUCCESS != mr_load (&data[i * fd_.size], &fd_, idx, mr_load_data))
	return (MR_FAILURE);
      i += row_count;
    }
  return (MR_SUCCESS);
}

static mr_status_t
mr_load_rarray_data (int idx, mr_load_data_t * mr_load_data)
{
  char * ra_data = mr_load_data->ptrs.ra.data[idx].data;
  mr_rarray_t * ra = (void*)&ra_data[-offsetof (mr_rarray_t, data)];
  int i, count = 0;
  mr_fd_t fd_ = mr_load_data->ptrs.ra.data[idx].fd;

  for (i = mr_load_data->ptrs.ra.data[idx].first_child; i >= 0; i = mr_load_data->ptrs.ra.data[i].next)
    ++count;
  fd_.mr_type_ext = MR_TYPE_EXT_NONE;
  fd_.name = mr_load_data->ptrs.ra.data[mr_load_data->ptrs.ra.data[idx].parent].fd.name;

  if (mr_load_data->ptrs.ra.data[idx].ref_idx >= 0)
    return (MR_SUCCESS);
  
  if (0 == count)
    {
      ra->alloc_size = ra->size = 0;
      ra->data = NULL;
    }
  else
    {
      ra->alloc_size = ra->size = count * fd_.size;
      ra->data = MR_MALLOC (ra->size);
      if (NULL == ra->data)
	{
	  ra->alloc_size = ra->size = 0;
	  ra->data = NULL;
	  MR_MESSAGE (MR_LL_FATAL, MR_MESSAGE_OUT_OF_MEMORY);
	  return (MR_FAILURE);
	}
      memset (ra->data, 0, ra->size);

      /* loop on subnodes */
      i = 0;
      for (idx = mr_load_data->ptrs.ra.data[idx].first_child; idx >= 0; idx = mr_load_data->ptrs.ra.data[idx].next)
	if (MR_SUCCESS != mr_load (&((char*)ra->data)[fd_.size * i++], &fd_, idx, mr_load_data))
	  return (MR_FAILURE);
    }
  return (MR_SUCCESS);
}

mr_status_t
mr_load_rarray_type (mr_fd_t * fdp, mr_status_t (*action) (mr_td_t *, void *), void * context)
{
  mr_td_t * tdp = mr_get_td_by_name ("mr_rarray_t");
  mr_status_t status = MR_FAILURE;
  
  if (NULL == tdp)
    MR_MESSAGE (MR_LL_ERROR, MR_MESSAGE_NO_TYPE_DESCRIPTOR, "mr_rarray_t");
  else
    {
      mr_td_t td = *tdp;
      int fields_count = td.fields.size / sizeof (td.fields.data[0]);
      mr_fd_ptr_t fields_data[fields_count];
      mr_fd_t * data_fdp;
      mr_fd_t fd;
      int i;

      memcpy (fields_data, td.fields.data, td.fields.size);
      td.fields.data = fields_data;
      for (i = 0; i < fields_count; ++i)
	{
	  data_fdp = fields_data[i].fdp;
	  if (0 == strcmp ("data", data_fdp->name.str))
	    break;
	}
      if (i >= fields_count)
	MR_MESSAGE (MR_LL_ERROR, MR_MESSAGE_RARRAY_FAILED);
      else
	{
	  fd = *fdp; /* make a copy of 'data' field descriptor */
	  fd.mr_type_ext = MR_TYPE_EXT_RARRAY_DATA;
	  fd.name = data_fdp->name;
	  fd.offset = data_fdp->offset;
	  fields_data[i].fdp = &fd; /* replace 'data' descriptor on a local copy */
	  mr_ic_new (&td.lookup_by_name, mr_fd_name_get_hash, mr_fd_name_cmp, "mr_fd_t", MR_IC_NONE);
	  mr_ic_index (&td.lookup_by_name, (mr_ic_rarray_t*)(void*)&td.fields, NULL);
	  status = action (&td, context);
	  mr_ic_free (&td.lookup_by_name, NULL);
	}
    }
  return (status);
}

TYPEDEF_STRUCT (mr_load_rarray_struct_t,
		int idx,
		(mr_load_data_t *, mr_load_data)
		)

static mr_status_t
mr_load_rarray_inner (mr_td_t * tdp, void * context)
{
  mr_load_rarray_struct_t * mr_load_rarray_struct = context;
  return (mr_load_struct_inner (mr_load_rarray_struct->idx, mr_load_rarray_struct->mr_load_data, tdp));
}

/**
 * MR_RARRAY load handler.
 * Save content of subnodes to resizeable array elements
 * (allocate/reallocate required memory).
 * @param idx node index
 * @param mr_load_data structures that holds context of loading
 * @return Status of read
 */
static mr_status_t
mr_load_rarray (int idx, mr_load_data_t * mr_load_data)
{
  mr_rarray_t * ra = mr_load_data->ptrs.ra.data[idx].data;
  mr_load_rarray_struct_t mr_load_rarray_struct = {
    .idx = idx,
    .mr_load_data = mr_load_data,
  };

  memset (ra, 0, sizeof (*ra));

  if (MR_SUCCESS != mr_load_rarray_type (&mr_load_data->ptrs.ra.data[idx].fd, mr_load_rarray_inner, &mr_load_rarray_struct))
    return (MR_FAILURE);
  ra->alloc_size = ra->size;

  return (MR_SUCCESS);
}

/**
 * MR_TYPE_EXT_POINTER load handler. Initiated as postponed call thru mr_load_pointer via stack.
 * Loads element into newly allocate memory.
 * @param idx node index
 * @param mr_load_data structures that holds context of loading
 * @return Status of read (0 - failure, !0 - success)
 */
static mr_status_t
mr_load_pointer_postponed (int idx, mr_load_data_t * mr_load_data)
{
  void ** data = mr_load_data->ptrs.ra.data[idx].data;
  mr_fd_t fd_ = mr_load_data->ptrs.ra.data[idx].fd;
  fd_.mr_type_ext = MR_TYPE_EXT_NONE;
  /* allocate memory */
  *data = MR_MALLOC (fd_.size);
  if (NULL == *data)
    {
      MR_MESSAGE (MR_LL_FATAL, MR_MESSAGE_OUT_OF_MEMORY);
      return (MR_FAILURE);
    }
  memset (*data, 0, fd_.size);
  /* load recursively */
  return (mr_load (*data, &fd_, mr_load_data->ptrs.ra.data[idx].first_child, mr_load_data));
}

/**
 * MR_POINTER_STRUCT load handler. Schedule element postponed loading via stack.
 * @param idx node index
 * @param mr_load_data structures that holds context of loading
 * @return Status of read
 */
static mr_status_t
mr_load_pointer (int idx, mr_load_data_t * mr_load_data)
{
  void ** data = mr_load_data->ptrs.ra.data[idx].data;
  /* default initializer */
  *data = NULL;
  if (mr_load_data->ptrs.ra.data[idx].ref_idx >= 0)
    return (MR_SUCCESS);
  if (mr_load_data->ptrs.ra.data[idx].first_child < 0)
    return (MR_SUCCESS);
  /* check whether pointer should have offsprings or not */
  if ((MR_TYPE_NONE != mr_load_data->ptrs.ra.data[idx].fd.mr_type) && (MR_TYPE_VOID != mr_load_data->ptrs.ra.data[idx].fd.mr_type))
    {
      int * idx_ = mr_rarray_append ((void*)&mr_load_data->mr_ra_idx, sizeof (mr_load_data->mr_ra_idx.data[0]));
      if (NULL == idx_)
	return (MR_FAILURE);
      *idx_ = idx;
      return (MR_SUCCESS);
    }

  return (MR_SUCCESS);
}

/**
 * MR_ANON_UNION load handler.
 * Load anonymous union
 * @param idx node index
 * @param mr_load_data structures that holds context of loading
 * @return Status of read
 */
static mr_status_t
mr_load_anon_union (int idx, mr_load_data_t * mr_load_data)
{
  mr_ptrdes_t * ptrdes = &mr_load_data->ptrs.ra.data[idx];
  /*
    Anonimous unions in C init style saved as named field folowed by union itself. Named field has type of zero length static string and must be inited by empty string. Here is an example.
    .anon_union_79 = "", {
    .union_float = 3.1415927410,
    },
  */
  if ((ptrdes->first_child < 0) && /* if node has no childs, then it is C init style anonumous union */
      (MR_VT_STRING == ptrdes->mr_value.value_type) && (NULL != ptrdes->mr_value.vt_string)
      && (0 == ptrdes->mr_value.vt_string[0]) && /* content must be an empty string */
      (ptrdes->next >= 0) && (NULL == mr_load_data->ptrs.ra.data[ptrdes->next].fd.name.str)) /* there should be a next node without name */
    {
      if (mr_load_data->ptrs.ra.data[idx].fd.name.str) /* sainity check - this field can't be NULL */
	mr_load_data->ptrs.ra.data[ptrdes->next].fd.name.str = MR_STRDUP (mr_load_data->ptrs.ra.data[idx].fd.name.str);
      return (MR_SUCCESS); /* now next node has a name and will be loaded by top level procedure */
    }
  return (mr_load_struct (idx, mr_load_data));
}

/**
 * Cleanup helper. Deallocates all dynamically allocated resources.
 * @param ptrs resizeable array with pointers descriptors
 * @return Status of read
 */
mr_status_t
mr_free_ptrs (mr_ra_mr_ptrdes_t ptrs)
{
  if (ptrs.ra.data)
    {
      int count = ptrs.ra.size / sizeof (ptrs.ra.data[0]);
      int i;
      for (i = 0; i < count; ++i)
	{
	  if (ptrs.ra.data[i].fd.type)
	    MR_FREE (ptrs.ra.data[i].fd.type);
	  ptrs.ra.data[i].fd.type = NULL;
	  if (ptrs.ra.data[i].fd.name.str)
	    MR_FREE (ptrs.ra.data[i].fd.name.str);
	  ptrs.ra.data[i].fd.name.str = NULL;
	  if ((MR_VT_UNKNOWN == ptrs.ra.data[i].mr_value.value_type)
	      || (MR_VT_STRING == ptrs.ra.data[i].mr_value.value_type)
	      || (MR_VT_ID == ptrs.ra.data[i].mr_value.value_type))
	    {
	      if (ptrs.ra.data[i].mr_value.vt_string)
		MR_FREE (ptrs.ra.data[i].mr_value.vt_string);
	      ptrs.ra.data[i].mr_value.vt_string = NULL;
	    }
	}
      MR_FREE (ptrs.ra.data);
      ptrs.ra.data = NULL;
      ptrs.ra.size = ptrs.ra.alloc_size = 0;
    }
  return (MR_SUCCESS);
}

/**
 * Init IO handlers Table
 */
static mr_load_handler_t mr_load_handler[] =
  {
    [MR_TYPE_NONE] = mr_load_none,
    [MR_TYPE_VOID] = mr_load_none,
    [MR_TYPE_ENUM] = mr_load_integer,
    [MR_TYPE_BITFIELD] = mr_load_bitfield,
    [MR_TYPE_BITMASK] = mr_load_integer,
    [MR_TYPE_BOOL] = mr_load_integer,
    [MR_TYPE_INT8] = mr_load_integer,
    [MR_TYPE_UINT8] = mr_load_integer,
    [MR_TYPE_INT16] = mr_load_integer,
    [MR_TYPE_UINT16] = mr_load_integer,
    [MR_TYPE_INT32] = mr_load_integer,
    [MR_TYPE_UINT32] = mr_load_integer,
    [MR_TYPE_INT64] = mr_load_integer,
    [MR_TYPE_UINT64] = mr_load_integer,
    [MR_TYPE_FLOAT] = mr_load_float,
    [MR_TYPE_COMPLEX_FLOAT] = mr_load_complex,
    [MR_TYPE_DOUBLE] = mr_load_float,
    [MR_TYPE_COMPLEX_DOUBLE] = mr_load_complex,
    [MR_TYPE_LONG_DOUBLE] = mr_load_float,
    [MR_TYPE_COMPLEX_LONG_DOUBLE] = mr_load_complex,
    [MR_TYPE_CHAR] = mr_load_char,
    [MR_TYPE_CHAR_ARRAY] = mr_load_char_array,
    [MR_TYPE_STRING] = mr_load_string,
    [MR_TYPE_STRUCT] = mr_load_struct,
    [MR_TYPE_FUNC] = mr_load_func,
    [MR_TYPE_FUNC_TYPE] = mr_load_func,
    [MR_TYPE_UNION] = mr_load_struct,
    [MR_TYPE_ANON_UNION] = mr_load_struct,
    [MR_TYPE_NAMED_ANON_UNION] = mr_load_anon_union,
  };

static mr_load_handler_t mr_ext_load_handler[] =
  {
    [MR_TYPE_EXT_ARRAY] = mr_load_array,
    [MR_TYPE_EXT_RARRAY] = mr_load_rarray,
    [MR_TYPE_EXT_RARRAY_DATA] = mr_load_rarray_data,
    [MR_TYPE_EXT_POINTER] = mr_load_pointer,
  };

/**
 * Public function. Load router. Load any object from internal representation graph.
 * @param data pointer on place to save data
 * @param fdp filed descriptor
 * @param idx node index
 * @param mr_load_data structures that holds context of loading
 * @return Status of read
 */
mr_status_t
mr_load (void * data, mr_fd_t * fdp, int idx, mr_load_data_t * mr_load_data)
{
  mr_status_t status = MR_FAILURE;

  if (0 == idx)
    {
      mr_load_data->mr_ra_idx.data = NULL;
      mr_load_data->mr_ra_idx.size = 0;
      mr_load_data->mr_ra_idx.alloc_size = 0;
    }

  if ((idx < 0) || (idx >= mr_load_data->ptrs.ra.size / sizeof (mr_load_data->ptrs.ra.data[0])))
    {
      MR_MESSAGE (MR_LL_ERROR, MR_MESSAGE_SAVE_IDX_RANGE_CHECK);
      return (MR_FAILURE);
    }

  mr_load_data->ptrs.ra.data[idx].data = data;
  if (mr_load_data->ptrs.ra.data[idx].fd.name.str && fdp->name.str)
    if (strcmp (fdp->name.str, mr_load_data->ptrs.ra.data[idx].fd.name.str))
      {
	MR_MESSAGE (MR_LL_ERROR, MR_MESSAGE_NODE_NAME_MISSMATCH, fdp->name.str, mr_load_data->ptrs.ra.data[idx].fd.name.str);
	return (MR_FAILURE);
      }

  if (mr_load_data->ptrs.ra.data[idx].fd.type && fdp->type)
    if (strcmp (fdp->type, mr_load_data->ptrs.ra.data[idx].fd.type))
      if (!((0 == strcmp (MR_VOIDP_T_STR, mr_load_data->ptrs.ra.data[idx].fd.type)) &&
	    (('*' == fdp->type[strlen (fdp->type) - 1]) || (MR_TYPE_FUNC == fdp->mr_type))))
	{
	  MR_MESSAGE (MR_LL_ERROR, MR_MESSAGE_NODE_TYPE_MISSMATCH, fdp->name.str, fdp->type, mr_load_data->ptrs.ra.data[idx].fd.type);
	  return (MR_FAILURE);
	}

  if ((NULL == mr_load_data->ptrs.ra.data[idx].fd.name.str) && (fdp->name.str))
    mr_load_data->ptrs.ra.data[idx].fd.name.str = MR_STRDUP (fdp->name.str);
  if ((NULL == mr_load_data->ptrs.ra.data[idx].fd.type) && (fdp->type))
    mr_load_data->ptrs.ra.data[idx].fd.type = MR_STRDUP (fdp->type);
  mr_load_data->ptrs.ra.data[idx].fd.size = fdp->size;
  mr_load_data->ptrs.ra.data[idx].fd.mr_type = fdp->mr_type;
  mr_load_data->ptrs.ra.data[idx].fd.mr_type_aux = fdp->mr_type_aux;
  mr_load_data->ptrs.ra.data[idx].fd.mr_type_ext = fdp->mr_type_ext;
  mr_load_data->ptrs.ra.data[idx].fd.param = fdp->param;

  /* route loading */
  if ((fdp->mr_type_ext < MR_TYPE_EXT_LAST) && mr_ext_load_handler[fdp->mr_type_ext])
    status = mr_ext_load_handler[fdp->mr_type_ext] (idx, mr_load_data);
  else if ((fdp->mr_type < MR_TYPE_LAST) && mr_load_handler[fdp->mr_type])
    status = mr_load_handler[fdp->mr_type] (idx, mr_load_data);
  else
    MR_MESSAGE_UNSUPPORTED_NODE_TYPE_ (fdp);

  /* set cross references at the upper level */
  if (0 == idx)
    {
      while (mr_load_data->mr_ra_idx.size > 0)
	{
	  mr_load_data->mr_ra_idx.size -= sizeof (mr_load_data->mr_ra_idx.data[0]);
	  mr_load_pointer_postponed (mr_load_data->mr_ra_idx.data[mr_load_data->mr_ra_idx.size / sizeof (mr_load_data->mr_ra_idx.data[0])], mr_load_data);
	}
      if (MR_SUCCESS == status)
	status = mr_set_crossrefs (mr_load_data);
      if (mr_load_data->mr_ra_idx.data)
	{
	  MR_FREE (mr_load_data->mr_ra_idx.data);
	  mr_load_data->mr_ra_idx.data = NULL;
	  mr_load_data->mr_ra_idx.size = 0;
	  mr_load_data->mr_ra_idx.alloc_size = 0;
	}
    }

  return (status);
}
