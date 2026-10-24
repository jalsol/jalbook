# jalbook

## Build and test

```sh
bazel build //...
bazel test //...
bazel run //jalbook:jalbook
```

C++26 via g++16; builds use `-O3 -march=native` and GCC LTO, linked with
GNU gold (see `.bazelrc`). Tests use Catch2.

## Debugging

`--config=dbg` builds with `-O0 -g3`, frame pointers, no LTO, and an
unstripped binary:

```sh
bazel build --config=dbg //jalbook:jalbook
gdb --args bazel-bin/jalbook/jalbook -i data/<message>.csv
```

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
