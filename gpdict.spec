#
# GPDICT specfile
#
# (C) Cyril Hrubis metan{at}ucw.cz 2013-2023
#

Summary: A dictionary viewer for a stardict compatible formats
Name: gpdict
Version: git
Release: 1
License: GPL-2.0-or-later
Group: Productivity/Office/Dictionary
Url: https://github.com/gfxprim/gpdict
Source: gpdict-%{version}.tar.bz2
BuildRequires: libgfxprim-devel
BuildRequires: libstardict-devel
BuildRequires: libgfxprim-curl-devel

BuildRoot: %{_tmppath}/%{name}-%{version}-buildroot

%description
A dictionary viewer for a stardict compatible formats

%prep
%setup -n gpdict-%{version}

%build
make %{?jobs:-j%jobs}

%install
DESTDIR="$RPM_BUILD_ROOT" make install

%files -n gpdict
%defattr(-,root,root)
%{_bindir}/gpdict
%{_sysconfdir}/gp_apps/
%{_sysconfdir}/gp_apps/gpdict/
%{_sysconfdir}/gp_apps/gpdict/*
%{_datadir}/applications/gpdict.desktop
%{_datadir}/gpdict/
%{_datadir}/gpdict/gpdict.png

%changelog
* Mon Jan 30 2023 Cyril Hrubis <metan@ucw.cz>

  Initial version.
