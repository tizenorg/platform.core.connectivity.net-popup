#sbs-git:pkgs/n/net-popup net.netpopup

Name:       net.netpopup
Summary:    Network Notification Popup application
Version:    0.1.16
Release:    6
Group:      App/Network
License:    Samsung Proprietary License
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
BuildRequires: gettext

%description
Network Notification Popup application


%prep
%setup -q

%build
cmake . -DCMAKE_INSTALL_PREFIX=%{_prefix}
make %{?jobs:-j%jobs}


%install
rm -rf %{buildroot}
%make_install


%files
%{_bindir}/net-popup
%{_prefix}/share/process-info/net-popup.ini
/opt/share/applications/net.netpopup.desktop
%{_prefix}/share/locale/*/LC_MESSAGES/net-popup.mo
