Name:		macfanctld
Version:	0.6
Release:	1%{?dist}
Summary:	Fan control daemon for MacBook

Group:		System Environment/Daemons
License:	GPLv3
URL:		https://github.com/MikaelStrom/macfanctld
Source0:	https://github.com/bpiddubnyi/macfanctld/archive/v%{version}.tar.gz

%description
This daemon is a customizable fan control to keep the working temperature 
within resonable limits. Se manpage macfanctld(1) for more information.


%prep
%setup -q


%build
make
gzip -k macfanctld.1


%install
install -D -p -m 755 macfanctld -t %{buildroot}%{_bindir}/
install -D -p -m 644 macfanctl.conf -t %{buildroot}%{_sysconfdir}/
install -D -p -m 644 macfanctld.1.gz -t %{buildroot}%{_mandir}/man1/
install -D -p -m 644 macfanctld.service -t %{buildroot}%{_libdir}/systemd/system/


%files
%{_bindir}/macfanctld
%config %{_sysconfdir}/macfanctl.conf
%{_mandir}/man1/macfanctld.1.gz
%{_libdir}/systemd/system/macfanctld.service



%changelog
* Wed Nov 04 2015 Borys piddubnyi <zhu@zhu.org.ua> - 0.6-1
- Initial RPM package
