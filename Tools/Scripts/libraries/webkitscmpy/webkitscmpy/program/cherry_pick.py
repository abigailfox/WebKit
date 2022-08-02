# Copyright (C) 2020-2022 Apple Inc. All rights reserved.
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
# THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS "AS IS" AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE FOR
# ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
# SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
# OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

import re
import sys

from .command import Command
from webkitscmpy import local, log, remote


class CherryPick(Command):
    name = 'cherry-pick'
    help = "Given an identifier, revision, hash or pull-request, normalize and checkout that commit." \
           " Pull requests expected in the form 'PR-#'"

    PR_RE = re.compile(r'^\[?[Pp][Rr][ -](?P<number>\d+)]?$')

    @classmethod
    def parser(cls, parser, loggers=None):
        parser.add_argument(
            'hash', nargs=1,
            type=str, default=None,
            help='Commit hash',
        )
        # parser.add_argument(
        #     '--remote', dest='remote', type=str, default=None,
        #     help='Specify remote to search for pull request from.',
        # )

    @classmethod
    def main(cls, args, repository, **kwargs):
        print("I am cherry picking this commit:", args.hash[0])
        # TODO: how to find commit from hash?
        target = args.argument[0]
        match = cls.PR_RE.match(target)
        print("match: ", match)

        if not repository:
            sys.stderr.write('No repository provided\n')
            return 1
        if not repository.path:
            sys.stderr.write('Cannot checkout on remote repository\n')
            return 1

        return 0
