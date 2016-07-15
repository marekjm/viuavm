#!/usr/bin/env bash

#
#   Copyright (C) 2016 Marek Marecki
#
#   This file is part of Viua VM.
#
#   Viua VM is free software: you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation, either version 3 of the License, or
#   (at your option) any later version.
#
#   Viua VM is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with Viua VM.  If not, see <http://www.gnu.org/licenses/>.
#

if [[ $(g++ --version | head -n 1 | grep -Po 'g\+\+ \(GCC\) 6' | cat) == 'g++ (GCC) 6' ]]; then
    CXX_STANDARD='c++14'
else
    CXX_STANDARD='c++11'
fi

sed -i "s:CXX_STANDARD=.*:CXX_STANDARD=$CXX_STANDARD:g" Makefile
