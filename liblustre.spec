%global githash %(sh -c "git rev-parse --short HEAD | tr -d '\n'")

Name:		liblustre
Version:	0.1.0
Release:	g%{githash}
Summary:	An alternate user space library for Lustre
Group:		System Environment/Libraries
# TODO: will need to change to LGPLv2+
License:	GPLv2+
URL:		https://github.com/fzago-cray/liblustre
Source0:	%{name}-%{version}.tar.xz

BuildRequires:	python-docutils check

%description

A alternate library for Lustre that can be installed in parallel with
liblustreapi. The APIs provided by liblustre are slightly different
than those provided by liblustreapi.

%package devel
Summary:	%{name} development package
Group:		Development/Libraries
Requires:	%{name} = %{version}

%description devel
Development package for %{name}.

%prep
%setup -q

%build
%configure
make %{?_smp_mflags}

%install
%make_install

%files
%defattr(-,root,root,-)
%{_libdir}/%{name}.so*

%files devel
%defattr(-,root,root,-)
%{_includedir}
%{_libdir}/pkgconfig
%{_libdir}/%{name}.la
%{_mandir}/man3/*
%{_mandir}/man7/*
