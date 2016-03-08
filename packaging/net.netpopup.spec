Name:       net.netpopup
Summary:    Network Notification Popup applicationa
Version:    0.2.79
Release:    1
Group:      App/Network
License:    Flora-1.1
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
BuildRequires: pkgconfig(capi-appfw-application)
BuildRequires: pkgconfig(gio-2.0)
BuildRequires: pkgconfig(glib-2.0)
BuildRequires: pkgconfig(notification)
BuildRequires: pkgconfig(appsvc)
BuildRequires: pkgconfig(vconf)
BuildRequires: pkgconfig(efl-extension)
BuildRequires: gettext
BuildRequires: edje-tools

%description
Network Notification Popup application

%prep
%setup -q


%build
cmake . -DCMAKE_INSTALL_PREFIX=%{_prefix}

make %{?_smp_mflags}


%install
%make_install

mkdir -p %{buildroot}%{_sysconfdir}/smack/accesses.d/
cp -v net.netpopup.efl %{buildroot}%{_sysconfdir}/smack/accesses.d/

#License
mkdir -p %{buildroot}%{_datadir}/license
cp LICENSE %{buildroot}%{_datadir}/license/net.netpopup


%files
%manifest net.netpopup.manifest
%{_bindir}/net-popup
%{_datadir}/packages/net.netpopup.xml
/usr/ug/res/edje/net-popup/*.edj
%{_datadir}/locale/*/LC_MESSAGES/net-popup.mo
%{_datadir}/license/net.netpopup
%{_sysconfdir}/smack/accesses.d/net.netpopup.efl
