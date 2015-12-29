/*
 * LGPL HEADER START
 *
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 *
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the GNU Lesser General Public License
 * (LGPL) version 2.1 or (at your discretion) any later version.
 * (LGPL) version 2.1 accompanies this distribution, and is available at
 * http://www.gnu.org/licenses/lgpl-2.1.html
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * LGPL HEADER END
 */
/*
 * lustre/utils/liblustreapi_layout.c
 *
 * lustreapi library for layout calls for interacting with the layout of
 * Lustre files while hiding details of the internal data structures
 * from the user.
 *
 * Author: Ned Bass <bass6@llnl.gov>
 */

#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <limits.h>
#include <sys/xattr.h>
#include <endian.h>

#include <lustre/lustre.h>

#include "internal.h"

/**
 * An Opaque data type abstracting the layout of a Lustre file.
 *
 * Duplicate the fields we care about from struct lov_user_md_v3.
 * Deal with v1 versus v3 format issues only when we read or write
 * files.
 */
struct lus_layout {
	uint32_t	llot_magic;
	uint64_t	llot_pattern;
	uint64_t	llot_pattern_flags;
	uint64_t	llot_stripe_size;
	uint64_t	llot_stripe_count;
	uint64_t	llot_stripe_offset;
	/** Indicates if llot_objects array has been initialized. */
	bool		llot_objects_are_valid;
	/* Add 1 so user always gets back a null terminated string. */
	char		llot_pool_name[LOV_MAXPOOLNAME + 1];
	struct		lov_user_ost_data_v1 llot_objects[0];
};

/* Helper functions for testing the validity of stripe attributes. */
static bool stripe_size_is_aligned(uint64_t size)
{
	return (size & (LOV_MIN_STRIPE_SIZE - 1)) == 0;
}

static bool stripe_size_is_too_big(uint64_t size)
{
	return size >= (1ULL << 32);
}

static bool stripe_count_is_valid(int64_t count)
{
	return count >= -1 && count <= LOV_MAX_STRIPE_COUNT;
}

static bool stripe_index_is_valid(int64_t idx)
{
	return idx >= -1 && idx <= LOV_V1_INSANE_STRIPE_COUNT;
}

/**
 * Byte-swap the fields of struct lov_user_md.
 */
static void __swab16s(uint16_t *x) { *x = __bswap_16(*x); }
static void __swab32s(uint32_t *x) { *x = __bswap_32(*x); }
static void
layout_swab_lov_user_md(struct lov_user_md *lum, int object_count)
{
	int i;
	struct lov_user_md_v3 *lumv3 = (struct lov_user_md_v3 *)lum;
	struct lov_user_ost_data *lod;

	__swab32s(&lum->lmm_magic);
	__swab32s(&lum->lmm_pattern);
	__swab32s(&lum->lmm_stripe_size);
	__swab16s(&lum->lmm_stripe_count);
	__swab16s(&lum->lmm_stripe_offset);

	if (lum->lmm_magic != LOV_MAGIC_V1)
		lod = lumv3->lmm_objects;
	else
		lod = lum->lmm_objects;

	for (i = 0; i < object_count; i++)
		__swab32s(&lod[i].l_ost_idx);
}

/**
 * Allocate storage for a lus_layout with \a num_stripes stripes.
 *
 * \param[in]   num_stripes	number of stripes in new layout
 * \param[out]  layout          a newly allocated layout
 *
 * \retval	0 on success, with a new layout
 * \retval	a negative errno on failure, with layout set to NULL.
 */
static int __layout_alloc(unsigned int num_stripes, struct lus_layout **layout)
{
	struct lus_layout *lo;
	size_t size = sizeof(*lo) +
		(num_stripes * sizeof(lo->llot_objects[0]));
	int rc;

	if (num_stripes > LOV_MAX_STRIPE_COUNT) {
		*layout = NULL;
		rc = -EINVAL;
	} else {
		*layout = calloc(1, size);
		rc = *layout ? 0 : -ENOMEM;
	}

	return rc;
}

/**
 * Copy the data from a lov_user_md to a newly allocated lus_layout.
 *
 * The caller is responsible for freeing the returned pointer.
 *
 * \param[in] lum	LOV user metadata structure to copy data from
 * \param[in] object_count  number of objects in the layout
 * \param[out] lo       newly allocated layout
 *
 * \retval		0 with a new allocated layout
 * \retval		a negative errno on failure
 */
