We don't publish binaries, that is up to the distributions.

Whether to update the minor or patch level is decided case-by-case.

To release a new version of Notion:

* `git tag -a 4.x.y -s -m "Notion 4.x.y"`
* `git push --tags`
* edit the release notes prepared by release-drafter at https://github.com/raboof/notion/releases and publish them under the just-created tag
