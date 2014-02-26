%bcond_with wayland

%define _usrdir /usr
%define _appdir %{_usrdir}/apps

Name:       org.tizen.net-popup
Summary:    Network Notification Popup application
Version:    0.2.1_17
Release:    1
Group:      App/Network
License:    Flora License
Source0:    %{name}-%{version}.tar.gz
Source1001: 	org.tizen.net-popup.manifest
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
cp %{SOURCE1001} .


%build
%cmake . -DCMAKE_INSTALL_PREFIX=%{_prefix} \
%if %{with wayland}
        -DWAYLAND_SUPPORT=On
%else
        -DWAYLAND_SUPPORT=Off
%endif

make %{?_smp_mflags}


%install
%make_install

mkdir -p %{buildroot}%{_sysconfdir}/smack/accesses.d/
cp -v org.tizen.net-popup.rule %{buildroot}%{_sysconfdir}/smack/accesses.d/

#License
mkdir -p %{buildroot}%{_datadir}/license
cp LICENSE.Flora %{buildroot}%{_datadir}/license/org.tizen.net-popup


%files
%manifest %{name}.manifest
%{_appdir}/org.tizen.net-popup/bin/net-popup
%{_datadir}/packages/org.tizen.net-popup.xml
%{_datadir}/license/org.tizen.net-popup
%{_datadir}/locale/*/LC_MESSAGES/net-popup.mo
%{_sysconfdir}/smack/accesses.d/org.tizen.net-popup.rule
