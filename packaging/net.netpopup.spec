Name:       net.netpopup
Summary:    Network Notification Popup application
Version:    0.2.1_10
Release:    1
Group:      App/Network
License:    Flora License
Source0:    %{name}-%{version}.tar.gz
BuildRequires: cmake
BuildRequires: pkgconfig(appcore-efl)
BuildRequires: pkgconfig(bundle)
BuildRequires: pkgconfig(dlog)
BuildRequires: pkgconfig(ecore)
BuildRequires: pkgconfig(edje)
BuildRequires: pkgconfig(elementary)
BuildRequires: pkgconfig(evas)
BuildRequires: pkgconfig(syspopup)
BuildRequires: pkgconfig(syspopup-caller)
BuildRequires: pkgconfig(glib-2.0)
BuildRequires: pkgconfig(status)
BuildRequires:	pkgconfig(notification)
BuildRequires:	pkgconfig(appsvc)
BuildRequires: gettext

%description
Network Notification Popup application

%prep
%setup -q


%build
cmake . -DCMAKE_INSTALL_PREFIX=%{_prefix}

make %{?_smp_mflags}


%install
%make_install

mkdir -p %{buildroot}%{_sysconfdir}/smack/accesses2.d/
cp -v net.netpopup.rule %{buildroot}%{_sysconfdir}/smack/accesses2.d/

#License
mkdir -p %{buildroot}%{_datadir}/license
cp LICENSE.Flora %{buildroot}%{_datadir}/license/net.netpopup


%files
%manifest net.netpopup.manifest
%{_bindir}/net-popup
%{_datadir}/process-info/net-popup.ini
%{_datadir}/packages/net.netpopup.xml
%{_datadir}/locale/*/LC_MESSAGES/net-popup.mo
%{_datadir}/license/net.netpopup
%{_sysconfdir}/smack/accesses2.d/net.netpopup.rule