static int layout_from_lum(const struct lov_user_md *lum,
			   size_t object_count,
			   struct lus_layout **lo)
{
	struct lus_layout *layout;
	size_t objects_sz;
	int rc;

	objects_sz = object_count * sizeof(lum->lmm_objects[0]);

	rc = __layout_alloc(object_count, &layout);
	if (rc)
		return rc;

	layout->llot_magic = LLAPI_LAYOUT_MAGIC;

	if (lum->lmm_pattern == LOV_PATTERN_RAID0)
		layout->llot_pattern = LLAPI_LAYOUT_RAID0;
	else
		/* Lustre only supports RAID0 for now. */
		layout->llot_pattern = lum->lmm_pattern;

	if (lum->lmm_pattern & LOV_PATTERN_F_RELEASED)
		layout->llot_pattern_flags = LLAPI_LAYOUT_RELEASED;

	if (lum->lmm_stripe_size == 0)
		layout->llot_stripe_size = LLAPI_LAYOUT_DEFAULT;
	else
		layout->llot_stripe_size = lum->lmm_stripe_size;

	if (lum->lmm_stripe_count == (typeof(lum->lmm_stripe_count))-1)
		layout->llot_stripe_count = LLAPI_LAYOUT_WIDE;
	else if (lum->lmm_stripe_count == 0)
		layout->llot_stripe_count = LLAPI_LAYOUT_DEFAULT;
	else
		layout->llot_stripe_count = lum->lmm_stripe_count;

	/* Don't copy lmm_stripe_offset: it is always zero
	 * when reading attributes. */

	if (lum->lmm_magic != LOV_USER_MAGIC_V1) {
		const struct lov_user_md_v3 *lumv3;

		lumv3 = (struct lov_user_md_v3 *)lum;
		snprintf(layout->llot_pool_name, sizeof(layout->llot_pool_name),
			 "%s", lumv3->lmm_pool_name);
		memcpy(layout->llot_objects, lumv3->lmm_objects, objects_sz);
	} else {
		const struct lov_user_md_v1 *lumv1;

		lumv1 = (struct lov_user_md_v1 *)lum;
		memcpy(layout->llot_objects, lumv1->lmm_objects, objects_sz);
	}
	if (object_count > 0)
		layout->llot_objects_are_valid = true;

	*lo = layout;

	return 0;
}

/**
 * Copy the data from a lus_layout to a newly allocated lov_user_md.
 *
 * The caller is responsible for freeing the returned pointer.
 *
 * The current version of this API doesn't support specifying the OST
 * index of arbitrary stripes, only stripe 0 via lmm_stripe_offset.
 * There is therefore no need to copy the lmm_objects array.
 *
 * \param[in] layout	the layout to copy from
 *
 * \retval	valid lov_user_md pointer on success
 * \retval	NULL if memory allocation fails
 */
static struct lov_user_md *layout_to_lum(const struct lus_layout *layout)
{
	struct lov_user_md *lum;
	size_t lum_size;
	uint32_t magic = LOV_USER_MAGIC_V1;

	if (strlen(layout->llot_pool_name) != 0)
		magic = LOV_USER_MAGIC_V3;

	/* The lum->lmm_objects array won't be
	 * sent to the kernel when we write the lum, so
	 * we don't allocate storage for it.
	 */
	lum_size = lov_user_md_size(0, magic);
	lum = malloc(lum_size);
	if (lum == NULL)
		return NULL;

	lum->lmm_magic = magic;

	if (layout->llot_pattern == LLAPI_LAYOUT_DEFAULT)
		lum->lmm_pattern = 0;
	else if (layout->llot_pattern == LLAPI_LAYOUT_RAID0)
		lum->lmm_pattern = 1;
	else
		lum->lmm_pattern = layout->llot_pattern;

	if (layout->llot_pattern_flags & LLAPI_LAYOUT_RELEASED)
		lum->lmm_pattern |= LOV_PATTERN_F_RELEASED;

	memset(&lum->lmm_oi, 0, sizeof(lum->lmm_oi));

	if (layout->llot_stripe_size == LLAPI_LAYOUT_DEFAULT)
		lum->lmm_stripe_size = 0;
	else
		lum->lmm_stripe_size = layout->llot_stripe_size;

	if (layout->llot_stripe_count == LLAPI_LAYOUT_DEFAULT)
		lum->lmm_stripe_count = 0;
	else if (layout->llot_stripe_count == LLAPI_LAYOUT_WIDE)
		lum->lmm_stripe_count = -1;
	else
		lum->lmm_stripe_count = layout->llot_stripe_count;

