#!/usr/bin/env python3

# SPDX-FileCopyrightText: 2020 Mattia Basaglia <dev@dragon.best>
# SPDX-License-Identifier: GPL-3.0-or-later

import os
import setuptools
from setuptools.dist import Distribution
from distutils.core import Extension

here = os.path.dirname(os.path.abspath(__file__))
with open(os.path.join(here, "README.md"), "r", encoding='utf8') as fh:
    long_description = fh.read()


setuptools.setup(
    name="${PROJECT_SLUG}",
    version="${PROJECT_VERSION}",
    author="${PROJECT_MANINTAINER_NAME}",
    author_email="mattia.basaglia@gmail.com",
    description="Python bindings for ${PROJECT_NAME}",
    long_description=long_description,
    long_description_content_type="text/markdown",
    url="https://glaxnimate.org/",
    license="GNU General Public License v3 or later (GPLv3+)",
    keywords="telegram stickers tgs lottie svg animation",
    #ext_modules=[Extension("${PROJECT_SLUG}", sources=[])],
    py_modules=["glaxnimate"],
    # https://pypi.org/classifiers/
    classifiers=[
        "Programming Language :: Python :: 3",
        "Development Status :: 3 - Alpha",
        "License :: OSI Approved :: GNU General Public License v3 or later (GPLv3+)",
        "Operating System :: POSIX :: Linux",
        "Operating System :: MacOS",
        "Operating System :: MacOS :: MacOS X",
        "Operating System :: Microsoft :: Windows",
        "Operating System :: Microsoft :: Windows :: Windows 10",
        "Topic :: Multimedia :: Graphics",
    ],
    zip_safe=True,
    python_requires=">=3",
    project_urls={
        "Code": "https://invent.kde.org/graphics/glaxnimate",
        "Documentation": "https://glaxnimate.org/",
        "Chat": "https://t.me/Glaxnimate",
        "Downloads": "https://glaxnimate.mattbas.org/download/",
        "Issues": "https://bugs.kde.org/enter_bug.cgi?product=glaxnimate",
    },
)
