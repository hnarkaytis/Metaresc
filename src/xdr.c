/* -*- C -*- */
/* I hate this bloody country. Smash. */
/* This file is part of Metaresc project */

#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <rpc/types.h>
#include <rpc/xdr.h>
#include <netinet/in.h>
#ifdef HAVE_CONFIG_H
# include <mr_config.h>
#endif /* HAVE_CONFIG_H */

#include <metaresc.h>
#include <mr_stringify.h>
#include <mr_ic.h>
#include <mr_load.h>
#include <mr_save.h>

TYPEDEF_FUNC (mr_status_t, xdr_load_handler_t, (XDR *, int, mr_ra_mr_ptrdes_t *))
TYPEDEF_FUNC (mr_status_t, xdr_save_handler_t, (XDR *, int, mr_ra_mr_ptrdes_t *))

static xdr_load_handler_t xdr_load_handler[];
static xdr_save_handler_t xdr_save_handler[];

/**
 * Loads int32_t from binary XDR stream.
 * @param xdrs XDR stream descriptor
 * @param lp pointer on int32_t
 * @return status of operation. 0 - failure, !0 - success.
 */
static bool_t
xdrra_getlong (XDR * xdrs, long * lp)
{
  mr_rarray_t * ra = (mr_rarray_t*)xdrs->x_private;
  char * base = ra->data;
  if (NULL == ra)
    return (FALSE);
  if (ra->size - xdrs->x_handy < sizeof (int32_t))
    return (FALSE);
  *(int32_t*)lp = (int32_t) ntohl ((*((int32_t *)(&base[xdrs->x_handy]))));
  xdrs->x_handy += sizeof (int32_t);
  return (TRUE);
}

/**
 * Saves int32_t into binary XDR stream.
 * @param xdrs XDR stream descriptor
 * @param lp pointer on int32_t
 * @return status of operation. 0 - failure, !0 - success.
 */
static bool_t
xdrra_putlong (XDR * xdrs, const long * lp)
{
  mr_rarray_t * ra = (mr_rarray_t*)xdrs->x_private;
  int32_t * data = mr_rarray_append (ra, sizeof (int32_t));
  if (NULL == ra)
    return (FALSE);
  if (NULL == data)
    return (FALSE);
  *data = htonl (*(int32_t*)lp);
  xdrs->x_handy += sizeof (int32_t);
  return (TRUE);
}

/**
 * Loads opaque bytes stream from binary XDR stream.
 * @param xdrs XDR stream descriptor
 * @param addr pointer of bytes stream
 * @param len length of byte array
 * @return status of operation. 0 - failure, !0 - success.
 */
static bool_t
xdrra_getbytes (XDR * xdrs, caddr_t addr, u_int len)
{
  mr_rarray_t * ra = (mr_rarray_t*)xdrs->x_private;
  char * base = ra->data;
  if (NULL == ra)
    return (FALSE);
  if (ra->size - xdrs->x_handy < len)
    return (FALSE);
  memcpy (addr, &base[xdrs->x_handy], len);
  xdrs->x_handy += len;
  return (TRUE);
}

/**
 * Saves opaque bytes stream into binary XDR stream.
 * @param xdrs XDR stream descriptor
 * @param addr pointer of bytes stream
 * @param len length of byte array
 * @return status of operation. 0 - failure, !0 - success.
 */
static bool_t
xdrra_putbytes (XDR * xdrs, const char * addr, u_int len)
{
  mr_rarray_t * ra = (mr_rarray_t*)xdrs->x_private;
  int32_t * data;
  if (NULL == ra)
    return (FALSE);
  data = mr_rarray_append (ra, len);
  if (NULL == data)
    return (FALSE);
  memcpy (data, addr, len);
  xdrs->x_handy += len;
  return (TRUE);
}

/**
 * Gets current stream position.
 * @param xdrs XDR stream descriptor
 * @return position of cursor
 */
static u_int
xdrra_getpostn (__const XDR * xdrs)
{
  return (xdrs->x_handy);
}

/**
 * Sets stream cursor position.
 * @param xdrs XDR stream descriptor
 * @param pos position to set
 * @return status
 */
static bool_t
xdrra_setpostn (XDR * xdrs, u_int pos)
{
  xdrs->x_handy = pos;
  return (TRUE);
}

/**
 * Returns pointer on opaque data from XDR stream.
 * @param xdrs XDR stream descriptor
 * @param len length of opaque data
 * @return pointer on buffer
 */
static int32_t *
xdrra_inline (XDR * xdrs, u_int len)
{
  mr_rarray_t * ra = (mr_rarray_t*)xdrs->x_private;
  char * base = ra->data;
  if (NULL == ra)
    return (FALSE);
  if (ra->size - xdrs->x_handy < len)
    return (NULL);
  return ((int32_t*)&base[xdrs->x_handy]);
}

/**
 * Deallocates XDR reader/writer for resizeable array.
 * @param xdrs XDR stream descriptor
 */
static void
xdrra_destroy (XDR * xdrs)
{
  mr_rarray_t * ra = (mr_rarray_t*)xdrs->x_private;
  if (NULL == ra)
    return;
  if (ra->data)
    MR_FREE (ra->data);
  ra->data = NULL;
  ra->size = ra->alloc_size = 0;
}

#ifdef HAVE_STRUCT_XDR_OPS_X_GETINT32
/**
 * Loads int32 from XDR stream.
 * @param xdrs XDR stream descriptor
 * @param ip pointer on int32
 * @return status
 */
static bool_t
xdrra_getint32 (XDR * xdrs, int32_t * ip)
{
  return (xdrra_getlong (xdrs, (long*)ip));
}
#endif /* HAVE_STRUCT_XDR_OPS_X_GETINT32 */

#ifdef HAVE_STRUCT_XDR_OPS_X_PUTINT32
/**
 * Puts int32 into XDR stream.
 * @param xdrs XDR stream descriptor
 * @param ip pointer on int32
 * @return status
 */
static bool_t
xdrra_putint32 (XDR * xdrs, const int32_t * ip)
{
  return (xdrra_putlong (xdrs, (long*)ip));
}
#endif /* HAVE_STRUCT_XDR_OPS_X_PUTINT32 */

/**
 * Constructor for XDR reader/writer for resizeable array.
 * @param xdrs XDR stream descriptor
 * @param ra resizeable array
 * @param op XDR operation XDR_DECODE XDR_ENCODE
 */