	if (layout->llot_stripe_offset == LLAPI_LAYOUT_DEFAULT)
		lum->lmm_stripe_offset = -1;
	else
		lum->lmm_stripe_offset = layout->llot_stripe_offset;

	if (lum->lmm_magic != LOV_USER_MAGIC_V1) {
		struct lov_user_md_v3 *lumv3 = (struct lov_user_md_v3 *)lum;

		strncpy(lumv3->lmm_pool_name, layout->llot_pool_name,
			sizeof(lumv3->lmm_pool_name));
	}

	return lum;
}

/**
 * Get the parent directory of a path.
 *
 * \param[in] path	path to get parent of
 * \param[out] buf	buffer in which to store parent path
 * \param[in] size	size in bytes of buffer \a buf
 */
static void get_parent_dir(const char *path, char *buf, size_t size)
{
	char *p;

	strncpy(buf, path, size);
	p = strrchr(buf, '/');

	if (p != NULL)
		*p = '\0';
	else if (size >= 2)
		strncpy(buf, ".", 2);
}

/**
 * Substitute unspecified attribute values in \a dest with
 * values from \a src.
 *
 * \param[in] src	layout to inherit values from
 * \param[in] dest	layout to receive inherited values
 */
static void inherit_layout_attributes(const struct lus_layout *src,
				      struct lus_layout *dest)
{
	if (dest->llot_pattern == LLAPI_LAYOUT_DEFAULT)
		dest->llot_pattern = src->llot_pattern;

	if (dest->llot_stripe_size == LLAPI_LAYOUT_DEFAULT)
		dest->llot_stripe_size = src->llot_stripe_size;

	if (dest->llot_stripe_count == LLAPI_LAYOUT_DEFAULT)
		dest->llot_stripe_count = src->llot_stripe_count;
}

/**
 * Test if all attributes of \a layout are specified.
 *
 * \param[in] layout	the layout to check
 *
 * \retval true		all attributes are specified
 * \retval false	at least one attribute is unspecified
 */
static bool is_fully_specified(const struct lus_layout *layout)
{
	return  layout->llot_pattern != LLAPI_LAYOUT_DEFAULT &&
		layout->llot_stripe_size != LLAPI_LAYOUT_DEFAULT &&
		layout->llot_stripe_count != LLAPI_LAYOUT_DEFAULT;
}

/**
 * Allocate and initialize a new layout with \a num_stripes stripes.
 *
 * \param[in] num_stripes    number of stripes in new layout
 * \param[out]  layout       allocated layout
 *
 * \retval	valid lus_layout pointer on success
 * \retval	0 on success, or negative errno on failure.
 */
int lus_layout_alloc(unsigned int num_stripes, struct lus_layout **layout)
{
	struct lus_layout *lo;
	int rc;

	rc = __layout_alloc(num_stripes, &lo);
	if (rc)
		return rc;

	/* Set defaults. */
	lo->llot_magic = LLAPI_LAYOUT_MAGIC;
	lo->llot_pattern = LLAPI_LAYOUT_DEFAULT;
	lo->llot_stripe_size = LLAPI_LAYOUT_DEFAULT;
	if (num_stripes == 0) {
		lo->llot_stripe_count = LLAPI_LAYOUT_DEFAULT;
		lo->llot_objects_are_valid = false;
	} else {
		lo->llot_stripe_count = num_stripes;
		lo->llot_objects_are_valid = true;
	}
	lo->llot_stripe_offset = LLAPI_LAYOUT_DEFAULT;
	lo->llot_pool_name[0] = '\0';

	*layout = lo;

	return 0;
}

/**
 * Check if the given \a lum_size is large enough to hold the required
 * fields in \a lum.
 *
 * \param[in] lum	the struct lov_user_md to check
 * \param[in] lum_size	the number of bytes in \a lum
 *
 * \retval true		the \a lum_size is too small
 * \retval false	the \a lum_size is large enough
 */
static bool layout_lum_truncated(const struct lov_user_md *lum,
				 size_t lum_size)
{
	uint32_t magic;

	if (lum_size < lov_user_md_size(0, LOV_MAGIC_V1))
		return false;

	if (lum->lmm_magic == __bswap_32(LOV_MAGIC_V1) ||
	    lum->lmm_magic == __bswap_32(LOV_MAGIC_V3))
		magic = __bswap_32(lum->lmm_magic);
	else
		magic = lum->lmm_magic;

	return lum_size < lov_user_md_size(0, magic);
}

