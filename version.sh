#!/bin/bash

# Determines the currently checked-out version of Notion.

# Notion is distributed in source form through Git. Official releases are
# identified by signed tags. This script will use the git command-line tool
# to determine the version based on the latest tag and commit history.

# If the working directory is not a tag, the last tag is prepended to
# the number of commits since the last tag plus the git hash.

# If the working directory is dirty, a timestamp is appended.

# If you absolutely require a source tarball, you can use the 'Download ZIP'
# functionality. That zip will contain a directory called `notion-[xxx]`,
# where `[xxx]` denotes the tag or branch from which the tarball was derived.

# Returns 0 and prints the version on stdout on success, without newline.
# Returns a non-0 value on failure

set -e

if [ -e ".git" ]; then
  # Git:
  echo -n `git describe`
  if [[ -n $(git status -s) ]]; then
    echo -n `date +"+%Y%m%d-%H%M"`
  fi
else
  # Not git, derive name from current directory as per github naming conventions:
  basename `pwd` | sed '/^notion-/!{q1}; {s/^notion-//}' | tr -d '\\n'
fi
