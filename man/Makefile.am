# Non-generated man pages.
dist_man_MANS = \
	llapi_hsm_action_progress.3 \
	llapi_hsm_action_get_fd.3 \
	llapi_hsm_action_get_dfid.3 \
	llapi_hsm_action_end.3 \
	llapi_hsm_copytool_unregister.3 \
	llapi_hsm_copytool_recv.3 \
	llapi_hsm_copytool_get_fd.3

# Generated man pages. The RST is distributed instead.
nodist_man_MANS = \
	liblustre.7 \
	llapi_create_volatile_by_fid.3 \
	llapi_hsm_action_begin.3 \
	llapi_hsm_copytool_register.3

# Generated man pages

.rst.3:
	@RST2MAN@ $? $@

.rst.7:
	@RST2MAN@ $? $@

man_MANS = $(MANFILES)

EXTRA_DIST = \
	liblustre.rst \
	llapi_create_volatile_by_fid.rst \
	llapi_hsm_action_begin.rst \
	llapi_hsm_copytool_register.rst

CLEANFILES = $(nodist_man_MANS)
