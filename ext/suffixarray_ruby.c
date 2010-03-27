/**********************************************************************

  suffixarray_ruby.c -

  Author:: yagihiro@gmail.com
  Copyright:: Copyright (C) 2010 Hiroki Yagita
  License:: MIT X License

**********************************************************************/

#include <ruby.h>
#include "sais.h"

/*
use case --------------------------------------------------------

text = "abracadabra"
sa = SuffixArray.new text
sa.text                                 #=> "abracadabra"
sa.ipoint                               #=> [10, 7, 0, 3, 5, 8, 1, 4, 6, 9, 2]
sa.search "ca"                          #=> 4

 */


struct SuffixArrayData {
	char *text;		/* input text */
	int *ipoint;		/* index points */
	int size;		/* input text size in bytes (== index points size) */
	const char *pattern;	/* search pattern string */
	size_t pattern_size;	/* _pattern_ length in bytes */
};

static VALUE rb_cSuffixArray;


/*#define DEBUG*/
#ifdef DEBUG
#  define DP(fmt, ...) printf(fmt, __VA_ARGS__)
#else
#  define DP(fmt, ...)
#endif


static VALUE
suffixarray_alloc(VALUE klass)
{
	return Data_Wrap_Struct(klass, 0, -1, 0);
}

static VALUE
suffixarray_initialize(VALUE self, VALUE text)
{
	struct SuffixArrayData *datap;

	/* construction */
	datap = ALLOC(struct SuffixArrayData);
	DATA_PTR(self) = datap;

	StringValue(text);

	datap->size = RSTRING(text)->len;

	datap->text = ALLOC_N(char, datap->size + 1);
	MEMCPY(datap->text, RSTRING(text)->ptr, char, datap->size);
	datap->text[datap->size] = '\0';

	datap->ipoint = ALLOC_N(int, datap->size);
	
	/* make suffix array */
	if (sais(datap->text, datap->ipoint, datap->size) != 0)
		rb_raise(rb_eArgError, "???");
}

/*
 * call-seq:
 *    sa.text                                 -> string
 *
 * Returns a string created from an argument of SuffixArray.new method.
 * 
 */
static VALUE
suffixarray_text(VALUE self)
{
	struct SuffixArrayData *datap;

	Check_Type(self, T_DATA);
	datap = DATA_PTR(self);
	
	return rb_str_new(datap->text, datap->size);
}

/*
 * call-seq:
 *    sa.ipoint                               -> array
 *
 * Returns a new array containing index points.
 *
 */
static VALUE
suffixarray_ipoint(VALUE self)
{
	struct SuffixArrayData *datap;
	VALUE ary;
	int i;

	Check_Type(self, T_DATA);
	datap = DATA_PTR(self);

	ary = rb_ary_new();
	for (i = 0; i < datap->size; i++) {
		rb_ary_push(ary, INT2FIX(datap->ipoint[i]));
	}

	return ary;
}


typedef int (*bsearch_compare_fn)(struct SuffixArrayData *data, int item);


/*
  boundary 0 1 2 3 4 5 6 7
            a b c c c d e
  index     0 1 2 3 4 5 6

  low & high are based on index mean.
 */
static int
__bsearch_lower_boundary(struct SuffixArrayData *data, int low, int high, bsearch_compare_fn fn)
{
	int lower = low - 1;
	int upper = high;
	int mid;

	while (lower + 1 != upper) {
		mid = (lower+upper) / 2;
		DP("%s: lower[%d] upper[%d] mid[%d]\n", __func__, lower, upper, mid);
		if (fn(data, data->ipoint[mid]) < 0) {
			lower = mid;
		} else {
			upper = mid;
		}
	}

	DP("%s: upper[%d]\n", __func__, upper);
	return upper;
}

static int
__bsearch_first(struct SuffixArrayData *data, int low, int high, bsearch_compare_fn fn)
{
	int boundary;

	boundary = __bsearch_lower_boundary(data, low, high, fn);
	DP("%s: boundary[%d]\n", __func__, boundary);
	if (data->size <= boundary || fn(data, data->ipoint[boundary]) != 0) {
		return -1;
	} else {
		return boundary;
	}
}

static int
__bsearch(struct SuffixArrayData *data, int low, int high, bsearch_compare_fn fn)
{
	return __bsearch_first(data, low, high, fn);
}

static int
__bsearch_upper_boundary(struct SuffixArrayData *data, int low, int high, bsearch_compare_fn fn)
{
	int lower = low - 1;
	int upper = high;
	int mid;

	while (lower + 1 != upper) {
		mid = (lower+upper) / 2;
		if (fn(data, data->ipoint[mid]) <= 0) {
			lower = mid;
		} else {
			upper = mid;
		}
	}

	return lower + 1;
}

static int
__bsearch_last(struct SuffixArrayData *data, int low, int high, bsearch_compare_fn fn)
{
	int boundary;

	boundary = __bsearch_upper_boundary(data, low, high, fn) - 1;
	if (boundary <= -1 || fn(data, data->ipoint[boundary]) != 0) {
		return -1;
	} else {
		return boundary;
	}
}


static int
__bsearch_compare(struct SuffixArrayData *data, int item)
{
	DP("%s: item[%d] pat[%s], text[item][%s]\n", __func__, item, data->pattern, &data->text[item]);
	return strncmp(&data->text[item], data->pattern, data->pattern_size);
}

/*
 * call-seq:
 *    sa.search "word"                        -> int
 *
 * Returns a value that result to search suffix array.
 */
static VALUE
suffixarray_search(VALUE self, VALUE pattern)
{
	struct SuffixArrayData *datap;
	int result;

	Check_Type(self, T_DATA);
	datap = DATA_PTR(self);
	datap->pattern = StringValueCStr(pattern);
	datap->pattern_size = strlen(datap->pattern);
	
	result = __bsearch(datap, 0, datap->size, __bsearch_compare);
	if (result < 0)
		return INT2FIX(result); /* failed, return -1 */

	return INT2FIX(datap->ipoint[result]);
}

void
Init_suffixarray()
{
	rb_cSuffixArray = rb_define_class("SuffixArray", rb_cObject);
	
	rb_define_alloc_func(rb_cSuffixArray, suffixarray_alloc);
	rb_define_private_method(rb_cSuffixArray, "initialize", suffixarray_initialize, 1);
	rb_define_method(rb_cSuffixArray, "text", suffixarray_text, 0);
	rb_define_method(rb_cSuffixArray, "ipoint", suffixarray_ipoint, 0);
	rb_define_method(rb_cSuffixArray, "search", suffixarray_search, 1);
}
