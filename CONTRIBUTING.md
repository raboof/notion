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
[Configuring and extending Notion with Lua](http://notion.sourceforge.net/notionconf/)
and [Notes for the module and patch writer](http://notion.sourceforge.net/notionnotes/)

## Releasing and versioning

Currently under discussion at https://github.com/raboof/notion/issues/121

## Getting in touch

Find us on IRC, in #notion on freenode