/**
 * Compute the number of elements in the lmm_objects array of \a lum
 * with size \a lum_size.
 *
 * \param[in] lum	the struct lov_user_md to check
 * \param[in] lum_size	the number of bytes in \a lum
 *
 * \retval		number of elements in array lum->lmm_objects
 */
static int layout_objects_in_lum(const struct lov_user_md *lum,
				 size_t lum_size)
{
	uint32_t magic;
	size_t base_size;

	if (lum_size < lov_user_md_size(0, LOV_MAGIC_V1))
		return 0;

	if (lum->lmm_magic == __bswap_32(LOV_MAGIC_V1) ||
	    lum->lmm_magic == __bswap_32(LOV_MAGIC_V3))
		magic = __bswap_32(lum->lmm_magic);
	else
		magic = lum->lmm_magic;

	base_size = lov_user_md_size(0, magic);

	if (lum_size <= base_size)
		return 0;
	else
		return (lum_size - base_size) / sizeof(lum->lmm_objects[0]);
}

/**
 * Return a layout from a given lustre.lov extended attribute.
 *
 * \param[in]  lum	 the extended lustre.lov attribute
 * \param[in]  lum_len	 size of the lum
 * \param[out] layout    returned layout. Set to NULL on error.
 *
 * \retval    0 on success, or negative errno on error.
 */
int lus_lovxattr_to_layout(struct lov_user_md *lum, size_t lum_len,
			   struct lus_layout **layout)
{
	int object_count;

	/* Return an error if we got a partial layout. */
	if (layout_lum_truncated(lum, lum_len)) {
		*layout = NULL;
		return -EINVAL;
	}

	object_count = layout_objects_in_lum(lum, lum_len);

	if (lum->lmm_magic == __bswap_32(LOV_MAGIC_V1) ||
	    lum->lmm_magic == __bswap_32(LOV_MAGIC_V3))
		layout_swab_lov_user_md(lum, object_count);

	return layout_from_lum(lum, object_count, layout);
}

/**
 * Get the striping layout for the file referenced by file descriptor \a fd.
 *
 * If the filesystem does not support the "lustre." xattr namespace, the
 * file must be on a non-Lustre filesystem, so set errno to ENOTTY per
 * convention.  If the file has no "lustre.lov" data, the file will
 * inherit default values, so return a default layout.
 *
 * If the kernel gives us back less than the expected amount of data,
 * we fail with -EINTR.
 *
 * \param[in] fd	open file descriptor
 * \param[out]  layout  requested layout
 *
 * \retval 0 on success
 * \retval a negative errno on failure, with layout set to NULL.
 */
int lus_layout_get_by_fd(int fd, struct lus_layout **layout)
{
	size_t lum_len;
	struct lov_user_md *lum;
	ssize_t bytes_read;
	int object_count;
	struct stat st;
	int rc;

	*layout = NULL;

	lum_len = XATTR_SIZE_MAX;
	lum = malloc(lum_len);
	if (lum == NULL)
		return -ENOMEM;

	bytes_read = fgetxattr(fd, XATTR_LUSTRE_LOV, lum, lum_len);
	if (bytes_read < 0) {
		if (errno == EOPNOTSUPP)
			rc = -ENOTTY;
		else if (errno == ENODATA)
			rc = lus_layout_alloc(0, layout);
		else
			rc = -errno;
		goto out;
	}

	/* Return an error if we got back a partial layout. */
	if (layout_lum_truncated(lum, bytes_read)) {
		rc = -EINTR;
		goto out;
	}

	object_count = layout_objects_in_lum(lum, bytes_read);

	/* Directories may have a positive non-zero lum->lmm_stripe_count
	 * yet have an empty lum->lmm_objects array. For non-directories the
	 * amount of data returned from the kernel must be consistent
	 * with the stripe count. */
	if (fstat(fd, &st) < 0) {
		rc = -errno;
		goto out;
	}

	if (!S_ISDIR(st.st_mode) && object_count != lum->lmm_stripe_count) {
		rc = -EINTR;
		goto out;
	}

	if (lum->lmm_magic == __bswap_32(LOV_MAGIC_V1) ||
	    lum->lmm_magic == __bswap_32(LOV_MAGIC_V3))
		layout_swab_lov_user_md(lum, object_count);

	rc = layout_from_lum(lum, object_count, layout);

out:
	free(lum);
	return rc;
}

