# jalbook

## Build and test

```sh
bazel build //...
bazel test //...
bazel run //jalbook:jalbook -- --input_path data/<message>.csv
```

C++26 via g++16; builds use `-O3 -march=native` and GCC LTO, linked with
GNU gold (see `.bazelrc`). Tests use Catch2.

## Benchmark

- Intel Core i5 1135G7.
- Physical cores 1 and 2 (a.k.a logical cores 1,2,5,6) isolated, nohz, rcu_nocbs; irq affinity to other cores.
- C-state at max, governor at `performance`.
- Measured with `rdtscp`, accommodated for overhead (similar to rigtorp/spartan).
- Explicit hugepage (didn't help improve much so removed).

```
TSC: 2.419 GHz (0.413 ns/cycle); 29-cycle (12.0 ns) measurement overhead subtracted
nanoseconds per OrderBook::HandleEvent (rdtscp; overhead subtracted)
op            count       mean        p50        p90        p99      p99.9     p99.99        max
add          329566       18.4       17.4       23.6       28.9       41.7      912.3     3964.5
cancel         5602       20.7       19.8       24.4       28.9      122.4      246.8      288.9
delete       300183       28.4       28.1       34.3       40.1       45.9      141.0      482.0
execute       29798       28.1       27.3       35.1       42.6      122.4      332.3      498.9
```

## Debugging

`--config=dbg` builds with `-O0 -g3`, frame pointers, no LTO, and an
unstripped binary:

```sh
bazel build --config=dbg //jalbook:jalbook
gdb --args bazel-bin/jalbook/jalbook --input_path data/<message>.csv
```

## Editor / language server

Generate `compile_commands.json` for clangd:

```sh
bazel run @hedron_compile_commands//:refresh_all
```

The file is gitignored. Rerun after changing a `BUILD.bazel`.

## TODO
- LOBSTER csv reader from scratch (?)
- Push/pop backoff(?)
- Logging from scratch
