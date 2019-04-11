# WAD Tool Suite

Copyright (C) 2018-2019 Matt Tropiano

## NOTICE

This code and the utility libraries are really, really not suitable for general consumption yet.

Caveat emptor.


## Introduction

This repo contains C code for various tiny utilities around Doom and Doom Engine-related things. 
It also didn't look like there was one definitive lib, so I took it upon myself to ever-so-slowly make it.


## Why?

There are currently a lack of command-line driven utilities for doing small WAD-related things like
merging, importing, or searching, and this can also serve as a common point for a C-language library
for lots of common Doom Engine things (which will probably be separated out into other repos at some point).

This sort of thing also helps with batching together lots of little WAD operations for when larger projects
need assembly - something sorely lacking on modern platforms in an automatable way.


Also, I needed an excuse to re-learn C. Everyone's gotta know C, right?


## Building this Stuff

A Makefile is provided for this project. This has been tested in the following environments
and toolchains:

* Windows 7 x64
* MinGW (GCC 4.8.1)

Yup, that's it. Who knows what the future holds!


## To Build

To build:

	make

To clean up the build directories:

	make clean


## Other

This program/library and the accompanying materials
are made available under the terms of the GNU Lesser Public License v2.1
which accompanies this distribution, and is available at
http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html

A copy of the LGPL should have been included in this release (LICENSE.txt).
If it was not, please contact me for a copy, or to notify me of a distribution
that has not included it. 

