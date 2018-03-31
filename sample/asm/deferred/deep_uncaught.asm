;
;   Copyright (C) 2017 Marek Marecki
;
;   This file is part of Viua VM.
;
;   Viua VM is free software: you can redistribute it and/or modify
;   it under the terms of the GNU General Public License as published by
;   the Free Software Foundation, either version 3 of the License, or
;   (at your option) any later version.
;
;   Viua VM is distributed in the hope that it will be useful,
;   but WITHOUT ANY WARRANTY; without even the implied warranty of
;   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;   GNU General Public License for more details.
;
;   You should have received a copy of the GNU General Public License
;   along with Viua VM.  If not, see <http://www.gnu.org/licenses/>.
;

.function: by_quux/0
    print (text %iota local "Hello from by_quux/0") local
    return
.end

.function: quux/0
    frame %0
    defer by_quux/0

    throw (integer %iota local 666) local

    return
.end

.function: by_bax/0
    print (text %iota local "Hello from by_bax/0") local
    return
.end

.function: bax/0
    frame %0
    defer by_bax/0

    frame %0
    call quux/0

    return
.end

.function: by_bay/0
    print (text %iota local "Hello from by_bay/0") local
    return
.end

.function: bay/0
    frame %0
    defer by_bay/0

    frame %0
    call bax/0

    return
.end

.function: by_baz/0
    print (text %iota local "Hello from by_baz/0") local
    return
.end

.function: baz/0
    frame %0
    defer by_baz/0

    frame %0
    call bay/0

    return
.end

.function: by_bar/0
    print (text %iota local "Hello from by_bar/0") local
    return
.end

.function: bar/0
    frame %0
    defer by_bar/0

    frame %0
    call baz/0

    return
.end

.function: by_foo/0
    print (text %iota local "Hello from by_foo/0") local
    return
.end

.function: foo/0
    frame %0
    defer by_foo/0

    frame %0
    call bar/0

    return
.end

.function: by_main/0
    print (text %iota local "Hello from by_main/0") local
    return
.end

.function: main/0
    frame %0
    defer by_main/0

    frame %0
    call foo/0

    izero %0 local
    return
.end
