ACLOCAL_AMFLAGS = -I m4

EXTRA_DIST = Changelog liblustre.spec.in liblustre.spec GITHASH

SUBDIRS = include lib man tests

# Packaging
rpm_dir=`pwd`/rpms
rpmbuild_opt=

check-valgrind:
	make -C tests check-valgrind

rpms: dist
	mkdir -p $(rpm_dir)/BUILD $(rpm_dir)/SPECS $(rpm_dir)/SRPMS $(rpm_dir)/RPMS
	rpmbuild $(rpmbuild_opt) --define="_topdir $(rpm_dir)" \
		--define="_prefix $(prefix)" -ta $(DIST_ARCHIVES)

srpm: dist
	mkdir -p $(rpm_dir)/BUILD $(rpm_dir)/SPECS $(rpm_dir)/SRPMS $(rpm_dir)/RPMS
	rpmbuild $(rpmbuild_opt) --define="_topdir $(rpm_dir)" \
		--define="_prefix $(prefix)" -ts $(DIST_ARCHIVES)