/**
 * Get the expected striping layout for a file at \a path.
 *
 * Substitute expected inherited attribute values for unspecified
 * attributes.  Unspecified attributes may belong to directories and
 * never-written-to files, and indicate that default values will be
 * assigned when files are created or first written to.  A default value
 * is inherited from the parent directory if the attribute is specified
 * there, otherwise it is inherited from the filesystem root.
 * Unspecified attributes normally have the value LLAPI_LAYOUT_DEFAULT.
 *
 * The complete \a path need not refer to an existing file or directory,
 * but some leading portion of it must reside within a lustre filesystem.
 * A use case for this interface would be to obtain the literal striping
 * values that would be assigned to a new file in a given directory.
 *
 * \param[in] path	path for which to get the expected layout
 * \param[out]  layout  requested layout
 *
 * \retval 0 on success
 * \retval a negative errno on failure, with layout set to NULL.
 */
static int layout_expected(const char *path, struct lus_layout **layout)
{
	struct lus_layout	*path_layout = NULL;
	struct lus_layout	*donor_layout;
	char			donor_path[PATH_MAX];
	struct stat st;
	int fd;
	int rc;

	*layout = NULL;

	fd = open(path, O_RDONLY);
	if (fd < 0) {
		if (errno != ENOENT)
			return -errno;
		rc = -errno;
	} else {
		rc = lus_layout_get_by_fd(fd, &path_layout);
		close(fd);
	}

	if (path_layout == NULL) {
		if (rc != -ENODATA && rc != -ENOENT)
			return rc;

		rc = lus_layout_alloc(0, &path_layout);
		if (rc != 0)
			return rc;
	}

	if (is_fully_specified(path_layout)) {
		*layout = path_layout;
		return 0;
	}

	rc = stat(path, &st);
	if (rc < 0 && errno != ENOENT) {
		rc = -errno;
		lus_layout_free(path_layout);
		return rc;
	}

	/* If path is a not a directory or doesn't exist, inherit unspecified
	 * attributes from parent directory. */
	if ((rc == 0 && !S_ISDIR(st.st_mode)) ||
	    (rc < 0 && errno == ENOENT)) {
		get_parent_dir(path, donor_path, sizeof(donor_path));
		rc = lus_layout_get_by_path(donor_path, 0, &donor_layout);
		if (donor_layout != NULL) {
			inherit_layout_attributes(donor_layout, path_layout);
			lus_layout_free(donor_layout);
			if (is_fully_specified(path_layout)) {
				*layout = path_layout;
				return 0;
			}
		}
	}

	/* Inherit remaining unspecified attributes from the filesystem root. */
#if 0
	//TODO llapi_search_mounts not there yet
	rc = llapi_search_mounts(path, 0, donor_path, NULL);
#endif
	if (rc < 0) {
		lus_layout_free(path_layout);
		return rc;
	}
	rc = lus_layout_get_by_path(donor_path, 0, &donor_layout);
	if (rc != 0) {
		lus_layout_free(path_layout);
		return rc;
	}

	inherit_layout_attributes(donor_layout, path_layout);
	lus_layout_free(donor_layout);

	*layout = path_layout;
	return 0;
}

/**
 * Get the striping layout for the file at \a path.
 *
 * If \a flags contains LAYOUT_GET_EXPECTED, substitute
 * expected inherited attribute values for unspecified attributes. See
 * layout_expected().
 *
 * \param[in] path	path for which to get the layout
 * \param[in] flags	flags to control how layout is retrieved
 * \param[out]  layout  requested layout
 *
 * \retval 0 on success
 * \retval a negative errno on failure, with layout set to NULL.
 */
int lus_layout_get_by_path(const char *path, uint32_t flags,
			   struct lus_layout **layout)
{
	int fd;
	int rc;

	*layout = NULL;

	if (flags & LAYOUT_GET_EXPECTED)
		return layout_expected(path, layout);

	fd = open(path, O_RDONLY);
	if (fd < 0)
		return -errno;

	rc = lus_layout_get_by_fd(fd, layout);
	close(fd);

	return rc;
}

/**
 * Get the layout for the file with FID \a fidstr in filesystem \a lustre_dir.
 *
 * \param[in] lfsh	  An opaque handle returned by lus_open_fs()
 * \param[in] fid	  Lustre identifier of file to get layout for
 * \param[out]  layout    requested layout
 *
 * \retval 0 on success
 * \retval a negative errno on failure, with layout set to NULL.
 */
