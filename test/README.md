# blah2 Test

**TODO: Tests not implemented yet. Describing desired behaviour for the time being.**

## Framework

The test framework is [catch2](https://github.com/catchorg/Catch2).

## Types

The test files are split across directories defined by the type of test.

- **Unit tests** will test the class in isolation. The directory structure mirrors *src*.
- **Functional tests** will test that expected outputs are achieved from defined inputs. An example would be checking the program turns a specific IQ data set to a specific delay-Doppler map. This test category will rely on golden data.
- **Comparison tests** will compare different methods of performing the same task. An example would be comparing 2 methods of clutter filtering. Metrics to be compared may include time and performance. Note there is no pass/fail criteria for comparison tests - this is purely for information.

## Usage

All tests are compiled when building, however tests be run manually.

- Run a single test case for "TestClass".

```
sudo docker compose run blah2-test TestClass
```

- Run all test cases.

```
sudo docker compose run blah2-test *
```