void
xdrra_create (XDR * xdrs, mr_rarray_t * ra, enum xdr_op op)
{
  static struct xdr_ops xdrra_ops =
    {
      .x_getlong = xdrra_getlong,
      .x_putlong = xdrra_putlong,
      .x_getbytes = xdrra_getbytes,
      .x_putbytes = xdrra_putbytes,
      .x_getpostn = (typeof (xdrra_ops.x_getpostn))xdrra_getpostn,
      .x_setpostn = xdrra_setpostn,
      .x_inline = xdrra_inline,
      .x_destroy = xdrra_destroy,
#ifdef HAVE_STRUCT_XDR_OPS_X_GETINT32
      .x_getint32 = xdrra_getint32,
#endif /* HAVE_STRUCT_XDR_OPS_X_GETINT32 */
#ifdef HAVE_STRUCT_XDR_OPS_X_PUTINT32
      .x_putint32 = xdrra_putint32,
#endif /* HAVE_STRUCT_XDR_OPS_X_PUTINT32 */
    };

  xdrs->x_op = op;
  xdrs->x_ops = &xdrra_ops;
  xdrs->x_private = (caddr_t)ra;
  xdrs->x_handy = 0;
}

/**
 * Set cross refernces within loaded data.
 * @param ptrs array with descriptor of loaded data
 * @return status
 */
static mr_status_t
mr_set_crossrefs (mr_ra_mr_ptrdes_t * ptrs)
{
  int count = ptrs->ra.size / sizeof (ptrs->ra.data[0]);
  mr_status_t status = MR_SUCCESS;
  int i;

  /* set all cross refernces */
  for (i = 0; i < count; ++i)
    if (ptrs->ra.data[i].ref_idx >= 0)
      {
	if (ptrs->ra.data[i].ref_idx >= count)
	  {
	    MR_MESSAGE (MR_LL_ERROR, MR_MESSAGE_UNDEFINED_REF_IDX, "ref_idx", ptrs->ra.data[i].ref_idx);
	    status = MR_FAILURE;
	  }
	else
	  {
	    void * data;

	    if (ptrs->ra.data[i].flags.is_content_reference)
	      data = *(void**)(ptrs->ra.data[ptrs->ra.data[i].ref_idx].data);
	    else
	      data = ptrs->ra.data[ptrs->ra.data[i].ref_idx].data;

	    switch (ptrs->ra.data[i].fd.mr_type_ext)
	      {
	      case MR_TYPE_EXT_POINTER:
	      case MR_TYPE_EXT_RARRAY_DATA:
		*(void**)ptrs->ra.data[i].data = data;
		break;
	      default:
		if (MR_TYPE_STRING == ptrs->ra.data[i].fd.mr_type)
		  *(char**)ptrs->ra.data[i].data = data;
		break;
	      }
	  }
      }
  return (status);
}

/**
 * Handler for RL_TYPE_NONE.
 * @param xdrs XDR stream descriptor
 * @param idx index of node in ptrs
 * @param ptrs array with descriptor of loaded data
 * @return status
 */
static mr_status_t
xdr_none (XDR * xdrs, int idx, mr_ra_mr_ptrdes_t * ptrs)
{
  return (xdr_void () ? MR_SUCCESS : MR_FAILURE);
}

/*
  LIBC implementation of xdr_char is too old and has some pitfalls. For readonly memory (statically inited strings) XDR_ENCODE operation produce GPF due to unnecessary write.

bool_t xdr_char (xdrs, cp)
     XDR *xdrs;
     char *cp;
{
  int i;

  i = (*cp);
  if (!xdr_int (xdrs, &i)) {
    return (FALSE);
  }
  *cp = i;
  return (TRUE);
}

  Here is workaround.
*/

/**
 * Handler for type char.
 * @param xdrs XDR stream descriptor
 * @param cp pointer on char
 * @return status
 */
static bool_t __attribute__((unused))
xdr_char_ (XDR * xdrs, char * cp)
{
  int32_t x = 0;
  if (XDR_ENCODE == xdrs->x_op)
    x = *cp;
  if (!xdr_int (xdrs, &x))
    return (FALSE);
  if (XDR_DECODE == xdrs->x_op)
    *cp = x;
  return (TRUE);
}

#ifndef HAVE_XDR_UINT8_T
#define xdr_uint8_t xdr_char_
#endif /* HAVE_XDR_UINT8_T */
#ifndef HAVE_XDR_INT8_T
#define xdr_int8_t xdr_char_
#endif /* HAVE_XDR_INT8_T */

/**
 * Handler for unsigned integer types.
 * @param xdrs XDR stream descriptor
 * @param idx index of node in ptrs
 * @param ptrs array with descriptor of loaded data
 * @return status
 */
static mr_status_t
xdr_uint_ (XDR * xdrs, int idx, mr_ra_mr_ptrdes_t * ptrs)
{
  bool_t status = FALSE;
  switch (ptrs->ra.data[idx].fd.size)
    {
    case sizeof (uint8_t):
      status = xdr_uint8_t (xdrs, ptrs->ra.data[idx].data);
      break;
    case sizeof (uint16_t):
      status = xdr_uint16_t (xdrs, ptrs->ra.data[idx].data);
      break;
    case sizeof (uint32_t):
      status = xdr_uint32_t (xdrs, ptrs->ra.data[idx].data);
      break;
    case sizeof (uint64_t):
      status = xdr_uint64_t (xdrs, ptrs->ra.data[idx].data);
      break;
    }
  return (status ? MR_SUCCESS : MR_FAILURE);
}

/**
 * Handler for signed integer types.
 * @param xdrs XDR stream descriptor
 * @param idx index of node in ptrs
 * @param ptrs array with descriptor of loaded data
 * @return status
 */
static mr_status_t
xdr_int_ (XDR * xdrs, int idx, mr_ra_mr_ptrdes_t * ptrs)
{
  bool_t status = FALSE;
  switch (ptrs->ra.data[idx].fd.size)
    {
    case sizeof (uint8_t):
      status = xdr_int8_t (xdrs, ptrs->ra.data[idx].data);
      break;
    case sizeof (uint16_t):
      status = xdr_int16_t (xdrs, ptrs->ra.data[idx].data);
      break;
    case sizeof (uint32_t):
      status = xdr_int32_t (xdrs, ptrs->ra.data[idx].data);
      break;
    case sizeof (uint64_t):
      status = xdr_int64_t (xdrs, ptrs->ra.data[idx].data);
      break;
    }
  return (status ? MR_SUCCESS : MR_FAILURE);
}

