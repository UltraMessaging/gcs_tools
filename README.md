# gcs_tools

A snapshot of some UM example applications (originally from UM version 6.14)
with modifications that are useful to the Global Customer Support organization
(GCS).
These modifications assist in supporting our customers.

# Table of contents

<sup>(table of contents from https://luciopaiva.com/markdown-toc/)</sup>

## COPYRIGHT AND LICENSE

All of the documentation and software included in this and any
other Informatica Ultra Messaging GitHub repository
Copyright (C) Informatica. All rights reserved.

Permission is granted to licensees to use
or alter this software for any purpose, including commercial applications,
according to the terms laid out in the Software License Agreement.

This source code example is provided by Informatica for educational
and evaluation purposes only.

THE SOFTWARE IS PROVIDED "AS IS" AND INFORMATICA DISCLAIMS ALL WARRANTIES
EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION, ANY IMPLIED WARRANTIES OF
NON-INFRINGEMENT, MERCHANTABILITY OR FITNESS FOR A PARTICULAR
PURPOSE.  INFORMATICA DOES NOT WARRANT THAT USE OF THE SOFTWARE WILL BE
UNINTERRUPTED OR ERROR-FREE.  INFORMATICA SHALL NOT, UNDER ANY CIRCUMSTANCES,
BE LIABLE TO LICENSEE FOR LOST PROFITS, CONSEQUENTIAL, INCIDENTAL, SPECIAL OR
INDIRECT DAMAGES ARISING OUT OF OR RELATED TO THIS AGREEMENT OR THE
TRANSACTIONS CONTEMPLATED HEREUNDER, EVEN IF INFORMATICA HAS BEEN APPRISED OF
THE LIKELIHOOD OF SUCH DAMAGES.

## REPOSITORY

See https://github.com/UltraMessaging/gcs_tools for code and documentation.

## INTRODUCTION

Many of the tools in this repository were cloned from example source files
of the same name (UM version 6.14).
They were copied because the support team requires changes to the tools that
are not appropriate for the example applications,
so we did not make the changes to the product examples.

### ENVIRONMENT

The commands and scripts in this repository assume four environment
variables are set up: LBM_LICENSE_INFO, LBM, and LD_LIBRARY_PATH.

Here's an example of setting them up:
````
export LD_LIBRARY_PATH LBM_LICENSE_INFO LBM
LBM_LICENSE_INFO="Product=LBM,UME,UMQ,UMDRO:Organization=UM RnD sford (RnD):Expiration-Date=never:License-Key=xxxx xxxx xxxx xxxx"
# Path to the install directory for the UM platform.
LBM="/home/sford/UMP_6.14/Linux-glibc-2.17-x86_64"
LD_LIBRARY_PATH="$LBM/lib"
````

### BUILD TEST TOOLS

The "bld.sh" script can be used to build the tools on Linux.
It relies on the "LBM" and "CP" [environment variables](#environment).
