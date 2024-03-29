# Copyright (C) 2010 Chris Jerdonek (cjerdonek@webkit.org)
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1.  Redistributions of source code must retain the above copyright
#     notice, this list of conditions and the following disclaimer.
# 2.  Redistributions in binary form must reproduce the above copyright
#     notice, this list of conditions and the following disclaimer in the
#     documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE FOR
# ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
# SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
# OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

# This module is required for Python to treat this directory as a package.

"""Autoinstalls third-party code required by WebKit."""

from __future__ import with_statement

import codecs
import os

from webkitpy.common.system.autoinstall import AutoInstaller

# Putting the autoinstall code into webkitpy/thirdparty/__init__.py
# ensures that no autoinstalling occurs until a caller imports from
# webkitpy.thirdparty.  This is useful if the caller wants to configure
# logging prior to executing autoinstall code.

# FIXME: Ideally, a package should be autoinstalled only if the caller
#        attempts to import from that individual package.  This would
#        make autoinstalling lazier than it is currently.  This can
#        perhaps be done using Python's import hooks as the original
#        autoinstall implementation did.

# FIXME: If any of these servers is offline, webkit-patch breaks (and maybe
# other scripts do, too). See <http://webkit.org/b/42080>.

# We put auto-installed third-party modules in this directory--
#
#     webkitpy/thirdparty/autoinstalled
thirdparty_dir = os.path.dirname(__file__)
autoinstalled_dir = os.path.join(thirdparty_dir, "autoinstalled")

# We need to download ClientForm since the mechanize package that we download
# below requires it.  The mechanize package uses ClientForm, for example,
# in _html.py.  Since mechanize imports ClientForm in the following way,
#
# > import sgmllib, ClientForm
#
# the search path needs to include ClientForm.  We put ClientForm in
# its own directory so that we can include it in the search path without
# including other modules as a side effect.
clientform_dir = os.path.join(autoinstalled_dir, "clientform")
installer = AutoInstaller(append_to_search_path=True,
                          target_dir=clientform_dir)
installer.install(url="http://pypi.python.org/packages/source/C/ClientForm/ClientForm-0.2.10.zip",
                  url_subpath="ClientForm.py")

# The remaining packages do not need to be in the search path, so we create
# a new AutoInstaller instance that does not append to the search path.
installer = AutoInstaller(target_dir=autoinstalled_dir)

installer.install(url="http://pypi.python.org/packages/source/m/mechanize/mechanize-0.1.11.zip",
                  url_subpath="mechanize")
installer.install(url="http://pypi.python.org/packages/source/p/pep8/pep8-0.5.0.tar.gz#md5=512a818af9979290cd619cce8e9c2e2b",
                  url_subpath="pep8-0.5.0/pep8.py")
installer.install(url="http://www.adambarth.com/webkit/eliza",
                  target_name="eliza.py")

rietveld_dir = os.path.join(autoinstalled_dir, "rietveld")
installer = AutoInstaller(target_dir=rietveld_dir)
installer.install(url="http://webkit-rietveld.googlecode.com/svn/trunk/static/upload.py",
                  target_name="upload.py")


# Since irclib and ircbot are two top-level packages, we need to import
# them separately.  We group them into an irc package for better
# organization purposes.
irc_dir = os.path.join(autoinstalled_dir, "irc")
installer = AutoInstaller(target_dir=irc_dir)
installer.install(url="http://surfnet.dl.sourceforge.net/project/python-irclib/python-irclib/0.4.8/python-irclib-0.4.8.zip",
                  url_subpath="irclib.py")
installer.install(url="http://surfnet.dl.sourceforge.net/project/python-irclib/python-irclib/0.4.8/python-irclib-0.4.8.zip",
                  url_subpath="ircbot.py")

pywebsocket_dir = os.path.join(autoinstalled_dir, "pywebsocket")
installer = AutoInstaller(target_dir=pywebsocket_dir)
installer.install(url="http://pywebsocket.googlecode.com/files/mod_pywebsocket-0.5.tar.gz",
                  url_subpath="pywebsocket-0.5/src/mod_pywebsocket")

readme_path = os.path.join(autoinstalled_dir, "README")
if not os.path.exists(readme_path):
    with codecs.open(readme_path, "w", "ascii") as file:
        file.write("This directory is auto-generated by WebKit and is "
                   "safe to delete.\nIt contains needed third-party Python "
                   "packages automatically downloaded from the web.")
