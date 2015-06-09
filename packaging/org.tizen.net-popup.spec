%bcond_with wayland
%bcond_with x

%define _usrdir /usr
%define _appdir %{_usrdir}/apps

Name:       org.tizen.net-popup
Summary:    Network Notification Popup application
Version:    0.2.1_18
Release:    0
Group:      App/Network
License:    Apache-2.0
Source0:    %{name}-%{version}.tar.gz
Source1001:    org.tizen.net-popup.manifest
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
BuildRequires: pkgconfig(notification)
BuildRequires: pkgconfig(appsvc)
BuildRequires: gettext

%description
Network Notification Popup application

%prep
%setup -q
cp %{SOURCE1001} .


%build
%cmake . -DCMAKE_INSTALL_PREFIX=%{_prefix} \
%if %{with wayland}
        -DWAYLAND_SUPPORT=On \
%else
        -DWAYLAND_SUPPORT=Off \
%endif
%if %{with x}
        -DX11_SUPPORT=On \
%else
        -DX11_SUPPORT=Off \
%endif
        #eol

make %{?_smp_mflags} V=1


%install
%make_install

mkdir -p %{buildroot}%{_sysconfdir}/smack/accesses.d/

#License
mkdir -p %{buildroot}%{_datadir}/license
cp LICENSE.APLv2 %{buildroot}%{_datadir}/license/org.tizen.net-popup


%files
%manifest %{name}.manifest
%{_appdir}/org.tizen.net-popup/bin/net-popup
%{_datadir}/packages/org.tizen.net-popup.xml
%license %{_datadir}/license/org.tizen.net-popup
%{_datadir}/locale/*/LC_MESSAGES/net-popup.mo
