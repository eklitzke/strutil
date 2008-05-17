#!/usr/bin/env python

from distutils.core import setup, Extension

setup(
	name             = 'strutil',
	version          = '0.01',
	description      = 'Fast string utilities for Python.',
	long_description = 'Fast string manipulation routines implemented in C.',
	author           = 'Evan Klitzke',
	author_email     = 'evan@yelp.com',
	license          = 'BSD License',
	ext_modules      = [Extension(name='strutil', sources=['strutil.c'])]
)