int lus_layout_get_by_fid(const struct lustre_fs_h *lfsh, const lustre_fid *fid,
			  struct lus_layout **layout)
{
	int fd;
	int rc;

	*layout = NULL;

	fd = lus_open_by_fid(lfsh, fid, O_RDONLY);
	if (fd < 0)
		return fd;

	rc = lus_layout_get_by_fd(fd, layout);
	close(fd);

	return rc;
}

/**
 * Free memory allocated for \a layout.
 *
 * \param[in] layout	layout to free
 */
void lus_layout_free(struct lus_layout *layout)
{
	free(layout);
}

/**
 * Get the stripe count of \a layout.
 *
 * \param[in] layout	layout to get stripe count from
 * \param[out] count	integer to store stripe count in
 *
 * \retval	0 on success
 * \retval	negative errno if arguments are invalid
 */
int lus_layout_stripe_get_count(const struct lus_layout *layout,
				uint64_t *count)
{
	if (layout == NULL || count == NULL ||
	    layout->llot_magic != LLAPI_LAYOUT_MAGIC)
		return -EINVAL;

	*count = layout->llot_stripe_count;

	return 0;
}

/*
 * The lus_layout API functions have these extra validity checks since
 * they use intuitively named macros to denote special behavior, whereas
 * the old API uses 0 and -1.
 */

static bool layout_stripe_count_is_valid(int64_t stripe_count)
{
	return stripe_count == LLAPI_LAYOUT_DEFAULT ||
		stripe_count == LLAPI_LAYOUT_WIDE ||
		(stripe_count != 0 && stripe_count != -1 &&
		 stripe_count_is_valid(stripe_count));
}

static bool layout_stripe_size_is_valid(uint64_t stripe_size)
{
	return stripe_size == LLAPI_LAYOUT_DEFAULT ||
		(stripe_size != 0 &&
		 stripe_size_is_aligned(stripe_size) &&
		 !stripe_size_is_too_big(stripe_size));
}

static bool layout_stripe_index_is_valid(int64_t stripe_index)
{
	return stripe_index == LLAPI_LAYOUT_DEFAULT ||
		(stripe_index >= 0 &&
		stripe_index_is_valid(stripe_index));
}

/**
 * Set the stripe count of \a layout.
 *
 * \param[in] layout	layout to set stripe count in
 * \param[in] count	value to be set
 *
 * \retval	0 on success
 * \retval	a negative errno if arguments are invalid
 */
int lus_layout_stripe_set_count(struct lus_layout *layout,
				uint64_t count)
{
	if (layout == NULL || layout->llot_magic != LLAPI_LAYOUT_MAGIC ||
	    !layout_stripe_count_is_valid(count))
		return -EINVAL;

	layout->llot_stripe_count = count;

	return 0;
}

/**
 * Get the stripe size of \a layout.
 *
 * \param[in] layout	layout to get stripe size from
 * \param[out] size	integer to store stripe size in
 *
 * \retval	0 on success
 * \retval	a negative errno if arguments are invalid
 */
int lus_layout_stripe_get_size(const struct lus_layout *layout,
			       uint64_t *size)
{
	if (layout == NULL || size == NULL ||
	    layout->llot_magic != LLAPI_LAYOUT_MAGIC)
		return -EINVAL;

	*size = layout->llot_stripe_size;

	return 0;
}

/**
 * Set the stripe size of \a layout.
 *
 * \param[in] layout	layout to set stripe size in
 * \param[in] size	value to be set
 *
 * \retval	0 on success
 * \retval	-1 if arguments are invalid
 */
int llapi_layout_stripe_size_set(struct lus_layout *layout,
				 uint64_t size)
{
	if (layout == NULL || layout->llot_magic != LLAPI_LAYOUT_MAGIC ||
	    !layout_stripe_size_is_valid(size)) {
		errno = EINVAL;
		return -1;
	}

	layout->llot_stripe_size = size;

	return 0;
}

/**
 * Get the RAID pattern of \a layout.
 *
 * \param[in] layout	layout to get pattern from
 * \param[out] pattern	integer to store pattern in
 *
 * \retval	0 on success
 * \retval	-1 if arguments are invalid
 */
int llapi_layout_pattern_get(const struct lus_layout *layout,
			     uint64_t *pattern)
{
	if (layout == NULL || pattern == NULL ||
	    layout->llot_magic != LLAPI_LAYOUT_MAGIC) {
		errno = EINVAL;
		return -1;
	}

	*pattern = layout->llot_pattern;

	return 0;
}

