#
# spec file for package atomiks
#
# Copyright (c) 2013, 2014, 2015 Mateusz Viste
#
# All modifications and additions to the file contributed by third parties
# remain the property of their copyright owners, unless otherwise agreed
# upon. The license for this file, and modifications and additions to the
# file, is the same license as for the pristine package itself (unless the
# license for the pristine package is not an Open Source License, in which
# case the license is the MIT License). An "Open Source License" is a
# license that conforms to the Open Source Definition (Version 1.9)
# published by the Open Source Initiative.
#
# Please submit bugfixes or comments via http://bugs.opensuse.org/
#

Name: atomiks
Version: 1.0.4
Release: 1%{?dist}
Summary: A faithful remake of, and a tribute to, Atomix, a classic puzzle game

Group: Amusements/Games/Logic

License: GPL-3.0+
URL: http://atomiks.sourceforge.net/
Source0: %{name}-%{version}.tar.gz

BuildRequires: SDL-devel
BuildRequires: SDL_mixer-devel
#BuildRequires: libmikmod-devel

%description
Atomiks is a faithful remake of, and a tribute to, Atomix, a classic puzzle game created by Softtouch & RoSt and published in 1990 by the Thalion Software company. Atomiks is free software, and shares no code with the original Atomix game.

%prep
%setup

%build
make

%check

%install
install -D atomiks %buildroot/%{_bindir}/atomiks
mkdir -p %buildroot/usr/share/icons/hicolor/64x64/apps/
install -D atomiks.png %buildroot/usr/share/icons/hicolor/64x64/apps/

%files
%dir /usr/
%dir /usr/share/
%dir /usr/share/icons/
%dir /usr/share/icons/hicolor/
%dir /usr/share/icons/hicolor/64x64/
%dir /usr/share/icons/hicolor/64x64/apps
%attr(644, root, root) %doc readme.txt license.txt history.txt
%attr(755, root, root) %{_bindir}/atomiks
%attr(644, root, root) /usr/share/icons/hicolor/64x64/apps/atomiks.png

%changelog
