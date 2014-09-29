# Wben building an rpm from random git revision create a tag
# something like:
#
#	git tag -a v1.0
#
# followed by
#
#	./bootstrap && ./configure && make dist

Name:		outlier
Version:	1.0
Release:	1
Summary:	Outlier analysis
Group:		Applications/Engineering
License:	BSD
URL:		https://github.com/kerolasa/outlier
Source0:	%{name}-%{version}.tar.xz
BuildRoot:	%{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)

BuildRequires: xz
BuildRequires: autoconf >= 2.59
BuildRequires: automake

%description
This command reads from input files, and does statistics analysis to
the values.  Output is textual information about input number ranges,
that will tell how much values vary and where outliers points are.  The
output can be used for example to determine reasonable alarming
criteria from metrics collections.

%prep
%setup -q

%build
%configure
make %{?_smp_mflags}

%install
[ "%{buildroot}" != / ] && %{__rm} -rf "%{buildroot}"
%{__make} install DESTDIR=%{buildroot}

%clean
rm -rf %{buildroot}

%files
%defattr(-,root,root,-)
%doc COPYING
%doc ChangeLog
%doc %_mandir/man*/*
%_bindir/*
%_datadir/%{name}/*

%changelog
* Mon Sep 29 2014  Sami Kerola <kerolasa@iki.fi>
- First version of the rpm spec file.
