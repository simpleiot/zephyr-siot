# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to
[Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

- improved workflow with scripts in `envsetup-zonit.sh`
- added frontend testing
- added elm-review for frontend
- updated package.json at project root

### PR 132
- Removed the K_ESSENTIAL flag from w1.c, to avoid kernel panic
- Discussed with Cliff how this should be changed for every process and updated to involve a time.sleep style functionality