/**
 * Set the RAID pattern of \a layout.
 *
 * \param[in] layout	layout to set pattern in
 * \param[in] pattern	value to be set
 *
 * \retval	0 on success
 * \retval	-1 if arguments are invalid or RAID pattern
 *		is unsupported
 */
int llapi_layout_pattern_set(struct lus_layout *layout, uint64_t pattern)
{
	if (layout == NULL || layout->llot_magic != LLAPI_LAYOUT_MAGIC) {
		errno = EINVAL;
		return -1;
	}

	if (pattern != LLAPI_LAYOUT_DEFAULT &&
	    pattern != LLAPI_LAYOUT_RAID0) {
		errno = EOPNOTSUPP;
		return -1;
	}

	layout->llot_pattern = pattern;

	return 0;
}

/**
 * Set the extra pattern flag of \a layout.
 *
 * \param[in] layout	       layout to set pattern in
 * \param[in] pattern_flags    flags to set. Only
 *                             LLAPI_LAYOUT_RELEASED is supported.
 *
 * \retval	0 on success
 * \retval	-1 on error, with errno set.
 */
int llapi_layout_pattern_flags_set(struct lus_layout *layout,
				   uint64_t pattern_flags)
{
	if (layout == NULL || layout->llot_magic != LLAPI_LAYOUT_MAGIC) {
		errno = EINVAL;
		return -1;
	}

	/* Reject if an unknown flag is set */
	if ((pattern_flags & ~(LLAPI_LAYOUT_RELEASED)) != 0) {
		errno = EOPNOTSUPP;
		return -1;
	}

	layout->llot_pattern_flags = pattern_flags;

	return 0;
}

/**
 * Set the OST index of stripe number \a stripe_number to \a ost_index.
 *
 * The index may only be set for stripe number 0 for now.
 *
 * \param[in] layout		layout to set OST index in
 * \param[in] stripe_number	stripe number to set index for
 * \param[in] ost_index		the index to set
 *
 * \retval	0 on success
 * \retval	-1 if arguments are invalid or an unsupported stripe number
 *		was specified
 */
int llapi_layout_ost_index_set(struct lus_layout *layout, int stripe_number,
			       uint64_t ost_index)
{
	if (layout == NULL || layout->llot_magic != LLAPI_LAYOUT_MAGIC ||
	    !layout_stripe_index_is_valid(ost_index)) {
		errno = EINVAL;
		return -1;
	}

	if (stripe_number != 0) {
		errno = EOPNOTSUPP;
		return -1;
	}

	layout->llot_stripe_offset = ost_index;

	return 0;
}

/**
 * Get the OST index associated with stripe \a stripe_number.
 *
 * Stripes are indexed starting from zero.
 *
 * \param[in]   layout	       Layout to get index from
 * \param[in]   stripe_number  Stripe number to get index for
 * \param[out]  idx	       Integer to store index in
 *
 * \retval	0 on success
 * \retval	-1 or error, with errno set
 */
int llapi_layout_ost_index_get(const struct lus_layout *layout,
			       uint64_t stripe_number, uint64_t *idx)
{
	if (layout == NULL || layout->llot_magic != LLAPI_LAYOUT_MAGIC ||
	    stripe_number >= layout->llot_stripe_count ||
	    idx == NULL  || layout->llot_objects_are_valid == false) {
		errno = EINVAL;
		return -1;
	}

	if (layout->llot_objects[stripe_number].l_ost_idx == -1)
		*idx = LLAPI_LAYOUT_DEFAULT;
	else
		*idx = layout->llot_objects[stripe_number].l_ost_idx;

	return 0;
}

/**
 *
 * Get the pool name of layout \a layout.
 *
 * \param[in] layout	layout to get pool name from
 * \param[out] dest	buffer to store pool name in
 * \param[in] n		size in bytes of buffer \a dest
 *
 * \retval	0 on success
 * \retval	-1 if arguments are invalid
 */
int llapi_layout_pool_name_get(const struct lus_layout *layout, char *dest,
			       size_t n)
{
	if (layout == NULL || layout->llot_magic != LLAPI_LAYOUT_MAGIC ||
	    dest == NULL) {
		errno = EINVAL;
		return -1;
	}

	strncpy(dest, layout->llot_pool_name, n);

	return 0;
}

/**
 * Set the name of the pool of layout \a layout.
 *
 * \param[in] layout	layout to set pool name in
 * \param[in] pool_name	pool name to set
 *
 * \retval	0 on success
 * \retval	-1 if arguments are invalid or pool name is too long
 */
