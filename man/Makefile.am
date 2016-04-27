# Non-generated man pages.
dist_man_MANS = \
	lus_hsm_action_progress.3 \
	lus_hsm_action_get_fd.3 \
	lus_hsm_action_get_dfid.3 \
	lus_hsm_action_end.3 \
	lus_hsm_copytool_unregister.3 \
	lus_hsm_copytool_recv.3 \
	lus_hsm_copytool_get_fd.3 \
	lus_close_fs.3

# Generated man pages. The RST is distributed instead.
nodist_man_MANS = \
	liblustre.7 \
	lus_create_volatile_by_fid.3 \
	lus_hsm_action_begin.3 \
	lus_hsm_copytool_register.3 \
	lus_stat_by_fid.3 \
	lus_mdt_stat_by_fid.3 \
	lus_open_fs.3

# Generated man pages

.rst.3:
	@RST2MAN@ $? $@

.rst.7:
	@RST2MAN@ $? $@

man_MANS = $(MANFILES)

EXTRA_DIST = \
	liblustre.rst \
	lus_create_volatile_by_fid.rst \
	lus_hsm_action_begin.rst \
	lus_hsm_copytool_register.rst \
	lus_stat_by_fid.rst \
	lus_mdt_stat_by_fid.rst \
	lus_open_fs.rst

CLEANFILES = $(nodist_man_MANS)
