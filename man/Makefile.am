MANFILES = liblustre.7 llapi_hsm_action_begin.3 \
	llapi_hsm_action_end.3 llapi_hsm_action_get_dfid.3 \
	llapi_hsm_action_get_fd.3 llapi_hsm_action_progress.3 \
	llapi_hsm_copytool_get_fd.3 llapi_hsm_copytool_recv.3 \
	llapi_hsm_copytool_register.3 llapi_hsm_copytool_unregister.3 \
	llapi_create_volatile.3

.rst.3:
	@RST2MAN@ $? $@

.rst.7:
	@RST2MAN@ $? $@

man_MANS = $(MANFILES)

EXTRA_DIST = *.rst

# TODO: should clean generated pages.