int llapi_layout_pool_name_set(struct lus_layout *layout,
			       const char *pool_name)
{
	char *ptr;

	if (layout == NULL || layout->llot_magic != LLAPI_LAYOUT_MAGIC ||
	    pool_name == NULL) {
		errno = EINVAL;
		return -1;
	}

	/* Strip off any 'fsname.' portion. */
	ptr = strchr(pool_name, '.');
	if (ptr != NULL)
		pool_name = ptr + 1;

	if (strlen(pool_name) > LOV_MAXPOOLNAME) {
		errno = EINVAL;
		return -1;
	}

	strncpy(layout->llot_pool_name, pool_name,
		sizeof(layout->llot_pool_name));

	return 0;
}

/* Helper for llapi_layout_file_open and llapi_layout_file_openat. */
static int file_open_internal(int dir_fd, const char *path,
			      int open_flags, mode_t mode,
			      const struct lus_layout *layout)
{
	int fd;
	int rc;
	int tmp;
	struct lov_user_md *lum;
	size_t lum_size;

	if (path == NULL ||
	    (layout != NULL && layout->llot_magic != LLAPI_LAYOUT_MAGIC)) {
		errno = EINVAL;
		return -1;
	}

	/* Object creation must be postponed until after layout attributes
	 * have been applied. */
	if (layout != NULL && (open_flags & O_CREAT))
		open_flags |= O_LOV_DELAY_CREATE;

	if (dir_fd == -1)
		fd = open(path, open_flags, mode);
	else
		fd = openat(dir_fd, path, open_flags, mode);

	if (layout == NULL || fd < 0)
		return fd;

	lum = layout_to_lum(layout);

	if (lum == NULL) {
		tmp = errno;
		close(fd);
		errno = tmp;
		return -1;
	}

	lum_size = lov_user_md_size(0, lum->lmm_magic);

	rc = fsetxattr(fd, XATTR_LUSTRE_LOV, lum, lum_size, 0);
	if (rc < 0) {
		tmp = errno;
		close(fd);
		errno = tmp;
		fd = -1;
	}

	free(lum);
	errno = errno == EOPNOTSUPP ? ENOTTY : errno;

	return fd;
}

/**
 * Open and possibly create a file with a given \a layout.
 *
 * If \a layout is NULL this function acts as a simple wrapper for
 * open().  By convention, ENOTTY is returned in errno if \a path
 * refers to a non-Lustre file.
 *
 * \param[in] path		name of the file to open
 * \param[in] open_flags	open() flags
 * \param[in] mode		permissions to create new file with
 * \param[in] layout		layout to create new file with
 *
 * \retval		non-negative file descriptor on successful open
 * \retval		-1 if an error occurred
 */
int llapi_layout_file_open(const char *path, int open_flags, mode_t mode,
			   const struct lus_layout *layout)
{
	return file_open_internal(-1, path, open_flags, mode, layout);
}

/**
 * Open and possibly create a file with a given \a layout.
 *
 * If \a layout is NULL this function acts as a simple wrapper for
 * open().  By convention, ENOTTY is returned in errno if \a path
 * refers to a non-Lustre file.
 *
 * \param[in] dir_fd		open descriptor for a directory
 * \param[in] path		relative name of the file to open
 * \param[in] open_flags	open() flags
 * \param[in] mode		permissions to create new file with
 * \param[in] layout		layout to create new file with
 *
 * \retval		non-negative file descriptor on successful open
 * \retval		-1 if an error occurred
 */
int llapi_layout_file_openat(int dir_fd, const char *path, int open_flags,
			     mode_t mode, const struct lus_layout *layout)
{
	return file_open_internal(dir_fd, path, open_flags, mode, layout);
}

/**
 * Create a file with a given \a layout.
 *
 * Force O_CREAT and O_EXCL flags on so caller is assured that file was
 * created with the given \a layout on successful function return.
 *
 * \param[in] path		name of the file to open
 * \param[in] open_flags	open() flags
 * \param[in] mode		permissions to create new file with
 * \param[in] layout		layout to create new file with
 *
 * \retval		non-negative file descriptor on successful open
 * \retval		-1 if an error occurred
 */
int llapi_layout_file_create(const char *path, int open_flags, int mode,
			     const struct lus_layout *layout)
{
	return file_open_internal(-1, path, open_flags | O_CREAT | O_EXCL,
				  mode, layout);
}
