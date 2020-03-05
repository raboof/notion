# Contributing to Notion

Contributions to Notion are greatly appreciated!

## Pull request model

We use the [standard GitHub PR mechanism](https://help.github.com/en/articles/creating-a-pull-request)
for contributions.

You are welcome to submit a PR straight away and start the discussion, though
for larger changes it might be good to discuss the approach in an issue
beforehand, to avoid disappointments later.

A pull request will be automatically built at Travis. It also does 'make test',
though the testing infrastructure is still in its infancy. Improvements in this
area are also much appreciated.

Anyone who is interested is invited to review pull requests. Possibly after
some back-and-forth, a member with push access may merge your PR. Please do
not merge your own PR's unless you are an admin.

## License

When creating a Pull Request, make sure you have the legal right to make this
contribution: all its contents have to be either your original work, or exist
under a license that allows inclusion in Notion.

Notion is licensed under the LGPL. The `notionflux(1)` utility (that is part of
mod_notionflux) links against
libreadline, so this is GPL, but we ask you to dual-license any contributions
to that utility under the LGPL as well. That way we could in theory replace
libreadline with an LGPL-compatible library later on.

## Documentation

The 2 main documents relevant when contributing to Notion are
[Configuring and extending Notion with Lua](https://raboof.github.io/notion-doc/notionconf/)
and [Notes for the module and patch writer](https://raboof.github.io/notion-doc/notionnotes/)

## Utilities

To run a built copy of notion from a checkout, with only
libraries/modules from the checkout, run something like

```
./notion/notion -nodefaultconfig $(./utils/dev-search-dirs.sh)
```

which is more or less equivalent to running

```
./notion/notion -nodefaultconfig \
-s de \
-s etc \
-s ioncore \
...
```

To run notion inside an existing X session, you may find it useful to
install Xephyr and run the scripts

- `utils/xephyr/start-xephyr.sh`
- `utils/xephyr/xephyr-notion.sh`

## Releasing and versioning

We don't publish binaries, that is up to the distributions.

Releasing is simply a matter of setting the relevant git tag
(by revising and publishing the [draft](https://github.com/raboof/notion/releases/)).
Whether to update the minor or patch level is decided case-by-case.

## Getting in touch

Find us on IRC, in #notion on freenode
