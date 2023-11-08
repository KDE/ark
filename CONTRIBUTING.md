# Contributing to Ark

## Coding Style

Ark follows the kdelibs/Qt coding style. For more information about them, please see:

 - https://community.kde.org/Policies/Kdelibs_Coding_Style
 - https://wiki.qt.io/Qt_Coding_Style
 - https://wiki.qt.io/Coding_Conventions

## Sending patches

To send patches for Ark, please use KDE's Gitlab instance at https://invent.kde.org/utilities/ark.

If you already have a KDE commit account, it is still preferable to contact
the maintainer instead of committing directly, at least to be a good citizen
and especially to avoid git mistakes (see the [Using git](#using-git) section).

## Using git

The development model adopted by Ark is simple and rely on git's easy merging
and branching capabilities. If in doubt, do not hesitate to ask!

First of all, you should do your work in a separate branch, and each commit
should be as atomic as possible. This way, if you are asked to make changes to
them, the rest of your work is not disturbed, and you can easily rebase.

New features are committed to the `master` branch, respecting KDE's Release
Schedule policies. This means the soft and hard freeze periods must be
respected, as well as the string freeze policy.

Bug fixes are committed to the latest stable branch (for example, `release/19.12`),
which is then merged into the `master` branch.  Do *NOT* cherry-pick commits
into multiple branches! It makes following history unnecessarily harder for no
reason.

### How to merge stable in master

To merge the stable branch into `master`, the following steps can be followed:

```bash
$ git checkout release/19.12 # Whatever the stable branch is
$ # hack, hack, hack
$ # commit
$ git checkout master
$ git merge --log --edit -s recursive -Xours release/19.12
```

Do not worry if unrelated commits (such as translation ones made by KDE's
translation infrastructure) are also merged: translation commits are
automatically reverted when needed, and other commits being merged should be
bug fixes by definition.

The merge command above will try to automatically resolve conflicts by
preferring the version of master. This is usually good, for example a conflict
often happens on the `RELEASE_SERVICE_VERSION_XXX` variables in the root
CMakeLists.txt file. On the stable branch you will find something like:

```cmake
    set (RELEASE_SERVICE_VERSION_MAJOR "16")
    set (RELEASE_SERVICE_VERSION_MINOR "04")
    set (RELEASE_SERVICE_VERSION_MICRO "02")
```

while on master you will find something like:

```cmake
    set (RELEASE_SERVICE_VERSION_MAJOR "16")
    set (RELEASE_SERVICE_VERSION_MINOR "07")
    set (RELEASE_SERVICE_VERSION_MICRO "70")
```

In this example, the `ours` strategy option will pick the master version,
which is what we want. However, when a more complex conflict happens,
the `ours` option might fail to autoresolve the conflict and might leave
the working tree in a weird state. You should *always* check what the merge
actually did by using (after the merge):

```bash
$ git diff origin/HEAD HEAD
```

If the diff output is not what you expect, you can reset the merge with:

```bash
$ git reset --hard origin/HEAD
```

Then you can do a simple merge which will *not* resolve conflicts:

```bash
$ git merge release/19.12
```

and finally resolve all conflicts by hand.

### Pushing your changes

When you are ready to push your commits to the remote repository,
you may need to update your local branch first.
Do *NOT* create unnecessary merge commits with
`git pull`, as these commits are completely avoidable and make following
history harder. Instead, you should *rebase* first (`git pull --rebase`).

### See also

Instructions in the KDE community wiki: https://community.kde.org/KDE_Utils/Ark
