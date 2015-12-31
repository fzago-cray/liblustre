====================================================
Converting a program using liblustreapi to liblustre
====================================================

This document describes the differences between upstream liblustreapi
and liblustre (this library).

liblustre is a smaller version of liblustreapi, from which it inherits
some design. All functions and symbols it exports are prefixed with
lus_ instead of llapi_.

Headers
~~~~~~~

There is only one header to include. Change::

  #include <lustre/liblustreapi.h>

to::

  #include <lustre/lustre.h>

Other Lustre headers, such as as lustre_user.h or the libcfs headers,
do not exist and must be removed.

New APIs
~~~~~~~~

A few new functions with no equivalent in liblustreapi have been
added::

  lus_open_fs / lus_close_fs
  lus_get_fsname
  lus_get_mountpoint
  lus_fid2parent
  lus_lovxattr_to_layout

Defines
~~~~~~~

Some defines are gone::

  LPU64: use "%llu" instead. Cast if necessary.
  LPX64: use "%\#llx". Cast if necessary.
  LPX64i: use "%llx". Cast if necessary.

LOV_MAXPOOLNAME is replaced by LUS_POOL_NAME_LEN, which has an extra
byte to store the NUL terminator.
i.e. LUS_POOL_NAME_LEN = LOV_MAXPOOLNAME + 1

Structures
~~~~~~~~~~

For consistency and to better reflect their roles, some stuctures
have been renamed::

  struct hsm_copyaction_private to struct lus_hsm_action_handle
  struct llapi_layout to struct lus_layout


API convertion
~~~~~~~~~~~~~~

All functions now return a 0 on success, or a negative errno on
failure. errno is not set anymore by the library. Some functions can
return a meaningful positive number on success (such as
lus_get_mdt_index_by_fid).

If the function returns some allocated memory (such as a layout), it
is done through a parameter.

Some functions need an open filesystem handle, returned by
lus_open_fs, which usually replaces a path.



OLD                              NEW                            COMMENTS

dot_lustre_name                | removed
hai_first                      | lus_hsm_hai_first
hai_next                       | lus_hsm_hai_next
llapi_chomp_string             | removed
llapi_create_volatile_idx      | lus_create_volatile_by_fid
llapi_error                    |                              | The library doesn't provide logging facilities for the
                                                              | application.
llapi_fd2fid   	               | lus_fd2fid
llapi_fd2parent                | lus_fd2parent
llapi_fid2path                 | lus_fid2path                 | will return an empty string instead of / if the
                                                              | mountpoint is given. recno and linkno can now be NULL.
llapi_file_open_pool           | lus_layout_file_open or lus_layout_file_openat
llapi_file_open_param          | lus_layout_file_open or lus_layout_file_openat
llapi_fswap_layouts            | lus_fswap_layouts
llapi_get_data_version         | lus_data_version_by_fd       | the last two parameters are swapped
llapi_get_mdt_index_by_fid     | lus_get_mdt_index_by_fid     | directly returns the MDT index
llapi_group_lock               | lus_group_lock               | the gid is now an uint64_t instead of an int
llapi_group_unlock             | lus_group_unlock             | the gid is now an uint64_t instead of an int
llapi_hsm_action_begin         | lus_hsm_action_begin
llapi_hsm_action_end           | lus_hsm_action_end
llapi_hsm_action_get_dfid      | lus_hsm_action_get_dfid
llapi_hsm_action_get_fd        | lus_hsm_action_get_fd
llapi_hsm_action_progress      | lus_hsm_action_progress
llapi_hsm_copytool_get_fd      | lus_hsm_copytool_get_fd
llapi_hsm_copytool_recv        | lus_hsm_copytool_recv        | reading an HSM event is non blocking. lus_hsm_copytool_recv
                                                              | can return -EWOULDBLOCK so the caller should handle that
                                                              | condition
llapi_hsm_copytool_register    | lus_hsm_copytool_register
llapi_hsm_copytool_unregister  | lus_hsm_copytool_unregister
llapi_hsm_current_action       | lus_hsm_current_action
llapi_hsm_import               | lus_hsm_import               | The striping information is now a layout structure. It
                                                              | returns a file descriptor which can be used by the caller
                                                              | to set the extended attributes. The fid parameter is no
                                                              | longer needed since it can be retrieved by the caller
                                                              | through the fd.
llapi_hsm_request              | lus_hsm_request
llapi_hsm_state_get            | lus_hsm_state_get
llapi_hsm_state_get_fd         | lus_hsm_state_get_fd
llapi_hsm_state_set            | lus_hsm_state_set
llapi_hsm_state_set_fd         | lus_hsm_state_set_fd
llapi_hsm_user_request_alloc   |                              | The caller can replace
                                                              |   hur = llapi_hsm_user_request_alloc(x, y)
                                                              | with
                                                              |   hur = malloc(lus_hsm_user_request_len(x, y))
                                                              | This is more flexible as the caller can reuse the hur,
                                                              | resetting it between calls to lus_hsm_request.
llapi_init                     | lus_init
llapi_initialized              | lus_initialized
llapi_layout_alloc             | lus_layout_alloc
llapi_layout_file_create       | lus_layout_file_create
llapi_layout_file_open         | lus_layout_file_open
llapi_layout_file_openat       | lus_layout_file_openat
llapi_layout_free              | lus_layout_free
llapi_layout_get_by_fd         | lus_layout_get_by_fd
llapi_layout_get_by_fid        | lus_layout_get_by_fid
llapi_layout_get_by_path       | lus_layout_get_by_path
llapi_layout_ost_index_get     | lus_layout_get_ost_index
llapi_layout_pool_name_get     | lus_layout_get_pool_name
llapi_layout_pattern_get       | lus_layout_pattern_get
llapi_layout_pattern_set       | lus_layout_pattern_set
llapi_layout_pattern_set_flags | lus_layout_pattern_set_flags
llapi_layout_ost_index_set     | lus_layout_set_ost_index
llapi_layout_pool_name_set     | lus_layout_set_pool_name
llapi_layout_stripe_count_get  | lus_layout_stripe_get_count
llapi_layout_stripe_size_get   | lus_layout_stripe_get_size
llapi_layout_stripe_count_set  | lus_layout_stripe_set_count
llapi_layout_stripe_size_set   | lus_layout_stripe_set_size
llapi_log_set_callback         | lus_log_set_callback
llapi_log_set_level            | lus_log_set_level
llapi_open_by_fid              | lus_open_by_fid
llapi_path2fid                 | lus_path2fid
llapi_path2parent              | lus_path2parent
llapi_printf                   |                              | The library doesn't provide logging facilities for the
                                                              | application.
llapi_stat_by_fid              | lus_stat_by_fid
llapi_stripe_limit_check       | llapi_layout_stripe_*_is_valid
