# jalbook

## Build and test

```sh
bazel build //...
bazel test //...
bazel run //jalbook:jalbook
```

C++26 via g++16; builds use `-O3 -march=native` and GCC LTO, linked with
GNU gold (see `.bazelrc`). Tests use Catch2.

## Editor / language server

Generate `compile_commands.json` for clangd:

```sh
bazel run @hedron_compile_commands//:refresh_all
```

The file is gitignored. Rerun after changing a `BUILD.bazel`.

## TODO
- HdrHistogram
- LOBSTER csv reader from scratch (?)
- Push/pop backoff(?)
- Logging from scratch