/**
 * Handler for type float.
 * @param xdrs XDR stream descriptor
 * @param idx index of node in ptrs
 * @param ptrs array with descriptor of loaded data
 * @return status
 */
static mr_status_t
xdr_float_ (XDR * xdrs, int idx, mr_ra_mr_ptrdes_t * ptrs)
{
  return (xdr_float (xdrs, ptrs->ra.data[idx].data) ? MR_SUCCESS : MR_FAILURE);
}

#define XDR_COMPLEX(NAME_SUFFIX, TYPE_NAME)					\
  static mr_status_t xdr_complex_ ## NAME_SUFFIX (XDR * xdrs, int idx, mr_ra_mr_ptrdes_t * ptrs) { \
    TYPE_NAME real = 0;							\
    TYPE_NAME imag = 0;							\
    complex TYPE_NAME x = 0;						\
    if (XDR_ENCODE == xdrs->x_op)					\
      {									\
	x = *(complex TYPE_NAME*)ptrs->ra.data[idx].data;		\
	real = __real__ x;						\
	imag = __imag__ x;						\
      }									\
    if (!xdr_ ## NAME_SUFFIX (xdrs, &real))				\
      return (MR_FAILURE);						\
    if (!xdr_ ## NAME_SUFFIX (xdrs, &imag))				\
      return (MR_FAILURE);						\
    if (XDR_DECODE == xdrs->x_op)					\
      {									\
	__real__ x = real;						\
	__imag__ x = imag;						\
	*(complex TYPE_NAME*)ptrs->ra.data[idx].data = x;		\
      }									\
    return (MR_SUCCESS);						\
  }

XDR_COMPLEX (float, float);

/**
 * Handler for type double.
 * @param xdrs XDR stream descriptor
 * @param idx index of node in ptrs
 * @param ptrs array with descriptor of loaded data
 * @return status
 */
static mr_status_t
xdr_double_ (XDR * xdrs, int idx, mr_ra_mr_ptrdes_t * ptrs)
{
  return (xdr_double (xdrs, ptrs->ra.data[idx].data) ? MR_SUCCESS : MR_FAILURE);
}

XDR_COMPLEX (double, double);

/**
 * Handler for type long double. Saves as opaque data binary representation of long double in memeory. Assumes that CPU uses ieee854 standard.
 * @param xdrs XDR stream descriptor
 * @param idx index of node in ptrs
 * @param ptrs array with descriptor of loaded data
 * @return status
 */
static bool_t
xdr_long_double (XDR * xdrs, long double * data)
{
  return (xdr_opaque (xdrs, (void*)data, MR_SIZEOF_LONG_DOUBLE));
}

static mr_status_t
xdr_long_double_ (XDR * xdrs, int idx, mr_ra_mr_ptrdes_t * ptrs)
{
  return (xdr_long_double (xdrs, ptrs->ra.data[idx].data) ? MR_SUCCESS : MR_FAILURE);
}

XDR_COMPLEX (long_double, long double);

/**
 * Handler for char arrays.
 * @param xdrs XDR stream descriptor
 * @param idx index of node in ptrs
 * @param ptrs array with descriptor of loaded data
 * @return status
 */
static mr_status_t
xdr_char_array_ (XDR * xdrs, int idx, mr_ra_mr_ptrdes_t * ptrs)
{
  uint32_t str_len;
  uint32_t max_size = ptrs->ra.data[idx].fd.size * ptrs->ra.data[idx].fd.param.array_param.count;

  if (XDR_ENCODE == xdrs->x_op)
    {
      str_len = strlen (ptrs->ra.data[idx].data) + 1;
      if ((str_len > max_size) && (0 != strcmp (ptrs->ra.data[idx].fd.type, "string_t")))
	str_len = max_size;
    }

  if (!xdr_uint32_t (xdrs, &str_len))
    return (MR_FAILURE);

  if (XDR_DECODE == xdrs->x_op)
    {
      if (0 == strcmp (ptrs->ra.data[idx].fd.type, "string_t"))
	{
	  void * data = MR_REALLOC (ptrs->ra.data[idx].data, str_len);
	  ptrs->ra.data[idx].data = data;
	  *(void**)ptrs->ra.data[idx - 1].data = data;
	  if (NULL == data)
	    {
	      MR_MESSAGE (MR_LL_FATAL, MR_MESSAGE_OUT_OF_MEMORY);
	      return (MR_FAILURE);
	    }
	}
      }
  return (xdr_opaque (xdrs, ptrs->ra.data[idx].data, str_len) ? MR_SUCCESS : MR_FAILURE);
}

/**
 * Save handler for char pointers.
 * @param xdrs XDR stream descriptor
 * @param idx index of node in ptrs
 * @param ptrs array with descriptor of loaded data
 * @return status
 */
static mr_status_t
xdr_save_string (XDR * xdrs, int idx, mr_ra_mr_ptrdes_t * ptrs)
{
  if (ptrs->ra.data[idx].ref_idx >= 0)
    {
      if (!xdr_int32_t (xdrs, &ptrs->ra.data[ptrs->ra.data[idx].ref_idx].idx))
	return (MR_FAILURE);
      if (!xdr_uint32_t (xdrs, (void*)&ptrs->ra.data[idx].flags))
	return (MR_FAILURE);
      return (MR_SUCCESS);
    }
  else
    {
      void ** str = ptrs->ra.data[idx].data;
      int32_t size = -1;
      if (!xdr_int32_t (xdrs, &ptrs->ra.data[idx].ref_idx))
	return (MR_FAILURE);
      if (NULL != *str)
	size = strlen (*str);
      if (!xdr_int32_t (xdrs, &size))
	return (MR_FAILURE);
      if (size < 0)
	return (MR_SUCCESS);
      if (!xdr_opaque (xdrs, *str, size))
	return (MR_FAILURE);
      return (MR_SUCCESS);
    }
}

/**
 * Load handler for char pointers.
 * @param xdrs XDR stream descriptor
 * @param idx index of node in ptrs
 * @param ptrs array with descriptor of loaded data
 * @return status
 */
static mr_status_t
xdr_load_string (XDR * xdrs, int idx, mr_ra_mr_ptrdes_t * ptrs)
{
  if (!xdr_int32_t (xdrs, &ptrs->ra.data[idx].ref_idx))
    return (MR_FAILURE);
  if (ptrs->ra.data[idx].ref_idx >= 0)
    {
      if (!xdr_uint32_t (xdrs, (void*)&ptrs->ra.data[idx].flags))
	return (MR_FAILURE);
      return (MR_SUCCESS);
    }
  else
    {
      char ** str = ptrs->ra.data[idx].data;
      int32_t size = -1;
      *str = NULL;
      if (!xdr_int32_t (xdrs, &size))
	return (MR_FAILURE);
      if (size < 0)
	return (MR_SUCCESS);
      *str = MR_MALLOC (size + 1);
      if (NULL == *str)
	{
	  MR_MESSAGE (MR_LL_FATAL, MR_MESSAGE_OUT_OF_MEMORY);
	  return (MR_FAILURE);
	}
      memset (*str, 0, size + 1);
      if (!xdr_opaque (xdrs, *str, size))
	return (MR_FAILURE);
      return (MR_SUCCESS);
    }
}

/**
 * Load handler for structures.
 * @param xdrs XDR stream descriptor
 * @param idx index of node in ptrs
 * @param ptrs array with descriptor of loaded data
 * @param tdp type descriptor
 * @return status
 */
static mr_status_t
xdr_load_struct_inner (XDR * xdrs, int idx, mr_ra_mr_ptrdes_t * ptrs, mr_td_t * tdp)
{
  char * data = ptrs->ra.data[idx].data;
  int i, count;

  if (NULL == tdp)
    {
      MR_MESSAGE (MR_LL_ERROR, MR_MESSAGE_NO_TYPE_DESCRIPTOR, ptrs->ra.data[idx].fd.type);
      return (MR_FAILURE);
    }

  if (tdp->mr_type != MR_TYPE_STRUCT)
    {
      MR_MESSAGE (MR_LL_WARN, MR_MESSAGE_TYPE_NOT_STRUCT, tdp->type.str);
      return (MR_FAILURE);
    }

  count = tdp->fields.size / sizeof (tdp->fields.data[0]);
  for (i = 0; i < count; ++i)
    {
      mr_fd_t * fdp = tdp->fields.data[i].fdp;
      if (MR_SUCCESS != xdr_load (&data[fdp->offset], fdp, xdrs, ptrs))
	return (MR_FAILURE);
    }
  return (MR_SUCCESS);
}

/**
 * Load handler for structures.
 * @param xdrs XDR stream descriptor
 * @param idx index of node in ptrs
 * @param ptrs array with descriptor of loaded data
 * @return status
 */
static mr_status_t
xdr_load_struct (XDR * xdrs, int idx, mr_ra_mr_ptrdes_t * ptrs)
{
  return (xdr_load_struct_inner (xdrs, idx, ptrs, mr_get_td_by_name (ptrs->ra.data[idx].fd.type)));
}

/*
  Union mr_ptr_t might be different on source and destination computer. That's why union branch could be identified only by name, not index.
 */
/**
 * Save handler for unions. Saves discriminator as a string.
 * @param xdrs XDR stream descriptor
 * @param idx index of node in ptrs
 * @param ptrs array with descriptor of loaded data
 * @return status
 */
static mr_status_t
xdr_save_union (XDR * xdrs, int idx, mr_ra_mr_ptrdes_t * ptrs)
{
  /* save union branch field name as string */
  char * dummy_str = "";
  mr_ptrdes_t ptrdes = { /* temporary pointer descriptor for this string */
    .data = &dummy_str,
    .ref_idx = -1,
    .flags = { .is_null = MR_FALSE, .is_referenced = MR_FALSE, .is_content_reference = MR_FALSE, },
  };

  if (ptrs->ra.data[idx].first_child >= 0)
    ptrdes.data = &ptrs->ra.data[ptrs->ra.data[idx].first_child].fd.name.str;

  mr_ra_mr_ptrdes_t ptrs_ = { .ra = { .alloc_size = sizeof (ptrdes), .size = sizeof (ptrdes), .data = &ptrdes, }, }; /* temporary resizeable array */
  return (xdr_save_string (xdrs, 0, &ptrs_));
}

/**
 * Load handler for unions.
 * @param xdrs XDR stream descriptor
 * @param idx index of node in ptrs
 * @param ptrs array with descriptor of loaded data
 * @return status
 */
static mr_status_t
xdr_load_union (XDR * xdrs, int idx, mr_ra_mr_ptrdes_t * ptrs)
{
  mr_td_t * tdp = mr_get_td_by_name (ptrs->ra.data[idx].fd.type); /* look up for type descriptor */
  char * data = ptrs->ra.data[idx].data;
  char * discriminator = NULL;
  mr_fd_t * fdp;
  mr_status_t status = MR_FAILURE;
  mr_ptrdes_t ptrdes = { .data = &discriminator, }; /* temporary pointer descriptor for union discriminator string */
  mr_ra_mr_ptrdes_t ptrs_ = { .ra = { .alloc_size = sizeof (ptrdes), .size = sizeof (ptrdes), .data = &ptrdes, }, }; /* temporary resizeable array */

  /* get pointer on structure descriptor */
  if (NULL == tdp)
    MR_MESSAGE (MR_LL_ERROR, MR_MESSAGE_NO_TYPE_DESCRIPTOR, ptrs->ra.data[idx].fd.type);
  else if ((tdp->mr_type != MR_TYPE_UNION) && (tdp->mr_type != MR_TYPE_ANON_UNION) && (tdp->mr_type != MR_TYPE_NAMED_ANON_UNION))
    MR_MESSAGE (MR_LL_ERROR, MR_MESSAGE_TYPE_NOT_UNION, tdp->type.str);
  else if (MR_SUCCESS != xdr_load_string (xdrs, 0, &ptrs_))
    MR_MESSAGE (MR_LL_ERROR, MR_MESSAGE_UNION_DISCRIMINATOR_ERROR, discriminator);
  else if (NULL == discriminator)
    MR_MESSAGE (MR_LL_ERROR, MR_MESSAGE_UNION_DISCRIMINATOR_ERROR, discriminator);
  else
    {
      if (0 == tdp->fields.size) /* check for an empty union */
	status = MR_SUCCESS;
      else
	{
	  fdp = mr_get_fd_by_name (tdp, discriminator);
	  if (NULL == fdp)
	    MR_MESSAGE (MR_LL_ERROR, MR_MESSAGE_UNION_DISCRIMINATOR_ERROR, discriminator);
	  else
	    status = xdr_load (data + fdp->offset, fdp, xdrs, ptrs);
	}
      MR_FREE (discriminator);
    }

  return (status);
}

/**
 * Saves temporary string into XDR stream.
 * @param xdrs XDR stream descriptor
 * @param str pointer on a string
 * @return status
 */
static mr_status_t
xdr_save_temp_string (XDR * xdrs, char ** str)
{
  mr_status_t status = MR_FAILURE;
  if (NULL != str)
    {
      mr_ptrdes_t ptrdes = { /* temporary pointer descriptor for this string */
	.data = str,
	.ref_idx = -1,
	.flags = { .is_null = MR_FALSE, .is_referenced = MR_FALSE, .is_content_reference = MR_FALSE, },
      };
      mr_ra_mr_ptrdes_t ptrs = { .ra = { .alloc_size = sizeof (ptrdes), .size = sizeof (ptrdes), .data = &ptrdes, }, }; /* temporary resizeable array */
      status = xdr_save_string (xdrs, 0, &ptrs);
    }
  return (status);
}

/**
 * Loads temporary string from XDR stream.
 * @param xdrs XDR stream descriptor
 * @param str pointer on a string
 * @return status
 */
static mr_status_t
xdr_load_temp_string (XDR * xdrs, char ** str)
{
  mr_ptrdes_t ptrdes = { .data = str, }; /* temporary pointer descriptor for union discriminator string */
  mr_ra_mr_ptrdes_t ptrs = { .ra = { .alloc_size = sizeof (ptrdes), .size = sizeof (ptrdes), .data = &ptrdes, }, }; /* temporary resizeable array */
  return (xdr_load_string (xdrs, 0, &ptrs));
}

/**
 * Saves enum value as a string.
 * @param xdrs XDR stream descriptor
 * @param idx index of node in ptrs
 * @param ptrs array with descriptor of loaded data
 * @return status
 */
static mr_status_t
xdr_save_enum (XDR * xdrs, int idx, mr_ra_mr_ptrdes_t * ptrs)
{
  char * value = mr_stringify_enum (&ptrs->ra.data[idx]);
  mr_status_t status = xdr_save_temp_string (xdrs, &value);
  if (NULL != value)
    MR_FREE (value);
  return (status);
}

/**
 * Saves bitmask value as a string.
 * @param xdrs XDR stream descriptor
 * @param idx index of node in ptrs
 * @param ptrs array with descriptor of loaded data
 * @return status
 */
static mr_status_t
xdr_save_bitmask (XDR * xdrs, int idx, mr_ra_mr_ptrdes_t * ptrs)
{
  char * value = mr_stringify_bitmask (&ptrs->ra.data[idx], MR_BITMASK_OR_DELIMITER);
  mr_status_t status = xdr_save_temp_string (xdrs, &value);
  if (NULL != value)
    MR_FREE (value);
  return (status);
}

/**
 * Saves function pointer as a string.
 * @param xdrs XDR stream descriptor
 * @param idx index of node in ptrs
 * @param ptrs array with descriptor of loaded data
 * @return status
 */
static mr_status_t
xdr_save_func (XDR * xdrs, int idx, mr_ra_mr_ptrdes_t * ptrs)
{
  char * value = mr_stringify_func (&ptrs->ra.data[idx]);
  mr_status_t status = xdr_save_temp_string (xdrs, &value);
  if (NULL != value)
    MR_FREE (value);
  return (status);
}

/**
 * Loads enum or bitmask.
 * @param xdrs XDR stream descriptor
 * @param idx index of node in ptrs
 * @param ptrs array with descriptor of loaded data
 * @return status
 */
static mr_status_t
xdr_load_stringified_type (XDR * xdrs, int idx, mr_ra_mr_ptrdes_t * ptrs)
{
  mr_status_t status = MR_FAILURE;
  mr_ptrdes_t ptrdes = { .fd = ptrs->ra.data[idx].fd, .data = ptrs->ra.data[idx].data, };
  mr_load_data_t mr_load_data = { .ptrs = { .ra = { .alloc_size = sizeof (ptrdes), .size = sizeof (ptrdes), .data = &ptrdes, } } };

  if (MR_SUCCESS != xdr_load_temp_string (xdrs, &ptrdes.mr_value.vt_string))
    return (MR_FAILURE);
  if (NULL == ptrdes.mr_value.vt_string)
    return (MR_FAILURE);

  ptrdes.mr_value.value_type = MR_VT_UNKNOWN;
  switch (ptrs->ra.data[idx].fd.mr_type)
    {
    case MR_TYPE_ENUM:
    case MR_TYPE_BITMASK:
      status = mr_load_integer (0, &mr_load_data);
      break;
    case MR_TYPE_FUNC:
    case MR_TYPE_FUNC_TYPE:
      status = mr_load_func (0, &mr_load_data);
      break;
    default:
      status = MR_FAILURE;
      break;
    }
  if (MR_VT_UNKNOWN == ptrdes.mr_value.value_type)
    MR_FREE (ptrdes.mr_value.vt_string);

  return (status);
}

/**
 * Saves/loads bit field value according its type.
 * @param xdrs XDR stream descriptor
 * @param fdp field descriptor
 * @param data pointer on data
 * @return status
 */
static mr_status_t
xdr_bitfield_value (XDR * xdrs, mr_fd_t * fdp, void * data)
{
  mr_ptrdes_t ptrdes = { .fd = *fdp, .data = data, };
  mr_ra_mr_ptrdes_t ptrs = { .ra = { .alloc_size = sizeof (ptrdes), .size = sizeof (ptrdes), .data = &ptrdes, } };
  ptrdes.fd.mr_type = ptrdes.fd.mr_type_aux;
  if (XDR_ENCODE == xdrs->x_op)
    return (xdr_save_handler[ptrdes.fd.mr_type] (xdrs, 0, &ptrs));
  else if (XDR_DECODE == xdrs->x_op)
    return (xdr_load_handler[ptrdes.fd.mr_type] (xdrs, 0, &ptrs));
  else
    return (MR_SUCCESS);
}

/**
 * Saves bit field into XDR stream.
 * @param xdrs XDR stream descriptor
 * @param idx index of node in ptrs
 * @param ptrs array with descriptor of loaded data
 * @return status
 */
static mr_status_t
xdr_save_bitfield (XDR * xdrs, int idx, mr_ra_mr_ptrdes_t * ptrs)
{
  uint64_t value;
  if (MR_SUCCESS != mr_save_bitfield_value (&ptrs->ra.data[idx], &value))
    return (MR_FAILURE);
  return (xdr_bitfield_value (xdrs, &ptrs->ra.data[idx].fd, &value));
}

/**
 * Loads bit field from XDR stream.
 * @param xdrs XDR stream descriptor
 * @param idx index of node in ptrs
 * @param ptrs array with descriptor of loaded data
 * @return status
 */
static mr_status_t
xdr_load_bitfield (XDR * xdrs, int idx, mr_ra_mr_ptrdes_t * ptrs)
{
  uint64_t value;
  if (MR_SUCCESS != xdr_bitfield_value (xdrs, &ptrs->ra.data[idx].fd, &value))
    return (MR_FAILURE);
  return (mr_load_bitfield_value (&ptrs->ra.data[idx], &value));
}

/**
 * Loads char array from XDR stream.
 * @param xdrs XDR stream descriptor
 * @param idx index of node in ptrs
 * @param ptrs array with descriptor of loaded data
 * @return status
 */
static mr_status_t
xdr_load_array (XDR * xdrs, int idx, mr_ra_mr_ptrdes_t * ptrs)
{
  int i;
  char * data = ptrs->ra.data[idx].data;
  mr_fd_t fd_ = ptrs->ra.data[idx].fd;
  int count = fd_.param.array_param.count;
  int row_count = fd_.param.array_param.row_count;

  if (1 == fd_.param.array_param.row_count)
    fd_.mr_type_ext = MR_TYPE_EXT_NONE; /* set extended type property to MR_NONE in copy of field descriptor */
  else
    {
      fd_.param.array_param.count = row_count;
      fd_.param.array_param.row_count = 1;
    }

  for (i = 0; i < count; i += row_count)
    if (MR_SUCCESS != xdr_load (&data[i * fd_.size], &fd_, xdrs, ptrs))
      return (MR_FAILURE);
  return (MR_SUCCESS);
}

/**
 * Saves pointer into binary XDR stream. First goes flags of the node
 * and if pointer is not NULL, then ref_idx goes next.
 * @param xdrs XDR stream descriptor
 * @param idx index of the node in nodes collection
 * @param ptrs resizeable array with pointers descriptors
 * @return status of operation. 0 - failure, !0 - success.
 */
static mr_status_t
xdr_save_pointer (XDR * xdrs, int idx, mr_ra_mr_ptrdes_t * ptrs)
{
  bool_t status = FALSE;
  if (!xdr_uint32_t (xdrs, (void*)&ptrs->ra.data[idx].flags))
    return (MR_FAILURE);
  if (ptrs->ra.data[idx].flags.is_null)
    return (MR_SUCCESS);
  if (ptrs->ra.data[idx].ref_idx >= 0)
    status = xdr_int32_t (xdrs, &ptrs->ra.data[ptrs->ra.data[idx].ref_idx].idx);
  else
    status = xdr_int32_t (xdrs, &ptrs->ra.data[idx].ref_idx);
  return (status ? MR_SUCCESS : MR_FAILURE);
}

/**
 * Loads pointer from binary XDR stream.
 * @param xdrs XDR stream descriptor
 * @param idx index of the node in nodes collection
 * @param ptrs resizeable array with pointers descriptors
 * @return status of operation
 */
static mr_status_t
xdr_load_pointer (XDR * xdrs, int idx, mr_ra_mr_ptrdes_t * ptrs)
{
  void ** data = ptrs->ra.data[idx].data;
  mr_fd_t fd_ = ptrs->ra.data[idx].fd;

  if (!xdr_uint32_t (xdrs, (void*)&ptrs->ra.data[idx].flags))
    return (MR_FAILURE);

  if (ptrs->ra.data[idx].flags.is_null)
    *data = NULL;
  else
    {
      if (!xdr_int32_t (xdrs, &ptrs->ra.data[idx].ref_idx))
	return (MR_FAILURE);
      if (ptrs->ra.data[idx].ref_idx < 0)
	{
	  *data = MR_MALLOC (fd_.size);
	  if (NULL == *data)
	    {
	      MR_MESSAGE (MR_LL_FATAL, MR_MESSAGE_OUT_OF_MEMORY);
	      return (MR_FAILURE);
	    }
	  memset (*data, 0, fd_.size);
	  fd_.mr_type_ext = MR_TYPE_EXT_NONE;
	  return (xdr_load (*data, &fd_, xdrs, ptrs));
	}
    }
  return (MR_SUCCESS);
}

/**
 * Saves resizeable array data field into XDR stream.
 * @param xdrs XDR stream descriptor
 * @param idx index of the node in nodes collection
 * @param ptrs resizeable array with pointers descriptors
 * @return status of operation
 */
static mr_status_t
xdr_save_rarray_data (XDR * xdrs, int idx, mr_ra_mr_ptrdes_t * ptrs)
{
  bool_t status = FALSE;
  if (!xdr_uint32_t (xdrs, (void*)&ptrs->ra.data[idx].flags))
    return (MR_FAILURE);

  if (ptrs->ra.data[idx].ref_idx >= 0)
    status = xdr_int32_t (xdrs, &ptrs->ra.data[ptrs->ra.data[idx].ref_idx].idx);
  else
    {
      char * ra_data = ptrs->ra.data[idx].data;
      mr_rarray_t * ra = (void*)&ra_data[-offsetof (mr_rarray_t, data)];
      if (!xdr_int32_t (xdrs, &ptrs->ra.data[idx].ref_idx))
	return (MR_FAILURE);
      switch (sizeof (ra->size))
	{
	case sizeof (int8_t):
	  status = xdr_int8_t (xdrs, (void*)&ra->size);
	  break;
	case sizeof (int16_t):
	  status = xdr_int16_t (xdrs, (int16_t*)(void*)&ra->size);
	  break;
	case sizeof (int32_t):
	  status = xdr_int32_t (xdrs, (int32_t*)(void*)&ra->size);
	  break;
	case sizeof (int64_t):
	  status = xdr_int64_t (xdrs, (int64_t*)(void*)&ra->size);
	  break;
	}

      if (!status)
	return (MR_FAILURE);

      if (ptrs->ra.data[idx].flags.is_opaque_data) /* content saved as opaque data */
	status = xdr_opaque (xdrs, ra->data, ra->size);
      else
	status = TRUE;
    }
  return (status ? MR_SUCCESS : MR_FAILURE);
}

/**
 * Loads resizeable array data field from XDR stream.
 * @param xdrs XDR stream descriptor
 * @param idx index of the node in nodes collection
 * @param ptrs resizeable array with pointers descriptors
 * @return status of operation
 */
static mr_status_t
xdr_load_rarray_data (XDR * xdrs, int idx, mr_ra_mr_ptrdes_t * ptrs)
{
  if (!xdr_uint32_t (xdrs, (void*)&ptrs->ra.data[idx].flags))
    return (MR_FAILURE);
  if (!xdr_int32_t (xdrs, &ptrs->ra.data[idx].ref_idx))
    return (MR_FAILURE);
  if (ptrs->ra.data[idx].ref_idx < 0)
    {
      mr_fd_t fd_ = ptrs->ra.data[idx].fd;
      char * ra_data = ptrs->ra.data[idx].data;
      mr_rarray_t * ra = (void*)&ra_data[-offsetof (mr_rarray_t, data)];
      bool_t status = FALSE;

      switch (sizeof (ra->size))
	{
	case sizeof (int8_t):
	  status = xdr_int8_t (xdrs, (void*)&ra->size);
	  break;
	case sizeof (int16_t):
	  status = xdr_int16_t (xdrs, (int16_t*)(void*)&ra->size);
	  break;
	case sizeof (int32_t):
	  status = xdr_int32_t (xdrs, (int32_t*)(void*)&ra->size);
	  break;
	case sizeof (int64_t):
	  status = xdr_int64_t (xdrs, (int64_t*)(void*)&ra->size);
	  break;
	}

      if (!status)
	return (MR_FAILURE);

      /* .size and .alloc_size will be loaded once again as fields of mr_rarray_t */
      ra->alloc_size = ra->size;
      ra->data = NULL;

      if (ra->size > 0)
	{
	  int i;
	  int count = ra->size / fd_.size;

	  ra->data = MR_MALLOC (ra->size);
	  if (NULL == ra->data)
	    {
	      MR_MESSAGE (MR_LL_FATAL, MR_MESSAGE_OUT_OF_MEMORY);
	      return (MR_FAILURE);
	    }
	  fd_.mr_type_ext = MR_TYPE_EXT_NONE;

	  memset (ra->data, 0, ra->size);
	  if (ptrs->ra.data[idx].flags.is_opaque_data)
	    return (xdr_opaque (xdrs, ra->data, ra->size) ? MR_SUCCESS : MR_FAILURE);
	  else
	    for (i = 0; i < count; ++i)
	      if (MR_SUCCESS != xdr_load (((char*)ra->data) + i * fd_.size, &fd_, xdrs, ptrs))
		return (MR_FAILURE);
	}
    }

  return (MR_SUCCESS);
}

TYPEDEF_STRUCT (xdr_load_rarray_struct_t,
		(XDR *, xdrs),
		int idx,
		(mr_ra_mr_ptrdes_t *, ptrs))

static mr_status_t
xdr_load_rarray_inner (mr_td_t * tdp, void * context)
{
  xdr_load_rarray_struct_t * xdr_load_rarray_struct = context;
  return (xdr_load_struct_inner (xdr_load_rarray_struct->xdrs,
				 xdr_load_rarray_struct->idx,
				 xdr_load_rarray_struct->ptrs,
				 tdp));
}

/**
 * Loads resizeable array from XDR stream.
 * @param xdrs XDR stream descriptor
 * @param idx index of the node in nodes collection
 * @param ptrs resizeable array with pointers descriptors
 * @return status of operation
 */
static mr_status_t
xdr_load_rarray (XDR * xdrs, int idx, mr_ra_mr_ptrdes_t * ptrs)
{
  mr_rarray_t * ra = ptrs->ra.data[idx].data;
  xdr_load_rarray_struct_t xdr_load_rarray_struct = {
    .xdrs = xdrs,
    .idx = idx,
    .ptrs = ptrs,
  };

  memset (ra, 0, sizeof (*ra));
  if (MR_SUCCESS != mr_load_rarray_type (&ptrs->ra.data[idx].fd, xdr_load_rarray_inner, &xdr_load_rarray_struct))
    return (MR_FAILURE);
  /* alloc_size is loaded from XDRS, but it should reflect amount of memory really allocated for data */
  ra->alloc_size = ra->size;

  return (MR_SUCCESS);
}

static bool
has_opaque_rarrays (mr_ra_mr_ptrdes_t * ptrs)
{
  int i;
  bool has_opaque_rarrays_flag = FALSE;
  for (i = ptrs->ra.size / sizeof (ptrs->ra.data[0]) - 1; i >= 0; --i)
    if (MR_TRUE == ptrs->ra.data[i].flags.is_opaque_data)
      {
	/* do not save sub-nodes */
	ptrs->ra.data[i].first_child = ptrs->ra.data[i].last_child = -1;
	has_opaque_rarrays_flag = TRUE;
      }
  return (has_opaque_rarrays_flag);
}

/**
 * Init save handlers Table
 */
static xdr_save_handler_t xdr_save_handler[] =
  {
    [MR_TYPE_NONE] = xdr_none,
    [MR_TYPE_VOID] = xdr_none,
    [MR_TYPE_ENUM] = xdr_save_enum,
    [MR_TYPE_BITFIELD] = xdr_save_bitfield,
    [MR_TYPE_BITMASK] = xdr_save_bitmask,
    [MR_TYPE_BOOL] = xdr_int_,
    [MR_TYPE_INT8] = xdr_int_,
    [MR_TYPE_UINT8] = xdr_uint_,
    [MR_TYPE_INT16] = xdr_int_,
    [MR_TYPE_UINT16] = xdr_uint_,
    [MR_TYPE_INT32] = xdr_int_,
    [MR_TYPE_UINT32] = xdr_uint_,
    [MR_TYPE_INT64] = xdr_int_,
    [MR_TYPE_UINT64] = xdr_uint_,
    [MR_TYPE_FLOAT] = xdr_float_,
    [MR_TYPE_COMPLEX_FLOAT] = xdr_complex_float,
    [MR_TYPE_DOUBLE] = xdr_double_,
    [MR_TYPE_COMPLEX_DOUBLE] = xdr_complex_double,
    [MR_TYPE_LONG_DOUBLE] = xdr_long_double_,
    [MR_TYPE_COMPLEX_LONG_DOUBLE] = xdr_complex_long_double,
    [MR_TYPE_CHAR] = xdr_int_,
    [MR_TYPE_CHAR_ARRAY] = xdr_char_array_,
    [MR_TYPE_STRING] = xdr_save_string,
    [MR_TYPE_STRUCT] = xdr_none,
    [MR_TYPE_FUNC] = xdr_save_func,
    [MR_TYPE_FUNC_TYPE] = xdr_save_func,
    [MR_TYPE_UNION] = xdr_save_union,
    [MR_TYPE_ANON_UNION] = xdr_save_union,
    [MR_TYPE_NAMED_ANON_UNION] = xdr_save_union,
  };

static xdr_save_handler_t ext_xdr_save_handler[] =
  {
    [MR_TYPE_EXT_ARRAY] = xdr_none,
    [MR_TYPE_EXT_RARRAY_DATA] = xdr_save_rarray_data,
    [MR_TYPE_EXT_POINTER] = xdr_save_pointer,
  };

static mr_status_t
xdr_save_node (mr_ra_mr_ptrdes_t * ptrs, int idx, void * context)
{
  XDR * xdrs = context;
  mr_fd_t * fdp = &ptrs->ra.data[idx].fd;
  if ((fdp->mr_type_ext < MR_TYPE_EXT_LAST) && ext_xdr_save_handler[fdp->mr_type_ext])
    {
      if (MR_SUCCESS != ext_xdr_save_handler[fdp->mr_type_ext] (xdrs, idx, ptrs))
	return (MR_FAILURE);
    }
  else if ((fdp->mr_type < MR_TYPE_LAST) && xdr_save_handler[fdp->mr_type])
    {
      if (MR_SUCCESS != xdr_save_handler[fdp->mr_type] (xdrs, idx, ptrs))
	return (MR_FAILURE);
    }
  else
    {
      MR_MESSAGE_UNSUPPORTED_NODE_TYPE_ (fdp);
      return (MR_FAILURE);
    }
  return (MR_SUCCESS);
}

/**
 * Public function. Save scheduler. Save any object into XDR stream.
 * @param xdrs XDR context structure
 * @param ptrs resizeable array with pointers descriptors
 * @return status
 */
mr_status_t
xdr_save (XDR * xdrs, mr_ra_mr_ptrdes_t * ptrs)
{
  if (has_opaque_rarrays (ptrs))
    {
      int idx_ = 0;
      mr_ptrs_ds (ptrs, mr_renumber_node, &idx_);
    }
  return (mr_ptrs_ds (ptrs, xdr_save_node, xdrs));
}

/**
 * Init load handlers Table
 */
static xdr_load_handler_t xdr_load_handler[] =
  {
    [MR_TYPE_NONE] = xdr_none,
    [MR_TYPE_VOID] = xdr_none,
    [MR_TYPE_ENUM] = xdr_load_stringified_type,
    [MR_TYPE_BITFIELD] = xdr_load_bitfield,
    [MR_TYPE_BITMASK] = xdr_load_stringified_type,
    [MR_TYPE_BOOL] = xdr_int_,
    [MR_TYPE_INT8] = xdr_int_,
    [MR_TYPE_UINT8] = xdr_uint_,
    [MR_TYPE_INT16] = xdr_int_,
    [MR_TYPE_UINT16] = xdr_uint_,
    [MR_TYPE_INT32] = xdr_int_,
    [MR_TYPE_UINT32] = xdr_uint_,
    [MR_TYPE_INT64] = xdr_int_,
    [MR_TYPE_UINT64] = xdr_uint_,
    [MR_TYPE_FLOAT] = xdr_float_,
    [MR_TYPE_COMPLEX_FLOAT] = xdr_complex_float,
    [MR_TYPE_DOUBLE] = xdr_double_,
    [MR_TYPE_COMPLEX_DOUBLE] = xdr_complex_double,
    [MR_TYPE_LONG_DOUBLE] = xdr_long_double_,
    [MR_TYPE_COMPLEX_LONG_DOUBLE] = xdr_complex_long_double,
    [MR_TYPE_CHAR] = xdr_int_,
    [MR_TYPE_CHAR_ARRAY] = xdr_char_array_,
    [MR_TYPE_STRING] = xdr_load_string,
    [MR_TYPE_STRUCT] = xdr_load_struct,
    [MR_TYPE_FUNC] = xdr_load_stringified_type,
    [MR_TYPE_FUNC_TYPE] = xdr_load_stringified_type,
    [MR_TYPE_UNION] = xdr_load_union,
    [MR_TYPE_ANON_UNION] = xdr_load_union,
    [MR_TYPE_NAMED_ANON_UNION] = xdr_load_union,
  };

static xdr_load_handler_t ext_xdr_load_handler[] =
  {
    [MR_TYPE_EXT_ARRAY] = xdr_load_array,
    [MR_TYPE_EXT_RARRAY] = xdr_load_rarray,
    [MR_TYPE_EXT_RARRAY_DATA] = xdr_load_rarray_data,
    [MR_TYPE_EXT_POINTER] = xdr_load_pointer,
  };

mr_status_t
xdr_load (void * data, mr_fd_t * fdp, XDR * xdrs, mr_ra_mr_ptrdes_t * ptrs)
{
  mr_ra_mr_ptrdes_t ptrs_ = { .ra = { .size = 0, .alloc_size = 0, .data = NULL } };
  mr_status_t status = MR_SUCCESS;
  int idx;

  if (NULL == ptrs)
    ptrs = &ptrs_;

  idx = mr_add_ptr_to_list (ptrs);
  if (idx < 0)
    return (MR_FAILURE);
  ptrs->ra.data[idx].data = data;
  ptrs->ra.data[idx].fd = *fdp;

  if ((fdp->mr_type_ext < MR_TYPE_EXT_LAST) && ext_xdr_load_handler[fdp->mr_type_ext])
    status = ext_xdr_load_handler[fdp->mr_type_ext] (xdrs, idx, ptrs);
  else if ((fdp->mr_type < MR_TYPE_LAST) && xdr_load_handler[fdp->mr_type])
    status = xdr_load_handler[fdp->mr_type] (xdrs, idx, ptrs);
  else
    MR_MESSAGE_UNSUPPORTED_NODE_TYPE_ (fdp);

  if ((MR_SUCCESS == status) && (0 == idx))
    status = mr_set_crossrefs (ptrs);

  if (ptrs_.ra.data)
    MR_FREE (ptrs_.ra.data);
  return (status);
}
